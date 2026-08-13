// direct copy of common.h from PA2

/**
 * common.h
 * --------
 * Shared declarations for the Secure FTP project
 *
 * Every client and server source file includes this header. It provides:
 *   - Integer <-> big-endian byte conversion
 *   - Reliable socket read
 *   - OpenSSL convenience wrappers for signing, verifying, encrypting, decrypting
 *   - Fernet-equivalent symmetric encrypt/decrypt (AES-128-CBC + HMAC-SHA256)
 *   - X.509 certificate loading and verification helpers
 */

#ifndef LIBBR_CRYPTO_H
#define LIBBR_CRYPTO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

/* ======================================================================
 * Constants
 * ====================================================================== */

/** AES-128-CBC block and key sizes (used by our Fernet-equivalent). */
#define AES_KEY_LEN 16
#define AES_IV_LEN 16
#define AES_BLOCK 16
#define HMAC_KEY_LEN 16
#define HMAC_LEN 32 /* SHA-256 output */

/**
 * Session key length = HMAC key (16) + AES key (16) = 32 bytes.
 */
#define SESSION_KEY_LEN (HMAC_KEY_LEN + AES_KEY_LEN)

/* ======================================================================
 * OpenSSL key/cert loading
 * ====================================================================== */

/**
 * Loads an RSA private key from a PEM file.
 * Returns an EVP_PKEY* on success, NULL on failure.
 * Caller must EVP_PKEY_free() the returned key.
 */
EVP_PKEY *load_private_key(const char *filename);

/**
 * Loads an X.509 certificate from a PEM file on disk.
 * Returns an X509* on success, NULL on failure.
 * Caller must X509_free() the returned cert.
 */
X509 *load_cert_file(const char *filename);

/**
 * Parses an X.509 certificate from a PEM-encoded byte buffer.
 * Returns an X509* on success, NULL on failure.
 * Caller must X509_free() the returned cert.
 */
X509 *load_cert_bytes(const unsigned char *data, int len);

/* ======================================================================
 * Certificate verification
 * ====================================================================== */

/**
 * Verifies that 'server_cert' was signed by the CA whose certificate
 * is at 'ca_cert_path'. Also checks validity period.
 * Returns 1 on success, 0 on failure. Prints diagnostics.
 */
int verify_server_cert(X509 *server_cert, const char *ca_cert_path);

/* ======================================================================
 * RSA-PSS signing and verification (Authentication Protocol)
 * ====================================================================== */

/**
 * Signs 'msg' (of 'msg_len' bytes) with the private key using RSA-PSS
 * with SHA-256 and maximum salt length.
 *
 * Returns a newly malloc'd signature buffer, and writes its length to *sig_len.
 * Returns NULL on failure. Caller must free().
 */
unsigned char *sign_message_pss(EVP_PKEY *priv_key,
                                const unsigned char *msg, size_t msg_len,
                                size_t *sig_len);

/**
 * Verifies an RSA-PSS signature on 'msg' using the public key from 'cert'.
 * Returns 1 if valid, 0 if invalid or error.
 */
int verify_message_pss(X509 *cert,
                       const unsigned char *sig, size_t sig_len,
                       const unsigned char *msg, size_t msg_len);

/* ======================================================================
 * Symmetric encryption (CP2)
 * ======================================================================
 */

/**
 * Generates a random 32-byte session key.
 * Writes SESSION_KEY_LEN bytes into 'key_out'.
 * Returns 0 on success, -1 on failure.
 */
int generate_session_key(unsigned char key_out[SESSION_KEY_LEN]);

/**
 * Encrypts 'plain' using AES-128-CBC + HMAC-SHA256.
 * Layout of output: IV (16) || ciphertext || HMAC (32).
 *
 * Returns a newly malloc'd buffer and writes its length to *out_len.
 * Returns NULL on failure.
 */
unsigned char *session_encrypt(const unsigned char key[SESSION_KEY_LEN],
                               const unsigned char *plain, size_t plain_len,
                               size_t *out_len);

/**
 * Decrypts a token produced by session_encrypt().
 * Verifies HMAC first; returns NULL if verification fails.
 *
 * Returns a newly malloc'd plaintext buffer, writes length to *out_len.
 */
unsigned char *session_decrypt(const unsigned char key[SESSION_KEY_LEN],
                               const unsigned char *token, size_t token_len,
                               size_t *out_len);

/* ======================================================================
 * Utility
 * ====================================================================== */

/** Prints the most recent OpenSSL error to stderr. */
void print_ssl_error(const char *context);

/** Returns wall-clock time in seconds */
double get_time(void);

#endif /* COMMON_H */
