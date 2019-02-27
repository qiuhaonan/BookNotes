#include <iostream>
#include <vector>
#include <tuple>
#include <chrono>
#include <future>
#include <fstream>
#include <infiniband/verbs.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netdb.h>
#include <malloc.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <endian.h>
#include <unistd.h>
#include <inttypes.h>
#include <byteswap.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <cassert>

#include "cmdline.h"

//#define POST_ALL 

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

#define SEQ 0
#define RAND 1
#define ODP_ONLY 2
#define ASYNC_ODP_ONLY 3

#define MALLOC 0
#define MMAP 1
#define ODP 0
#define REG 1

struct memory_info
{
	public:
		uint64_t	mem_addr_val;
		uint32_t	mem_rkey;
		uint32_t	mem_len;
		uint32_t 	qp_number;
		char 		gid[33];
	public:
		memory_info():mem_addr_val(0), mem_rkey(0), mem_len(0), qp_number(0)
		{
			memset(gid, 0, 33);
		}
};

class infiniband_odp
{
	public:
		std::string			device_name;
		std::string 		server_name;
		struct ibv_context 	*ctx;
		struct ibv_qp 		*qp;
		struct ibv_cq 		*cq;
		struct ibv_pd 		*pd;
		struct ibv_mr 		*mr;
		void* 				mem_addr;
		memory_info			local_info;
		memory_info			remote_info;
		int 				tcp_port;
		int 				ib_port;
		int			 		traffic_class;
		int 				io_size;
		int 				socket_fd;
		int 				batch_size;
		int 				iteration;
        int                 communication_mode;
        int                 huge_page;
        int                 memory_type;
		uint64_t 			old_offset;
		uint16_t			gid_index;
		union ibv_gid 		dgid;
		union ibv_gid 		sgid;
		
	public:
		infiniband_odp(std::string name, std::string server, int ib_port, int tcp_port, int mem_size, int io_size, int batch, int gid_index, int iter, int cm, int huge_page, int recvodp);
		int init_device();
		int prefetch_mr_test();
        int async_prefetch_mr_test();
		int prefetch_mr(void* addr, int length);
		int init_server_socket();
		int init_client_socket();
		int create_tx_rx_queue();
		int to_rts();
		int exchange_info();
		uint64_t choose_next_addr();
        int post_all(struct ibv_send_wr* wr, struct ibv_sge* sg_list);
        int post_pipeline(struct ibv_send_wr* wr, struct ibv_sge* sg_list);
		int run_test();
        void to_file(std::vector<double>& data);
		void gid_to_wire_gid(const union ibv_gid* gid, char wgid[]);
		void wire_gid_to_gid(const char* wgid, union ibv_gid* gid);
		void syn();
};

infiniband_odp::infiniband_odp(std::string name, std::string server, int _ib_port, int _tcp_port, 
							   int _mem_size, int _io_size, int _batch_size, int _gid_index, int iter, int cm, int _huge_page, int _memory_type):
	device_name(name), server_name(server), ib_port(_ib_port), tcp_port(_tcp_port), io_size(_io_size), 
	batch_size(_batch_size), gid_index(_gid_index), iteration(iter), communication_mode(cm), huge_page(_huge_page), memory_type(_memory_type)
{
	ctx = nullptr;
	qp = nullptr;
	cq = nullptr;
	mr = nullptr;
	socket_fd = -1;
	traffic_class = 0;
	old_offset = 0;
    local_info.mem_len = _mem_size;
	memset(&dgid, 0, sizeof(dgid));
	memset(&sgid, 0, sizeof(sgid));
}

int infiniband_odp::init_device()
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

    if(huge_page == MALLOC)
    	mem_addr = malloc(local_info.mem_len);
    else
        mem_addr = mmap(NULL, local_info.mem_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    memset(mem_addr, 'a', local_info.mem_len);
	if (!mem_addr) {
		cerr << "failed to allocate memory" << endl;
		return 1;
	}

	// check the odp attribute
	struct ibv_exp_device_attr device_attr;
	device_attr.comp_mask = IBV_EXP_DEVICE_ATTR_ODP | IBV_EXP_DEVICE_ATTR_EXP_CAP_FLAGS;

	ibv_exp_query_device(ctx, &device_attr);

	if (device_attr.exp_device_cap_flags & IBV_EXP_DEVICE_ODP)
	{
		cout << device_name << " supports ODP" << endl;
		if(device_attr.odp_caps.general_odp_caps & IBV_EXP_ODP_SUPPORT_IMPLICIT)
		{
			cout << device_name << " supports Implicit ODP" << endl;
		}
		else
		{
			cout << device_name << " does not support Implicit ODP" << endl;
		}
	}
	else 
	{
		cout << device_name << " does not support ODP" << endl;
	}

    if(memory_type == ODP)
    {
	  struct ibv_exp_reg_mr_in in;
	  memset(&in, 0, sizeof(in));
	  in.pd = pd;
	  in.addr = 0;
	  in.length = IBV_EXP_IMPLICIT_MR_SIZE;
      //cout << "IBV_EXP_IMPLICIT_MR_SIZE:" << hex << IBV_EXP_IMPLICIT_MR_SIZE << dec << endl;
	  in.exp_access = IBV_EXP_ACCESS_ON_DEMAND | IBV_EXP_ACCESS_LOCAL_WRITE | IBV_EXP_ACCESS_REMOTE_WRITE ;
	  in.comp_mask = 0;

	  mr = ibv_exp_reg_mr(&in);
      prefetch_mr(mem_addr, local_info.mem_len);
      std::cout << "memory type : ODP" << std::endl;
    }
    else
    {
      mr = ibv_reg_mr(pd, mem_addr, local_info.mem_len, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);
      std::cout << "memory type : Registered" << std::endl;
    }

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

int infiniband_odp::async_prefetch_mr_test()
{
	for(long mem_size = 2; mem_size <= 1073741824; mem_size = mem_size * 2)
	{
        auto addr = memalign(4096, mem_size);
		memset(addr, 'a', mem_size);
		for(int test_index = 0; test_index < 100; ++test_index)
		{
			auto start = std::chrono::high_resolution_clock::now();
            auto handle = std::async(std::launch::async, [=](){
                int ret = prefetch_mr(addr, mem_size);
                return ret;
            });
			auto mid = std::chrono::high_resolution_clock::now();
            auto ret = handle.get();
			auto end = std::chrono::high_resolution_clock::now();

			double call_lat = std::chrono::duration_cast<std::chrono::microseconds>(mid - start).count();
			double wait_lat = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
			cout << "prefetch mr ret="<<ret<<", size="<< mem_size<< " Bytes, call_latency=" << call_lat << " us, wait_latency=" << wait_lat << " us" << endl;
		}
		cout << endl;
		free(addr);
	}
}

int infiniband_odp::prefetch_mr_test()
{
    //auto addr = mmap(NULL, 1073741824, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);;
	for(long mem_size = 2; mem_size <= 1073741824; mem_size = mem_size * 2)
	{
		auto addr = malloc(mem_size);
        //auto addr = memalign(4096, mem_size);
        //auto addr = memalign(64, mem_size);
		memset(addr, 'a', mem_size);
		struct ibv_exp_prefetch_attr prefetch_attr;
		prefetch_attr.flags = IBV_EXP_PREFETCH_WRITE_ACCESS;
		prefetch_attr.addr = addr;
		prefetch_attr.length = mem_size;
		prefetch_attr.comp_mask = 0;
		for(int test_index = 0; test_index < 100; ++test_index)
		{
			auto start = std::chrono::high_resolution_clock::now();
			int ret = ibv_exp_prefetch_mr(mr, &prefetch_attr);
			auto end = std::chrono::high_resolution_clock::now();
			if (ret)
			{
				cerr << "prefetch mr failed, errno : " << ret << endl;
				return -1;
			}
			else
			{
				double lat = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
				cout << "prefetch mr succeed, size="<< mem_size<< " Bytes, latency=" << lat << " us" << endl;
			}
		}
		cout << endl;
		free(addr);
	}
}

int infiniband_odp::init_server_socket()
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

int infiniband_odp::init_client_socket()
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

int infiniband_odp::create_tx_rx_queue()
{
	struct ibv_qp_init_attr attr;
    memset(&attr, 0, sizeof(attr));
	attr.qp_type = IBV_QPT_RC;
	attr.send_cq = cq;
	attr.recv_cq = cq;
	attr.cap.max_send_wr  = 100;
	attr.cap.max_recv_wr=64;
	attr.cap.max_send_sge = 1;
	attr.cap.max_recv_sge=1;
	attr.sq_sig_all=0;

	qp = ibv_create_qp(pd, &attr);
	if (!qp) {
		cerr << "failed to create qp" << endl;
		return -1;
	}
	local_info.qp_number = qp->qp_num;
	return 0;
}

int infiniband_odp::exchange_info()
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
		remote_info.qp_number = ntohl(remote_memory_info.qp_number);
        cout << "Remote QP Number:" << remote_info.qp_number << endl;
		wire_gid_to_gid(remote_memory_info.gid, &dgid);

		memory_info local_memory_info;
		local_memory_info.mem_addr_val = htonll((uintptr_t)mem_addr);
		local_memory_info.mem_len = htonl(local_info.mem_len);
		local_memory_info.mem_rkey = htonl(local_info.mem_rkey);
		local_memory_info.qp_number = htonl(local_info.qp_number);
        cout << "Local QP Number:" << local_info.qp_number << endl;
		gid_to_wire_gid(&sgid, local_memory_info.gid);

		n = write(socket_fd, &local_memory_info, sizeof(memory_info));
		if ( n != sizeof(memory_info))
		{
			cerr << "Couldn't send local address" << endl;
			return -1;
		}
	}
	else // client
	{
		int n = 0;

		memory_info local_memory_info;
		local_memory_info.mem_addr_val = htonll((uintptr_t)mem_addr);
		local_memory_info.mem_len = htonl(local_info.mem_len);
		local_memory_info.mem_rkey = htonl(local_info.mem_rkey);
		local_memory_info.qp_number = htonl(local_info.qp_number);
        cout << "Local QP Number:" << local_info.qp_number << endl;
		gid_to_wire_gid(&sgid, local_memory_info.gid);
        cout << "local gid len:" << strlen(local_memory_info.gid) << endl;

		n = write(socket_fd, &local_memory_info, sizeof(memory_info));
        cout << "write len:" << n << endl;
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
        cout << "read len:" << n << endl;
		remote_info.mem_addr_val = ntohll(remote_memory_info.mem_addr_val);
		remote_info.mem_len = ntohl(remote_memory_info.mem_len);
		remote_info.mem_rkey = ntohl(remote_memory_info.mem_rkey);
		remote_info.qp_number = ntohl(remote_memory_info.qp_number);
        cout << "Remote QP Number:" << remote_info.qp_number << endl;
        cout << "remote gid len:" << strlen(remote_memory_info.gid) << endl; 
		wire_gid_to_gid(remote_memory_info.gid, &dgid);
	}
}

int infiniband_odp::to_rts()
{
	long int attr_mask = 0;
	struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
	attr.qp_state        = IBV_QPS_INIT;
	attr.pkey_index      = 0;
	attr.port_num        = ib_port;
	attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;

	if (ibv_modify_qp(qp, &attr,
			      IBV_EXP_QP_STATE		|
			      IBV_EXP_QP_PKEY_INDEX	|
			      IBV_EXP_QP_PORT		|
			      IBV_EXP_QP_ACCESS_FLAGS)) {
		cerr << "Failed to modify QP to INIT" << endl;
		return 1;
	}

	attr.qp_state	=IBV_QPS_RTR;
    attr.path_mtu	=IBV_MTU_1024;
    attr.dest_qp_num	= remote_info.qp_number;
    attr.max_dest_rd_atomic= 16;
    attr.min_rnr_timer	=0x12;
    attr.rq_psn		=0;
    attr.ah_attr.is_global	=1;
    attr.ah_attr.grh.sgid_index  = gid_index;
    attr.ah_attr.grh.traffic_class = traffic_class;
    attr.ah_attr.grh.dgid	= dgid;
    attr.ah_attr.port_num	= ib_port;
	
	attr_mask=IBV_QP_STATE|IBV_QP_PATH_MTU|IBV_QP_DEST_QPN|IBV_QP_RQ_PSN|IBV_QP_AV|IBV_QP_MIN_RNR_TIMER|IBV_QP_MAX_DEST_RD_ATOMIC;

	if (ibv_modify_qp(qp, &attr, attr_mask)) {
		cerr << "Failed to modify QP to RTR" << endl;
		return 1;
	}

	attr.qp_state	    = IBV_QPS_RTS;
	attr.timeout	    = 0x12;
	attr.retry_cnt	    = 7;
	attr.rnr_retry	    = 7;
    attr.sq_psn         = 0;
	attr.max_rd_atomic  = 16;

	attr_mask=IBV_QP_STATE|IBV_QP_TIMEOUT|IBV_QP_RETRY_CNT|IBV_QP_RNR_RETRY|IBV_QP_SQ_PSN|IBV_QP_MAX_QP_RD_ATOMIC;

	if (ibv_modify_qp(qp, &attr, attr_mask)) {
		cerr << "Failed to modify QP to RTS" << endl;
		return 1;
	}

	return 0;
}

void infiniband_odp::wire_gid_to_gid(const char* wgid, union ibv_gid* gid)
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

void infiniband_odp::gid_to_wire_gid(const union ibv_gid* gid, char wgid[])
{
    int i=0;
    for(;i<4;++i)
    {   
        sprintf(&wgid[i*8],"%08x",htonl(*(uint32_t*)(gid->raw+i*4)));
    }   
}

uint64_t infiniband_odp::choose_next_addr()
{
	if(communication_mode == SEQ)
	{
		old_offset = (old_offset + io_size) % local_info.mem_len;
		return old_offset;
	}
	else 
	{
		struct timeval now;
		gettimeofday(&now, NULL);
		srand(now.tv_sec + now.tv_usec);
		uint64_t offset = rand();
		offset = (offset % (local_info.mem_len - io_size));
		return offset;
	}
}

int infiniband_odp::prefetch_mr(void* addr, int length)
{
	struct ibv_exp_prefetch_attr prefetch_attr;
	prefetch_attr.flags = IBV_EXP_PREFETCH_WRITE_ACCESS;
	prefetch_attr.addr = addr;
	prefetch_attr.length = length;
	prefetch_attr.comp_mask = 0;
	return ibv_exp_prefetch_mr(mr, &prefetch_attr);
}

int infiniband_odp::post_all(struct ibv_send_wr* wr, struct ibv_sge* sg_list)
{
	int poll_cnt = 0;
	struct ibv_send_wr *bad_wr;
	for(int j = 0; j < batch_size; ++j)
	{
		uint64_t offset = choose_next_addr();
		memset(&wr[j], 0, sizeof(struct ibv_send_wr));
		wr[j].num_sge = 1;
		sg_list[j].addr = (uint64_t)((unsigned long)mem_addr + offset);
		sg_list[j].length = io_size;
		sg_list[j].lkey = mr->lkey;
		wr[j].sg_list = &sg_list[j];
		wr[j].wr.rdma.rkey = remote_info.mem_rkey;
		wr[j].wr.rdma.remote_addr = remote_info.mem_addr_val + offset;
		wr[j].opcode = IBV_WR_RDMA_WRITE;
		wr[j].send_flags = 0;
		wr[j].next = (j == (batch_size-1))? nullptr : &wr[j+1];
        void* target_addr = (void*)((char*)mem_addr + offset);
		if(memory_type == ODP)
          prefetch_mr(target_addr, io_size);
	}
	wr[batch_size-1].send_flags = IBV_SEND_SIGNALED;

	int err = ibv_post_send(qp, &wr[0], &bad_wr);
    
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
    return 0;
}

int infiniband_odp::post_pipeline(struct ibv_send_wr* wr, struct ibv_sge* sg_list)
{
	int poll_cnt = 0;
	struct ibv_send_wr *bad_wr;
	for(int j = 0; j < batch_size; ++j)
	{
		uint64_t offset = choose_next_addr();
		memset(&wr[j], 0, sizeof(struct ibv_send_wr));
		wr[j].num_sge = 1;
		sg_list[j].addr = (uint64_t)((unsigned long)mem_addr + offset);
		sg_list[j].length = io_size;
		sg_list[j].lkey = mr->lkey;
		wr[j].sg_list = &sg_list[j];
		wr[j].wr.rdma.rkey = remote_info.mem_rkey;
		wr[j].wr.rdma.remote_addr = remote_info.mem_addr_val + offset;
		wr[j].opcode = IBV_WR_RDMA_WRITE;
		wr[j].send_flags = (j == (batch_size-1))? IBV_SEND_SIGNALED : 0;
		wr[j].next = nullptr;
	    int err = ibv_post_send(qp, &wr[j], &bad_wr);
	    if (err) {
		    cerr << "failed to post send request" << endl;
    		return -1;
	    }
        void* target_addr = (void*)((char*)mem_addr + offset);
		//prefetch_mr(target_addr, io_size);
	}

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
    return 0;
}

int infiniband_odp::run_test()
{
	struct ibv_send_wr wr[batch_size];
	struct ibv_sge	sg_list[batch_size];
	int i;
    
    std::vector<double> data;
	for (i = 0; i < iteration; ++i) {
	    auto start = std::chrono::high_resolution_clock::now();
        #ifdef POST_ALL
        assert(post_all(wr, sg_list) == 0);
        #else
        assert(post_pipeline(wr, sg_list) == 0);
        #endif
        auto end = std::chrono::high_resolution_clock::now();
        double lat = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
        data.push_back(lat);
	}
    to_file(data);
    return 0;
}

void infiniband_odp::to_file(std::vector<double>& data)
{
    std::string file_name = std::to_string(local_info.mem_len) + "B_" + ((communication_mode == SEQ)?"SEQ":"RAND");
    std::ofstream file(file_name);
    for(auto lat:data)
        file << lat << std::endl;
    file.flush();
    file.close();
}

void infiniband_odp::syn()
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
    opt.add<int>("memory", 'm', "memory size", false, 2097152, cmdline::range(64, 1073741824));
    opt.add<int>("data", 'S', "data size", false, 1024, cmdline::range(8, 1048576));
    opt.add<int>("batch", 'n', "batch size", false, 1, cmdline::range(1, 1024));
    opt.add<int>("gid", 'g', "gid index", false, 3);
    opt.add<int>("iteration", 'I', "Iteration", false, 1000, cmdline::range(1, 10000000));
    opt.add<int>("mode", 'M', "Communication Mode", false, SEQ, cmdline::range(0, 3));
    opt.add<int>("huge", 'H', "Huge Page", false, 0, cmdline::range(0,1));
    opt.add<int>("memorytype", 'r', "using odp or registered memory", false, 0, cmdline::range(0,1));
    opt.parse_check(argc, argv);
    
    string device_name {opt.get<string>("device")};
    string server_name {opt.get<string>("server")};
    int ib_port = opt.get<int>("ib");
    int tcp_port = opt.get<int>("tcp");
    int mem_size = opt.get<int>("memory");
    int io_size = opt.get<int>("data");
    int batch_size = opt.get<int>("batch");
    int gid_index = opt.get<int>("gid");
    int iteration = opt.get<int>("iteration");
    int cm = opt.get<int>("mode");
    int huge_page = opt.get<int>("huge");
    int memory_type = opt.get<int>("memorytype");
    infiniband_odp device(device_name, server_name, ib_port, tcp_port, mem_size, io_size, batch_size, gid_index, iteration, cm, huge_page, memory_type);
    
    cout << "memory info size:" << sizeof(memory_info) << endl; 
    device.init_device();
	if(cm == ODP_ONLY && memory_type == ODP)
	{
		device.prefetch_mr_test();
	}
    else if(cm == ASYNC_ODP_ONLY && memory_type == ODP)
    {
        device.async_prefetch_mr_test();
    }
	else
	{
		if (server_name.empty())
			device.init_server_socket();
		else
			device.init_client_socket();
		device.create_tx_rx_queue();
		device.exchange_info();
		device.to_rts();
		if (server_name.empty())
			device.run_test();
		device.syn();
	}

    return 0;
}
