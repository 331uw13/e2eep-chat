#ifndef E2EEP_PACKETS_HPP
#define E2EEP_PACKETS_HPP

#include <string>
#include <initializer_list>

#include "cryptography.hpp"

static constexpr size_t     RECEIVE_MAX_SIZE = 1024*1024-1;
static constexpr char       PACKET_DATA_SEPARATOR = '\x1F';
static constexpr uint16_t   MAX_ELEMENTS_PER_PACKET = 8;
static constexpr size_t     PACKET_ELEMENT_MAX_SIZE = 1024*2;

//static constexpr int    RECEIVE_MAX_ELEMENTS = 128;
//static constexpr int    RECEIVE_MAX_ELEMENT_SIZE = 1024*2;


//Packets::send_packet(PacketID::MESSAGE, { iv, cipher, sender_publickey,  });





enum PacketID : int {
    EMPTY = 0xFF,
   
    // Server may send unencrypted error messages to clients.
    // these messages _dont_ hold any potentially sensetive data just few words about what happened.
    ERROR_MESSAGE,

    // When client joins a server, it sends their nickname and public key.
    // data: { nickname, public_key }
    VERIFY_JOIN,
   
    // If the given nickname is valid (less than 24 bytes long)
    // it will accept it and send this packet id.
    JOIN_ACCEPTED,

    // If the nickname is not valid this packet id is sent to client.
    JOIN_DENIED,
   
    // "Someone joined, here is their public"
    SOMEONE_JOINED,

    // data: { iv, cipher, msg_signature, key_signature, sender_public_key, recipient_public_key, ... }
    MESSAGE,
    
    // Client can ask the server for public keys of online users.
    ASKING_PUBLIC_KEYS,
};

struct Packet {
    PacketID id;
    std::array<std::string, MAX_ELEMENTS_PER_PACKET> elements;
    uint16_t num_elements;
   
    Packet() {
        this->clear();
    }

    void clear() {
        id = PacketID::EMPTY;
        num_elements = 0;
        elements.fill("");
    }
};

namespace Packets {

    void send_packet(int sockfd, PacketID packet_id, std::initializer_list<std::string> data);
    bool parse_data(Packet* packet, const std::string& recv_data);

    bool receive_data(int sockfd, std::array<char, RECEIVE_MAX_SIZE>* data);
};




#endif
