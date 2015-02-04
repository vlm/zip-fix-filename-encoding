#include <sys/types.h>
#include <stdint.h>

#include "UTF8String.h"

/*
 * This is the table of length expectations.
 * The second half of this table is only applicable to the long sequences.
 */
static int UTF8String_ht[2][16] = {
	{ /* 0x0 ... 0x7 */
	  /* 0000..0111 */
	  1, 1, 1, 1, 1, 1, 1, 1,
	  /* 1000..1011(0), 1100..1101(2), 1110(3), 1111(-1) */
	  0, 0, 0, 0, 2, 2, 3, -1 },
	{ /* 0xF0 .. 0xF7 */
	  /* 11110000..11110111 */
	  4, 4, 4, 4, 4, 4, 4, 4,
	  5, 5, 5, 5, 6, 6, -1, -1 }
};
static int32_t UTF8String_mv[7] = { 0, 0,
	0x00000080,
	0x00000800,
	0x00010000,
	0x00200000,
	0x04000000
};

/* Internal aliases for return codes */
#define	U8E_TRUNC	-1	/* UTF-8 sequence truncated */
#define	U8E_ILLSTART	-2	/* Illegal UTF-8 sequence start */
#define	U8E_NOTCONT	-3	/* Continuation expectation failed */
#define	U8E_NOTMIN	-4	/* Not minimal length encoding */
#define	U8E_EINVAL	-5	/* Invalid arguments */

static ssize_t
UTF8String__process(const char *s, size_t l, uint32_t *dst, size_t dstlen) {
	size_t length;
	const uint8_t *buf = (uint8_t *)s;
	const uint8_t *end = (uint8_t *)buf + l;
	uint32_t *dstend = dst + dstlen;

	for(length = 0; buf < end; length++) {
		int ch = *buf;
		const uint8_t *cend;
		int32_t value;
		int want;

		/* Compute the sequence length */
		want = UTF8String_ht[0][ch >> 4];
		switch(want) {
		case -1:
			/* Second half of the table, long sequence */
			want = UTF8String_ht[1][ch & 0x0F];
			if(want != -1) break;
			/* Fall through */
		case 0:
			return U8E_ILLSTART;
		}

		/* assert(want >= 1 && want <= 6) */

		/* Check character sequence length */
		if(buf + want > end) return U8E_TRUNC;

		value = ch & (0xff >> want);
		cend = buf + want;
		for(buf++; buf < cend; buf++) {
			ch = *buf;
			if(ch < 0x80 || ch > 0xbf) return U8E_NOTCONT;
			value = (value << 6) | (ch & 0x3F);
		}
		if(value < UTF8String_mv[want])
			return U8E_NOTMIN;
		if(dst < dstend)
			*dst++ = value;	/* Record value */
	}

	if(dst < dstend) *dst = 0;	/* zero-terminate */

	return length;
}


ssize_t
UTF8String_length(const char *s, size_t l) {
	if(s) {
		return UTF8String__process(s, l, 0, 0);
	} else {
		return U8E_EINVAL;
	}
}
