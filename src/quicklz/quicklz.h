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

#define __quicklz_worstcase(size) (size) + ((size) / 8) + 1
#define __quicklz_hashtable sizeof(uintptr_t) * 4096

extern int quicklz_compress(const unsigned char *from, unsigned char *to,
			    size_t uncompressed_size, size_t *compressed_size,
			    uintptr_t *hashtable);
extern int quicklz_decompress(const unsigned char *from, unsigned char *to,
			      size_t compressed_size);
