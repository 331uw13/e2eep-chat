

#include "server.hpp"
#include "../../shared/log.hpp"


int main(int argc, char** argv) {

    assign_logfile("server.log");

    Server server;
    ServerConfig server_config = {
        .max_clients = 10,
        .port = 4045,
        .welcome_msg = "Welcome to the testing server."
    };


    if(!server.start(server_config)) {
        close_logfile();
        return 1;
    }

    server.stop();


    log_print(INFO, "Closing logfile and exiting...");
    close_logfile();
    return 0;
}




