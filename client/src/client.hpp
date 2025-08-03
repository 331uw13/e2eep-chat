#ifndef E2EEP_CLIENT_HPP
#define E2EEP_CLIENT_HPP

#include <netinet/in.h>



class Client {
    public:


        bool connect2proxy();
        bool connect2server(const char* onion_addr, uint16_t port);
        void disconnect();
        
        int sockfd;
        struct sockaddr_in proxyaddr;


    private:
};




#endif
