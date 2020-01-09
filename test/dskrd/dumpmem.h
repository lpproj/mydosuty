/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

#if !defined DUMP_H_INCLUDED
# define DUMP_H_INCLUDED

# define DUMP_WITH_ASCII		0x0100U
# define DUMP_ADDRESS			0x0200U
# define DUMP_ADDRESS_ALIGNED	0x0600U

# if defined __cpluslpus
extern "C" {
# endif
void dumpmem(const void far *mem, size_t length, unsigned flags);
# if defined __cpluslpus
}
# endif

#endif /* DUMP_H_INCLUDED */

#if defined INCLUDE_DUMP_BODY && !defined DUMP_BODY_INCLUDED
# define DUMP_BODY_INCLUDED

void dumpmem(const void far *mem, size_t length, unsigned flags)
{
	const unsigned char far *p0 = mem;
	unsigned per_line = (flags & 0xff);
	unsigned total_cnt = 0;
	
	if (per_line == 0) per_line = 16;

	while(total_cnt < length) {
		const unsigned char far *pl = p0 + total_cnt;
		unsigned offset_line = 0;
		unsigned cnt_line;
		if (flags & DUMP_ADDRESS) {
			if ((flags & DUMP_ADDRESS_ALIGNED)==DUMP_ADDRESS_ALIGNED) {
				unsigned poff = FP_OFF(pl);
				offset_line = poff % per_line;
				pl = MK_FP(FP_SEG(pl), (poff - offset_line));
			}
			printf("%04X:%04X ", FP_SEG(pl), FP_OFF(pl));
		}
		else {
			printf("%04X ", total_cnt);
		}
		for(cnt_line=0; cnt_line < per_line; cnt_line++) {
			if (total_cnt + cnt_line >= length) break;
			if (cnt_line < offset_line)
				printf("   ");
			else
				printf(" %02X", pl[cnt_line]);
		}
		printf("\n");
		total_cnt += (cnt_line - offset_line);
	}
}


#endif /* DUMP_BODY_INCLUDED */
