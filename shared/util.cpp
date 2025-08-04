#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>
#include <poll.h>
#include <string>

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

static bool Util__parse_response(const std::string& data, PacketID* id_out, std::string* msg_out) {

    if(!id_out && !msg_out) {
        log_print(ERROR, "No packet id or message pointers. nothing to do!");
        return false;
    }

    // "<ID>0x100<ID>:<_empty_>"

    if(id_out) {
        std::string::size_type id_begin_pos = data.find("<ID>");
        if(id_begin_pos == std::string::npos) {
            log_print(ERROR, "Invalid packet format (no <ID> begin).");
            return false;
        }

        id_begin_pos += 4;
        std::string::size_type id_end_pos = data.find("<ID>", id_begin_pos);
        if(id_begin_pos == std::string::npos) {
            log_print(ERROR, "Invalid packet format (no <ID> end).");
            return false;
        }

        std::string test = data.substr(id_begin_pos, id_end_pos - id_begin_pos);
        if(test.empty()) {
            log_print(ERROR, "Invalid packet id is empty.");
            return false;
        }

        *id_out = (PacketID)std::stoi(test.c_str(), 0, 16);
    }

    if(msg_out) {
        std::string::size_type msg_begin_pos = data.find("<ID>:");
        if(msg_begin_pos == std::string::npos) {
            log_print(ERROR, "Invalid packet format (no begin for msg).");
            return false;
        }
        msg_begin_pos += 5;
        *msg_out = data.substr(msg_begin_pos, data.size() - msg_begin_pos);
    }


    return true;
}

bool Util::receive_data(int fd, PacketID* id_out, std::string* msg_out) {

    char resdata[PACKET_MAX_SIZE+1] = { 0 };
    if(sam3tcpReceiveStr(fd, resdata, sizeof(resdata)) >= 0) {
        return Util__parse_response(resdata, id_out, msg_out);
    }

    return false;
}
    
void Util::send_packet(int fd, PacketID p_id, const std::string& msg) {
    if(fd < 0) {
        return;
    }
    if((p_id < PacketID::EMPTY) || (p_id >= PacketID::PACKET_MAX)) {
        return;
    }
    sam3tcpPrintf(fd, "<ID>%x<ID>:%s\n", p_id, msg.c_str());
}

