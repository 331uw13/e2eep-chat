#include "commands.hpp"
#include "client.hpp"
#include "../../shared/util.hpp"


void CMDHandler::handle_cmd(Client* client, const std::string input) {
    if(input.empty()) { return; }


    if(input == "/leave") {
        Packet packet = {
            .id = PacketID::LEAVING,
            .data = "",
            .nickname = client->name,
            .session_token = client->session_token
        };
        Util::send_packet(client->conn->fd, packet);
        client->running = false;
    }
    else
    if(input == "/online") {
        
    }
}



