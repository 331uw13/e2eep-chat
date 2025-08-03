#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstring>

#include "client.hpp"
#include "../../shared/log.hpp"
#include "../../shared/util.hpp"


#define SOCKS5_PROXY_IP "127.0.0.1"
#define SOCKS5_PROXY_PORT 9050




bool Client::connect2proxy() {

    printf("SOCKS5 Proxy address: %s\n", SOCKS5_PROXY_IP);
    printf("SOCKS5 Proxy port: %i\n", SOCKS5_PROXY_PORT);

    this->proxyaddr.sin_family = AF_INET;
    this->proxyaddr.sin_port = htons(SOCKS5_PROXY_PORT);

    // Convert IPv4 address to binary form.
    inet_pton(AF_INET, SOCKS5_PROXY_IP, &this->proxyaddr.sin_addr);

    this->sockfd = -1;
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(this->sockfd < 0) {
        log_print(ERROR, "Failed to open socket.");
        return false;
    }


    int connect_result =
        connect(this->sockfd, 
                (struct sockaddr*)&this->proxyaddr,
                sizeof(this->proxyaddr));

    if(connect_result != 0) {
        log_print(ERROR, "Failed to connect to proxy.");
        close(this->sockfd);
        return false;
    }

    return true;
}


bool Client::connect2server(const char* onion_addr, uint16_t port) {
    printf("Trying to connect to server with port %i\n", port);
    if(!onion_addr) {
        log_print(ERROR, "Empty onion address is not valid.");
        return false;
    }

    size_t onion_addr_len = strlen(onion_addr);
    if(onion_addr_len < 60) {
        log_print(ERROR, "Invalid onion address.");
        return false;
    }
    

    uint8_t method_packet[3] = {
        0x05, // Version
        0x01,  // Number of methods.
        0x0   // Method. No authentication required.
    };

    send(this->sockfd, method_packet, sizeof(method_packet), 0);

    int8_t method_response[2] = { 0 };
    if(Util::wait_for_socket(this->sockfd, 
                Util::DEFAULT_TIMEOUT, Util::TimeoutAct::IS_ERROR)) {
        recv(this->sockfd, method_response, 2, MSG_WAITALL);
    }
    else { return false; }

    /*
    if(!Util::wait_for_data(
                this->sockfd,
                method_response, 2,
                Util::DEFAULT_TIMEOUT)) {
        return false;
    }
    */

    if(method_response[1] != 0) {
        log_print(ERROR, "Proxy method selection failed.\n");
        return false;
    }

    printf("Proxy method response is good.\n");

    constexpr uint32_t CONNECT_PACKET_SIZE = 512;
    int8_t connect_packet[CONNECT_PACKET_SIZE] = {
        0x05,  // Version.
        0x01,  // CMD: Connect.
        0x0,   // <Reserved>
        0x03,  // Address type: Domain.
        (int8_t)onion_addr_len
    };

    uint32_t data_size = 5;

    memmove(connect_packet + data_size,
            onion_addr,
            onion_addr_len);
    data_size += onion_addr_len;

    if(data_size+2 > CONNECT_PACKET_SIZE) {
        log_print(ERROR, "Too much data for connection request.");
        return false;
    }

    connect_packet[data_size++] = (port >> 8) & 0xFF;
    connect_packet[data_size++] = (port)      & 0xFF;

    printf("Sending connection request...\n");
    send(this->sockfd, connect_packet, data_size, 0);

    printf("Waiting for response...\n");
    int8_t connect_response[10] = { 0 };
    if(Util::wait_for_socket(this->sockfd, 
                Util::DEFAULT_TIMEOUT, Util::TimeoutAct::IS_ERROR)) {
        recv(this->sockfd, connect_response, 10, MSG_WAITALL);
    }
    else { return false; }
    /*
    if(!Util::wait_for_data(
                this->sockfd,
                connect_response, 10,
                Util::DEFAULT_TIMEOUT)) {
        return false;
    }*/

    //uint8_t connect_response[10] = { 0 };
    //recv(this->sockfd, connect_response, 10, 0);
   
    switch(connect_response[1]) {
        case 0x00:
            printf("Connected.\n");
            break;
        case 0x01:
            printf("General SOCKS server failure.\n");
            break;
        case 0x02:
            printf("Connection not allowed by ruleset\n");
            break;
        case 0x03:
            printf("Network unreachable.\n");
            break;
        case 0x04:
            printf("Host unreachable.\n");
            break;
        case 0x05:
            printf("Connection refused.\n");
            break;
        case 0x06:
            printf("TTL expired.\n");
            break;
        case 0x07:
            printf("Command not supported.\n");
            break;
        case 0x08:
            printf("Address type not supported.\n");
            break;
    }

    return (connect_response[1] == 0x00);
}


void Client::disconnect() {
    if(this->sockfd > 0) {
        close(this->sockfd);
        this->sockfd = -1;
        printf("Disconnected.\n");
    }
}



