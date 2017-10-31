#define MAX_ROUTE_INFO 2
#define MAX_ARP_SIZE  2
#define MAX_DEVICE 2
#define MAX_BUFFER 2048

struct route_item
{
    char destination[16];
    char gateway[16];
    char netmask[16];
    char interface[16];
}route_info[MAX_ROUTE_INFO]

int route_item_index=0;

struct arp_table_item
{
    char ip_addr[16];
    char mac_addr[18];
}arp_table[MAX_ARP_SIZE];

int arp_item_index=0;

struct device_item
{
    char interface[14];
    char mac_addr[18];
}device[MAX_DEVICE];

int device_index=0;