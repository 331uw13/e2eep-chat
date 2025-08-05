#ifndef CLIENT_UTIL_HPP
#define CLIENT_UTIL_HPP

#include <cstdint>
#include <string>
#include "packets.hpp"
#include "session_token.hpp"
#include "ext/libsam3.h"


static constexpr size_t NICKNAME_MAX = 24;


struct Packet {
    PacketID       id;
    std::string    data;
    std::string    nickname;
    token_t        session_token;


    void clear() {
        id = PacketID::EMPTY;
        data.clear();
        nickname.clear();
    }
};



namespace Util {

    constexpr int DEFAULT_TIMEOUT = 6;

    enum TimeoutAct {
        DO_NOTHING,
        IS_ERROR
    };

    // 'true'  is returned when the socket has data ready.
    // 'false' is returned if timeout is reached or error happened.
    bool socket_ready_inms(int sockfd, int timeout_ms, TimeoutAct timeout_action);

    bool server_receive(int fd, Packet* packet);
    bool receive_idnmsg(int fd, PacketID* id_out, std::string* msg_out);
  
    void send_packet(int fd, const Packet& packet);
    void send_simple_packet(int fd, PacketID p_id, const std::string& msg = "<no-data>");
};



#endif
