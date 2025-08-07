#include <cstdint>
#include <openssl/err.h>
#include <openssl/params.h>
#include <openssl/kdf.h>
#include <openssl/core_names.h>
#include <openssl/ec.h>

#include "cryptography.hpp"
#include "log.hpp"


/*

   Alice and Bob generates a private and public key.
   Both Alice and Bob will exchange their public keys.

   Both can now calculate the shared secret with their own private key and other's public key.

   The shared secret is passed to HMAC-based extract and expand key deriviation function (HKDF).
   The result from HKDF is the shared key.

   the message is encrypted/decrypted with the shared key (Not shared secret!)

*/


static void _openssl_error(const char* what) {
    char buffer[256] = { 0 };
    ERR_error_string(ERR_get_error(), buffer);
    log_print(ERROR, "OpenSSL: (%s) %s", what, buffer);
}


uint8_t* Crypto__HKDF(
        uint8_t* shared_secret, 
        const hkdfsalt_t& salt,
        const char* label
){
    uint8_t* out = NULL;
    EVP_KDF* kdf = NULL;
    EVP_KDF_CTX* kdfctx = NULL;

    kdf = EVP_KDF_fetch(NULL, "HKDF", NULL);
    kdfctx = EVP_KDF_CTX_new(kdf); 

    OSSL_PARAM params[5] = {
        OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, SN_sha256, strlen(SN_sha256)),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, shared_secret, X25519_KEYLEN),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, (void*)label, strlen(label)),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, (void*)salt.data(), salt.size()),
        OSSL_PARAM_construct_end()
    };

    out = (uint8_t*)malloc(SHARED_KEY_LENGTH);
    if(!EVP_KDF_derive(kdfctx, out, SHARED_KEY_LENGTH, params)) {
        _openssl_error("EVP_KDF_derive");
        return NULL;
    }

    EVP_KDF_CTX_free(kdfctx);
    return out;
}

Crypto::Keypair Crypto::new_keypair_x25519() {
    Keypair keypair;

    size_t private_key_len = 0;
    size_t public_key_len = 0;
    
    EVP_PKEY* pkey = NULL;
    EVP_PKEY_CTX* pkctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);

    if(EVP_PKEY_keygen_init(pkctx) <= 0) {
        _openssl_error("EVP_PKEY_keygen_init");
        goto out;
    }

    EVP_PKEY_keygen(pkctx, &pkey);

    // Get private key length.
    if(EVP_PKEY_get_raw_private_key(pkey, NULL, &private_key_len) <= 0) {
        _openssl_error("EVP_PKEY_get_raw_private_key");
        goto out;
    }
    if(private_key_len != X25519_KEYLEN) {
        log_print(ERROR, "Private key length doesnt match expected value.");
        goto out;
    }

    keypair.private_key = (uint8_t*)malloc(private_key_len);
    if(EVP_PKEY_get_raw_private_key(pkey, keypair.private_key, &private_key_len) <= 0) {
        _openssl_error("EVP_PKEY_get_raw_private_key");
        free(keypair.private_key);
        keypair.private_key = NULL;
        goto out;
    }

    // Get public key length.
    if(EVP_PKEY_get_raw_public_key(pkey, NULL, &public_key_len) <= 0) {
        _openssl_error("EVP_PKEY_get_raw_public_key");
        free_key(&keypair.private_key, X25519_KEYLEN);
        goto out;
    }
    if(public_key_len != X25519_KEYLEN) {
        log_print(ERROR, "Public key length doesnt match expected value.");
        free_key(&keypair.private_key, X25519_KEYLEN);
        goto out;
    }

    keypair.public_key = (uint8_t*)malloc(public_key_len);
    if(EVP_PKEY_get_raw_public_key(pkey, keypair.public_key, &public_key_len) <= 0) {
        _openssl_error("EVP_PKEY_get_raw_public_key");
        free(keypair.public_key);
        keypair.public_key = NULL;
        free_key(&keypair.private_key, X25519_KEYLEN);
        goto out;
    }

out:
    if(pkey) {
        EVP_PKEY_free(pkey);
    }
    EVP_PKEY_CTX_free(pkctx);
    return keypair;
}

    
void Crypto::free_keypair(struct Keypair* keypair) {
    if(keypair->public_key) {
        free_key(&keypair->public_key, X25519_KEYLEN);
    }
    if(keypair->private_key) {
        free_key(&keypair->private_key, X25519_KEYLEN);
    }
}
    

uint8_t* Crypto::derive_shared_key(
        uint8_t* private_key,
        uint8_t* peer_public_key,
        const hkdfsalt_t& hkdf_salt,
        const char* hkdf_info
){

    uint8_t* shared_secret = NULL;
    uint8_t* shared_key = NULL;
    size_t secret_length = 0;

    EVP_PKEY* ppublic_pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL, peer_public_key, X25519_KEYLEN);
    EVP_PKEY* private_pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, private_key, X25519_KEYLEN);

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_pkey, NULL);

    if(EVP_PKEY_derive_init(ctx) <= 0) {
        _openssl_error("EVP_PKEY_derive_init");
        goto out;
    }

    if(EVP_PKEY_derive_set_peer(ctx, ppublic_pkey) <= 0) {
        _openssl_error("EVP_PKEY_derive_set_peer");
        goto out;
    }


    // Get shared secret length.
    if(EVP_PKEY_derive(ctx, NULL, &secret_length) <= 0) {
        _openssl_error("EVP_PKEY_derive (length)");
        goto out;
    }

    shared_secret = (uint8_t*)malloc(secret_length);
    if(EVP_PKEY_derive(ctx, shared_secret, &secret_length) <= 0) {
        _openssl_error("EVP_PKEY_derive (data)");
        free(shared_secret);
        goto out;
    }


    shared_key = Crypto__HKDF(shared_secret, hkdf_salt, hkdf_info);
    
    free_key(&shared_secret, secret_length);

out:
    EVP_PKEY_free(ppublic_pkey);
    EVP_PKEY_free(private_pkey);
    EVP_PKEY_CTX_free(ctx);
    return shared_key;
}



void Crypto::free_key(uint8_t** keyptr, size_t length) {
    for(size_t i = 0; i < length; i++) {
        (*keyptr)[i] = 0;
    }
    free(*keyptr);
    *keyptr = NULL;
}


