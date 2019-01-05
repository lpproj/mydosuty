/*
ROMSPCOM: split or combine rom image
          (single image into two even/odd images, or vice versa)


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

#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char *myarg[4];

enum { MODE_NONE = 0, MODE_SPLIT, MODE_COMBINE };
int op_mode = MODE_NONE;
int opt_help = 0;
int opt_overwrite = 0;

int myopt(int argc, char *argv[])
{
    unsigned nparam = 0;

    while(argc > 0) {
        char *s = *argv;
        if (*s == '-')
        {
            ++s;
            switch(*s) {
                case '?': case 'H': case 'h':
                    opt_help = 1;
                    break;
                case 's': op_mode = MODE_SPLIT; break;
                case 'c': op_mode = MODE_COMBINE; break;
                case 'f': opt_overwrite = 1; break;
            }
        }
        else
        {
            if (nparam < ((sizeof(myarg) / sizeof(myarg[0]) - 1))) {
                myarg[nparam++] = s;
                myarg[nparam] = NULL;
            }
        }
        ++argv;
        --argc;
    }

    return (int)nparam;
}

void bootlogo(void)
{
    printf("ROMSPCOM: split or combine rom image");
    printf(" (built at " __DATE__ " " __TIME__ ")\n");
}

void usage(void)
{
    const char msg[] = 
        "Usage: romspcom -s source_rom output_rom_even output_rom_odd\n"
        "  or:  romspcom -c source_rom_even source_rom_odd output_rom\n"
        "\n"
        "  -s  split single image into even/odd images\n"
        "  -c  combine even/odd images into single output_rom\n"
        ;
    printf("%s", msg);
}


static
FILE *my_fopen(const char *fn, const char *fm)
{
#if (INT_MAX <= 32767)
# define FBUF 1024
#else
# define FBUF 4096
#endif
    FILE *f = fopen(fn, fm);
    if (f) {
        if (setvbuf(f, NULL, _IOFBF, FBUF) != 0) {
            fprintf(stderr, "FATAL: memory allocation failure.\n");
            exit(1);
        }
    }
    else {
        fprintf(stderr, "ERROR: can't open '%s'\n", fn);
    }
    return f;
}


int split_romfile(const char *src, const char *dest_even, const char *dest_odd)
{
    int rc = 0;
    FILE *fs, *fe, *fo;
    
    fs = my_fopen(src, "rb");
    if (fs) fe = my_fopen(dest_even, "wb");
    if (fe) fo = my_fopen(dest_odd, "wb");
    if (fs && fe && fo) {
        long cnt = 0;
        int c;
        while((c = fgetc(fs)) != EOF) {
            if (fputc(c, (cnt&1) ? fo : fe ) == EOF) {
                fprintf(stderr, "ERROR: '%s' write error\n", (cnt&1) ? dest_odd : dest_even);
                rc = -1;
                break;
            }
            ++cnt;
        }
        printf("%ld bytes red\n", cnt);
    }
    else {
        rc = -1;
    }
    if (fe) fclose(fe);
    if (fo) fclose(fo);
    if (fs) fclose(fs);

    return rc;
}

int combine_romfile(const char *src_even, const char *src_odd, const char *dest)
{
    int rc = 0;
    FILE *fe, *fo, *fd;
    int done_warn = 0;
    
    fe = my_fopen(src_even, "rb");
    if (fe) fo = my_fopen(src_odd, "rb");
    if (fo) fd = my_fopen(dest, "wb");
    if (fe && fo && fd) {
        long cnt = 0;
        int ce, co;
        while(1) {
            ce = fgetc(fe);
            co = fgetc(fo);
            if (ce == EOF && co == EOF) break;
            if ((ce == EOF) ^ (co == EOF)) {
                if (!done_warn) {
                    fprintf(stderr, "WARNING: different size between EVEN rom and ODD rom.\n");
                    done_warn = 1;
                }
            }
            rc = fputc(ce, fd) == EOF;
            if (!rc) {
                ++cnt;
                rc = fputc(co, fd) == EOF;
                if (!rc) ++cnt;
            }
            if (rc != 0) {
                fprintf(stderr, "ERROR: '%s' write error\n", dest);
                rc = -1;
                break;
            }
        }
        printf("%ld bytes written\n", cnt);
    }
    else {
        rc = -1;
    }
    if (fd) fclose(fd);
    if (fe) fclose(fe);
    if (fo) fclose(fo);

    return rc;
}


int main(int argc, char *argv[])
{
    int nparam;

    bootlogo();
    nparam = myopt(argc-1, argv+1);

    if ((nparam < 3 || op_mode == MODE_NONE) && !opt_help) {
        printf("Type 'ROMSPCOM -?' to help.\n");
        return 1;
    }
    if (opt_help) {
        usage();
        return 0;
    }
    if (op_mode == MODE_SPLIT) {
        return split_romfile(myarg[0], myarg[1], myarg[2]);
    }
    else if (op_mode == MODE_COMBINE) {
        return combine_romfile(myarg[0], myarg[1], myarg[2]);
    }

    return -1;
}

