#ifndef E2EEP_PACKETS_HPP
#define E2EEP_PACKETS_HPP

#include <string>
#include "cryptography.hpp"

static constexpr size_t RECEIVE_MAX_SIZE = 1024*2-1;

struct Packet {  // Note: Lengths are defined in "cryptography.hpp"
   
    /*
    std::array<char, IV_LENGTH>      iv;           // Initial vector.    
    std::array<char, KEY_LENGTH>     public_key;   // Sender public key.
    std::array<char, CIPHER_LENGTH>  cipher;       // Encrypted message.
    */
};



namespace Packets {

    void send_packet(int sockfd, const struct Packet& packet);

};




#endif
