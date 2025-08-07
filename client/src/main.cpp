#include <stdio.h>

#include "client.hpp"
#include "../../shared/log.hpp"

#include "../../shared/cryptography.hpp"



void print_key(uint8_t* k, size_t len) {
    for(size_t i = 0; i < len; i++) {
        printf("%x,", k[i]);
    }
    printf("\n");
}


int main(int argc, char** argv) {

    printf("Alice's Private and Public keys.\n");
    Crypto::Keypair keypair_A = Crypto::new_keypair_x25519();
    print_key(keypair_A.private_key, X25519_KEYLEN);
    print_key(keypair_A.public_key, X25519_KEYLEN);
    printf("\n");

    printf("Bob's Private and Public keys.\n");
    Crypto::Keypair keypair_B = Crypto::new_keypair_x25519();
    print_key(keypair_B.private_key, X25519_KEYLEN);
    print_key(keypair_B.public_key, X25519_KEYLEN);
    printf("\n");

    
    hkdfsalt_t salt;
    RAND_bytes(salt.data(), salt.size());

    uint8_t* shared_key_A = Crypto::derive_shared_key(
            keypair_A.private_key, keypair_B.public_key,
            salt, "Encryption");


    uint8_t* shared_key_B = Crypto::derive_shared_key(
            keypair_B.private_key, keypair_A.public_key,
            salt, "Encryption");

    print_key(shared_key_A, SHARED_KEY_LENGTH);
    printf("\n");
    print_key(shared_key_B, SHARED_KEY_LENGTH);

    Crypto::free_key(&shared_key_A, SHARED_KEY_LENGTH);
    Crypto::free_key(&shared_key_B, SHARED_KEY_LENGTH);
    Crypto::free_keypair(&keypair_A);
    Crypto::free_keypair(&keypair_B);
    
    return 0;


    /*
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
    */
}
