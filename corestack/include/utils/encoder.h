#ifndef ENCODER_H
#define ENCODER_H

#include <stddef.h>

 // Number of base64 characters needed to encode `len` bytes, excluding the
 // terminating NUL. Callers must allocate BASE64_ENCODED_LEN(len) + 1 bytes.
#define BASE64_ENCODED_LEN(len) ((((len) + 2) / 3) * 4)

static const char BASE64_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

 /**
  * @brief Encodes len bytes of data into base64 and stores the result in out.
  *
  * 3 input bytes become 4 output characters: the 24 bits are split into
  * 4 6-bit groups, each indexing the alphabet above. A tail of one or two bytes is
  * zero-extended and padded with '=' so the length stays a multiple of four.
  * @param in Data to be encoded
  * @param len Length of data
  * @param out Base64 Data.
  * @return size_t character count, excluding termianting NUL
  */
static inline size_t base64_encode(const unsigned char *in, size_t len, char *out)
{
    size_t o = 0;
    size_t i = 0;

    // Encode each full 3-byte group into 4 base64 characters
    for (; i + 2 < len; i += 3) {
        unsigned v = ((unsigned)in[i] << 16) | ((unsigned)in[i + 1] << 8) | in[i + 2];
        out[o++] = BASE64_ALPHABET[(v >> 18) & 63];
        out[o++] = BASE64_ALPHABET[(v >> 12) & 63];
        out[o++] = BASE64_ALPHABET[(v >> 6) & 63];
        out[o++] = BASE64_ALPHABET[v & 63];
    }

    // Encode the remaining 1-2 bytes, padding with '=' if needed
    if (i < len) {
        unsigned v = (unsigned)in[i] << 16;
        if (i + 1 < len) v |= (unsigned)in[i + 1] << 8;
        out[o++] = BASE64_ALPHABET[(v >> 18) & 63];
        out[o++] = BASE64_ALPHABET[(v >> 12) & 63];
        out[o++] = (i + 1 < len) ? BASE64_ALPHABET[(v >> 6) & 63] : '=';
        out[o++] = '=';
    }

    // NULL terminate the string
    out[o] = '\0';
    return o;
}

#endif
