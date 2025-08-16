#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>

#include <cstring>
#include <chrono>

#include "server.hpp"
#include "../../shared/log.hpp"
#include "../../shared/util.hpp"
//#include "../../shared/packets.hpp"



bool Server::start(const ServerConfig& config) {
    m_config = config;

    this->sockfd = -1;
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        log_print(ERROR, "Failed to open socket.");
        return false;
    }

    bzero(&this->addr, sizeof(this->addr));

    this->addr.sin_family = AF_INET;
    this->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this->addr.sin_port = htons(config.port);

    socklen_t opt = 1;
    setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int bind_result = 
        bind(this->sockfd, (struct sockaddr*)&this->addr, sizeof(this->addr));

    if(bind_result != 0) {
        log_print(ERROR, "Failed to bind name to socket.");
        log_print(ERROR, strerror(errno));
        close(this->sockfd);
        this->sockfd = -1;
        return false;
    }



    constexpr int queue_size = 5;
    int listen_result = listen(this->sockfd, queue_size);
    if(listen_result != 0) {
        log_print(ERROR, "Failed to listen to socket. Trying again in 3 seconds...");
        log_print(ERROR, strerror(errno));
        close(this->sockfd);
        this->sockfd = -1;
        return false;
    }

    printf("Server is listening on port %i\n", config.port);

    m_threads_exit = false;
    m_accept_clients_th = std::thread(&Server::m_accept_clients_th__func, this);
    m_receive_th        = std::thread(&Server::m_receive_th__func, this);

    while(true) {

        printf("\n");
        printf("> ");
        fflush(stdout);

        // TEMPORARY FOR TESTING STUFF.
        char buf[16+1] = { 0 };
        ssize_t read_len = read(1, buf, 16);

        if(read_len > 1) {
            if(strcmp(buf, "end\n") == 0) {
                break;
            }
            else
            if(strcmp(buf, "online\n") == 0) {
                printf("Clients online: %li / %i\n", m_clients.size(), m_config.max_clients);
            }
            else {
                printf("Unknown command. Commands are:\n"
                        "'end'      : shutdown server.\n"
                        "'online'   : number of clients online\n"
                        );
            }
        }
        // ----------------
    }

    printf("Closing threads...\n");
    m_threads_exit = true;

    m_accept_clients_th.join();
    m_receive_th.join();

    return true;
}


void Server::stop() {
    printf("Server shutdown...\n");

    for(size_t i = 0; i < m_clients.size(); i++) {
        close(m_clients[i].fd);
    }

    if(this->sockfd > 0) {
        close(this->sockfd);
        printf("Socket closed.\n");
    }
}

// TODO: Optimize thread delays.
//      - make them slower if there is no traffic in X amout of seconds / minutes.
//      - make them faster if there is alot of traffic.

void Server::m_receive_th__func() {
    std::array<char, RECEIVE_MAX_SIZE> recv_data;
    Packet packet;
    // TODO:
    // * clear recv_data after use.
    //
    while(!m_threads_exit) {

        m_clients_mutex.lock();
        for(ServerClient& sc : m_clients) {

            if(sc.fd < 0) {
                continue;
            }
            if(!Util::socket_ready_inms(sc.fd, 100)) {
                continue;
            }

            if(!Packets::receive_data(sc.fd, &recv_data)) {
                close(sc.fd);
                sc.fd = -1;
                printf("Client disconnected.\n");
                continue;
            }

            printf("\033[90mGOT: %s\033[0m\n", recv_data.data());

            if(!Packets::parse_data(&packet, recv_data.data())) {
                continue;
            }

            if(!sc.accepted && (packet.id != PacketID::VERIFY_JOIN)) {
                Packets::send_packet(sc.fd, PacketID::ERROR_MESSAGE, { "PLEASE VERIFY" });
                continue;
            }

            printf("ID = %i\n", packet.id);
            printf("Elements = %i\n", packet.num_elements);
            for(size_t i = 0; i < packet.num_elements; i++) {
                printf("%li -> %s\n", i, packet.elements[i].c_str());
            }

            switch(packet.id) {

                case PacketID::VERIFY_JOIN:
                    if(!m_verify_client_join(sc, packet)) {
                        continue;
                    }
                    break;

                case PacketID::MESSAGE:
                    break;

                case PacketID::ASKING_PUBLIC_KEYS:
                    break;
            }
        
        }

        m_clients_mutex.unlock();
        std::this_thread::sleep_for(
                std::chrono::milliseconds(100));
    }
}
        
bool Server::m_verify_client_join(ServerClient& sc, const Packet& packet) {
    if(packet.num_elements != 2) {
        Packets::send_packet(sc.fd, PacketID::ERROR_MESSAGE, { "REQUEST NOT ACCEPTED" });
        return false;
    }
    
    const std::string& name = packet.elements[0];
    if(name.size() > 24) {
        Packets::send_packet(sc.fd, PacketID::ERROR_MESSAGE, { "NICKNAME TOO LONG" });
        return false;
    }

    if(name.empty()) {
        uint8_t randbytes[2] = { 0 };
        RAND_bytes(randbytes, 2);
        sc.nickname = "unknown-";
        for(int i = 0; i < 2; i++) {
            sc.nickname += std::to_string(randbytes[i]);
        }
    }
    else {
        sc.nickname = name;
    }

    printf("%s: %s OK.\n", __func__, sc.nickname.c_str());
    sc.accepted = true;
    sc.public_key_hexstr = packet.elements[1];
    Packets::send_packet(sc.fd, PacketID::JOIN_ACCEPTED, {});
    return true;
}

void Server::m_accept_clients_th__func() {
    while(!m_threads_exit) {
        int client_fd = 0;
        struct sockaddr_in client;
        socklen_t socklen;
      

        if(Util::socket_ready_inms(this->sockfd, 1000)) {
            client_fd = accept(this->sockfd, (struct sockaddr*)&client, &socklen);

            if(client_fd < 0) {
                log_print(ERROR, "Failed to accept client.");   
            }

            printf("Client connected!\n");
            ServerClient server_client(client_fd);

            m_clients_mutex.lock();
            m_clients.push_back(server_client);
            m_clients_mutex.unlock();
        }
        
        std::this_thread::sleep_for(
                std::chrono::milliseconds(500));
    }
}



