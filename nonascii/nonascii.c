/*
    nonascii: a simple filter for 'ShiftJIS-unaware' C/C++

    to build with gcc:
    gcc -Wall -O2 -s -o nonascii nonascii.c

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
#include <stdio.h>
#include <stdlib.h>

#ifndef PROGNAME
#define PROGNAME "nonascii"
#endif

enum {
    OPT_NONE = 0,
    OPT_ESC,
    OPT_ESC_SJIS_SLASH
};

int esc_file(FILE *fo, FILE *fi, int opt)
{
    unsigned char sj_lead = 0;
    int c;

    while((c = fgetc(fi)) != EOF) {
        unsigned char uc = (unsigned char)c;
        if (opt == OPT_ESC_SJIS_SLASH) {
            if (sj_lead) {
                c = fprintf(fo, (uc=='\\') ? "%c\\" : "%c", uc);
                sj_lead = 0;
            }
            else if ((uc >= 0x81 && uc <= 0x9f) || (uc >= 0xe0 && uc <= 0xfc)) {
                sj_lead = uc;
                c = fputc(c, fo);
            }
        }
        else if (opt == OPT_ESC) {
           if (uc >= 0x80) c = fprintf(fo, "\\x%02x", uc);
           else c = fputc(c, fo);
        }
        else {
            c = fputc(c, fo);
        }
        if (c < 0) return -1;
    }

    return 0;
}


int optHelp;
int optN;
int optS;
int optX;
int optV;
char *optI;
char *optO;

int mygetopt(int argc, char **argv)
{
    int rc = 0;

    while(--argc > 0) {
        char c, *s;
        s = *++argv;
        c = *s;
        if (c == '/' || (c == '-' && s[1] != '\0')) {
            c = *++s;
            if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
            switch(c) {
                case 'H': case '?':
                    optHelp = 1;
                    break;
                case 'N': optN = 1; optS = optX = 0; break;
                case 'S': optS = 1; optX = optN = 0; break;
                case 'X': optX = 1; optS = optN = 0; break;
                case 'V': ++optV; break;

                case 'I': case 'O':
                    ++s;
                    if (*s == '\0') {
                        s = NULL;
                        if (argc > 1) {
                            --argc;
                            s = *++argv;
                            if (s && *s == '-' && s[1] != '\0') s = NULL;
                        }
                    }
                    else if (*s == ':' || *s == '=') ++s;
                    if (!s || !*s) {
                        fprintf(stderr, "ERROR: need parameter for -I or -N\n");
                        return -1;
                    }
                    if (c == 'I') optI = s;
                    if (c == 'O') optO = s;
                    break;
            }
        }
        else {
            if (!optI) optI = s;
            else if (!optO) optO = s;
        }
    }

    if (optI && *optI == '-' && optI[1] == '\0') optI = NULL;
    if (optO && *optO == '-' && optO[1] == '\0') optO = NULL;

    return rc;
}

void usage(void)
{
    const char msg[] =
        "usage:\n"
        "  " PROGNAME " -s [-i input_file] [-o output_file]\n"
        "  " PROGNAME " -x [-i input_file] [-o output_file]\n"
        "\n"
        "    -s     add \\ to Shift_JIS character which the latter byte is \\ (0x5c)\n"
        "    -x     convert non-ascii (0x80~0xff) character to \\xnn\n"
        "\n"
        "    -i     specify input file (default: stdin)\n"
        "    -o     specify output file (default: stdout)\n"
        ;
    printf("%s", msg);
}

int main(int argc, char **argv)
{
    int rc;

    rc = mygetopt(argc, argv);
    if (optV > 0) {
        fprintf(stderr, "opt N:%d S:%d X:%d V:%d I:%s O:%s\n",optN,optS,optX,optV,optI?optI:"(null)",optO?optO:"(null)");
    }
    if (rc < 0 || (!optHelp && !(optN||optS||optX))) {
        fprintf(stderr, "Type '" PROGNAME " -?' to help.\n");
    }
    else if (optHelp) {
        usage();
    }
    else {
        int opt = OPT_NONE;
        FILE *fi, *fo;
        fi = optI ? fopen(optI, "r") : stdin;
        if (!fi) {
            fprintf(stderr, "ERROR: can't open '%s'.\n", optI);
            return 1;
        }
        fo = optO ? fopen(optO, "w") : stdout;
        if (!fo) {
            fprintf(stderr, "ERROR: can't open '%s'.\n", optO);
            return 1;
        }
        if (optS) opt = OPT_ESC_SJIS_SLASH;
        if (optX) opt = OPT_ESC;
        rc = esc_file(fo, fi, opt);
        fflush(fo);
        if (fo != stdout) fclose(fo);
    }

    return (rc < 0);
}

