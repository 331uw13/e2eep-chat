#ifndef E2EEP_CLIENT_HPP
#define E2EEP_CLIENT_HPP


#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "../../shared/ext/libsam3.h"


struct Message {
    int name_color;
    std::string message;
};


class Client {
    public:

        bool connect2server(const char* destination, const char* nickname);
        void disconnect();

        void enter_loop();


        bool session_open;

        Sam3Session session;
        Sam3Connection* conn;
        //int sockfd;
        //struct sockaddr_in proxyaddr;

        int term_width;
        int term_height;

    private:

        std::thread          m_msgrecv_th;
        void                 m_msgrecv_th__func();

        std::mutex           m_msgbuf_mutex;
        std::vector<Message> m_msgbuf;

        void m_draw_tui();

        std::string m_input;
        bool m_running;

        std::atomic<bool> m_threads_running { true };
};




#endif
