// These are the offsets of various parts of a request APDU packet. INS
// identifies the requested command (see above), and P1 and P2 are parameters
// to the command.
#define CLA          0xAA // Alex and Alex
#define OFFSET_CLA   0x00
#define OFFSET_INS   0x01
#define OFFSET_P1    0x02
#define OFFSET_P2    0x03
#define OFFSET_LC    0x04
#define OFFSET_CDATA 0x05

// Use Radix's DER decode function for signing?
// (this shouldn't have any functional impact).
#define DER_DECODE_RADIX 0

// MACROS
#define PLOC() PRINTF("\n%s - %s:%d \n", __FILE__, __func__, __LINE__);
#define assert(x) \
    if (x) {} else { PLOC(); PRINTF("Assertion failed\n"); THROW (EXCEPTION); }
#define FAIL(x) \
    { \
        PLOC();\
        PRINTF("Radix ledger app failed: %s\n", x);\
        THROW(EXCEPTION); \
    }

// Constants
#define SHA256_HASH_DIGEST_BYTE_COUNT 32
#define RADIX_ADDRESS_BYTE_COUNT 38 // MagicByte(1) + PubKeyCompr(33) + Checksum(4)
#define RADIX_ADDRESS_BASE58_CHAR_COUNT 51
#define PUBLIC_KEY_COMPRESSEED_BYTE_COUNT 33
#define ECDSA_SIGNATURE_BYTE_COUNT 64

// exception codes
#define SW_DEVELOPER_ERR 0x6B00
#define SW_INVALID_PARAM 0x6B01
#define SW_IMPROPER_INIT 0x6B02
#define SW_USER_REJECTED 0x6985
#define SW_OK            0x9000

// macros for converting raw bytes to uint64_t
#define U8BE(buf, off) (((uint64_t)(U4BE(buf, off))     << 32) | ((uint64_t)(U4BE(buf, off + 4)) & 0xFFFFFFFF))
#define U8LE(buf, off) (((uint64_t)(U4LE(buf, off + 4)) << 32) | ((uint64_t)(U4LE(buf, off))     & 0xFFFFFFFF))

// FUNCTIONS

// Convert un-compressed zilliqa public key to a compressed form.
void compressPubKey(cx_ecfp_public_key_t *publicKey);

// pubkeyToRadixAddress converts a Ledger pubkey to a Radix wallet address.
void pubkeyToRadixAddress(uint8_t *dst, cx_ecfp_public_key_t *publicKey);

// deriveRadixPubKey derives a key pair from a BIP32 path and the Ledger
// seed. Returns the public key (private key is not needed).
void deriveRadixPubKey(uint32_t *bip32path, cx_ecfp_public_key_t *publicKey);

// // Three functions to stream the signature process. See deriveAndSign to do in a single operation.
// void deriveAndSignInit(zil_ecschnorr_t *T, uint32_t index);
// void deriveAndSignContinue(zil_ecschnorr_t *T, const uint8_t *msg, unsigned int msg_len);
// int deriveAndSignFinish(zil_ecschnorr_t *T, uint32_t index, unsigned char *dst, unsigned int dst_len);

// // deriveAndSign derives an ECFP private key from an user specified index and the Ledger seed,
// // and uses it to produce a ECDSA_SIGNATURE_BYTE_COUNT length signature of the provided message
// // The key is cleared from memory after signing.
// void deriveAndSign(uint8_t *dst, uint32_t dst_len, uint32_t index, const uint8_t *msg, unsigned int msg_len);

// Converts a null-terminated buffer containing Qa to Radix / Li
// assert (zil/li_buf_len >= RADIX_TOKEN_UINT128_BUF_LEN);
// void qa_to_zil(const char* qa, char* zil_buf, int zil_buf_len);
// void qa_to_li(const char* qa, char* li_buf, int li_buf_len);

// BYTE UTILS

// bin2hex converts binary to hex and appends a final NUL byte.
void bin2hex(uint8_t *dst, uint64_t dstlen, uint8_t *data, uint64_t inlen);

// bin64b2dec converts an unsigned integer to a decimal string and appends a
// final NUL byte. It returns the length of the string.
int bin64b2dec(uint8_t *dst, uint32_t dst_len, uint64_t n);

// Given a hex string with numhexchar characters, convert it
// to byte sequence and place in "bin" (which must be allocated
// with at least numhexchar/2 bytes already).
void hex2bin(uint8_t *hexstr, unsigned numhexchars, uint8_t *bin);

// Equivalent to what is there in stdlib.
int strncmp( const char * s1, const char * s2, size_t n );
// Equivalent to what is there in stdlib.
size_t strlen(const char *str);
// Equivalent to what is there in stdlib.
char* strcpy(char *dst, const char *src);