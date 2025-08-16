#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstring>

#include "client.hpp"
#include "../../shared/packets.hpp"
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
        0x05,  // Version
        0x01,  // Number of methods.
        0x0    // Method. No authentication required.
    };

    send(this->sockfd, method_packet, sizeof(method_packet), 0);

    int8_t method_response[2] = { 0 };
    if(Util::socket_ready_inms(this->sockfd, Util::DEFAULT_TIMEOUT)) {
        recv(this->sockfd, method_response, 2, MSG_WAITALL);
    }
    else {
        fprintf(stderr, "Could not send method packet.\n");
        return false;
    }

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

    printf("Sending connection request...\n"
            "%s\n", onion_addr);
    send(this->sockfd, connect_packet, data_size, 0);

    printf("Waiting for response...\n");
    constexpr int CONNECT_RESP_SIZE = 10;
    constexpr int CONNECT_TIMEOUT_SECONDS = 30;
    int8_t connect_response[CONNECT_RESP_SIZE] = { 0 };

    if(Util::socket_ready_inms(this->sockfd, CONNECT_TIMEOUT_SECONDS*1000)) {
        recv(this->sockfd, connect_response, CONNECT_RESP_SIZE, MSG_WAITALL);
    }
    else { 
        fprintf(stderr, "Server didnt respond in %i seconds.\n", CONNECT_TIMEOUT_SECONDS);
        return false;
    }

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

    m_server_closed = false;

    return (connect_response[1] == 0x00);
}

void Client::disconnect() {
    if(m_has_keypair) {
        Crypto::free_keypair(&m_keypair);
    }
    if(this->sockfd > 0) {
        close(this->sockfd);
        this->sockfd = -1;
        printf("Disconnected.\n");
    }
}

bool Client::verify_join(const char* nickname) {
    bool result = false;   

    printf("Waiting for verify response...\n");
    Packets::send_packet(this->sockfd, PacketID::VERIFY_JOIN, 
            { nickname, bytes_to_hexstr(m_keypair.public_key, X25519_KEYLEN) });

    if(!Util::socket_ready_inms(this->sockfd, 30*1000)) {
        log_print(ERROR, "Waited too long for server to verifyn join...");
        return false;
    }

    std::array<char, RECEIVE_MAX_SIZE> recv_data;
    if(!Packets::receive_data(this->sockfd, &recv_data)) {
        printf("< Server closed >\n");
        return false;
    }
    
    Packet packet;
    if(!Packets::parse_data(&packet, recv_data.data())) {
        log_print(ERROR, "Failed to parse data.");
        return false;
    }

    if(!packet.elements[0].empty()) {
        fprintf(stderr, "\033[31mServer: %s\033[0m\n", packet.elements[0].c_str());
    }

    return (packet.id == PacketID::JOIN_ACCEPTED);
}

void Client::m_recv_data_th__func() {
    Packet packet;
    std::array<char, RECEIVE_MAX_SIZE> recv_data;

    while(!m_threads_exit) {
        
        if(Util::socket_ready_inms(this->sockfd, 500)) {

            if(!Packets::receive_data(this->sockfd, &recv_data)) {
                m_server_closed = true;
                printf("< Server closed >\n");
                break;
            }
            
            printf("GOT:|%s|\n", recv_data.data());

            if(!Packets::parse_data(&packet, recv_data.data())) {
                continue;
            }

            switch(packet.id) {
                case PacketID::ERROR_MESSAGE:
                    if(packet.num_elements > 0) {
                        fprintf(stderr, "\033[31mServer: %s\033[0m\n", packet.elements[0].c_str());
                    }
                    break;

                case PacketID::JOIN_ACCEPTED:
                    break;

                case PacketID::JOIN_DENIED:
                    break;

                    // Message from another client.
                case PacketID::MESSAGE:
                    break;
            }

            packet.clear();
            //m_msg_queue_mutex.lock();
            //m_msg_queue_mutex.unlock();
        }

        std::this_thread::sleep_for(
                std::chrono::milliseconds(500));
    }
}

void Client::generate_keys() {
    if(m_has_keypair) {
        Crypto::free_keypair(&m_keypair);
    }
    m_keypair = Crypto::new_keypair_x25519();
    m_has_keypair = true;
}


void Client::m_send_test() {

    std::string message = "the message reads here, or does it?";



}

void Client::enter_interaction_loop() {
    
    m_recv_data_th = std::thread(&Client::m_recv_data_th__func, this);

    while(true) {
        // Temporary for testing purposes.

        char input_buf[16+1] = { 0 };
        size_t read_len = read(1, input_buf, 16);

        if(read_len <= 1) {
            continue;
        }

        if(strcmp(input_buf, "end\n") == 0) {
            m_threads_exit = true;
            break;
        }
        else
        if(strcmp(input_buf, "send\n") == 0) {
            m_send_test();
        }



    }

    printf("Disconnecting...\n");
    m_threads_exit = true;
    m_recv_data_th.join();
}




