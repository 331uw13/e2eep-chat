#ifndef CLIENT_UTIL_HPP
#define CLIENT_UTIL_HPP

#include <cstdint>


namespace Util {

    constexpr int DEFAULT_TIMEOUT = 6000;

    // 'true' is returned when the socket has data ready.
    // 'false'is returned if timeout is reached or error happened.
    bool socket_ready_inms(int sockfd, int timeout_ms);
    
    int random_int(int min, int max, int* seed);

};



#endif
