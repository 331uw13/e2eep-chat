#include <sys/socket.h>

#include "packets.hpp"



void Packets::send_packet(int sockfd, const struct Packet& packet) {

    /*

    std::string data;
    data.reserve(1024*2);

    data += std::string(std::begin(packet.iv), std::end(packet.iv));
    data += "#";
    data += std::string(std::begin(packet.cipher), std::end(packet.cipher));
    data += "#";
    data += std::string(std::begin(packet.public_key), std::end(packet.public_key));

    printf("SEND:|%s|\n", data.c_str());
    send(sockfd, data.c_str(), data.size(), 0);

    */
}



