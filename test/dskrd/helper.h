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

#if !defined HELPER_H_INCLUDED
# define HELPER_H_INCLUDED

# if defined __cpluslpus
extern "C" {
# endif
void myfmemcpy(void far *dst, const void far * const src, unsigned size);
void *mymalloc_debug(size_t n, unsigned long line);
void * filetomem(const char *fname, size_t length);
long memtofile(const char *fname, const void *mem, size_t length);
void far *get_1stdosdev(void);

# define peeko(p,o) *((unsigned char far *)(p) + (o))
# define peekow(p,o) *(unsigned short far *)((unsigned char far *)(p) + (o))
# define peekod(p,o) *(unsigned long far *)((unsigned char far *)(p) + (o))

# if defined __cpluslpus
}
# endif

# define mymalloc(n)  mymalloc_debug(n, __LINE__)
# ifdef DEBUG
#  define PLINE  { printf("line %lu\n", (unsigned long)(__LINE__)); }
# else
#  define PLINE
# endif

#endif /* HELPER_H_INCLUDED */

#if defined INCLUDE_HELPER_BODY && !defined HELPER_BODY_INCLUDED
# define HELPER_BODY_INCLUDED

void *mymalloc_debug(size_t n, unsigned long line)
{
	void *m = malloc(n ? n : 1);
	if (!m) {
		fprintf(stderr, "FATAL: memory allocation failure (%ubytes, at line %lu)\n", n, line);
		exit(-1);
	}
	return m;
}

void myfmemcpy(void far *dst, const void far * const src, unsigned size)
{
	const unsigned char far *s = (const unsigned char far *)src;
	unsigned char far *d = (unsigned char far *)dst;
	while(size--) *d++ = *s++;
}

static FILE * f2msub_fopen(const char *fname, const char *desc)
{
	FILE *f = fopen(fname, desc);
	if (!f) {
		fprintf(stderr, "Can't open '%s'\n");
	}
	return f;
}
void *filetomem(const char *fname, size_t length)
{
	char *m = mymalloc(length);
	FILE *f = f2msub_fopen(fname, "rb");

	fread(m, 1, length, f);
	fclose(f);
	return m;
}
long memtofile(const char *fname, const void *mem, size_t length)
{
	FILE *f = f2msub_fopen(fname, "wb");
	long n;
	
	n = fwrite(mem, 1, length, f);
	if (ferror(f)) n = -1L;
	fclose(f);
	return n;
}


void far *get_1stdosdev(void)
{
	unsigned offset = 0x22;
	union REGS r;
	struct SREGS sr;
	r.x.ax = 0x3000;
	intdosx(&r, &r, &sr);
	if (r.h.al == 2) offset = 0x17;
	else if (r.h.al == 3 && r.h.ah < 10) offset = 0x28;
	r.h.ah = 0x52;
	intdosx(&r, &r, &sr);
	return MK_FP(sr.es, r.x.bx + offset);
}

static const char helper_asmcode_devcall[] = {
	0x55,				/* PUSH BP */
	0x89, 0xE5,			/* MOV BP,SP */
	0x83, 0xEC, 0x04,	/* SUB SP,4 */
	0x53,				/* PUSH BX */
	0x56,				/* PUSH SI */
	0x57,				/* PUSH DI */
	0x1E,				/* PUSH DS */
	0x06,				/* PUSH ES */
	0xC4, 0x5E, 0x0A,	/* LES BX,[BP+2 + 4 + 4] */
	0xC5, 0x76, 0x06,	/* LDS SI,[BP+2 + 4] */
	0x8B, 0x44, 0x06,	/* MOV AX,[SI+6] */
	0x89, 0x46, 0xFC,	/* MOV [BP-4],AX */
	0x8C, 0x5E, 0xFE,	/* MOV [BP-2],DS */
	0x56,				/* PUSH SI */
	0x1E,				/* PUSH DS */
	0xFF, 0x5E, 0xFC,	/* CALL FAR [BP-4] */
	0x1F,				/* POP DS */
	0x5E,				/* POP SI */
	0x8B, 0x44, 0x08,	/* MOV AX,[SI+8] */
	0x89, 0x46, 0xFC,	/* MOV [BP-4],AX */
	0xFF, 0x5E, 0xFC,	/* CALL FAR [BP-4] */
	0x07,				/* POP ES */
	0x1F,				/* POP DS */
	0x5F,				/* POP DI */
	0x5E,				/* POP SI */
	0x5B,				/* POP BX */
	0x83, 0xC4, 0x04,	/* ADD SP,4 */
	0x5D,				/* POP BP */
	0xCB				/* RETF */
};

#ifdef LSI_C
typedef void (far *HELPER_DEVCALL_LSIC)(unsigned p0, unsigned p1, unsigned p2, unsigned p3, const void far *devhdr, void far *reqhdr);
# define devcall(d,r) (*(HELPER_DEVCALL_LSIC)helper_asmcode_devcall)(0,0,0,0,d,r)
#else
typedef void (far cdecl *HELPER_DEVCALL_CDECL)(const void far *devhdr, void far *reqhdr);
# define devcall(d,r) (*(HELPER_DEVCALL_CDECL)helper_asmcode_devcall)(d,r)
#endif


#endif /* HELPER_BODY_INCLUDED */


