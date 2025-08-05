#ifndef E2EEP_SERVER_HPP
#define E2EEP_SERVER_HPP

#include <vector>
#include <cstdint>
#include <mutex>
#include <thread>
#include <atomic>
#include <map>
//#include <netinet/in.h>

#include "server_client.hpp"
#include "../../shared/util.hpp"
#include "../../shared/session_token.hpp"
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
        
        bool is_good_packet(const Packet& packet);
        bool is_safe_msg(const std::string& data);

        void broadcast(const std::string& msg);

    private:

        ServerConfig              m_config;
        std::vector<ServerClient> m_clients;
        std::mutex                m_clients_mutex;

        std::atomic<bool> m_threads_running { true };

        std::thread  m_accept_clients_th;
        void         m_accept_clients_th__func();

        std::thread  m_packetrecv_th;
        void         m_packetrecv_th__func();

        std::map<std::string, token_t>  m_name_token_map;

        bool m_verify_client_name(Sam3Connection* conn, const std::string& name);


        // ---- Private functions ----
};





#endif
