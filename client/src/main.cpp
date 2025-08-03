#include <stdio.h>

#include "client.hpp"
#include "../../shared/log.hpp"



#define ONION_ADDRESS\
    "wz3gxkjd72ak23x5n4g5tz6o352jnacqrrswxotcv4h3ioxptd22ucid.onion"


int main(int argc, char** argv) {
    /*
    if(argc != 2) {
        printf("Usage: %s <onion address>\n", argv[0]);
        return 1;
    }
    */

    assign_logfile("client.log");

    Client client;
    if(!client.connect2proxy()) {
        close_logfile();
        return 1;
    }

    client.connect2server(ONION_ADDRESS, 4045);
    
    client.disconnect();

    close_logfile();
    return 0;
}
