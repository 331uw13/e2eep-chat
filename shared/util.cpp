#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>
#include <poll.h>

#include "util.hpp"
#include "log.hpp"


bool Util::socket_ready_inms(int sockfd, int timeout_ms) {

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    nfds_t num_fds = 1;

    int result = poll(&pfd, num_fds, timeout_ms);

    if(result < 0){
        log_print(ERROR, "%s", strerror(errno));
    }

    return (result == 1);
}


int Util::random_int(int min, int max, int* seed) {
    *seed = 0x343FD * *seed + 0x269EC3;
    return (*seed >> 16 & 0x7FFF) % (max - min) + min;
}


