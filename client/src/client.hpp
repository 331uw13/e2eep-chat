#ifndef E2EEP_CLIENT_HPP
#define E2EEP_CLIENT_HPP


#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "../../shared/session_token.hpp"
#include "../../shared/ext/libsam3.h"


static constexpr char COMMAND_PREFIX = '/';


struct Message {
    int name_color;
    std::string message;
};


class Client {
    public:

        // 'destination':  server's public key.
        // 'nickname':     user's name displayed to the server and other clients.
        bool connect2server(const char* destination, const char* nickname);
        void disconnect();

        // Handle client interaction with server.
        void enter_interaction_loop();
        bool running;

        bool session_open;

        Sam3Session session;
        Sam3Connection* conn;

        // Terminal width and height.
        int term_width;
        int term_height;

        // When set to 'true'
        // "interaction loop" will redraw everything as needed
        // can be used when new data has arrived.
        // the value is set to 'false' after redraw is done.
        std::atomic<bool> redraw { false };


        token_t       session_token;
        std::string   name;

    private:


        std::thread          m_msgrecv_th;
        void                 m_msgrecv_th__func();

        std::mutex           m_msgbuf_mutex;
        std::vector<Message> m_msgbuf;
        std::string          m_input;

        std::atomic<bool>    m_threads_running { true };
        
        // --- Private functions ---
        void m_draw_tui();
};




#endif
