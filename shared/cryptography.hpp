#ifndef CRYPTOGRAPHY_HPP
#define CRYPTOGRAPHY_HPP

#include <array>
#include <openssl/evp.h>
#include <openssl/rand.h>

//static constexpr size_t IV_LENGTH = 32;
static constexpr size_t HKDF_SALT_LENGTH = 32;
static constexpr size_t X25519_KEYLEN = 32;
static constexpr size_t SHARED_KEY_LENGTH = 64;

typedef std::array<uint8_t, HKDF_SALT_LENGTH> hkdfsalt_t;


namespace Crypto {

    struct Keypair {
        uint8_t* public_key;
        uint8_t* private_key;
    };

    Keypair new_keypair_x25519();
    void    free_keypair(struct Keypair* keypair);

    // Derive shared secret from private key and peer's public key.
    // then the shared secret is passed to HKDF and returned.
    // Remember to free the shared key with 'free_key'
    uint8_t* derive_shared_key(
            uint8_t* private_key,
            uint8_t* peer_public_key,
            const hkdfsalt_t& hkdf_salt,
            const char* hkdf_info);

    // Fills the whole array with 0 and free it after that.
    void free_key(uint8_t** keyptr, size_t length);
};


#endif
