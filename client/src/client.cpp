#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ncurses.h>

#include <cstring>

#include "client.hpp"
#include "commands.hpp"
#include "../../shared/log.hpp"
#include "../../shared/util.hpp"
#include "../../shared/colors.hpp"



bool Client::connect2server(const char* destination, const char* nickname) {
    constexpr int DESTKEY_SIZE = 616;
    char destkey[DESTKEY_SIZE+1] = { 0 };

    FILE* file = fopen(destination, "rb");
    if(file != NULL) {
        bool read_ok = (fread(destkey, DESTKEY_SIZE, 1, file) != 1);
        fclose(file);

        if(!read_ok) {
            log_print(ERROR, "Failed to read '%s'", destination);
            return false;
        }
    }


    if(!sam3CheckValidKeyLength(destkey)) {
        log_print(ERROR, "'%s' has invalid key length!",
                destination);
        return false;
    }

    printf("Creating session...\n");
    if(sam3CreateSilentSession(&this->session,
                SAM3_HOST_DEFAULT,
                SAM3_PORT_DEFAULT,
                SAM3_DESTINATION_TRANSIENT,
                SAM3_SESSION_STREAM,
                EdDSA_SHA512_Ed25519, NULL) < 0) {
        log_print(ERROR, "Failed to create session.");
        return false;
    }

    this->session_open = true;

    printf("Connecting...\n");
    this->conn = NULL;
    this->conn = sam3StreamConnect(&this->session, destkey);
    if(!this->conn) {
        log_print(ERROR, "Failed to connect. %s", this->session.error);
        return false;
    }



    // Server will ask for user nickname.
   
    PacketID r_id = PacketID::EMPTY;
    if(!Util::receive_idnmsg(this->conn->fd, &r_id, NULL)) {
        log_print(ERROR, "Server didnt ask for nickname?. %s", this->conn->error);
        return false;
    }

    if(r_id != PacketID::ASKING_NICKNAME) {
        log_print(ERROR, "Expected 0x%x. got 0x%x",
                PacketID::ASKING_NICKNAME, r_id);
        return false;
    }


    Util::send_simple_packet(this->conn->fd, PacketID::NICKNAME, nickname);
    printf("\033[90m(server is validating nickname...)\033[0m\n");

    PacketID name_resp_id = PacketID::EMPTY;
    std::string name_resp_str = ""; // NOTE: Probably want this outside of this scope.
    if(!Util::receive_idnmsg(this->conn->fd, &name_resp_id, &name_resp_str)) {
        log_print(ERROR, "Could not receive name verification response.");
        return false;
    }

    if(name_resp_id == PacketID::NICKNAME_BAD) {
        printf("[Server]: %s\n", name_resp_str.c_str());
        return false;
    }
  

    // The session token should be in the name response.
    if(name_resp_str.size() != TOKEN_SIZE) {
        log_print(ERROR, "Session token size doesnt match expected size. Try reconnecting?");
        log_print(ERROR, "Expected size = %li, got %li", TOKEN_SIZE, name_resp_str.size());
        return false;
    }

    for(size_t i = 0; i < this->session_token.size(); i++) {
        this->session_token[i] = (uint8_t)name_resp_str[i];
    }

    name = nickname;
    m_msgrecv_th = std::thread(&Client::m_msgrecv_th__func, this);


    this->running = true;
    return true;
}


void Client::disconnect() {
    printf("Disconnecting...\n");
    
    m_threads_running = false;
    m_msgrecv_th.join();
    
    if(this->conn) {
        sam3CloseConnection(this->conn);
        printf("Connection closed.\n");
        this->conn = NULL;
    }
    if(this->session_open) {
        sam3CloseSession(&this->session);
        printf("Session closed.\n");
        this->session_open = false;
    }
}


void Client::m_msgrecv_th__func() {
    std::string msg = "";
    msg.reserve(1024);
    while(m_threads_running) {

        msg.clear();
        PacketID p_id = PacketID::EMPTY;

        if(!Util::socket_ready_inms(this->conn->fd, 200, Util::TimeoutAct::DO_NOTHING)) {
            goto skip_msg;
        }
        if(!Util::receive_idnmsg(this->conn->fd, &p_id, &msg)) {
            log_print(ERROR, "Failed to receive data. %s", this->conn->error);
            goto skip_msg;
        }

        //log_print(INFO, "> |%s|", msg.c_str());
        // Client received a packet from the server...

        switch(p_id) {
            case PacketID::MESSAGE:
                m_msgbuf_mutex.lock();
                m_msgbuf.push_back((Message) { .name_color = Color::WHITE, .message = msg });
                m_msgbuf_mutex.unlock();
                this->redraw = true;
                break;

        }

skip_msg:
        std::this_thread::sleep_for(
                std::chrono::milliseconds(500));
    }
}


#define USE_COLOR(c) attron(COLOR_PAIR(c))




void Client::m_draw_tui() {
    
    getmaxyx(stdscr, this->term_height, this->term_width);
    const int input_x = 2;
    const int input_y = this->term_height - 2;
    move(input_y, input_x);
    clrtoeol();

    USE_COLOR(Color::DARK_BEIGE_0);
    wborder(stdscr, '|', '|', '=', '=', '+', '+', '+', '+');
    move(input_y-1, input_x-1);
    whline(stdscr, '-', (term_width-input_x)-1);


    USE_COLOR(Color::WHITE);
    m_msgbuf_mutex.lock();
    for(size_t i = 0; i < m_msgbuf.size(); i++) {
        mvaddstr(i+1, 2, m_msgbuf[i].message.c_str());
    }
    m_msgbuf_mutex.unlock();

    // Input "box".
    USE_COLOR(Color::GREEN);
    mvaddstr(input_y, input_x, m_input.c_str());
    mvaddch(input_y, input_x + m_input.size(), '<');
}


void Client::enter_interaction_loop() {
    m_input.clear();
    m_draw_tui(); // getch() will block the first draw.

    timeout(10);

    while(this->running) {

        char input_ch = getch();
        if(input_ch == ERR) {
            if(this->redraw) {
                m_draw_tui();
                refresh();
                this->redraw = false;
            }
            continue;
        }

        if((input_ch >= 0x20) && (input_ch <= 0x7E)) {
            /* printable ascii */
            m_input.push_back(input_ch);
        }
        else
        if(input_ch == 0x0A) {
            /*<ENTER>*/

            if(!m_input.empty()) {
                if(m_input[0] == COMMAND_PREFIX) {
                    CMDHandler::handle_cmd(this, m_input);
                }
                else {
                    /* Normal message */
                    Packet packet = {
                        .id = PacketID::MESSAGE,
                        .data = m_input,
                        .nickname = name,
                        .session_token = this->session_token
                    };

                    Util::send_packet(this->conn->fd, packet);
                }
            }
    
            m_input.clear();
        }

        m_draw_tui();
        refresh();
    }

}



