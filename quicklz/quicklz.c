/*
quicklz - QuickLZ compression
Copyright (C) 2006  Lasse Reinhold (lar@quicklz.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 Based on QuickLZ (http://www.quicklz.com,
                   http://neopsis.com/projects/seom/)
 Copyright 2006, Lasse Reinhold (lar@quicklz.com)
           2007, Tomas Carnecky (tom@dbservice.com)
*/

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include "quicklz.h"

#define u8ptr(ptr) ((u_int8_t *) ptr)
#define u8(ptr) *u8ptr(ptr)
#define u16ptr(ptr) ((u_int16_t *) (ptr))
#define u16(ptr) *u16ptr(ptr)
#define u32ptr(ptr) ((u_int32_t *) (ptr))
#define u32(ptr) *u32ptr(ptr)
#define quicklz_hash(val) (((val) >> 12) ^ (val)) & 0x0fff

static void quicklz_expand(uintptr_t to, uintptr_t from, size_t len)
{
	uintptr_t end = to + len;

	if (from + len > to) {
		while (to < end)
			u8(to++) = u8(from++);
	} else
		memcpy((void *) to, (void *) from, len);
}

__attribute__ ((visibility ("default")))
int quicklz_compress(const unsigned char *_from, unsigned char *_to,
		     size_t uncompressed_size, size_t *compressed_size,
		     uintptr_t *hashtable)
{
	unsigned char counter = 0;
	unsigned char *cbyte;
	uintptr_t from, to, orig, len, offs, end;
	unsigned int i;
	unsigned char val;
	unsigned short hash;

	from = (uintptr_t) _from;
	to = (uintptr_t) _to;
	end = from + uncompressed_size;

	cbyte = u8ptr(to++);
	for (i = 0; i < 4096; i++)
		hashtable[i] = from;

	/* RLE test assumes that 5 bytes are available */
	while (from < end - 5) {
		if (u32(from) == u32(from + 1)) {
			/*
			 at least 5 subsequent bytes are same => use RLE
			 RLE: 1111 aaaa  aaaa aaaa  bbbb bbbb (24b)
			 where a (12b) is length (-5) and
			       b (8b) is value
			*/
			val = u8(from);
			from += 5;
			orig = from;
			/* max length is 0x0fff (12b) */
			len = (orig + 0x0fff) < end ? (orig + 0x0fff) : end;
			while ((from < len) && (val == u8(from)))
				from++;
			u8(to++) = 0xf0 | ((from - orig) >> 8);
			u8(to++) = (from - orig);
			u8(to++) = val;
			*cbyte = (*cbyte << 1) | 1;
		} else {
			/*
			 hash table is used to find possible match for some
			 at least 24- or 32-bit string
			*/
			hash = quicklz_hash(u32(from));
			orig = hashtable[hash]; /* fetch possible match */
			hashtable[hash] = from; /* store this position */

			offs = from - orig;
			if ((offs < 131072) && /* max lz offs is 17b */
			    (offs > 3) && /* we test at least 4 bytes */
			    ((u32(orig) & 0xffffff) == (u32(from) & 0xffffff))) {
			    	/* at least 3 bytes match */
				if (u32(orig) == u32(from)) {
					/* at least 4 bytes match */
					*cbyte = (*cbyte << 1) | 1;
					len = 0;

					while ((u8(orig + len + 4) == u8(from + len + 4)) &&
					       (len < 2047) && /* max len is 11b */
					       (from + len + 4 < end))
						len++;
					from += len + 4;

					if ((len < 8) && (offs < 1024)) {
						/*
						 LZ: 101a aabb  bbbb bbbb (16b)
						 where a (3b) is length (-4) and
						       b (10b) is offs
						*/
						u8(to++) = 0xa0 | (len << 2) | (offs >> 8);
						u8(to++) = offs;
					} else if ((len < 32) && (offs < 65536)) {
						/*
						 LZ: 110a aaaa  bbbb bbbb  bbbb bbbb (24b)
						 where a (5b) is length (-4) and
						       b (16b) is offs
						*/
						u8(to++) = 0xc0 | len;
						u8(to++) = (offs >> 8);
						u8(to++) = offs;
					} else {
						/*
						 LZ: 1110 aaaa  aaaa aaab  bbbb bbbb  bbbb bbbb (32b)
						 where a (11b) is length (-4) and
						       b (17b) is offs
						*/
						u8(to++) = 0xe0 | (len >> 7);
						u8(to++) = (len << 1) | (offs >> 16);
						u8(to++) = (offs >> 8);
						u8(to++) = offs;
					}
				} else {
					/* just 3 bytes match */
					if (offs < 128) {
						/*
						 LZ: 0aaa aaaa (8b)
						 where a (7b) is offs and
						       length is always 3
						*/
						u8(to++) = offs;
						*cbyte = (*cbyte << 1) | 1;
						from += 3;
					} else if (offs < 8192) {
						/*
						 LZ: 100a aaaa  aaaa aaaa (16b)
						 where a (13b) is offs and
						       length is always 3
						*/
						u8(to++) = 0x80 | offs >> 8;
						u8(to++) = offs;
						*cbyte = (*cbyte << 1) | 1;
						from += 3;
					} else {
						/* plain value */
						u8(to++) = u8(from++);
						*cbyte = (*cbyte << 1);
					}
				}
			} else {
				/* plain value */
				u8(to++) = u8(from++);
				*cbyte = (*cbyte << 1);
			}
		}

		/*
		 There is a control byte for every 8 commands or
		 plain values. 0 means plain value and 1 RLE or LZ msg.
		*/
		if (++counter == 8) {
			cbyte = u8ptr(to++);
			counter = 0;
		}
	}

	/* last bytes */
	while (from < end) {
		u8(to++) = u8(from++);
		*cbyte = (*cbyte << 1);

		if (++counter == 8) {
			cbyte = u8ptr(to++);
			counter = 0;
		}
	}

	*cbyte = (*cbyte << (8 - counter));

	*compressed_size = to - (uintptr_t) _to;
	return 0;
}

__attribute__ ((visibility ("default")))
int quicklz_decompress(const unsigned char *_from, unsigned char *_to,
		       size_t uncompressed_size)
{
	uintptr_t end, from, to, offs, len;
	unsigned char cbyte, counter;

	from = (uintptr_t) _from;
	to = (uintptr_t) _to;
	end = to + uncompressed_size;
	cbyte = u8(from++);
	counter = 0;

	while (to < end - 5) {
		if (cbyte & (1 << 7)) { /* LZ match or RLE sequence */
			if ((u8(from) & 0x80) == 0) { /* 7bits offs */
				offs = u8(from);
				quicklz_expand(to, to - offs, 3);
				to += 3;
				from += 1;
			} else if ((u8(from) & 0x60) == 0) { /* 13bits offs */
				offs = ((u8(from) & 0x1f) << 8) | u8(from + 1);
				quicklz_expand(to, to - offs, 3);
				to += 3;
				from += 2;
			} else if ((u8(from) & 0x40) == 0) { /* 10bits offs, 3bits length */
				len = ((u8(from) >> 2) & 7) + 4;
				offs = ((u8(from) & 0x03) << 8) | u8(from + 1);
				quicklz_expand(to, to - offs, len);
				to += len;
				from += 2;
			} else if ((u8(from) & 0x20) == 0) { /* 16bits offs, 5bits length */
				len = (u8(from) & 0x1f) + 4;
				offs = (u8(from + 1) << 8) | u8(from + 2);
				quicklz_expand(to, to - offs, len);
				to += len;
				from += 3;
			} else if ((u8(from) & 0x10) == 0) { /* 17bits offs, 11bits length */
				len = (((u8(from) & 0x0f) << 7) | (u8(from + 1) >> 1)) + 4;
				offs = ((u8(from + 1) & 0x01) << 16) | (u8(from + 2) << 8) | (u8(from + 3));
				quicklz_expand(to, to - offs, len);
				to += len;
				from += 4;
			} else { /* RLE sequence */
				len = (((u8(from) & 0x0f) << 8) | u8(from + 1)) + 5;
				memset(u8ptr(to), u8(from + 2), len);
				to += len;
				from += 3;
			}
		} else /* literal */
			u8(to++) = u8(from++);

		cbyte <<= 1;
		if (++counter == 8) { /* fetch control byte */
			cbyte = u8(from++);
			counter = 0;
		}
	}

	while (to < end) {
		u8(to++) = u8(from++);
		if (++counter == 8) {
			counter = 0;
			++from;
		}
	}

	return 0;
}
