#include <sys/socket.h>

#include "packets.hpp"


void Packets::send_packet(int sockfd, PacketID packet_id, std::initializer_list<std::string> data) {
    
    // TODO: Encrypt the packet id.
    std::string full_data = "("+std::to_string(packet_id)+")";

    bool first = true;
    for(const std::string& str : data) {
        if(!first) {
            full_data.push_back(PACKET_DATA_SEPARATOR);
        }
        full_data += str;
        first = false;
    }
    
    printf("\033[34mSEND: %s\033[0m\n", full_data.c_str());

    full_data.push_back('\n');
    full_data.push_back(PACKET_DATA_SEPARATOR);
    full_data.push_back('\0');

    send(sockfd, full_data.c_str(), full_data.size(), 0);
}

// (packetId)!data[0]!data[1]!...(packetId)!data[0]!data[1]!data[2]...
    
bool Packets::parse_data(Packet* packet, const std::string& recv_data) {
    packet->clear(); 
    size_t packet_element_idx = 0;

    const size_t recv_data_size = recv_data.size();
    for(size_t i = 0; i < recv_data_size; i++) {
        bool is_end = (i+1 >= recv_data_size);
        char c = recv_data.at(i);

        if(c == '(') { /* Read packet id for current packet reference. */
            if(packet->id != PacketID::EMPTY) {
                log_print(ERROR, "Trying to re-set packet id.");
                return false;
            }
            if(is_end) {
                log_print(ERROR, "Unexpected '(' at end of 'recv_data'");
                return false;
            }

            std::string::size_type id_end = recv_data.find(')', i);
            if(id_end == std::string::npos) {
                log_print(ERROR, "Could not find where ID ends.");
                return false;
            }

            const std::string packetid_str = recv_data.substr(i+1, id_end);
            if(packetid_str.empty()) {
                log_print(ERROR, "PacketID string is empty");
                return false;
            }

            if(packetid_str.size() > 32) {
                log_print(ERROR, "PacketID string is way too long");
                return false;
            }

            int64_t id = std::stoi(packetid_str.c_str());
            if(id == 0) {
                log_print(ERROR, "Invalid packet ID");
                return false;
            }

            packet->id = (PacketID)id;
            i += packetid_str.size();
            if(i >= recv_data_size) {
                log_print(ERROR, "Unexpected end after 'packet id'.");
                return false;
            }
        }
        else
        if(c == PACKET_DATA_SEPARATOR) {
            if(packet_element_idx+1 >= packet->elements.size()) {
                log_print(ERROR, "Too many elements for packet. Ignored.");
                return false;
            }

            packet->num_elements++;
            packet_element_idx++;
        }
        else
        // Allow only printable ASCII characters.
        if(c >= 0x20 && c <= 0x7E) {
            if(packet_element_idx >= packet->elements.size()) {
                log_print(ERROR, "Trying to write to too large index.");
                return false;
            }
            std::string& element = packet->elements.at(packet_element_idx);
            element.push_back(c);
            if(element.size() >= PACKET_ELEMENT_MAX_SIZE) {
                log_print(ERROR, "Packet element is too large. Ignored.");
                return false;
            }
        }
    }

    return true;
}

bool Packets::receive_data(int sockfd, std::array<char, RECEIVE_MAX_SIZE>* data) {
    ssize_t recv_size = recv(sockfd, data->data(), data->size(), 0);
    return recv_size != 0;
}


