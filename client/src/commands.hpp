#ifndef E2EEP_CLIENT_COMMANDS_HPP
#define E2EEP_CLIENT_COMMANDS_HPP

#include <string>

class Client;


namespace CMDHandler {


    void handle_cmd(Client* client, const std::string input);


};




#endif
