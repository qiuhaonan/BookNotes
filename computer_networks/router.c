#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include "head.h"

route_info=[
    {.destination="192.168.2.0",.gateway="192.168.2.1",.netmask="255.255.255.0",.interface="192.168.2.1"},
    {.destination="192.168.3.0",.gateway="192.168.3.1",.netmask="255.255.255.0",.interface="192.168.3.1"}
    ];
route_item_index=2;
arp_table=[
    {.ip_addr="192.168.2.3",.mac_addr=""},
    {.ip_addr="192.168.2.4",.mac_addr=""}
];
arp_item_index=2;
device=[
    {.interface="192.168.2.1",.mac_addr=""},
    {.interface="192.168.2.2",.mac_addr=""}
];
device_index=2;

int get_repost_interface(const char* dest)
{
    int index=0;
    in_addr_t dest_i=inet_addr(dest);
    for(;index<route_item_index;++index)
    {
        in_addr_t netmask=inet_addr(route_item[index].netmask);
        in_addr_t cmp_dest=inet_addr(route_item[index].destination);
        in_addr_t result=dest_i&netmask;
        if(result==cmp_dest)
        {
            printf("find target interface!\n");
            return index;
        }
    }
    printf("error:couldn't find target interface!\n");
    return -1;
}

int get_mac_addr(const char* interface_target)
{
    int index=0;
    for(;index<arp_item_index;++index)
    {
        if(!strcmp(interface_target,arp_table[index].interface))
        {
            printf("find target interface!\n");
            return index;
        }
    }
    printf("error:couldn't find target interface\n");
    return -1;
}

int main(int argc,char* argv[])
{
    int sockfd;
    int len;
    int proto;
    char buffer[BUFFER_MAX];
    char* eth_head;
    int n_send;

    unsigned char *p;
    struct sockaddr_in connection;

    sockfd=socket(PF_SOCKET,SOCK_RAW,ETH_P_ALL);
    if(sockfd<0)
    {
        printf("%d\n",sockfd);
        printf("error :create raw socket\n");
        return -1;
    }

    while(1)
    {
        len=recvfrom(sockfd,buffer,2048,0,NULL,NULL);
        if(len<42)
        {
            printf("error when recv msg\n");
            return -1;
        }
        eth_head=buffer;
        p=eth_head;

        //modify the destination MAC address

        //connect 
        connection.sin_family=AF_INET;
        connection.sin_addr.s_addr=inet_addr("192.168.2.2");
        connection.sin_addr.sin_port=htons();

        n_send=sendto(sockfd,p,len,0,(struct sockaddr*)&connection,sizeof(struct sockaddr));
        if(n_send<0)
        {
            printf("sendto error");
        }
    }
}