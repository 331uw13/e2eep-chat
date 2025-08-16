#ifndef SERVER_CLIENT_HPP
#define SERVER_CLIENT_HPP

#include <string>


class ServerClient {
    public:

        int fd;
        std::string nickname;
        std::string session_token;

        std::string public_key_hexstr;
       
        bool  accepted;

        ServerClient(int _fd) {
            fd = _fd;
            accepted = false;
        }

    private:

};





#endif
