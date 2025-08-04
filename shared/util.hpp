#ifndef CLIENT_UTIL_HPP
#define CLIENT_UTIL_HPP

#include <cstdint>
#include <string>
#include "packets.hpp"
#include "ext/libsam3.h"

namespace Util {

    constexpr int DEFAULT_TIMEOUT = 6;

    enum TimeoutAct {
        DO_NOTHING,
        IS_ERROR
    };

    // 'true' is returned when the socket has data ready.
    // 'false'is returned if timeout is reached or error happened.
    /*REMOVE*/bool wait_for_socket(int sockfd, int timeout_seconds, TimeoutAct timeout_action);

    bool receive_data(int fd, PacketID* id_out, std::string* msg_out);
    void send_packet(int fd, PacketID p_id, const std::string& msg = "<no-data>");
};



#endif
