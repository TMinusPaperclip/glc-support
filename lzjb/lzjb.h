#ifndef _LZJB_H_
#define _LZJB_H_

/* worst case: no copy items + 8bit bitmaps */
#define __lzjb_worstcase(size) (size) + ((size) / 8) + 1

size_t lzjb_compress(void *src, void *dst, size_t src_len);
size_t lzjb_decompress(void *src, void *dst, size_t src_len, size_t dst_len);

#endif
