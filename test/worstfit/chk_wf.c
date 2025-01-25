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

#include <windows.h>
#ifndef WC_NO_BEST_FIT_CHARS
#define WC_NO_BEST_FIT_CHARS 0x400 /* winnls.h */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int do_help;
int verbose = 0;
DWORD conv_flags = 0;
UINT codepage = CP_ACP;
unsigned long point_end = 0xffffU;

const char *myoptcmp(const char *arg, const char *str)
{
    const char *rs = NULL;
    size_t na = strlen(arg);
    size_t ns = strlen(str);
    if (na >= ns || strncmp(arg, str, ns) == 0) {
        if (arg[ns] == '\0' || arg[ns] == '=' || arg[ns] == ':') {
            rs = arg + ns;
        }
    }
    return rs;
}

int my_getopt(int argc, char **argv)
{
    int rc = 0;
    while(argc-- > 1) {
        const char *s0 = *++argv;
        if ((*s0 == '/' || *s0 == '-') && s0[1] == '?') do_help = 1;
        if ((*s0 == '/' || *s0 == '-') && s0[1] == 'v') ++verbose;
        else if (*s0 == '-' && s0[1] == '-') {
            const char *s = s0 + 2;
            const char *sc = NULL;
            if (strcmp(s, "help") == 0) do_help = 1;
            else if (strcmp(s, "verbose") == 0) ++verbose;
            else if (strcmp(s, "quiet") == 0 || strcmp(s, "silent") == 0) verbose = 0;
            else if (strcmp(s, "utf16") == 0 || strcmp(s, "utf-16") == 0) point_end = 0x10ffffU;
            else if (strcmp(s, "no-best-fit") == 0 || strcmp(s, "no-best-fit-chars") == 0) conv_flags |= WC_NO_BEST_FIT_CHARS;
            else if (strcmp(s, "compositecheck") == 0) conv_flags |= WC_COMPOSITECHECK;
            else if (strcmp(s, "discardns") == 0) conv_flags |= (WC_COMPOSITECHECK | WC_DISCARDNS);
            else if ((sc = myoptcmp(s, "cp")) != NULL || (sc = myoptcmp(s, "codepage")) != NULL) {
                if (*sc == '\0') {
                    if (argc-- <= 1) break;
                    sc = *++argv;
                }
                else {
                    ++sc;
                }
                codepage = (UINT)strtoul(sc, NULL, 0);
            }
        }
    }
    return rc;
}


void usage(void)
{
    const char msg[] =
        "Usage: chk_wf [--codepage codepage_number] [--no-best-fit-chars] [--compositecheck] [--discardns] [--utf16] [--verbose]\n"
        "options:\n"
        "  --codepage\n"
        "      specify codepage by decimal number (--codepage 0 as default)\n"
        "  --no-best-fit-chars\n"
        "      set the flag WC_NO_BEST_FIT_CHARS\n"
        "  --compositecheck\n"
        "      set the flag WC_COMPOSITECHECK\n"
        "  --discardns\n"
        "      set the flag WC_DISCARDNS (and WC_COMPOSITECHECK)\n"
        "  --utf16\n"
        "      also check out of BMP (U+10000..U+10FFFF)\n";
        ;
    printf("%s\n", msg);
}

int main(int argc, char **argv)
{
    char badfilechars[] = { '\"', '/', ':', '<', '>', '\\', '|' , '?', '*', 0, 0 };
    const char *cpmsg = "";
    unsigned long fbcharcount = 0;
    unsigned n;
    CHAR s[128];
    WCHAR ws[128];
    int nwc = 1;
    BOOL bFB;

    my_getopt(argc, argv);
    if (do_help) {
        usage();
        return 0;
    }

    switch(codepage) {
        case CP_ACP: cpmsg = " (CP_ACP)"; break;
        case CP_OEMCP: cpmsg = " (CP_OEMCP)"; break;
        case 2: cpmsg = " (CP_MACCP)"; break;
        case 3: cpmsg = " (CP_THREAD_ACP)"; break;
        case 65000: cpmsg = " (CP_UTF7)"; break;
        case 65001: cpmsg = " (CP_UTF8)"; break;
    }
    printf("Codepage: %u%s\n", codepage, cpmsg);
    printf("dwFlags:  0x%lX\n", (unsigned long)conv_flags);
    if (conv_flags & WC_COMPOSITECHECK) {
        printf("  WC_COMPOSITECHECK\n");
        if (conv_flags & WC_DEFAULTCHAR) printf("    WC_DEFAULTCHAR\n");
        if (conv_flags & WC_DISCARDNS) printf("    WC_DISCARDNS\n");
        if (conv_flags & WC_SEPCHARS) printf("    WC_SEPCHARS\n");
    }
    if (conv_flags & WC_NO_BEST_FIT_CHARS) printf("  WC_NO_BEST_FIT_CHARS\n");

    // quick check codepage supported (EnumSystemCodePages will help paranoids...)
    ws[0] = (WCHAR)' ';
    ws[1] = (WCHAR)'\0';
    SetLastError(0);
    n = WideCharToMultiByte(codepage, conv_flags, ws, 1, NULL, 0, NULL, NULL);
    if (n == 0) {
        unsigned long dw = GetLastError();
        printf("ERROR: guess unsupported %s (lasterror=%lu)\n", dw == ERROR_INVALID_FLAGS ? "flags" : "codepage", dw);
        return 1;
    }

    if (verbose > 0) printf("codepoint %04X...\n", 0);
    for(n = 1; n <= point_end; ++n) {
        int i;
        if (n == 0xd800) {  // skip surrogate pair and private-use area
            n = 0xf900;
            if (verbose > 0) printf("codepoint %04X...\n", n);
            --n;
            continue;
        }
        if (verbose > 0 && (n & 0xfff) == 0) {
            printf("codepoint %04X...\n", n);
        }
        if (n <= 0xffff) {
            ws[0] = (WCHAR)n;
        }
        else {
            ws[0] = 0xd800 + (((n >> 10) & 0x7ff) - 0x40);
            ws[1] = 0xdc00 + (n & 0x3ff);
            nwc = 2;
        }
        ws[nwc] = (WCHAR)'\0';
        s[0] = s[1] = s[2] = s[3] = s[4] = '\0';
        bFB = FALSE;
        i = WideCharToMultiByte(codepage, conv_flags, ws, 1, s, sizeof(s)-1, NULL, &bFB);
        if (i == 0) continue;
        s[i] = '\0';
        if (!bFB) {
            if (n != (WCHAR)(s[0]) && (s[0] == '\x9' || (s[0] >= 0x20 && s[0] <= 0x7f))) {
                const char *bc = badfilechars;
                printf("fallback: U+%04X -> 0x%02X", n, s[0]);
                if (s[0] >= ' ' && s[0] <= 0x7e) printf(" (%c)", s[0]);
                while(*bc) {
                    if (s[0] == *bc++) {
                        printf(" *badfilechar*");
                        break;
                    }
                }
                // a bit additional information about security-sensitive characters
                // for command line string (via shell)...
                if (s[0] == '%') printf(" !percent!");
                if (s[0] == '-') printf(" !hyphen!");
                if (s[0] == '&') printf(" !ampersand!");
                if (s[0] == ',') printf(" !comma!");
                if (s[0] == ';') printf(" !semicolon!");
                if (s[0] == '(' || s[0] == ')') printf(" !parenthesis!");
                // (mostly for posix-style shells/tools)
                if (s[0] == '$') printf(" !dollar!");
                if (s[0] == '\'') printf(" !quote!");
                if (s[0] == '`') printf(" !grave!");
                if (s[0] == '[' || s[0] == ']') printf(" !bracket!");
                if (s[0] == '{' || s[0] == '}') printf(" !curly!");
                if (s[0] == '~') printf(" !tilde!");
                if (s[0] == '#') printf(" !sharp!");
                printf("\n");
                ++fbcharcount;
            }
        }
    }

    return 0;
}
