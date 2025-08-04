#ifndef E2EEP_SERVER_HPP
#define E2EEP_SERVER_HPP

#include <vector>
#include <cstdint>
#include <mutex>
#include <thread>
#include <atomic>
//#include <netinet/in.h>

#include "server_client.hpp"
#include "../../shared/ext/libsam3.h"


struct ServerConfig {
    uint32_t max_clients;
    int port;

    std::string welcome_msg;
};

class Server {
    public:

        bool start(const ServerConfig& config);
        void stop();

        Sam3Session session;
        //int sockfd;
        //struct sockaddr_in addr;


    private:


        ServerConfig              m_config;
        std::vector<ServerClient> m_clients;
        std::mutex                m_clients_mutex;

        std::atomic<bool> m_threads_running { true };

        std::thread  m_accept_clients_th;
        void         m_accept_clients_th__func();

        std::thread  m_packetrecv_th;
        void         m_packetrecv_th__func();


        bool m_verify_client_name(Sam3Connection* conn, const std::string& name);
};





#endif
