#ifndef E2EEP_CLIENT_HPP
#define E2EEP_CLIENT_HPP

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <netinet/in.h>
#include "../../shared/cryptography.hpp"

class Client {
    public:

        Client() {
            m_has_keypair = false;
        }

        bool connect2proxy();
        bool connect2server(const char* onion_addr, uint16_t port);
        void disconnect();
        bool verify_join(const char* nickname);

        void generate_keys();
        void enter_interaction_loop();

        int sockfd;
        struct sockaddr_in proxyaddr;


    private:

        void         m_recv_data_th__func();
        std::thread  m_recv_data_th;
        
        std::vector<std::string> m_msg_queue;
        std::mutex               m_msg_queue_mutex;

        std::atomic<bool>        m_threads_exit;
        std::atomic<bool>        m_server_closed;
        bool m_has_keypair;
        Crypto::Keypair m_keypair;

        void m_send_test();
};




#endif
