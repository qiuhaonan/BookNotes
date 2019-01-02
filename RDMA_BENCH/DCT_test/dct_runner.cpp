#include <iostream>
#include <vector>
#include <tuple>
#include <chrono>
#include <infiniband/verbs.h>
#include <sys/socket.h>
#include <netdb.h>
#include <malloc.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <endian.h>
#include <unistd.h>
#include <inttypes.h>
#include <byteswap.h>
#include <errno.h>

#include "cmdline.h"

using namespace std;

#if __BYTE_ORDER == __LITTLE_ENDIAN
static inline uint64_t ntohll(uint64_t x)
{
    return bswap_64(x);
}
static inline uint64_t htonll(uint64_t x)
{
    return bswap_64(x);
}
#elif __BYTE_ORDER == __BIG_ENDIAN
static inline uint64_t ntohll(uint64_t x)
{
    return x;
}
static inline uint64_t htonll(uint64_t x)
{
    return x;
}
#else
#error __BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN
#endif

#define DCT_KEY 2018
#define ONE_TO_ALL 0
#define ONE_TO_ONE 1

enum DCTPARA {LOCAL_DCT_NUM, REMOTE_DCT_NUM};

struct memory_info
{
	public:
		uint64_t	mem_addr_val;
		uint32_t	mem_rkey;
		uint32_t	mem_len;
		char 		gid[33];
	public:
		memory_info():mem_addr_val(0), mem_rkey(0), mem_len(0)
		{
			memset(gid, 0, 33);
		}
};

class infiniband_dct
{
	public:
		std::string			device_name;
		std::string 		server_name;
		struct ibv_context 	*ctx;
		struct ibv_qp 		*qp;
		struct ibv_cq 		*cq;
		struct ibv_pd 		*pd;
		struct ibv_srq		*srq;
		struct ibv_mr 		*mr;
		struct ibv_ah 		*ah;
		void* 				mem_addr;
		memory_info			local_info;
		memory_info			remote_info;
		int 				tcp_port;
		int 				ib_port;
		int			 		traffic_class;
		int 				io_size;
		int 				dct_connection_number;
		int 				socket_fd;
		int 				srq_num;
		int 				iteration;
        int                 communication_mode;
		uint16_t			gid_index;
		union ibv_gid 		dgid;
		union ibv_gid 		sgid;
		std::vector<struct ibv_exp_dct*> dct_vec;
		std::vector<std::tuple<uint32_t, uint32_t>> dct_para; 
		
	public:
		infiniband_dct(std::string name, std::string server, int ib_port, int tcp_port, int mem_size, int io_size, int dct_connection_number, int gid_index, int iter, int cm);
		int init_device();
		int init_server_socket();
		int init_client_socket();
		int create_tx_rx_queue();
		int to_rts();
		int exchange_info();
		int create_dct();
		int run_test();
		void gid_to_wire_gid(const union ibv_gid* gid, char wgid[]);
		void wire_gid_to_gid(const char* wgid, union ibv_gid* gid);
		void syn();
};

infiniband_dct::infiniband_dct(std::string name, std::string server, int _ib_port, int _tcp_port, 
							   int _mem_size, int _io_size, int _dct_connection_number, int _gid_index, int iter, int cm):
	device_name(name), server_name(server), ib_port(_ib_port), tcp_port(_tcp_port), io_size(_io_size), 
	dct_connection_number(_dct_connection_number), gid_index(_gid_index), iteration(iter), communication_mode(cm)
{
	ctx = nullptr;
	qp = nullptr;
	cq = nullptr;
	srq = nullptr;
	mr = nullptr;
	ah = nullptr;
	socket_fd = -1;
	srq_num = -1;
	traffic_class = 0;
    local_info.mem_len = _mem_size;
	memset(&dgid, 0, sizeof(dgid));
	memset(&sgid, 0, sizeof(sgid));

	for(int i=0; i<dct_connection_number; ++i)
	{
		dct_para.push_back(std::make_tuple(0, 0)); 
	}
}

int infiniband_dct::init_device()
{
	struct ibv_device	**dev_list;
	struct ibv_device	*ib_dev;

	dev_list = ibv_get_device_list(NULL);
	if (!dev_list) {
		cerr << "Failed to get IB devices list" << endl;
		return 1;
	}

	if (device_name.empty() == true) {
		ib_dev = *dev_list;
		if (!ib_dev) {
			cerr << "No IB devices found" << endl;
			return 1;
		}
	} else {
        int i=0;
		for (; dev_list[i]; ++i)
        {
			std::string name(ibv_get_device_name(dev_list[i]));
			if(name == device_name)
				break;
        }
		ib_dev = dev_list[i];
		if (!ib_dev) {
			cerr << "IB device " << device_name << " not found" << endl;
			return 1;
		}
	}

	ctx = ibv_open_device(ib_dev);
	if (!ctx) {
		cerr << "Couldn't get context for " << string(ibv_get_device_name(ib_dev)) << endl;
		return 1;
	}

	pd = ibv_alloc_pd(ctx);
	if (!pd) {
		cerr << "failed to allocate pd" << endl;
		return 1;
	}

	mem_addr = malloc(local_info.mem_len);
	if (!mem_addr) {
		cerr << "failed to allocate memory" << endl;
		return 1;
	}

	mr = ibv_reg_mr(pd, mem_addr, local_info.mem_len, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
	if (!mr) {
		cerr << "failed to create mr" << endl;
		return 1;
	}
	local_info.mem_rkey = mr->rkey;

	cq = ibv_create_cq(ctx, 128, NULL, NULL, 0);
	if (!cq) {
		cerr << "failed to create cq" << endl;
		return -1;
	}

	if (gid_index >= 0)
	{
		if (ibv_query_gid(ctx, ib_port, gid_index, &sgid))
		{
			cerr << "can't read sgid of index " << gid_index << endl;
			return 1;
		}
	}
}

int infiniband_dct::init_server_socket()
{
	struct addrinfo *res, *t;
	struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	char *service;
	int n;
	int sockfd = -1, connfd;
	int err;

	if (asprintf(&service, "%d", tcp_port) < 0)
		return -1;

	n = getaddrinfo(NULL, service, &hints, &res);

	if (n < 0) {
		fprintf(stderr, "%s for port %d\n", gai_strerror(n), tcp_port);
		free(service);
		return -1;
	}

	for (t = res; t; t = t->ai_next) {
		sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
		if (sockfd >= 0) {
			n = 1;

			setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));

			if (!bind(sockfd, t->ai_addr, t->ai_addrlen))
				break;
			close(sockfd);
			sockfd = -1;
		}
	}

	freeaddrinfo(res);
	free(service);

	if (sockfd < 0) {
		cerr << "Couldn't listen to port " << tcp_port << endl;
		return -1;
	}

	err = listen(sockfd, 1);
	if (err)
		return -1;

	connfd = accept(sockfd, NULL, 0);
	close(sockfd);
	if (connfd < 0) {
		cerr << "accept() failed" << endl;
		return -1;
	}
	socket_fd = connfd;
}

int infiniband_dct::init_client_socket()
{
	struct addrinfo *res, *t;
	struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	char *service;
	int n;
	int err;

	if (asprintf(&service, "%d", tcp_port) < 0)
		return -1;

	n = getaddrinfo(server_name.c_str(), service, &hints, &res);

	if (n < 0) {
		fprintf(stderr, "%s for %s:%d\n", gai_strerror(n), server_name, tcp_port);
		free(service);
		return -1;
	}

	for (t = res; t; t = t->ai_next) {
		socket_fd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
		if (socket_fd >= 0) {
			if (!connect(socket_fd, t->ai_addr, t->ai_addrlen))
				break;
			close(socket_fd);
			socket_fd = -1;
		}
	}

	freeaddrinfo(res);
	free(service);

	if (socket_fd < 0) {
		cerr << "Couldn't connect to " << server_name << ":" << tcp_port << endl;
		return -1;
	}
}

int infiniband_dct::create_tx_rx_queue()
{
	struct ibv_qp_init_attr_ex attr;
    memset(&attr, 0, sizeof(attr));
	attr.send_cq = cq;
	attr.recv_cq = cq;
	attr.cap.max_send_wr  = 100;
	attr.cap.max_send_sge = 1;
	attr.qp_type = IBV_EXP_QPT_DC_INI;
	attr.pd = pd;
	attr.comp_mask = IBV_QP_INIT_ATTR_PD;

	qp = ibv_create_qp_ex(ctx, &attr);
	if (!qp) {
		cerr << "failed to create qp" << endl;
		return -1;
	}

	struct ibv_srq_init_attr srq_attr;
    memset(&srq_attr, 0, sizeof(srq_attr));
	srq_attr.attr.max_wr = 100;
	srq_attr.attr.max_sge = 1;

	srq = ibv_create_srq(pd, &srq_attr);
	if (!srq)
	{
		cerr << "Couldn't create SRQ" << endl;
		return -1;
	}
	//ibv_get_srq_num(srq, &srq_num);

}

int infiniband_dct::create_dct()
{
	for(int i=0; i<dct_connection_number; ++i)
	{
		struct ibv_exp_dct_init_attr dctattr;
        memset(&dctattr, 0, sizeof(dctattr));
		dctattr.pd = pd,
		dctattr.cq = cq,
		dctattr.srq = srq,
		dctattr.dc_key = DCT_KEY,
		dctattr.port = ib_port;
		dctattr.access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;
		dctattr.min_rnr_timer = 2;
		dctattr.tclass = traffic_class;
		dctattr.flow_label = 0;
		dctattr.mtu = IBV_MTU_1024;
		dctattr.pkey_index = 0;
		dctattr.gid_index = gid_index;
		dctattr.hop_limit = 64;
		dctattr.create_flags = 0;
		dctattr.inline_size = 0;
		dctattr.create_flags |= 0;
        dctattr.comp_mask = 0;

		struct ibv_exp_dct* dct = ibv_exp_create_dct(ctx, &dctattr);
		if (!dct)
		{
			cerr << "create dct " << i <<" failed" << endl;
			return -1;
		}
		
		dct_vec.push_back(dct);
		std::get<LOCAL_DCT_NUM>(dct_para[i]) = dct->dct_num;
	}
}

int infiniband_dct::exchange_info()
{
	if(server_name.empty()) // server, 先交换内存信息，再交换dct信息
	{
		int n = 0;
		memory_info remote_memory_info;
		n = read(socket_fd, &remote_memory_info, sizeof(memory_info));
		if (n != sizeof(memory_info))
		{
			cerr << "server read: " << n << "/"<< sizeof(memory_info) << ": Couldn't read complete info" << endl;
			return -1;
		}
		remote_info.mem_addr_val = ntohll(remote_memory_info.mem_addr_val);
		remote_info.mem_len = ntohl(remote_memory_info.mem_len);
		remote_info.mem_rkey = ntohl(remote_memory_info.mem_rkey);
		wire_gid_to_gid(remote_memory_info.gid, &dgid);

		memory_info local_memory_info;
		local_memory_info.mem_addr_val = htonll((uintptr_t)mem_addr);
		local_memory_info.mem_len = htonl(local_info.mem_len);
		local_memory_info.mem_rkey = htonl(local_info.mem_rkey);
		gid_to_wire_gid(&sgid, local_memory_info.gid);

		n = write(socket_fd, &local_memory_info, sizeof(memory_info));
		if ( n != sizeof(memory_info))
		{
			cerr << "Couldn't send local address" << endl;
			return -1;
		}

		for(int i=0; i<dct_connection_number; ++i)
		{
			uint32_t local_dct_num = htonl(std::get<LOCAL_DCT_NUM>(dct_para[i]));

			n = write(socket_fd, &local_dct_num, sizeof(uint32_t));
			if(n != sizeof(uint32_t))
			{
				cerr << "Couldn't send local" << i << " dct info" << endl;
				return -1;
			}

			uint32_t remote_dct_num = 0;
			n = read(socket_fd, &remote_dct_num, sizeof(uint32_t));
			if (n != sizeof(uint32_t))
			{
				cerr << "server read: " << n << "/" << sizeof(uint32_t) << ": Couldn't read complete dct info" << endl;
				return -1;
			}
			std::get<REMOTE_DCT_NUM>(dct_para[i]) = ntohl(remote_dct_num);
		}
	}
	else // client
	{
		int n = 0;

		memory_info local_memory_info;
		local_memory_info.mem_addr_val = htonll((uintptr_t)mem_addr);
		local_memory_info.mem_len = htonl(local_info.mem_len);
		local_memory_info.mem_rkey = htonl(local_info.mem_rkey);
		gid_to_wire_gid(&sgid, local_memory_info.gid);

		n = write(socket_fd, &local_memory_info, sizeof(memory_info));
		if ( n != sizeof(memory_info))
		{
			cerr << "Couldn't send local address" << endl;
			return -1;
		}

		memory_info remote_memory_info;
		n = read(socket_fd, &remote_memory_info, sizeof(memory_info));
		if (n != sizeof(memory_info))
		{
			cerr << "client read: " << n << "/"<< sizeof(memory_info) << ": Couldn't read complete info" << endl;
			return -1;
		}
		remote_info.mem_addr_val = ntohll(remote_memory_info.mem_addr_val);
		remote_info.mem_len = ntohl(remote_memory_info.mem_len);
		remote_info.mem_rkey = ntohl(remote_memory_info.mem_rkey);
		wire_gid_to_gid(remote_memory_info.gid, &dgid);

		for(int i=0; i<dct_connection_number; ++i)
		{
			uint32_t remote_dct_num;
			n = read(socket_fd, &remote_dct_num, sizeof(uint32_t));
			if (n != sizeof(uint32_t))
			{
				cerr << "client read: " << n << "/" << sizeof(uint32_t) << ": Couldn't read complete dct info" << endl;
				return -1;
			}
			std::get<REMOTE_DCT_NUM>(dct_para[i]) = ntohl(remote_dct_num);

			uint32_t local_dct_num = htonl(std::get<LOCAL_DCT_NUM>(dct_para[i]));

			n = write(socket_fd, &local_dct_num, sizeof(uint32_t));
			if(n != sizeof(uint32_t))
			{
				cerr << "Couldn't send local" << i << " dct info" << endl;
				return -1;
			}
		}
	}
}

int infiniband_dct::to_rts()
{
	struct ibv_ah_attr	ah_attr;
	memset(&ah_attr, 0, sizeof(ah_attr));
	ah_attr.dlid	      = 0;
	ah_attr.sl            = 0;
	ah_attr.src_path_bits = 0;
	ah_attr.port_num      = ib_port;
	ah_attr.is_global = 1;
	ah_attr.grh.hop_limit = 64;
	ah_attr.grh.sgid_index = gid_index;
	ah_attr.grh.dgid = dgid;
	ah = ibv_create_ah(pd, &ah_attr);
	if (!ah) {
		cerr << "failed to create ah" << endl;
		return -1;
	}

	struct ibv_exp_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
	attr.qp_state        = IBV_QPS_INIT;
	attr.pkey_index      = 0;
	attr.port_num        = ib_port;
	attr.qp_access_flags = 0;
	attr.dct_key = DCT_KEY;

	long int attr_mask = 0;

	if (ibv_exp_modify_qp(qp, &attr,
			      IBV_EXP_QP_STATE		|
			      IBV_EXP_QP_PKEY_INDEX	|
			      IBV_EXP_QP_PORT		|
			      IBV_EXP_QP_DC_KEY)) {
		cerr << "Failed to modify QP to INIT" << endl;
		return 1;
	}

	attr.qp_state		= IBV_QPS_RTR;
	attr.max_dest_rd_atomic	= 16;
	attr.path_mtu		= IBV_MTU_1024;
	attr.ah_attr.is_global	= 1;
	attr.ah_attr.grh.sgid_index = gid_index;
	attr.ah_attr.grh.hop_limit = 64;
	attr.ah_attr.grh.dgid = dgid;

	attr.ah_attr.dlid	= 0;
	attr.ah_attr.port_num	= ib_port;
	attr.ah_attr.sl		= 0;
	attr.dct_key		= DCT_KEY;
	
    attr_mask = IBV_EXP_QP_STATE | IBV_EXP_QP_PATH_MTU | IBV_EXP_QP_AV;

	if (ibv_exp_modify_qp(qp, &attr, attr_mask)) {
		cerr << "Failed to modify QP to RTR" << endl;
		return 1;
	}

	attr.qp_state	    = IBV_QPS_RTS;
	attr.timeout	    = 14;
	attr.retry_cnt	    = 7;
	attr.rnr_retry	    = 7;
	attr.max_rd_atomic  = 16;
	if (ibv_exp_modify_qp(qp, &attr, IBV_EXP_QP_STATE	|
					      IBV_EXP_QP_TIMEOUT	|
					      IBV_EXP_QP_RETRY_CNT	|
					      IBV_EXP_QP_RNR_RETRY	|
					      IBV_EXP_QP_MAX_QP_RD_ATOMIC)) {
		cerr << "Failed to modify QP to RTS" << endl;
		return 1;
	}

	return 0;
}

void infiniband_dct::wire_gid_to_gid(const char* wgid, union ibv_gid* gid)
{
    char tmp[9];
    uint32_t v32;
    int i=0;
    for(tmp[8]=0;i<4;++i)
    {   
        memcpy(tmp,wgid+i*8,8);
        sscanf(tmp,"%x",&v32);
        *(uint32_t*)(&gid->raw[i*4])=ntohl(v32);
    }   
}

void infiniband_dct::gid_to_wire_gid(const union ibv_gid* gid, char wgid[])
{
    int i=0;
    for(;i<4;++i)
    {   
        sprintf(&wgid[i*8],"%08x",htonl(*(uint32_t*)(gid->raw+i*4)));
    }   
}

/*
int infiniband_dct::run_test()
{
	struct ibv_exp_send_wr wr;
	struct ibv_exp_send_wr *bad_wr;
	struct ibv_sge	sg_list;
	int err, i;
	int poll_cnt = 0;

	for (i = 0; i < iteration; ++i) {

		auto start = std::chrono::high_resolution_clock::now();
		for(int j = 0; j < dct_connection_number; ++j)
		{
			memset(&wr, 0, sizeof(struct ibv_exp_send_wr));
            memset(&sg_list, 0, sizeof(struct ibv_sge));
			wr.num_sge = 1;
			sg_list.addr = (uint64_t)(unsigned long)mem_addr;
			sg_list.length = io_size;
			sg_list.lkey = mr->lkey;
			wr.sg_list = &sg_list;
			wr.dc.ah = ah;
			wr.dc.dct_access_key = DCT_KEY;
			wr.dc.dct_number = (communication_mode == ONE_TO_ALL) ? std::get<REMOTE_DCT_NUM>(dct_para[j]) : std::get<REMOTE_DCT_NUM>(dct_para[0]);
			wr.wr.rdma.rkey = remote_info.mem_rkey;
			wr.wr.rdma.remote_addr = remote_info.mem_addr_val;
			wr.exp_opcode = IBV_EXP_WR_RDMA_WRITE;
			wr.exp_send_flags = (j == (dct_connection_number - 1))? IBV_EXP_SEND_SIGNALED : 0;
			wr.next = nullptr;

			err = ibv_exp_post_send(qp, &wr, &bad_wr);
			if (err) {
				cerr << "failed to post " << j << " send request for " << strerror(err) << endl;
				return -1;
			}
		}

		{
			int num;
			struct ibv_wc wc;
			do {
				num = ibv_poll_cq(cq, 1, &wc);
				if (num < 0) {
					cerr << "failed to poll cq" << endl;
					return -1;
				} else if (num) {
					poll_cnt++;
				}
			} while (!num);
			if (wc.status != IBV_WC_SUCCESS) {
				cerr << "completion with error " << wc.status << endl;
				return -1;
			}
		}
        auto end = std::chrono::high_resolution_clock::now();
        double lat = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
        if(communication_mode == ONE_TO_ALL)
        {
            cout << "Send 1 x " << io_size << " Bytes data to " << dct_connection_number << " x DCT(s) cost " << lat << " us " << endl;
        }
        else
        {
            cout << "Send " << dct_connection_number << " x " << io_size << " Bytes data to 1 x DCT cost " << lat << " us " << endl;
        }
	}
}
*/

int infiniband_dct::run_test()
{
	struct ibv_exp_send_wr wr[dct_connection_number];
	struct ibv_exp_send_wr *bad_wr;
	struct ibv_sge	sg_list[dct_connection_number];
	int err, i;
	int poll_cnt = 0;

	for (i = 0; i < iteration; ++i) {
		for(int j = 0; j < dct_connection_number; ++j)
		{
			memset(&wr[j], 0, sizeof(struct ibv_exp_send_wr));
			wr[j].num_sge = 1;
			sg_list[j].addr = (uint64_t)(unsigned long)mem_addr;
			sg_list[j].length = io_size;
			sg_list[j].lkey = mr->lkey;
			wr[j].sg_list = &sg_list[j];
			wr[j].dc.ah = ah;
			wr[j].dc.dct_access_key = DCT_KEY;
			wr[j].dc.dct_number = (communication_mode == ONE_TO_ALL) ? std::get<REMOTE_DCT_NUM>(dct_para[j]) : std::get<REMOTE_DCT_NUM>(dct_para[0]);
			wr[j].wr.rdma.rkey = remote_info.mem_rkey;
			wr[j].wr.rdma.remote_addr = remote_info.mem_addr_val;
			wr[j].exp_opcode = IBV_EXP_WR_RDMA_WRITE;
			wr[j].exp_send_flags = 0;
			wr[j].next = (j == (dct_connection_number-1))? nullptr : &wr[j+1];
		}
		wr[dct_connection_number-1].exp_send_flags = IBV_EXP_SEND_SIGNALED;

		auto start = std::chrono::high_resolution_clock::now();
		err = ibv_exp_post_send(qp, &wr[0], &bad_wr);
     
		if (err) {
			cerr << "failed to post send request" << endl;
			return -1;
		} else {
			int num;
			struct ibv_wc wc;
			do {
				num = ibv_poll_cq(cq, 1, &wc);
				if (num < 0) {
					cerr << "failed to poll cq" << endl;
					return -1;
				} else if (num) {
					poll_cnt++;
				}
			} while (!num);
			if (wc.status != IBV_WC_SUCCESS) {
				cerr << "completion with error " << wc.status << endl;
				return -1;
			}
		}
        auto end = std::chrono::high_resolution_clock::now();
        double lat = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
        if(communication_mode == ONE_TO_ALL)
        {
            cout << "Send 1 x " << io_size << " Bytes data to " << dct_connection_number << " x DCT(s) cost " << lat << " us " << endl;
        }
        else
        {
            cout << "Send " << dct_connection_number << " x " << io_size << " Bytes data to 1 x DCT cost " << lat << " us " << endl;
        }
	}
}

void infiniband_dct::syn()
{
	int n = 0;
	if(server_name.empty())
	{
		char c = 'a';
		n = write(socket_fd, &c, sizeof(c));
		if(n != sizeof(c))
		{
			cerr << "Server Couldn't send syn data " << endl;
			return ;
		}
		n = read(socket_fd, &c, sizeof(c));
		if(n != sizeof(c))
		{
			cerr << "Server Couldn't receive syn data " << endl;
			return ;
		}
	}
	else
	{
		char c = 'a';
		n = read(socket_fd, &c, sizeof(c));
		if(n != sizeof(c))
		{
			cerr << "Client Couldn't receive syn data " << endl;
			return ;
		}
		n = write(socket_fd, &c, sizeof(c));
		if(n != sizeof(c))
		{
			cerr << "Client Couldn't send syn data " << endl;
			return ;
		}
	}
}

int main(int argc, char* argv[])
{
    cmdline::parser opt;
    opt.add<std::string>("device", 'd', "device name", false, "mlx5_0");
    opt.add<std::string>("server", 's', "server name", false, "");
    opt.add<int>("ib", 'i', "ib port", false, 1, cmdline::range(1, 2));
    opt.add<int>("tcp", 't', "tcp port", false, 18515, cmdline::range(1025, 65536));
    opt.add<int>("memory", 'm', "memory size", false, 1048576, cmdline::range(4096, 1073741824));
    opt.add<int>("data", 'S', "data size", false, 1024, cmdline::range(8, 65536));
    opt.add<int>("DCT", 'n', "DCT connection number", false, 1, cmdline::range(1, 1024));
    opt.add<int>("gid", 'g', "gid index", false, 3);
    opt.add<int>("iteration", 'I', "Iteration", false, 1000, cmdline::range(1, 10000000));
    opt.add<int>("mode", 'M', "Communication Mode", false, ONE_TO_ALL, cmdline::range(0, 1));
    opt.parse_check(argc, argv);
    
    string device_name {opt.get<string>("device")};
    string server_name {opt.get<string>("server")};
    int ib_port = opt.get<int>("ib");
    int tcp_port = opt.get<int>("tcp");
    int mem_size = opt.get<int>("memory");
    int io_size = opt.get<int>("data");
    int dct_connection_number = opt.get<int>("DCT");
    int gid_index = opt.get<int>("gid");
    int iteration = opt.get<int>("iteration");
    int cm = opt.get<int>("mode");
    infiniband_dct device(device_name, server_name, ib_port, tcp_port, mem_size, io_size, dct_connection_number, gid_index, iteration, cm);
    
    device.init_device();
	if(server_name.empty())
		device.init_server_socket();
	else
		device.init_client_socket();
	device.create_tx_rx_queue();
	device.create_dct();
	device.exchange_info();
	device.to_rts();
	device.run_test();
	device.syn();

    return 0;
}
