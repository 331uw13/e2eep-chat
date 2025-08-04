#ifndef SERVER_CLIENT_HPP
#define SERVER_CLIENT_HPP

#include <string>
#include "../../shared/ext/libsam3.h"
#include "../../shared/packets.hpp"


class ServerClient {
    public:

        Sam3Connection* conn;
        std::string name;


        ServerClient(
                Sam3Connection* _conn
        ){
            conn = _conn;
            name = "Unknown";
        }



    private:

};





#endif
