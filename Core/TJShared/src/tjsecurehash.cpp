/* This file is part of TJShow. TJShow is free software: you 
 * can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later 
 * version.
 * 
 * TJShow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with TJShow.  If not, see <http://www.gnu.org/licenses/>. */
 
 #include "../include/tjcrypto.h"
#include "../include/tjutil.h"
using namespace tj::shared;
#include <string>

typedef unsigned int uint32;


// These functions perform MD5 operations. The simplest call is MD5Sum to
// generate the MD5 sum of the given data.
//
// You can also compute the MD5 sum of data incrementally by making multiple
// calls to MD5Update:
//    MD5Context ctx; // intermediate MD5 data: do not use
//    MD5Init(&ctx);
//    MD5Update(&ctx, data1, length1);
//    MD5Update(&ctx, data2, length2);
//    ...
//
//    MD5Digest digest; // the result of the computation
//    MD5Final(&digest, &ctx);
//
// You can call MD5DigestToBase16 to generate a string of the digest.

// The output of an MD5 operation
typedef struct MD5Digest_struct {
  unsigned char a[16];
} MD5Digest;

// Used for storing intermediate data during an MD5 computation. Callers
// should not access the data.
typedef char MD5Context[88];

// Computes the MD5 sum of the given data buffer with the given length.
// The given 'digest' structure will be filled with the result data.
void MD5Sum(const void* data, size_t length, MD5Digest* digest);

// Initializes the given MD5 context structure for subsequent calls to
// MD5Update.
void MD5Init(MD5Context* context);

// For the given buffer of data, updates the given MD5 context with the sum of
// the data. You can call this any number of times during the computation,
// exept that MD5Init must have been called first.
void MD5Update(MD5Context* context, const void* buf, size_t len);

// Finalizes the MD5 operation and fills the buffer with the digest.
void MD5Final(MD5Digest* digest, MD5Context* pCtx);

// Converts a digest into human-readable hexadecimal.
std::string MD5DigestToBase16(const MD5Digest& digest);

// Returns the MD5 (in hexadecimal) of a string.
std::string MD5String(const std::string& str);

// The original file was copied from sqlite, and was in the public domain.
// Modifications Copyright 2006 Google Inc. All Rights Reserved

/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

struct Context {
  uint32 buf[4];
  uint32 bits[2];
  unsigned char in[64];
};

/* Note: this code is harmless on little-endian machines. */
static void byteReverse (unsigned char *buf, unsigned longs) {
    uint32 t;
    do {
            t = (uint32)((unsigned)buf[3]<<8 | buf[2]) << 16 |
                        ((unsigned)buf[1]<<8 | buf[0]);
            *(uint32 *)buf = t;
            buf += 4;
    } while (--longs);
}

/* The four core functions - F1 is optimized somewhat */
/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
        ( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void MD5Transform(uint32 buf[4], const uint32 in[16]){
        register uint32 a, b, c, d;

        a = buf[0];
        b = buf[1];
        c = buf[2];
        d = buf[3];

        MD5STEP(F1, a, b, c, d, in[ 0]+0xd76aa478,  7);
        MD5STEP(F1, d, a, b, c, in[ 1]+0xe8c7b756, 12);
        MD5STEP(F1, c, d, a, b, in[ 2]+0x242070db, 17);
        MD5STEP(F1, b, c, d, a, in[ 3]+0xc1bdceee, 22);
        MD5STEP(F1, a, b, c, d, in[ 4]+0xf57c0faf,  7);
        MD5STEP(F1, d, a, b, c, in[ 5]+0x4787c62a, 12);
        MD5STEP(F1, c, d, a, b, in[ 6]+0xa8304613, 17);
        MD5STEP(F1, b, c, d, a, in[ 7]+0xfd469501, 22);
        MD5STEP(F1, a, b, c, d, in[ 8]+0x698098d8,  7);
        MD5STEP(F1, d, a, b, c, in[ 9]+0x8b44f7af, 12);
        MD5STEP(F1, c, d, a, b, in[10]+0xffff5bb1, 17);
        MD5STEP(F1, b, c, d, a, in[11]+0x895cd7be, 22);
        MD5STEP(F1, a, b, c, d, in[12]+0x6b901122,  7);
        MD5STEP(F1, d, a, b, c, in[13]+0xfd987193, 12);
        MD5STEP(F1, c, d, a, b, in[14]+0xa679438e, 17);
        MD5STEP(F1, b, c, d, a, in[15]+0x49b40821, 22);

        MD5STEP(F2, a, b, c, d, in[ 1]+0xf61e2562,  5);
        MD5STEP(F2, d, a, b, c, in[ 6]+0xc040b340,  9);
        MD5STEP(F2, c, d, a, b, in[11]+0x265e5a51, 14);
        MD5STEP(F2, b, c, d, a, in[ 0]+0xe9b6c7aa, 20);
        MD5STEP(F2, a, b, c, d, in[ 5]+0xd62f105d,  5);
        MD5STEP(F2, d, a, b, c, in[10]+0x02441453,  9);
        MD5STEP(F2, c, d, a, b, in[15]+0xd8a1e681, 14);
        MD5STEP(F2, b, c, d, a, in[ 4]+0xe7d3fbc8, 20);
        MD5STEP(F2, a, b, c, d, in[ 9]+0x21e1cde6,  5);
        MD5STEP(F2, d, a, b, c, in[14]+0xc33707d6,  9);
        MD5STEP(F2, c, d, a, b, in[ 3]+0xf4d50d87, 14);
        MD5STEP(F2, b, c, d, a, in[ 8]+0x455a14ed, 20);
        MD5STEP(F2, a, b, c, d, in[13]+0xa9e3e905,  5);
        MD5STEP(F2, d, a, b, c, in[ 2]+0xfcefa3f8,  9);
        MD5STEP(F2, c, d, a, b, in[ 7]+0x676f02d9, 14);
        MD5STEP(F2, b, c, d, a, in[12]+0x8d2a4c8a, 20);

        MD5STEP(F3, a, b, c, d, in[ 5]+0xfffa3942,  4);
        MD5STEP(F3, d, a, b, c, in[ 8]+0x8771f681, 11);
        MD5STEP(F3, c, d, a, b, in[11]+0x6d9d6122, 16);
        MD5STEP(F3, b, c, d, a, in[14]+0xfde5380c, 23);
        MD5STEP(F3, a, b, c, d, in[ 1]+0xa4beea44,  4);
        MD5STEP(F3, d, a, b, c, in[ 4]+0x4bdecfa9, 11);
        MD5STEP(F3, c, d, a, b, in[ 7]+0xf6bb4b60, 16);
        MD5STEP(F3, b, c, d, a, in[10]+0xbebfbc70, 23);
        MD5STEP(F3, a, b, c, d, in[13]+0x289b7ec6,  4);
        MD5STEP(F3, d, a, b, c, in[ 0]+0xeaa127fa, 11);
        MD5STEP(F3, c, d, a, b, in[ 3]+0xd4ef3085, 16);
        MD5STEP(F3, b, c, d, a, in[ 6]+0x04881d05, 23);
        MD5STEP(F3, a, b, c, d, in[ 9]+0xd9d4d039,  4);
        MD5STEP(F3, d, a, b, c, in[12]+0xe6db99e5, 11);
        MD5STEP(F3, c, d, a, b, in[15]+0x1fa27cf8, 16);
        MD5STEP(F3, b, c, d, a, in[ 2]+0xc4ac5665, 23);

        MD5STEP(F4, a, b, c, d, in[ 0]+0xf4292244,  6);
        MD5STEP(F4, d, a, b, c, in[ 7]+0x432aff97, 10);
        MD5STEP(F4, c, d, a, b, in[14]+0xab9423a7, 15);
        MD5STEP(F4, b, c, d, a, in[ 5]+0xfc93a039, 21);
        MD5STEP(F4, a, b, c, d, in[12]+0x655b59c3,  6);
        MD5STEP(F4, d, a, b, c, in[ 3]+0x8f0ccc92, 10);
        MD5STEP(F4, c, d, a, b, in[10]+0xffeff47d, 15);
        MD5STEP(F4, b, c, d, a, in[ 1]+0x85845dd1, 21);
        MD5STEP(F4, a, b, c, d, in[ 8]+0x6fa87e4f,  6);
        MD5STEP(F4, d, a, b, c, in[15]+0xfe2ce6e0, 10);
        MD5STEP(F4, c, d, a, b, in[ 6]+0xa3014314, 15);
        MD5STEP(F4, b, c, d, a, in[13]+0x4e0811a1, 21);
        MD5STEP(F4, a, b, c, d, in[ 4]+0xf7537e82,  6);
        MD5STEP(F4, d, a, b, c, in[11]+0xbd3af235, 10);
        MD5STEP(F4, c, d, a, b, in[ 2]+0x2ad7d2bb, 15);
        MD5STEP(F4, b, c, d, a, in[ 9]+0xeb86d391, 21);

        buf[0] += a;
        buf[1] += b;
        buf[2] += c;
        buf[3] += d;
}

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void MD5Init(MD5Context *pCtx) {
        struct Context *ctx = (struct Context *)pCtx;
        ctx->buf[0] = 0x67452301;
        ctx->buf[1] = 0xefcdab89;
        ctx->buf[2] = 0x98badcfe;
        ctx->buf[3] = 0x10325476;
        ctx->bits[0] = 0;
        ctx->bits[1] = 0;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void MD5Update(MD5Context *pCtx, const void *inbuf, size_t len) {
        struct Context *ctx = (struct Context *)pCtx;
        const unsigned char* buf = (const unsigned char*)inbuf;
        uint32 t;

        /* Update bitcount */

        t = ctx->bits[0];
        if ((ctx->bits[0] = t + ((uint32)len << 3)) < t)
                ctx->bits[1]++; /* Carry from low to high */
        ctx->bits[1] += static_cast<uint32>(len >> 29);

        t = (t >> 3) & 0x3f;    /* Bytes already in shsInfo->data */

        /* Handle any leading odd-sized chunks */

        if ( t ) {
                unsigned char *p = (unsigned char *)ctx->in + t;

                t = 64-t;
                if (len < t) {
                        memcpy(p, buf, len);
                        return;
                }
                memcpy(p, buf, t);
                byteReverse(ctx->in, 16);
                MD5Transform(ctx->buf, (uint32 *)ctx->in);
                buf += t;
                len -= t;
        }

        /* Process data in 64-byte chunks */

        while (len >= 64) {
                memcpy(ctx->in, buf, 64);
                byteReverse(ctx->in, 16);
                MD5Transform(ctx->buf, (uint32 *)ctx->in);
                buf += 64;
                len -= 64;
        }

        /* Handle any remaining bytes of data. */

        memcpy(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void MD5Final(MD5Digest* digest, MD5Context *pCtx) {
        struct Context *ctx = (struct Context *)pCtx;
        unsigned count;
        unsigned char *p;

        /* Compute number of bytes mod 64 */
        count = (ctx->bits[0] >> 3) & 0x3F;

        /* Set the first char of padding to 0x80.  This is safe since there is
           always at least one byte free */
        p = ctx->in + count;
        *p++ = 0x80;

        /* Bytes of padding needed to make 64 bytes */
        count = 64 - 1 - count;

        /* Pad out to 56 mod 64 */
        if (count < 8) {
                /* Two lots of padding:  Pad the first block to 64 bytes */
                memset(p, 0, count);
                byteReverse(ctx->in, 16);
                MD5Transform(ctx->buf, (uint32 *)ctx->in);

                /* Now fill the next block with 56 bytes */
                memset(ctx->in, 0, 56);
        } else {
                /* Pad block to 56 bytes */
                memset(p, 0, count-8);
        }
        byteReverse(ctx->in, 14);

        /* Append length in bits and transform */
        ((uint32 *)ctx->in)[ 14 ] = ctx->bits[0];
        ((uint32 *)ctx->in)[ 15 ] = ctx->bits[1];

        MD5Transform(ctx->buf, (uint32 *)ctx->in);
        byteReverse((unsigned char *)ctx->buf, 4);
        memcpy(digest->a, ctx->buf, 16);
        memset(ctx, 0, sizeof(ctx));    /* In case it's sensitive */
}

std::string MD5DigestToBase16(const MD5Digest& digest){
  static char const zEncode[] = "0123456789abcdef";

  std::string ret;
  ret.resize(32);

  int j = 0;
  for(int i = 0; i < 16; i ++){
    int a = digest.a[i];
    ret[j++] = zEncode[(a>>4)&0xf];
    ret[j++] = zEncode[a & 0xf];
  }
  return ret;
}

void MD5Sum(const void* data, size_t length, MD5Digest* digest) {
  MD5Context ctx;
  MD5Init(&ctx);
  MD5Update(&ctx, static_cast<const unsigned char*>(data), length);
  MD5Final(digest, &ctx);
}

SecureHash::SecureHash() {
	MD5Context* ctx = reinterpret_cast<MD5Context*>(malloc(sizeof(MD5Context)));
	MD5Init(ctx);
	_data = reinterpret_cast<void*>(ctx);
}

SecureHash::~SecureHash() {
	free(_data);
}

void SecureHash::AddData(const void* data, size_t length) {
	MD5Context* ctx = reinterpret_cast<MD5Context*>(_data);
	MD5Update(ctx, data, length);
}

std::string SecureHash::GetHashAsString() {
	MD5Digest digest;
	MD5Final(&digest, reinterpret_cast<MD5Context*>(_data));
	return MD5DigestToBase16(digest);
}

void SecureHash::AddString(const wchar_t* data) {
	AddData(data, wcslen(data));
}

void SecureHash::AddFile(const String& path) {
	#ifdef TJ_OS_WIN
		HANDLE file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, NULL, NULL, NULL);
		if(file==INVALID_HANDLE_VALUE) {
			Throw(L"Could not open file for hashing", ExceptionTypeError);
		}

		unsigned char buffer[1024];
		DWORD read = 0;
		while(ReadFile(file, buffer, 1024, &read, NULL)) {
			if(read==0) break; // EOF
			AddData(buffer, read);
		}

		CloseHandle(file);
	#endif
	
	#ifdef TJ_OS_POSIX
		std::string mbsPath = Mbs(path);
		FILE* fp = fopen(mbsPath.c_str(), "rb");
		unsigned char buffer[1024];
		if(fp!=NULL) {
			unsigned int read = 1;
			while(read>0) {
				read = fread(buffer, 1, 1024, fp);
				if(read>0) {
					AddData(buffer, read);
				}
			}
		}
		fclose(fp);
	#endif
}