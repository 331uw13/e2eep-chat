#ifndef CLIENT_UTIL_HPP
#define CLIENT_UTIL_HPP

#include <cstdint>


namespace Util {

    constexpr int DEFAULT_TIMEOUT = 6;

    enum TimeoutAct {
        DO_NOTHING,
        IS_ERROR
    };

    // 'true' is returned when the socket has data ready.
    // 'false'is returned if timeout is reached or error happened.
    bool wait_for_socket(int sockfd, int timeout_seconds, TimeoutAct timeout_action);

        
};



#endif
