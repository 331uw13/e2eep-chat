#include <stdio.h>

#include "client.hpp"
#include "../../shared/log.hpp"
#include "../../shared/cryptography.hpp"

/*
void print_key(uint8_t* k, size_t len) {
    for(size_t i = 0; i < len; i++) {
        printf("%x,", k[i]);
    }
    printf("\n");
}
*/

int main(int argc, char** argv) {

    assign_logfile("client.log");


    std::array<uint8_t, 32> key;
    RAND_bytes(key.data(), 32);
    
    std::array<uint8_t, 32> iv;
    RAND_bytes(iv.data(), 32);


    std::string message = "Hello! this is a simple message.";
    Crypto::add_random_padding(message);
    size_t cipher_size;
    uint8_t* bytes = Crypto::Encrypt_AES256CBC(key.data(), iv.data(), message, &cipher_size);
   

    std::string cipher_hexstr = bytes_to_hexstr(bytes, cipher_size);
    printf("%s\n", cipher_hexstr.c_str());

    ByteArray array = hexstr_to_bytes(cipher_hexstr);

    //ByteArray<2048> normal = hexstr_to_bytes<2048>(cipher_hexstr);

    printf("===== CIPHER =====\n\033[90m");
    for(int i = 0; i < cipher_size; i++) {
        printf("%x, ", bytes[i]);
        if((i%16) == 0 && i > 0) {
            printf("\n");
        }
    }
    printf("\033[0m\n\n");


    printf("===== BYTES BACK =====\n\033[90m");
    for(int i = 0; i < array.size; i++) {
        printf("%x, ", array.data[i]);
        if((i%16) == 0 && i > 0) {
            printf("\n");
        }
    }
    printf("\033[0m\n\n");



    /*
    std::string decrypted = Crypto::Decrypt_AES256CBC(key.data(), iv.data(), bytes, cipher_size);
    printf("===== ORIGINAL MESSAGE ======\n\033[90m");
    printf("%s\n", decrypted.c_str());
    printf("\033[0m\n\n");

    */
    
    if(bytes) { free(bytes); }
    close_logfile();
    return 0;
}


int main2(int argc, char** argv) {



    if(argc != 2) {
        printf("Usage: %s <.onion Address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    assign_logfile("client.log");

    Client client;
    if(!client.connect2proxy()) {
        close_logfile();
        return 1;
    }

    client.connect2server(argv[1], 4045);
    client.disconnect();

    close_logfile();
    return 0;
 
    /*
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
    */

}
