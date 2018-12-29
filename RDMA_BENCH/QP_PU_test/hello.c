#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <endian.h>
#include <inttypes.h>
#include <byteswap.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <malloc.h>
#include <signal.h>
#include "sock.h"
#include "head.h"
#include "get_clock.h"

#define MAX_POLL_TIMEOUT 100000
#define CTX_POLL_BATCH 16
#define DEF_CACHE_LINE_SIZE 64
#define PIPELINE_MODE

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

long start_usec = 0;
long end_usec = 0;

struct config_t {
    const char		*dev_name;
    char 			*server_name;
    u_int32_t		tcp_port;
    int 			ib_port;
};

struct config_t config= {
    "mlx5_0",
    NULL,
    19875,
    1
};

struct mem_data {
    uint64_t	addr;
    uint32_t	rkey;
    char        gid[33];
};

struct resources {
    struct ibv_qp			**qp;
    struct ibv_cq			*cq;
    struct ibv_send_wr		*sr;               // WQE array
    int 			batch_size;                // number of WQE posted in one ibv_post_send call
    int             cq_mod;                    // CQ depth
    int             tx_depth;                  // QP depth
    int 			verbose;                   // output infomation choice
    int 			mem_size;                  // registered memory size
    int				io_size;                   // data size that one WQE contains
    struct ibv_sge	        *sge;              // sge array corresponding to WQE array
    void				    *tx_buf;           // data buffer for sending 
    void				    *rx_buf;           // data buffer for receiving
    int             qp_number;                    // the total number of established QP
    int             batch_signal;                 // a cqe every batch_signal x wqes
    int             is_local_cacheline_aligned;   // if aligning the start address of local sending buffer to cacheline
    int             is_remote_cacheline_aligned;  // if aligning the start address of remote sending buffer to cacheline
    int             local_cacheline_miss_counter; // the counter of WQEs that the start address of local sending buffers unaligned to cacheline 
    int             remote_cacheline_miss_counter;// the counter of WQEs that the start address of remote receiving buffers unaligned to cacheline
    int             local_aligned_offset;         // offset for aligning the local registered memory start address to cacheline
    int             remote_aligned_offset;        // offset for aligning the local registered memory start address to cacheline
    int             cache_line_size;              // cacheline size of the system
    int             system_page_size;             // system page size (usually 4KB)
    int             use_hugetable;                // if using 2MB hugetable (should be reserved by cmd before running the program)
    int             is_using_aligned_alloc;       // if using memalign rather than malloc
    int				traffic_class;                // traffic class for each QP (8 bits = DSCP+ECN)
    int				sgid_index;                   // gid index, (see show_gids cmd)
    int             type;                         // 0 for latency flow test, 1 for throughput flow test
    int				run_infinitely;               // if keeping running
    int             duplex_mode;                  // if the remote side sends data to local side
    int				iteration;                    // the iteration of sending data, only when the run_infinitely is disabled
    int				perform_warmup;               // if performing warmup for RNIC
    int 			event_or_poll;                // poll CQ, 1 for event channel, 0 for busy polling default, 
    int 				    sock;                 // socket fd, for exchanging metadata
    struct mem_data	    	remote_props;         // remote metadata
    struct ibv_port_attr	port_attr;            // port attributes
    struct ibv_context		*ib_ctx;              // rdma context
    struct ibv_pd			*pd;                  // rdma protection domain
    struct ibv_comp_channel	*cc;                  // rdma event channel, related to CQ
    struct ibv_mr			*tx_mr;               // registered sending memory region, related to tx_buf
    struct ibv_mr           *rx_mr;               // registered receiving memory region, related to rx_buf
    union  ibv_gid         	dgid;                 // remote gid, for roce(v1, v2) network
    union  ibv_gid          sgid;                 // local gid, for roce(v1, v2) network
} __attribute__((aligned(DEF_CACHE_LINE_SIZE)));

struct resources 	res;

/// choose the start address of sending buffer from the whole registered sending buffer, while aligning the start address to the cacheline.
static inline uint64_t random_offset_cache_line_aligned(struct resources* res, int aligned_offset)
{
    struct timeval now;
    int range = (res->mem_size - res->io_size - aligned_offset) / res->cache_line_size;
    gettimeofday(&now,NULL);
    srand(now.tv_sec + now.tv_usec);
    return (rand()%range)*res->cache_line_size + aligned_offset;
}

/// choose the start address of sending buffer from the whole registered sending buffer randomly.
static inline uint64_t random_offset(struct resources* res)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    srand(now.tv_sec + now.tv_usec);
    uint64_t offset = rand();
    offset = (offset % ( res->mem_size - res->io_size));
    return offset;
}

/// get the time point, us resolution
static inline long tick(long start)
{
    struct timeval now;
    gettimeofday(&now,NULL);
    long usec = now.tv_sec * 1000000 + now.tv_usec;
    usec = usec -start;
    return usec;
}

/// wrapper of ibv_post_send, while counting the start time right after posting WQEs, because the API call also costs time.
static int post_send_fast_compress(struct resources* res, int qp_index)
{
    struct ibv_send_wr	*bad_wr;
    int			rc;

    rc=ibv_post_send(res->qp[qp_index],res->sr,&bad_wr);
    //start_usec = tick(0);
    if(rc) {
        fprintf(stderr,"failed to post SR on QP:%d\n", qp_index);
        return 1;
    }

    return 0;
}

/// choose the start address of sending buffer for each WQE according to different alignment requirement, while counting the number of unaligned WQEs.
static int choose_addr_for_wqe(struct resources* res)
{
    int i=0, local_offset=0, remote_offset=0;
    for(; i < res->batch_size; ++i) {
        if(res->is_local_cacheline_aligned == 1)
            local_offset = random_offset_cache_line_aligned(res, res->local_aligned_offset);
        else 
            local_offset = random_offset(res);
        
        if(res->is_remote_cacheline_aligned == 1)
            remote_offset = random_offset_cache_line_aligned(res, res->remote_aligned_offset);
        else
            remote_offset = random_offset(res);
        
        res->sge[i].addr=(uintptr_t)res->tx_buf + local_offset;
        res->sr[i].wr.rdma.remote_addr=res->remote_props.addr + remote_offset;
        if((res->sge[i].addr % res->cache_line_size) != 0)
            res->local_cacheline_miss_counter++;
        if((res->sr[i].wr.rdma.remote_addr % res->cache_line_size) != 0)
            res->remote_cacheline_miss_counter++;
        //printf("data len : %d\n", res->sr[i].sg_list->length);
    }
}

/// high-level random post send
static int post_send_fast_random(struct resources* res, int qp_index)
{
    choose_addr_for_wqe(res);
    return post_send_fast_compress(res, qp_index);
}

/// modify the QP state to init state
static int modify_qp_to_init(struct ibv_qp* qp)
{
    struct ibv_qp_attr	qp_attr;
    int			mask;
    int 			rc;
    memset(&qp_attr,0,sizeof(qp_attr));
    qp_attr.qp_state	=IBV_QPS_INIT;
    qp_attr.pkey_index	=0;
    qp_attr.port_num	=config.ib_port;
    qp_attr.qp_access_flags	=IBV_ACCESS_REMOTE_WRITE;

    mask=IBV_QP_STATE|IBV_QP_PKEY_INDEX|IBV_QP_PORT|IBV_QP_ACCESS_FLAGS;

    rc=ibv_modify_qp(qp,&qp_attr,mask);
    if(rc) {
        fprintf(stderr,"failed to modify qp state to INIT\n");
        fprintf(stderr,"return %d\n",rc);
        return rc;
    }
    return 0;
}

/// modify the QP state to ready to receive state
static int modify_qp_to_rtr(struct resources* res,struct ibv_qp* qp,uint32_t remote_qpn)
{
    struct ibv_qp_attr	qp_attr;
    int 			mask;
    int 			rc;

    memset(&qp_attr,0,sizeof(qp_attr));

    qp_attr.qp_state	=IBV_QPS_RTR;
    qp_attr.path_mtu	=IBV_MTU_1024;
    qp_attr.dest_qp_num	=remote_qpn;
    qp_attr.max_dest_rd_atomic= 16;
    qp_attr.min_rnr_timer	=0x12;
    qp_attr.rq_psn		=0;
    qp_attr.ah_attr.is_global	=1;
    qp_attr.ah_attr.grh.sgid_index  =res->sgid_index;
    qp_attr.ah_attr.grh.traffic_class = res->traffic_class;
    qp_attr.ah_attr.grh.dgid	=res->dgid;
    qp_attr.ah_attr.port_num	=config.ib_port;

    mask=IBV_QP_STATE|IBV_QP_PATH_MTU|IBV_QP_DEST_QPN|IBV_QP_RQ_PSN|IBV_QP_AV|IBV_QP_MIN_RNR_TIMER|IBV_QP_MAX_DEST_RD_ATOMIC;

    rc=ibv_modify_qp(qp,&qp_attr,mask);
    if(rc) {
        fprintf(stderr,"failed to modify qp to rtr, errno = %d\n", rc);
        return 1;
    }
    return 0;
}

/// modify the QP state to ready to send state
static int modify_qp_to_rts(struct ibv_qp *qp)
{
    struct ibv_qp_attr	qp_attr;
    int 			mask;
    int 			rc;

    memset(&qp_attr,0,sizeof(qp_attr));
    qp_attr.qp_state	=IBV_QPS_RTS;
    qp_attr.timeout		=0x12;
    qp_attr.retry_cnt	=6;
    qp_attr.rnr_retry	=0;
    qp_attr.sq_psn		=0;
    qp_attr.max_rd_atomic   =16;

    mask=IBV_QP_STATE|IBV_QP_TIMEOUT|IBV_QP_RETRY_CNT|IBV_QP_RNR_RETRY|IBV_QP_SQ_PSN|IBV_QP_MAX_QP_RD_ATOMIC;

    rc=ibv_modify_qp(qp,&qp_attr,mask);
    if(rc) {
        fprintf(stderr,"failed to modify qp state to RTS\n");
        return rc;
    }
    return 0;
}

/// initializing all parameters to default value
static void resources_init(struct resources *res)
{
    res->qp		=NULL;
    res->cq		=NULL;
    res->sr		=NULL;
    res->cq_mod = 128;
    res->tx_depth = 128; // default
    res->verbose = 0;
    res->mem_size   =0;
    res->io_size    =0;
    res->batch_size =0;
    res->sge	=NULL;
    res->tx_buf	=NULL;
    res->rx_buf =NULL;
    res->qp_number = 1;
    res->batch_signal = 1;
    res->is_local_cacheline_aligned = 0;
    res->is_remote_cacheline_aligned = 0;
    res->local_aligned_offset = 0;
    res->remote_aligned_offset = 0;
    res->local_cacheline_miss_counter=0;
    res->remote_cacheline_miss_counter=0;
    res->cache_line_size = DEF_CACHE_LINE_SIZE;
    res->system_page_size = sysconf(_SC_PAGESIZE);
    res->use_hugetable = 0;
    res->is_using_aligned_alloc = 0;
    res->traffic_class =0;
    res->sgid_index =0;
    res->type = 0;
    res->run_infinitely = 0;
    res->duplex_mode = 0;
    res->iteration = 0;
    res->perform_warmup=0;
    res->event_or_poll=0;
    res->sock=-1;
    res->ib_ctx	=NULL;
    res->cc		=NULL;
    res->pd		=NULL;
    res->tx_mr  =NULL;
    res->rx_mr	=NULL;
}

/// create all resources for communication
static int  resources_create(struct resources *res)
{
    struct ibv_device 	**dev_list=NULL;
    int 			dev_num;
    int 			mr_flags=0;
    struct ibv_qp_init_attr	qp_init_attr;
    struct ibv_device	*ib_dev=NULL;
    int 			cq_size=0;
    struct mem_data local_data, remote_data;


    if(config.server_name) {
        res->sock=sock_client_connect(config.server_name,config.tcp_port);
        if(res->sock<0) {
            fprintf(stderr,"failed to establish TCP connection to server %s,port %d\n",config.server_name,config.ib_port);
            return -1;
        }
    } else {
        res->sock=sock_daemon_connect(config.tcp_port);
        if(res->sock<0) {
            fprintf(stderr,"failed to establish TCP connection on port %d\n",config.tcp_port);
            return -1;
        }
    }

    dev_list=ibv_get_device_list(&dev_num);

    int i=0;
    for(; i<dev_num; ++i) {
        if(!config.dev_name) {
            fprintf(stdout,"unspecified device name,use device found first\n");
            res->ib_ctx=ibv_open_device(dev_list[i]);
            break;
        }
        if(!strcmp(config.dev_name,ibv_get_device_name(dev_list[i]))) {
            ib_dev=dev_list[i];
            break;
        }
    }
    if(!ib_dev) {
        fprintf(stderr,"IB device %s wasn't found \n",config.dev_name);
        return 1;
    }

    res->ib_ctx=ibv_open_device(ib_dev);
    if(!res->ib_ctx) {
        fprintf(stdout,"couldn't get device\n");
        return -1;
    }
    ib_dev=NULL;

    if(ibv_query_port(res->ib_ctx,config.ib_port,&res->port_attr)) {
        fprintf(stderr,"ibv_query_port on port %u failed\n",config.ib_port);
        return 1;
    }
    res->pd=ibv_alloc_pd(res->ib_ctx);
    if(!res->pd) {
        fprintf(stderr,"failed to alloc pd\n");
        return -1;
    }
    res->cc = ibv_create_comp_channel(res->ib_ctx);
    cq_size= res->tx_depth;
    res->cq=ibv_create_cq(res->ib_ctx,cq_size,NULL,res->cc,0);
    if(!res->cq) {
        fprintf(stderr,"failed to create cq, cq_size:%d\n", cq_size);
        return -1;
    }

    size_t size = res->mem_size;
    printf("Register %d Bytes memory, the IO buffer size:%d Bytes\n", res->mem_size, res->io_size);
    char* func_name = NULL;
    if(res->use_hugetable == 1) {
        res->tx_buf=mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        res->rx_buf=mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        func_name = "HugeTable\0";
    } else if(res->is_using_aligned_alloc == 1) {
        res->tx_buf=memalign(res->system_page_size, size);
        res->rx_buf=memalign(res->system_page_size, size);
        func_name = "Memalign\0";
    } else {
        res->tx_buf=malloc(size);
        res->rx_buf=malloc(size);
        func_name = "Malloc\0";
    }

    uintptr_t address_value = (uintptr_t)(res->tx_buf);
    if((address_value % res->cache_line_size) != 0) {
        res->local_aligned_offset = res->cache_line_size - (address_value % res->cache_line_size);
    }
    printf("In %s, for aligning to cacheline size, the local tx buffer should offset %d bytes.\n", func_name, res->local_aligned_offset);

    if((!res->tx_buf) || (!res->rx_buf)) {
        fprintf(stderr,"failed to %s buf\n", func_name);
        return -1;
    }

    memset(res->tx_buf,'a',size);
    memset(res->rx_buf,'0',size);

    mr_flags=IBV_ACCESS_REMOTE_WRITE|IBV_ACCESS_LOCAL_WRITE;

    res->tx_mr=ibv_reg_mr(res->pd, res->tx_buf, size, mr_flags);
    res->rx_mr=ibv_reg_mr(res->pd, res->rx_buf, size, mr_flags);
    if((!res->tx_mr) || (!res->rx_mr) ) {
        fprintf(stderr,"failed to register memory region\n");
        return -1;
    }

    if(ibv_query_gid(res->ib_ctx,config.ib_port,res->sgid_index,&(res->sgid))) {
        fprintf(stderr,"failed to query gid\n");
        return -1;
    }

    gid_to_wire_gid(&(res->sgid),local_data.gid);
    local_data.addr       =htonll((uintptr_t)res->rx_buf);
    local_data.rkey       =htonl(res->rx_mr->rkey);

    if(sock_sync_data(res->sock,!config.server_name,sizeof(struct mem_data),&local_data,&remote_data)) {
        return -1;
    }

    wire_gid_to_gid(remote_data.gid,&(res->dgid));
    remote_data.addr       =ntohll(remote_data.addr);
    remote_data.rkey	   =ntohl(remote_data.rkey);

    if(res->is_remote_cacheline_aligned == 1) {
        if(remote_data.addr % res->cache_line_size != 0) 
            res->remote_aligned_offset = res->cache_line_size - (remote_data.addr % res->cache_line_size);
        printf("For aligning to cacheline size, the remote buffer should be offset %d bytes.\n", res->remote_aligned_offset);
    }

    res->remote_props      =remote_data;

    res->qp = malloc(sizeof(struct ibv_qp*) * res->qp_number);
    memset(&qp_init_attr,0,sizeof(qp_init_attr));

    qp_init_attr.send_cq=res->cq;
    qp_init_attr.recv_cq=res->cq;
    qp_init_attr.qp_type=IBV_QPT_RC;
    qp_init_attr.sq_sig_all=0;
    qp_init_attr.cap.max_send_wr=res->tx_depth;
    qp_init_attr.cap.max_recv_wr=64;
    qp_init_attr.cap.max_send_sge=1;
    qp_init_attr.cap.max_recv_sge=1;

    int qp_iter = 0;
    for(; qp_iter < res->qp_number; ++qp_iter)
    {
        res->qp[qp_iter] = NULL; // init to NULL ptr
        res->qp[qp_iter] = ibv_create_qp(res->pd, &qp_init_attr);
        if (!res->qp[qp_iter])
        {
            fprintf(stderr, "failed to create qp:%d \n", qp_iter);
            return -1;
        }
    }

    //pre allocate send wr and send sge for randomly accessing remote memory
    res->sr = (struct ibv_send_wr*)malloc(sizeof(struct ibv_send_wr) * res->batch_size);
    memset(res->sr,0,sizeof(struct ibv_send_wr) * res->batch_size);
    res->sge = (struct ibv_sge*)malloc(sizeof(struct ibv_sge) * res->batch_size);
    memset(res->sge,0,sizeof(struct ibv_sge) * res->batch_size);
    i = 0;
    int local_offset = 0, remote_offset = 0;
    for(; i < res->batch_size; ++i) {
        if(res->is_local_cacheline_aligned == 1)
            local_offset = random_offset_cache_line_aligned(res, res->local_aligned_offset);
        else 
            local_offset = random_offset(res);
        
        if(res->is_remote_cacheline_aligned == 1)
            remote_offset = random_offset_cache_line_aligned(res, res->remote_aligned_offset);
        else
            remote_offset = random_offset(res);

        res->sge[i].addr=(uintptr_t)res->tx_buf + local_offset;
        res->sge[i].length=res->io_size;
        res->sge[i].lkey=res->tx_mr->lkey;

        res->sr[i].next= (i<res->batch_size-1)?&(res->sr[i+1]):NULL;
        res->sr[i].wr_id=0;
        res->sr[i].sg_list=&(res->sge[i]);
        res->sr[i].num_sge=1;
        res->sr[i].opcode=IBV_WR_RDMA_WRITE;
        res->sr[i].send_flags = 0;
        res->sr[i].wr.rdma.remote_addr=res->remote_props.addr + remote_offset;
        res->sr[i].wr.rdma.rkey	      =res->remote_props.rkey;
        res->sr[i].send_flags = 0;
    }
    res->sr[i-1].send_flags= IBV_SEND_SIGNALED;

    return 0;
}

static int connect_qp(struct resources* res, int qp_index);

/// establish one QP by exchaning the metadata and modify the QP state. (Batch to be done)
static int establish_batch_connection(struct resources* res)
{
    int rc = 0;
    int qp_index = 0;
    for(; qp_index<res->qp_number; ++qp_index)
    {
        rc = connect_qp(res, qp_index);
        if (rc)
        {
            fprintf(stderr, "failed to connect qp:%d\n", qp_index);
            return -1;
        }
    }
    return 0;
}

/// exchaning the QP metadata and modify the QP state.
static int connect_qp(struct resources* res, int qp_index)
{
    int rc = 0;
    uint32_t local_qp_num, remote_qp_num;

    if(modify_qp_to_init(res->qp[qp_index])) {
        fprintf(stderr,"failed to modify qp to init\n");
        return -1;
    }

    local_qp_num     =htonl(res->qp[qp_index]->qp_num);

    rc=sock_sync_data(res->sock,!config.server_name,sizeof(uint32_t),&local_qp_num,&remote_qp_num);

    if(rc<0) {
        fprintf(stderr,"sock_sync_data failed\n");
        return -1;
    }

    remote_qp_num     =ntohl(remote_qp_num);

    rc=modify_qp_to_rtr(res,res->qp[qp_index],remote_qp_num);
    if(rc) {
        fprintf(stderr,"failed to modify qp to rtr\n");
        return -1;
    }

    rc=modify_qp_to_rts(res->qp[qp_index]);
    if(rc) {
        fprintf(stderr,"failed to modify qp state from reset to rts\n");
        return rc;
    }

    if(sock_sync_ready(res->sock,!config.server_name)) {
        fprintf(stderr,"sync after qps are moved to RTS\n");
        return 1;
    }
    return 0;
}

/// poll cq by sleeping on the event channel.
static int event_one_completion(struct resources* res)
{
    struct ibv_wc 		wc;
    int 			    rc=0;
    rc = ibv_req_notify_cq(res->cq,0);
    if(rc) {
        printf("Couldn't request CQ notification");
        return -1;
    }
    void* ev_ctx = NULL;
    rc = ibv_get_cq_event(res->cc,&(res->cq),&ev_ctx);
    if(rc) {
        printf("Failed to get CQ event, errno = %d\n", errno);
        return -1;
    }
    ibv_ack_cq_events(res->cq,1);
    ibv_poll_cq(res->cq,1,&wc);
    if(wc.status!=IBV_WC_SUCCESS) {
        fprintf(stderr,"got bad compeltion with status:0x%x\n",wc.status);
        return -1;
    }
    return 0;
}

/// poll cq by busy polling
static int poll_one_completion(struct resources* res)
{
    struct ibv_wc		wc;
    unsigned long		start_time_msec;
    unsigned long		cur_time_msec;
    struct 	timeval		cur_time;
    int 			poll_result;
    int 			rc=0;
    gettimeofday(&cur_time,NULL);
    start_time_msec=(cur_time.tv_sec * 1000)+(cur_time.tv_usec/1000);
    do {
        poll_result=ibv_poll_cq(res->cq,1,&wc);
        gettimeofday(&cur_time,NULL);
        cur_time_msec=(cur_time.tv_sec*1000)+(cur_time.tv_usec/1000);
    } while((poll_result==0)&&((cur_time_msec-start_time_msec)<MAX_POLL_TIMEOUT));

    if(poll_result<0) {
        fprintf(stderr,"poll cq failed\n");
        rc=1;
    } else if(poll_result==0) {
        fprintf(stderr,"completion wasn't found in the cq after timeout\n");
        rc=1;
    } else {
        if(wc.status!=IBV_WC_SUCCESS) {
            fprintf(stderr,"got bad compeltion with status:0x%x,vendor syndrome:0x%x\n",wc.status,wc.vendor_err);
            rc=1;
        }
    }
    return rc;
}

/// poll cq according to different requirements.
static int poll_completion(struct resources* res)
{
    if(res->event_or_poll == 0)
        return poll_one_completion(res);
    else
        return event_one_completion(res);
}

/// post sending 1000 x WQEs for warmup the RNIC cache
static int warm_up(struct resources* res)
{
    int i = 0;
    for( ; i < 1000; ++i) {
        post_send_fast_compress(res, 0); // during warmup, qp_index = 0
        poll_one_completion(res);
    }
}

/// destroy the resources after tests.
static int resources_destroy(struct resources *res)
{
    int rc=0;
    long a, b;
    a = tick(0);
    if(res->qp) {
        int i = 0;
        for(;i< res->qp_number; ++i)
        {
            if (ibv_destroy_qp(res->qp[i]))
            {
                fprintf(stderr, "failed to destroy qp\n");
                rc = 1;
            }
        }
    }
    if(res->cq) {
        if(ibv_destroy_cq(res->cq)) {
            fprintf(stderr,"failed to destroy cq\n");
            rc=1;
        }
    }
    if(res->tx_mr) {
        if(ibv_dereg_mr(res->tx_mr)) {
            fprintf(stderr,"failed to dereg tx mr\n");
            rc=1;
        }
    }
    
    if(res->rx_mr) {
        if(ibv_dereg_mr(res->rx_mr)) {
            fprintf(stderr,"failed to dereg rx mr\n");
            rc=1;
        }
    }

    if(res->pd) {
        if(ibv_dealloc_pd(res->pd)) {
            fprintf(stderr,"failed to dealloc pd\n");
            rc=1;
        }
    }
    if(res->ib_ctx) {
        if(ibv_close_device(res->ib_ctx)) {
            fprintf(stderr,"failed to close device\n");
            rc=1;
        }
    }
    if(res->use_hugetable == 1) {
        if(res->tx_buf)
            munmap(res->tx_buf, res->mem_size);
        if(res->rx_buf)
            munmap(res->rx_buf, res->mem_size);
    } else {
        if(res->tx_buf)
            free(res->tx_buf);
        if(res->rx_buf)
            free(res->rx_buf);
    }
    if(res->sock>=0)
        if(close(res->sock)) {
            fprintf(stderr,"failed to close device\n");
            rc=1;
        }

    if(res->sr != NULL)
        free(res->sr);
    if(res->sge != NULL)
        free(res->sge);
    b = tick(a);
    fprintf(stderr, "resource destroy cost %lld us", b);
    return rc;
}

static void usage(const char *argv0)
{
    fprintf(stdout,"Usage:0\n");
    fprintf(stdout,"%s    start a server and wait for connection\n",argv0);
    fprintf(stdout,"%s    <host> connect to server at <host>\n",argv0);
    fprintf(stdout,"\n");
    fprintf(stdout,"Options:\n");
    fprintf(stdout," -p, --port=<port> listen on /connect to port <post> (default 18515)");
}

/// run ETS flow by posting the WQE in one-by-one mode, not pipeline, computing the bandwidth
static void run_infinitely_throughput(struct resources* res)
{
    long start = 0, end = 0;
    long iter = 0;
    double base_th = 10000, GB = 1000000000;
    base_th = base_th * res->batch_size * res->io_size;
    base_th = base_th / GB;
    base_th = base_th * 8000000;
    start = tick(0);
    int qp_index = 0;
    for(; ;) {
        if(post_send_fast_random(res, qp_index % res->qp_number)) {
            fprintf(stderr,"failed to post sr\n");
            return ;
        }
        ++qp_index;
        if(poll_completion(res)) {
            fprintf(stderr,"poll completion failed\n");
            return ;
        }
        if(res->verbose == 1) {
            ++iter;
            if(iter >= 10000) {
                end = tick(start);
                double ex_th = base_th / end;
                printf("Traffic Class= %d, %d x %dB write on %dB memory achieve %f Gbps\n", res->traffic_class, res->batch_size, res->io_size, res->mem_size, ex_th);
                fflush(stdout);
                start = tick(0);
                iter = 0;
            }
        }
    }
}

/// infinitely run the Strict flow by posting the WQE in one-by-one mode, computing the latency for each WQE.
static void run_infinitely_latency(struct resources* res)
{
    int qp_index = 0;
    long start , end;
    for(; ;) {
        start = tick(0);
        if(post_send_fast_random(res, qp_index % res->qp_number)) {
            fprintf(stderr,"failed to post sr\n");
            return ;
        }
        ++qp_index;
        if(poll_completion(res)) {
            fprintf(stderr,"poll completion failed\n");
            return ;
        }
        if(res->verbose == 1) {
            end = tick(start);
            printf("Traffic Class= %d, %d x %dB write on %dB memory cost %d us\n", res->traffic_class, res->batch_size, res->io_size, res->mem_size, end_usec);
            fflush(stdout);
        }
    }
}


/// run the Strict flow by posting the WQE in one-by-one mode, computing the latency for each WQE, running N iterations.
static void run_iteration(struct resources* res)
{
    long start, end;
    printf("iteration = %d, qp_num = %d, data_size = %d\n", res->iteration, res->qp_number, res->io_size);
    struct ibv_send_wr *bad_wr;
    int qp_index = 0, batch_signal = res->batch_signal, qp_sig_index = 0;
    int outer_iter_total = res->iteration / batch_signal;
    start = tick(0);
    for(int i=0; i<outer_iter_total; ++i) {
        qp_sig_index = i % res->qp_number;
        for(int inner_iter = 0; inner_iter < batch_signal; ++inner_iter)
        {
            if(inner_iter == (batch_signal - qp_sig_index - 1))
            {
                res->sr[res->batch_size-1].send_flags= IBV_SEND_SIGNALED;
            }
            else 
                res->sr[res->batch_size-1].send_flags= 0;
            if(ibv_post_send(res->qp[qp_index], res->sr, &bad_wr))
            {
                fprintf(stderr, "failed to post SR on QP:%d, inner_iter:%d, total_iter:%d\n", qp_index, inner_iter, i);
                return ;
            }
            qp_index = (qp_index + 1) % res->qp_number; //recycle
        }

        if (poll_one_completion(res)) // poll one cqe every 32 wqes
        {
            fprintf(stderr, "poll completion failed\n");
            return;
        }
    }
    end = tick(start);
    double total_traffic = res->iteration ;
    total_traffic = total_traffic * res->io_size;
    total_traffic = total_traffic / (1024 * 1024);
    printf("Post %f MB traffic on total %d x qp cost %lld us ", total_traffic, res->qp_number, end);
    fflush(stdout);
}

/// responding to ctrl+c signal and output the statistics.
static void sig_kill(int signum)
{
    fprintf(stderr, "PID: %d got signal: %d , Transfer %d Bytes, local cacheline unaligned counter : %d, remote cacheline unaligned counter : %d \n", getpid(), signum, res.io_size, res.local_cacheline_miss_counter, res.remote_cacheline_miss_counter);
    /*if(sock_sync_ready(res.sock,!config.server_name)) {
        fprintf(stderr,"sync before end of test\n");
    }*/
    //fprintf(stderr, "after sock sync\n");
    fflush(stderr);
    exit(0);
}

/// output the critical parameters.
static void print_parameters(struct resources* res)
{
    printf("------------------------Test Parameters-------------------------------\n");
    printf("Use HugeTable = %d\n", res->use_hugetable);
    printf("Use Local Cacheline Aligned = %d\n", res->is_local_cacheline_aligned);
    printf("Use Remote Cacheline Aligned = %d\n", res->is_remote_cacheline_aligned);
    printf("-----------------------------END--------------------------------------\n");

}

int main(int argc,char* argv[])
{

    int 			rc=1;
    resources_init(&res);

    while(1) {
        int c;
        static struct option long_options[]= {
            {.name="tcp-port",.has_arg=1,	.val='p'},
            {.name="mem-size",.has_arg=1,	.val='m'},
            {.name="io-size",.has_arg=1,	.val='o'},
            {.name="batch-size",.has_arg=1, .val='b'},
            {.name="traffic-class",.has_arg=1,.val='t'},
            {.name="hugetable",.has_arg=0, .val='h'},
            {.name="local-cacheline-align",.has_arg=0, .val='L'},
            {.name="remote-cacheline-align", .has_arg=0, .val='R'},
            {.name="using-alloc-align", .has_arg=0, .val='A'},
            {.name="gid-index",.has_arg=1,	.val='g'},
            {.name="verbose", .has_arg=0, 	.val='v'},
            {.name="connection-num", .has_arg=1, .val='q'},
            {.name="batch-signal", .has_arg=1, .val='B'},
            {.name="type", .has_arg=0, .val='T'},
            {.name="iteration",.has_arg=1,	.val='I'},
            {.name="ib-dev",.has_arg=1,	.val='d'},
            {.name="duplex-mode",.has_arg=0,.val='D'},
            {.name="ib-port",.has_arg=1,	.val='i'},
            {.name="run-infinitely",.has_arg=0,.val='r'},
            {.name="warm-up",.has_arg=0,.val='w'},
            {.name="event",.has_arg=0,.val='e'},
            {.name="tx-depth",.has_arg=1,.val='P'},
            {.name=NULL,	.has_arg=0,	.val='\0'}
        };
        c=getopt_long(argc,argv,"p:m:o:b:t:g:vI:d:i:rweTP:hALRDq:B:",long_options,NULL);
        if(c==-1)
            break;
        switch(c) {
        case 'p':
            config.tcp_port=strtoul(optarg,NULL,0);
            break;
        case 'm':
            res.mem_size=strtoul(optarg,NULL,0);
            break;
        case 'o':
            res.io_size=strtoul(optarg,NULL,0);
            break;
        case 'b':
            res.batch_size=strtoul(optarg,NULL,0);
            break;
        case 'B':
            res.batch_signal=strtoul(optarg,NULL,0);
            break;
        case 'q':
            res.qp_number=strtoul(optarg, NULL, 0);
            break;
        case 't':
            res.traffic_class=strtoul(optarg,NULL,0);
            break;
        case 'g':
            res.sgid_index = strtoul(optarg,NULL,0);
            break;
        case 'v':
            res.verbose = 1;
            break;
        case 'T':
            res.type = 1;
            break;
        case 'r':
            res.run_infinitely = 1;
            break;
        case 'I':
            res.iteration = strtoul(optarg,NULL,0);
            break;
        case 'd':
            config.dev_name=strdup(optarg);
            break;
        case 'D':
            res.duplex_mode = 1;
            break;
        case 'w':
            res.perform_warmup = 1;
            break;
        case 'e':
            res.event_or_poll = 1;
            break;
        case 'h':
            res.use_hugetable = 1;
            break;
        case 'L':
            res.is_local_cacheline_aligned = 1;
            break;
        case 'R':
            res.is_remote_cacheline_aligned = 1;
            break;
        case 'A':
            res.is_using_aligned_alloc = 1;
            break;
        case 'i':
            config.ib_port=strtoul(optarg,NULL,0);
            break;
        case 'P': {
            res.tx_depth = strtoul(optarg,NULL,0);
            printf("tx_depth:%d\n", res.tx_depth);
            if(res.tx_depth >= 8)
                res.cq_mod = res.tx_depth * 0.75;
            else res.cq_mod = res.tx_depth;
        }
        break;
            //default:usage(argv[0]);
            //	return 1;
        }
    }

    if(optind==argc-1)
        config.server_name=strdup(argv[optind]);
    else if(optind<argc) {
        usage(argv[0]);
        return 1;
    }

    print_parameters(&res);
    if(resources_create(&res)) {
        fprintf(stderr,"failed to create resources\n");
        goto cleanup;
    }

    if(establish_batch_connection(&res)) {
        fprintf(stderr,"failed to connect qps\n");
        goto cleanup;
    }

    struct sigaction newact;
    newact.sa_handler = sig_kill;
    sigemptyset(&newact.sa_mask);
    newact.sa_flags = 0;
    sigaction(SIGTERM, &newact, NULL);

    if(!config.server_name) {
        if(res.perform_warmup == 1)
            warm_up(&res);
        if(res.run_infinitely == 1) {
            if(res.type == 0)
                run_infinitely_latency(&res);
            else {
#ifdef PIPELINE_MODE
                //run_infinitely_throughput_pipeline(&res);
#else
                run_infinitely_throughput(&res);
#endif
            }
        } else
            run_iteration(&res);
    } else {
        if(res.duplex_mode == 1) {
            //run_infinitely_throughput_pipeline(&res);
        }
    }

    if(sock_sync_ready(res.sock,!config.server_name)) {
        fprintf(stderr,"sync before end of test\n");
        goto cleanup;
    }
    rc=0;
cleanup:
    if(resources_destroy(&res)) {
        fprintf(stderr,"failed to destroy resources\n");
        rc=1;
    }
    if(rc == 1)
        fprintf(stdout, "\nerror exit, PID: %d transfer %d Bytes, local cacheline unaligned counter : %d, remote cacheline unaligned counter : %d \n", getpid(), res.io_size, res.local_cacheline_miss_counter, res.remote_cacheline_miss_counter);
    else 
        fprintf(stdout, "\nnormal exit, PID: %d transfer %d Bytes, local cacheline unaligned counter : %d, remote cacheline unaligned counter : %d \n", getpid(), res.io_size, res.local_cacheline_miss_counter, res.remote_cacheline_miss_counter);
    fflush(stdout);
    return rc;
}
