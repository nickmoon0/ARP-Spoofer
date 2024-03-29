#include "Session.h"

#include <arpa/inet.h>

#include <cstring>
#include <iostream>

#include <linux/if_packet.h>

#include <net/ethernet.h>
#include <netinet/ip.h>

#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>

/*
 * Constructor
 */

Session::Session(std::string if_name, std::string target_mac, std::string target_ip, std::string sender_mac, std::string sender_ip)
{
    try
    {
        this->interface = new EthInterface(if_name.c_str());
        this->sniffer = new Sniffer(interface->get_if_index());

        // If interface and sniffer created successfully
        printInterface();

        this->target_ip = target_ip;
        this->target_mac = target_mac;
        this->sender_ip = sender_ip;
        this->sender_mac = sender_mac;
    }
    catch (std::runtime_error e)
    {
        throw e;
    }
}

Session::~Session()
{
    delete interface;
    delete sniffer;
}

/*
 * Methods
 */

// Start session
void Session::start()
{
    ARP_Packet* ap;
    unsigned char frame[ARP_Packet::ARP_SIZE];

    bool sessionRunning = true;
    while (sessionRunning)
    {
        // Clear frame buffer and receive data
        memset(frame, 0, sizeof(frame));
        sniffer->receiveData(frame);

        // Create packet with received data and transmit response
        try
        {
            ap = new ARP_Packet(frame, interface->get_if_mac());

            // If false, DONT filter frame
            if (!filterFrame(ap->getArpReq()))
            {
                // Send response before printing to reduce delay
                sendResponse(ap->getArpRes());

                // Print out ARP frame data
                std::cout << "---------------------" << std::endl;
                std::cout << std::endl << "Received ARP request:" << std::endl;
                ARP_Packet::printArpHeader(ap->getArpReq());
                std::cout << std::endl << "Sent ARP response:" << std::endl;
                ARP_Packet::printArpHeader(ap->getArpRes());
                std::cout << std::endl;
            }

            delete ap;
        }
        catch (std::runtime_error e)
        {
            throw e;
        }
    }
}

void Session::sendResponse(struct arp_header* arpHeader)
{
    // create an ethernet header big enough to encapsulate arp response
    int arpSize = 28;
    u_int8_t ethHeader[ARP_Packet::ARP_SIZE];
    
    struct sockaddr_ll address = {0};

    int sock;
    int bytes;
    
    memcpy(ethHeader, arpHeader->target_mac, HARDWARE_LENGTH * sizeof(u_int8_t));
    memcpy(ethHeader + HARDWARE_LENGTH, arpHeader->sender_mac, HARDWARE_LENGTH * sizeof(u_int8_t));
    ethHeader[2 * HARDWARE_LENGTH] = ETH_P_ARP / 256;
    ethHeader[2 * HARDWARE_LENGTH + 1] = ETH_P_ARP % 256;

    // Encapsulate ARP header
    memcpy(&ethHeader[ETH_HEADER_LEN], arpHeader, sizeof(arp_header));

    // Fill out address struct (sockaddr_ll)
    address.sll_family = AF_PACKET;
    address.sll_ifindex = interface->get_if_index();
    address.sll_halen = htons(HARDWARE_LENGTH);
    memcpy(address.sll_addr, arpHeader->sender_mac, HARDWARE_LENGTH * sizeof(u_int8_t));

    // Create socket to send data
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sock < 0)
    {
        throw std::runtime_error("Failed to create socket");
    }

    bytes = sendto(sock, ethHeader, ARP_Packet::ARP_SIZE, 0, (struct sockaddr*)&address, sizeof(address));
    if (bytes <= 0)
    {
        throw std::runtime_error("Failed to send response");
    }

    close(sock);
}

// If false, DONT filter frame
bool Session::filterFrame(struct arp_header* arpReq)
{
    char* ip_addr;
    char* mac_addr;

    // Check if source IP matches
    ip_addr = convertIP(arpReq->sender_ip);
    if (!strcmp((const char*)ip_addr, sender_ip.c_str()))
    {
        // Didnt get filtered
        return false;
    }
    free(ip_addr);

    // Check if source MAC matches
    mac_addr = convertMAC(arpReq->sender_mac);
    if (!strcmp(mac_addr, sender_mac.c_str()))
    {
        // Didnt get filtered
        return false;
    }
    free(mac_addr);

    // Check if target IP matches
    ip_addr = convertIP(arpReq->target_ip);
    if (!strcmp((const char*)ip_addr, target_ip.c_str()))
    {
        // Didnt get filtered
        return false;
    }
    free(ip_addr);

    // Got filtered
    return true;
}

char* Session::convertIP(u_int8_t ip[])
{
    int addressSize = PROTOCOL_LENGTH * 3 + 4;
    char* ip_addr = (char*)malloc(addressSize);
    snprintf(ip_addr, addressSize, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    return ip_addr;
}

char* Session::convertMAC(unsigned char* mac)
{
    int addressSize = HARDWARE_LENGTH * 2 + 6;
    char *mac_addr = (char*)malloc(addressSize);
    snprintf(mac_addr, addressSize, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return mac_addr;
}

// Just to print the interface details for the user
void Session::printInterface()
{
    unsigned char* mac;

    // Use 2 spaces instead of tab. Looks neater this way
    std::cout << "Interface: " << interface->get_if_name() << std::endl;
    std::cout << "  index: " << interface->get_if_index() << std::endl;
    std::cout << "  ip: " << interface->get_if_ip() << std::endl;
    
    mac = interface->get_if_mac();
    std::cout << "  mac: ";
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    std::cout << std::endl;
}