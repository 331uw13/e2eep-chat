#ifndef E2EEP_SERVER_HPP
#define E2EEP_SERVER_HPP

#include <vector>
#include <cstdint>
#include <mutex>
#include <thread>
#include <atomic>
#include <netinet/in.h>

#include "server_client.hpp"



struct ServerConfig {
    uint32_t max_clients;
    int port;
};


class Server {
    public:

        bool start(const ServerConfig& config);
        void stop();

        int sockfd;
        struct sockaddr_in addr;




    private:
        ServerConfig              m_config;
        std::vector<ServerClient> m_clients;
        std::mutex                m_clients_mutex;

        std::thread  m_accept_clients_th;
        void         m_accept_clients_th__func();

        std::thread  m_receive_th;
        void         m_receive_th__func();

        std::atomic<bool>   m_threads_exit;
};





#endif
