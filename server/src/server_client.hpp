#ifndef SERVER_CLIENT_HPP
#define SERVER_CLIENT_HPP

#include <string>


class ServerClient {
    public:

        int fd;
        std::string name;

        ServerClient(int _fd, const std::string& _name) {
            fd = _fd;
            name = _name;
        }

    private:

};





#endif
