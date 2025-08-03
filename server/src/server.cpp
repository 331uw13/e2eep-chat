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
            else
            if(strcmp(buf, "msgs\n") == 0) {
                printf("no messages.\n");
            }
            else {
                printf("Unknown command. Commands are:\n"
                        "'end'      : shutdown server.\n"
                        "'online'   : number of clients online\n"
                        "'msgs'     : read messages from clients\n"
                        );
            }
        }
        // ----------------
    }

    printf("Closing threads...\n");
    m_mutex.lock();
    m_threads_exit = true;
    m_mutex.unlock();

    m_accept_clients_th.join();

    return true;
}


void Server::stop() {
    printf("Server shutdown...\n");

    if(this->sockfd > 0) {
        close(this->sockfd);
        printf("Socket closed.\n");
    }
}


void Server::m_accept_clients_th__func() {
    while(true) {
        m_mutex.lock();
        if(m_threads_exit) {
            break;
        }
        //printf("\n%s\n", __func__);

        int client_fd = 0;
        struct sockaddr_in client;
        socklen_t socklen;
        
        if(Util::wait_for_socket(this->sockfd, 3, Util::TimeoutAct::DO_NOTHING)) {
            client_fd = accept(this->sockfd, (struct sockaddr*)&client, &socklen);

            if(client_fd < 0) {
                log_print(ERROR, "Failed to accept client.");   
            }

            ServerClient server_client(client_fd, "test_user");
            m_clients.push_back(server_client);
        }
        
        m_mutex.unlock();
        std::this_thread::sleep_for(
                std::chrono::milliseconds(1000));
    }
}



