#ifndef E2EEP_PACKETS_HPP
#define E2EEP_PACKETS_HPP



static constexpr size_t PACKET_MAX_SIZE = 1024 * 2;

enum PacketID : int {
    EMPTY = 0xFF,
    ASKING_NICKNAME, // Server is asking for client name
    NICKNAME,      // Client sent name.
    NICKNAME_BAD,  // Server didnt accept client name.
    NICKNAME_OK,   // Server accepted client name.
    MESSAGE,       // Server/Client sent message.
    LEAVING,       // Client is leaving.
    
    PACKET_MAX
};




#endif
