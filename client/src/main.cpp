#include <stdio.h>

#include "client.hpp"
#include "../../shared/log.hpp"
#include "../../shared/cryptography.hpp"



void print_usage(char* arg0) {
    
    printf( "E2EEP-CHAT (End to end encrypted private chat.)\n"
            " --newid    :  Generate new fingerprint.\n"
            " --connect  :  Connect to e2eep-server. \033[90m%s <.onion address>\033[0m\n"
            "\n"
            "NOTE: Do not join servers you do not trust!\n"
            ,
            arg0
            );

}


void generate_fingerprint_and_exit() {

    printf("Generate fingerprint here.\n");


    exit(EXIT_SUCCESS);
}

bool flag_exists(const int argc, const char** argv, const char* flag) {
    for(int i = 1; i < argc; i++) {
        if(strcmp(flag, argv[i]) == 0) {
            return true;
        }
    }
    return false;
}


int main(int argc, char** argv) {

    if(flag_exists(argc, argv, "--newid")) {
        
    }


    assign_logfile("client.log");

    Client client;
    if(!client.connect2proxy()) {
        close_logfile();
        return 1;
    }

    if(!client.connect2server(argv[1], 4045)) {
        goto out;
    }
    
    client.generate_keys();
    
    if(!client.verify_join("")) { // Server will choose a nickname if its empty.
        goto out;
    }

    client.enter_interaction_loop();

out:
    client.disconnect();

    close_logfile();
    return 0;
}
