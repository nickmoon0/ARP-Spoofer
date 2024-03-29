#ifndef ARP_PACKET_H
#define ARP_PACKET_H

#include <sys/types.h>

#define HARDWARE_LENGTH 6
#define PROTOCOL_LENGTH 4
#define ETH_HEADER_LEN 14

// ARP header used to send requests
struct arp_header
{
    u_int16_t htype;
    u_int16_t ptype;
    u_int8_t hlen;
    u_int8_t plen;
    u_int16_t opcode;
    u_int8_t sender_mac[HARDWARE_LENGTH];
    u_int8_t sender_ip[PROTOCOL_LENGTH];
    u_int8_t target_mac[HARDWARE_LENGTH];
    u_int8_t target_ip[PROTOCOL_LENGTH];
} typedef arp_header;

// Used to store and parse data
class ARP_Packet
{
public:

    static void printArpHeader(struct arp_header* header);
    static void printAddress(u_int8_t address[], int addressSize);

    // Size in bytes
    const static int ARP_SIZE = ETH_HEADER_LEN + sizeof(arp_header);
    
    ARP_Packet(unsigned char* packet, unsigned char* local_mac);
    ~ARP_Packet();

    arp_header* getArpRes();
    arp_header* getArpReq();

private:

    void parseAddresses(unsigned char* packet, unsigned char* local_mac);
    
    void createArpReq(unsigned char* packet);
    void createArpRes();

    u_int16_t htype;
    u_int16_t ptype;
    u_int8_t hlen;
    u_int8_t plen;
    u_int16_t opcode;
    u_int8_t sender_mac[HARDWARE_LENGTH];
    u_int8_t sender_ip[PROTOCOL_LENGTH];
    u_int8_t target_mac[HARDWARE_LENGTH];
    u_int8_t target_ip[PROTOCOL_LENGTH];

    struct arp_header* arpRes;
    struct arp_header* arpReq;
};

#endif
