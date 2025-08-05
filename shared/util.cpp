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


bool Util::socket_ready_inms(int sockfd, int timeout_ms, TimeoutAct timeout_action) {

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    nfds_t num_fds = 1;

    int result = poll(&pfd, num_fds, timeout_ms);

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

// Format: "<ID>PACKETID<ID>:DATA"
bool Util::receive_idnmsg(int fd, PacketID* id_out, std::string* msg_out) {

    char resdata[PACKET_MAX_SIZE+1] = { 0 };
    if(sam3tcpReceiveStr(fd, resdata, sizeof(resdata)) >= 0) {
        if(!id_out && !msg_out) {
            log_print(ERROR, "No packet id or message pointers. nothing to do!");
            return false;
        }

        std::string data(resdata);

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

    return false;
}


bool Util__parse_response_field(std::string* out, const std::string& data,  const std::string& begin_tag, const std::string& end_tag) {
    bool result = false;
    std::string::size_type begin_idx;
    std::string::size_type end_idx;


    begin_idx = data.find(begin_tag);
    if(begin_idx == std::string::npos) {
        goto out;
    }

    begin_idx += begin_tag.size(); 

    end_idx = data.find(end_tag, begin_idx);
    if(end_idx == std::string::npos) {
        goto out;
    }

    *out = data.substr(begin_idx, end_idx - begin_idx);
 
    result = true;
out:
    return result;
}


// Format: <ID>packet_id<!ID><TOKEN>session_token<!TOKEN><NAME>client_name<!NAME><DATA>message<!DATA>
bool Util::server_receive(int fd, Packet* packet) {
    bool result = false;
    char resdata[PACKET_MAX_SIZE+1] = { 0 };
    if(sam3tcpReceiveStr(fd, resdata, sizeof(resdata)) >= 0) {
        std::string data(resdata);
        std::string buffer;
        buffer.reserve(256);

        buffer.clear();
        if(!Util__parse_response_field(&buffer, data, "<ID>", "<!ID>")) {
            goto out;
        }

        packet->id = (PacketID)std::stoi(buffer.c_str(), 0, 16);
        
        buffer.clear();
        if(!Util__parse_response_field(&buffer, data, "<TOKEN>", "<!TOKEN>")) {
            goto out;
        }
        if(buffer.size() != packet->session_token.size()) {
            goto out;
        }
        
        for(size_t i = 0; i < packet->session_token.size(); i++) {
            packet->session_token[i] = (uint8_t)buffer[i];
        }


        buffer.clear();
        if(!Util__parse_response_field(&buffer, data, "<NAME>", "<!NAME>")) {
            goto out;
        }

        packet->nickname = buffer;


        buffer.clear();
        if(!Util__parse_response_field(&buffer, data, "<DATA>", "<!DATA>")) {
            goto out;
        }

        packet->data = buffer;


        result = true;
    }

out:
    return result;
}


// Format: "<ID>PACKETID<ID>:DATA"
void Util::send_simple_packet(int fd, PacketID p_id, const std::string& msg) {
    if(fd < 0) {
        return;
    }
    if((p_id < PacketID::EMPTY) || (p_id >= PacketID::PACKET_MAX)) {
        return;
    }
    sam3tcpPrintf(fd, "<ID>%x<ID>:%s\n", p_id, msg.c_str());
}

    
// Format: <ID>packet_id<!ID><TOKEN>session_token<!TOKEN><NAME>client_name<!NAME><DATA>message<!DATA>
void Util::send_packet(int fd, const Packet& packet) {
    if(fd < 0) {
        return;
    }
    if((packet.id < PacketID::EMPTY) || (packet.id >= PacketID::PACKET_MAX)) {
        return;
    }
    if(packet.nickname.empty()) {
        return;
    }

    sam3tcpPrintf(fd, 
            "<ID>%x<!ID>"
            "<TOKEN>%s<!TOKEN>"
            "<NAME>%s<!NAME>"
            "<DATA>%s<!DATA>\n"
            ,
            packet.id,
            packet.session_token.data(),
            packet.nickname.c_str(),
            packet.data.c_str()
            );   
}



