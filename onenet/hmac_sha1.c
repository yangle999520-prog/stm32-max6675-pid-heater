/* Streaming HMAC-SHA1: bounded buffers for the STM32F030's 4 KB RAM. */
#include "hmac_sha1.h"
#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t h[5];
    uint32_t bytes;
    unsigned int used;
    unsigned char block[64];
} SHA1_Context;

static uint32_t rol(uint32_t x, unsigned int n)
{
    return (x << n) | (x >> (32 - n));
}

static void sha1_block(SHA1_Context *ctx)
{
    uint32_t w[16], a, b, c, d, e, f, k, temp;
    unsigned int t, j;
    for(t = 0; t < 16; t++) {
        j = 4 * t;
        w[t] = ((uint32_t)ctx->block[j] << 24) |
               ((uint32_t)ctx->block[j + 1] << 16) |
               ((uint32_t)ctx->block[j + 2] << 8) | ctx->block[j + 3];
    }
    a = ctx->h[0]; b = ctx->h[1]; c = ctx->h[2];
    d = ctx->h[3]; e = ctx->h[4];
    for(t = 0; t < 80; t++) {
        if(t >= 16)
            w[t & 15] = rol(w[(t - 3) & 15] ^ w[(t - 8) & 15] ^
                            w[(t - 14) & 15] ^ w[t & 15], 1);
        if(t < 20) { f = (b & c) | (~b & d); k = 0x5a827999; }
        else if(t < 40) { f = b ^ c ^ d; k = 0x6ed9eba1; }
        else if(t < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdc; }
        else { f = b ^ c ^ d; k = 0xca62c1d6; }
        temp = rol(a, 5) + f + e + k + w[t & 15];
        e = d; d = c; c = rol(b, 30); b = a; a = temp;
    }
    ctx->h[0] += a; ctx->h[1] += b; ctx->h[2] += c;
    ctx->h[3] += d; ctx->h[4] += e;
}

static void sha1_init(SHA1_Context *ctx)
{
    ctx->h[0] = 0x67452301; ctx->h[1] = 0xefcdab89;
    ctx->h[2] = 0x98badcfe; ctx->h[3] = 0x10325476;
    ctx->h[4] = 0xc3d2e1f0; ctx->bytes = 0; ctx->used = 0;
}

static void sha1_update(SHA1_Context *ctx, const unsigned char *data, unsigned int len)
{
    unsigned int n;
    ctx->bytes += len;
    while(len) {
        n = 64 - ctx->used;
        if(n > len) n = len;
        memcpy(ctx->block + ctx->used, data, n);
        ctx->used += n; data += n; len -= n;
        if(ctx->used == 64) { sha1_block(ctx); ctx->used = 0; }
    }
}

static void sha1_finish(SHA1_Context *ctx, unsigned char *digest)
{
    uint32_t bits_low = ctx->bytes << 3, bits_high = ctx->bytes >> 29;
    unsigned int i;
    ctx->block[ctx->used++] = 0x80;
    if(ctx->used > 56) {
        memset(ctx->block + ctx->used, 0, 64 - ctx->used);
        sha1_block(ctx); ctx->used = 0;
    }
    memset(ctx->block + ctx->used, 0, 56 - ctx->used);
    for(i = 0; i < 4; i++) {
        ctx->block[56 + i] = (unsigned char)(bits_high >> (24 - 8 * i));
        ctx->block[60 + i] = (unsigned char)(bits_low >> (24 - 8 * i));
    }
    sha1_block(ctx);
    for(i = 0; i < 20; i++)
        digest[i] = (unsigned char)(ctx->h[i / 4] >> (24 - 8 * (i % 4)));
}

void hmac_sha1(unsigned char *key, int key_length,
               unsigned char *data, int data_length, unsigned char *digest)
{
    SHA1_Context ctx;
    unsigned char pad[64];
    unsigned int i;
    memset(pad, 0, sizeof(pad));
    if(key_length > 64) {
        sha1_init(&ctx);
        sha1_update(&ctx, key, (unsigned int)key_length);
        sha1_finish(&ctx, pad);
    } else {
        memcpy(pad, key, (unsigned int)key_length);
    }
    for(i = 0; i < 64; i++) pad[i] ^= 0x36;
    sha1_init(&ctx);
    sha1_update(&ctx, pad, 64);
    sha1_update(&ctx, data, (unsigned int)data_length);
    sha1_finish(&ctx, digest);
    for(i = 0; i < 64; i++) pad[i] ^= 0x36 ^ 0x5c;
    sha1_init(&ctx);
    sha1_update(&ctx, pad, 64);
    sha1_update(&ctx, digest, 20);
    sha1_finish(&ctx, digest);
}
