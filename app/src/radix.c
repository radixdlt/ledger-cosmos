#include <stdbool.h>
#include <stdint.h>
#include <os.h>
#include <cx.h>
#include "radix.h"
#include "os_io_seproxyhal.h"

#define KEY_SEED_BYTE_COUNT 32

void getKeySeed(
    uint8_t* keySeed, 
    uint32_t *bip32path
) {

    BEGIN_TRY {
        TRY {
            io_seproxyhal_io_heartbeat();
            os_perso_derive_node_bip32(CX_CURVE_256K1, bip32path, 5, keySeed, NULL);
            io_seproxyhal_io_heartbeat();
        }
        CATCH_OTHER(e) {
            os_memset(keySeed, 0, KEY_SEED_BYTE_COUNT);
            switch (e) {
                case EXCEPTION_SECURITY: {
                    PRINTF("FAILED call 'os_perso_derive_node_bip32', error: 'EXCEPTION_SECURITY' (==%d)\n", e);
                    break;
                }
                default: {
                    PRINTF("FAILED call 'os_perso_derive_node_bip32', unknown error: %d\n", e);
                    break;
                }
            }
            // Rethrow so that program terminates
            THROW(e);
        }
        FINALLY {
        }
    }
    END_TRY;
}

void compressPubKey(cx_ecfp_public_key_t *publicKey) {
    // Uncompressed key has 0x04 + X (32 bytes) + Y (32 bytes).
    if (publicKey->W_len != 65 || publicKey->W[0] != 0x04) {
        PRINTF("compressPubKey: Input public key is incorrect\n");
        THROW(SW_INVALID_PARAM);
    }

    // check if Y is even or odd. Assuming big-endian, just check the last byte.
    if (publicKey->W[64] % 2 == 0) {
        // Even
        publicKey->W[0] = 0x02;
    } else {
        // Odd
        publicKey->W[0] = 0x03;
    }

    publicKey->W_len = PUBLIC_KEY_COMPRESSEED_BYTE_COUNT;
}

void deriveRadixPubKey(
    uint32_t *bip32path, 
    cx_ecfp_public_key_t *publicKey
) {
    cx_ecfp_private_key_t pk;

    uint8_t keySeed[KEY_SEED_BYTE_COUNT];
    getKeySeed(keySeed, bip32path);
    cx_ecfp_init_private_key(CX_CURVE_SECP256K1, keySeed, 32, &pk);

    assert (publicKey);
    cx_ecfp_init_public_key(CX_CURVE_SECP256K1, NULL, 0, publicKey);
    cx_ecfp_generate_pair(CX_CURVE_SECP256K1, publicKey, &pk, 1);
    PRINTF("Uncompressed public key:\n %.*H \n\n", publicKey->W_len, publicKey->W);

    compressPubKey(publicKey);

    os_memset(keySeed, 0, sizeof(keySeed));
    os_memset(&pk, 0, sizeof(pk));
}

// void deriveAndSign(uint8_t *dst, uint32_t dst_len, uint32_t index, const uint8_t *msg, unsigned int msg_len) {
//     PRINTF("deriveAndSign: index: %d\n", index);
//     PRINTF("deriveAndSign: msg: %.*H \n", msg_len, msg);

//     uint8_t keySeed[KEY_SEED_BYTE_COUNT];
//     getKeySeed(keySeed, index);

//     cx_ecfp_private_key_t privateKey;
//     cx_ecfp_init_private_key(CX_CURVE_SECP256K1, keySeed, 32, &privateKey);
//     PRINTF("deriveAndSign: privateKey: %.*H \n", privateKey.d_len, privateKey.d);

//     if (dst_len != ECDSA_SIGNATURE_BYTE_COUNT)
//         THROW (INVALID_PARAMETER);

//     zil_ecschnorr_sign(&privateKey, msg, msg_len, dst, dst_len);
//     PRINTF("deriveAndSign: signature: %.*H\n", ECDSA_SIGNATURE_BYTE_COUNT, dst);

//     // Erase private keys for better security.
//     os_memset(keySeed, 0, sizeof(keySeed));
//     os_memset(&privateKey, 0, sizeof(privateKey));
// }

// void deriveAndSignInit(zil_ecschnorr_t *T, uint32_t index)
// {
//     PRINTF("deriveAndSignInit: index: %d\n", index);

//     uint8_t keySeed[KEY_SEED_BYTE_COUNT];
//     getKeySeed(keySeed, index);
//     cx_ecfp_private_key_t privateKey;
//     cx_ecfp_init_private_key(CX_CURVE_SECP256K1, keySeed, 32, &privateKey);
//     PRINTF("deriveAndSignInit: privateKey: %.*H \n", privateKey.d_len, privateKey.d);
//     zil_ecschnorr_sign_init (T, &privateKey);

//     // Erase private keys for better security.
//     os_memset(keySeed, 0, sizeof(keySeed));
//     os_memset(&privateKey, 0, sizeof(privateKey));
// }

// void deriveAndSignContinue(zil_ecschnorr_t *T, const uint8_t *msg, unsigned int msg_len)
// {
//     PRINTF("deriveAndSignContinue: msg: %.*H \n", msg_len, msg);

//     zil_ecschnorr_sign_continue(T, msg, msg_len);
// }

// int deriveAndSignFinish(zil_ecschnorr_t *T, uint32_t index, unsigned char *dst, unsigned int dst_len)
// {
//     PRINTF("deriveAndSignFinish: index: %d\n", index);

//     uint8_t keySeed[KEY_SEED_BYTE_COUNT];
//     getKeySeed(keySeed, index);
//     cx_ecfp_private_key_t privateKey;
//     cx_ecfp_init_private_key(CX_CURVE_SECP256K1, keySeed, 32, &privateKey);
//     PRINTF("deriveAndSignFinish: privateKey: %.*H \n", privateKey.d_len, privateKey.d);

//     if (dst_len != ECDSA_SIGNATURE_BYTE_COUNT)
//         THROW (INVALID_PARAMETER);

//     uint32_t s = zil_ecschnorr_sign_finish(T, &privateKey, dst, dst_len);
//     PRINTF("deriveAndSignFinish: signature: %.*H\n", ECDSA_SIGNATURE_BYTE_COUNT, dst);

//     // Erase private keys for better security.
//     os_memset(keySeed, 0, sizeof(keySeed));
//     os_memset(&privateKey, 0, sizeof(privateKey));

//     return s;
// }

void pubkeyToRadixAddress(uint8_t *dst, cx_ecfp_public_key_t *publicKey) {
    // // 3. Apply SHA2-256 to the pub key
    // uint8_t digest[SHA256_HASH_DIGEST_BYTE_COUNT];
    // cx_hash_sha256(publicKey->W, publicKey->W_len, digest, SHA256_HASH_DIGEST_BYTE_COUNT);
    // PRINTF("sha256: %.*H\n", SHA256_HASH_DIGEST_BYTE_COUNT, digest);

    // // LSB 20 bytes of the hash is our address.
    // for (unsigned i = 0; i < 20; i++) {
    //     dst[i] = digest[i+12];
    // }
    THROW(0x8999); // not impl yet
}

void bin2hex(uint8_t *dst, uint64_t dstlen, uint8_t *data, uint64_t inlen) {
    if(dstlen < 2*inlen + 1)
        THROW(SW_INVALID_PARAM);
    static uint8_t const hex[] = "0123456789abcdef";
    for (uint64_t i = 0; i < inlen; i++) {
        dst[2 * i + 0] = hex[(data[i] >> 4) & 0x0F];
        dst[2 * i + 1] = hex[(data[i] >> 0) & 0x0F];
    }
    dst[2 * inlen] = '\0';
}

static uint8_t hexchar2bin (unsigned char c) {
    switch (c)
    {
        case '0': return 0x0;
        case '1': return 0x1;
        case '2': return 0x2;
        case '3': return 0x3;
        case '4': return 0x4;
        case '5': return 0x5;
        case '6': return 0x6;
        case '7': return 0x7;
        case '8': return 0x8;
        case '9': return 0x9;
        case 'a': case 'A': return 0xa;
        case 'b': case 'B': return 0xb;
        case 'c': case 'C': return 0xc;
        case 'd': case 'D': return 0xd;
        case 'e': case 'E': return 0xe;
        case 'f': case 'F': return 0xf;
    default:
        THROW(SW_INVALID_PARAM);
    }
}

// Given a hex string with numhexchar characters, convert it
// to byte sequence and place in "bin" (which must be allocated
// with at least numhexchar/2 bytes already).
void hex2bin(uint8_t *hexstr, unsigned numhexchars, uint8_t *bin) {
    if (numhexchars % 2 != 0 || numhexchars == 0)
        THROW(SW_INVALID_PARAM);

    unsigned hexstr_start = 0;
    if (hexstr[0] == '0' && (hexstr[1] == 'x' || hexstr[1] == 'X')) {
        hexstr_start += 2;
    }

    for (unsigned binidx = 0, idx = 0; idx < numhexchars; idx += 2, binidx++) {
        uint8_t msn = hexchar2bin(hexstr[idx+hexstr_start]);
        msn <<= 4;
        uint8_t lsn = hexchar2bin(hexstr[idx+hexstr_start+1]);
        bin[binidx] = msn | lsn;
    }
}

int bin64b2dec(uint8_t *dst, uint32_t dst_len, uint64_t n) {
    if (n == 0) {
        if (dst_len < 2)
            FAIL("Insufficient destination buffer length to represent 0");
        dst[0] = '0';
        dst[1] = '\0';
        return 1;
    }
    // determine final length
    uint32_t len = 0;
    for (uint64_t nn = n; nn != 0; nn /= 10) {
        len++;
    }

    if (dst_len < len+1)
        FAIL("Insufficient destination buffer length for decimal representation.");

    // write digits in big-endian order
    for (int i = len - 1; i >= 0; i--) {
        dst[i] = (n % 10) + '0';
        n /= 10;
    }
    dst[len] = '\0';
    return len;
}

// https://stackoverflow.com/a/32567419/2128804
int strncmp( const char * s1, const char * s2, size_t n )
{
    while ( n && *s1 && ( *s1 == *s2 ) ) {
        ++s1;
        ++s2;
        --n;
    }
    if ( n == 0 ) {
        return 0;
    } else {
        return ( *(unsigned char *)s1 - *(unsigned char *)s2 );
    }
}

// https://stackoverflow.com/a/1733294/2128804
size_t strlen(const char *str)
{
    const char *s;

    for (s = str; *s; ++s)
      ;
    return (s - str);
}

// copy a c-string (including the terminating '\0'.
char *strcpy(char *dst, const char *src)
{
    unsigned i = 0;
    if (src == NULL) {
        dst[i] = '\0';
        return dst;
    }

    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return dst;
}

// /* Filter out leading zero's and non-digit characters in a null terminated string. */
// static void cleanse_input(char *buf) {
//   int len = strlen(buf);
//   assert (len < RADIX_TOKEN_UINT128_BUF_LEN);
//   int nextpos = 0;
//   bool seen_nonzero = false;

//   for (int i = 0; i < len; i++) {
//     char c = buf[i];
//     if (c == '0' && !seen_nonzero) {
//       continue;
//     }
//     if (isdigit(c)) {
//       seen_nonzero = true;
//       buf[nextpos++] = c;
//     }
//   }
//   assert (nextpos <= len);

//   if (nextpos == 0)
//     buf[nextpos++] = '0';

//   buf[nextpos] = '\0';
// }

// /* Removing trailing 0s and ".". */
// static void remove_trailing_zeroes(char *buf)
// {
//   int len = strlen(buf);
//   assert(len < RADIX_TOKEN_UINT128_BUF_LEN);

//   for (int i = len-1; i >= 0; i--) {
//     if (buf[i] == '0')
//       buf[i] = '\0';
//     else if (buf[i] == '.') {
//       buf[i] = '\0';
//       break;
//     } else {
//       break;
//     }
//   }
// }

// #define QA_RADIX_TOKEN_SHIFT 12
// #define QA_LI_SHIFT 6

/* Given a null terminated sequence of digits (value < UINT128_MAX),
 * divide it by "shift" and pretty print the result. */
// static void ToRadix(char *input, char *output, int shift)
// {
//   int len = strlen(input);
//   assert(len > 0 && len < RADIX_TOKEN_UINT128_BUF_LEN);
//   assert(shift == QA_RADIX_TOKEN_SHIFT || shift == QA_LI_SHIFT);

//   if (len <= shift) {
//     strcpy(output, "0.");
//     /* Insert (shift - len) 0s. */
//     for (int i = 0; i < (shift - len); i++) {
//       /* A bit inefficient, but it's ok, at most shift iterations. */
//       strcat(output, "0");
//     }
//     strcat(output, input);
//     remove_trailing_zeroes(output);
//     return;
//   }

//   /* len >= shift+1. Copy the first len-shift characters. */
//   strncpy(output, input, len - shift);
//   /* append a decimal point. */
//   strcpy(output + len - shift, ".");
//   /* copy the remaining characters in input. */
//   strcat(output, input + len - shift);
//   /* Remove trailing zeroes (after the decimal point). */
//   remove_trailing_zeroes(output);
// }

// void qa_to_zil(const char* qa, char* zil_buf, int zil_buf_len)
// {
//   int qa_len = strlen(qa);
//   assert(zil_buf_len >= RADIX_TOKEN_UINT128_BUF_LEN && qa_len < RADIX_TOKEN_UINT128_BUF_LEN);

//   char qa_buf[RADIX_TOKEN_UINT128_BUF_LEN];
//   strcpy(qa_buf, qa);
//   /* Cleanse the input. */
//   cleanse_input(qa_buf);
//   /* Convert Qa to Radix. */
//   ToRadix(qa_buf, zil_buf, QA_RADIX_TOKEN_SHIFT);
// }

// void qa_to_li(const char* qa, char* li_buf, int li_buf_len)
// {
//   int qa_len = strlen(qa);
//   assert(li_buf_len >= RADIX_TOKEN_UINT128_BUF_LEN && qa_len < RADIX_TOKEN_UINT128_BUF_LEN);

//   char qa_buf[RADIX_TOKEN_UINT128_BUF_LEN];
//   strcpy(qa_buf, qa);
//   /* Cleanse the input. */
//   cleanse_input(qa_buf);
//   /* Convert Qa to Li. */
//   ToRadix(qa_buf, li_buf, QA_LI_SHIFT);
// }
