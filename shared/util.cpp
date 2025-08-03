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


bool Util::wait_for_socket(int sockfd, int timeout_seconds, TimeoutAct timeout_action) {

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    nfds_t num_fds = 1;

    int result = poll(&pfd, num_fds, 1000*timeout_seconds);

    if(result > 0) {
        return true; // Ok to read.
    }
    else
    if(result == 0) {
        if(timeout_action == TimeoutAct::IS_ERROR) {
            log_print(ERROR, "Timed out.");
        }
    }
    else {
        log_print(ERROR, "%s", strerror(errno));
    }

    return false;
}



