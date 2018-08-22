/*
SPDX-License-Identifier: Unlicense

mtcp2td : import DHCP config from mTCP (mtcp.cfg) to TEEN (teen.def)

to build:
  wcl -zq -s -os mtcp2td.c   (OpenWatcom)
  lcc mtcp2td.c -lintlib     (LSI-C86 3.30c trial)
  tcc mtcp2tc.c              (Borland Turbo C++ 1.01)


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

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* for unlink() */
#if (defined(__unix__) || defined(__linux__))
#include <unistd.h>
#else
#include <io.h>
#endif

#define VERSION "0.00"

#define STDIO_VBUF_SIZE 1024
#define LINEBUF_SIZE 256

int optHelp = 0;
int do_verbose = 0;


static
void *my_malloc(size_t n, unsigned lineno)
{
    void *p;

    if (n == 0) n = 1;
    if (!(p = malloc(n))) {
        fprintf(stderr, "Fatal: memory allocation failure at line %u (%ubytes)\n", lineno, (unsigned)n);
        exit(-1);
    }
    return p;
}
#define malloc(n)  my_malloc(n,__LINE__)


static
int  fgetline(void *buffer, size_t buflen, FILE *fi)
{
    char *p = buffer;
    int c;
    int ncline = 0;

    if (buffer && buflen > 0) memset(buffer, 0, buflen);
    while((c = fgetc(fi)) != EOF) {
        if (ncline < INT_MAX) ++ncline;
        if (c == '\r') continue;
        if (c == '\n') break;
        if (ncline < buflen) {
            *p++ = (char)(unsigned char)c;
        }
    }

    if (ncline == 0 && c == EOF) {
        return EOF;
    }

    return ncline;
}


#define is_eol(c) ((c) == '\r' || (c) == '\n' || (c) == '\x1a' || (c) == '\0')
#define is_sp(c) ((c) == ' ' || (c) == '\t')
#define is_comment(c) ((c) == ';' || (c) == '#')
#define nochar(c) (is_eol(c) || is_sp(c) || is_comment(c))

char *skipsp(const char *s)
{
    while(1) {
        unsigned char c = *s;
        if (is_eol(c)) break;
        if (!is_sp(c)) break;
        ++s;
    }
    return (char *)s;
}

int token_len(const char *s_org)
{
    const char *s = s_org;

    while(1) {
        unsigned char c = *s;
        if (nochar(c)) break;
        ++s;
    }

    return s - s_org;
}

int my_memcasecmp(const void *m0, const void *m1, size_t len)
{
    const char *s0 = m0;
    const char *s1 = m1;
    int rc = 0;

    while(len > 0) {
        rc = toupper(*s0) - toupper(*s1);
        if (rc != 0) break;
        ++s0;
        ++s1;
        --len;
    }

    return rc;
}


char *my_basepos(const char *pathname)
{
    size_t n = strlen(pathname);
    while(n) {
        char c = *pathname;
        if (c == '\0' || c == '/' || c == '\\' || c == ':') break;
        --n;
    }
    return (char *)pathname + n;
}

#if 1
char *my_extpos(const char *filename)
{
    while(*filename && *filename != '.') ++filename;
    return (char *)filename;
}
#else
# define my_extpos(s)  strchrnul(s,'.')
#endif


enum {
    TEEN_SECTION_NONE = 0,
    TEEN_SECTION_ETHERNET,
    TEEN_SECTION_RESOLVER,
    TEEN_SECTION_PD
};

enum {
    TEEN_SUBSECTION_NONE = 0,
    TEEN_SUBSECTION_ETHERNET,
    TEEN_SUBSECTION_NETIF,
};


typedef struct {
    const char *mtcp_entry;
    int teen_section;
    int teen_subsect;
    const char *teen_entry_id;
    char *mtcp_value;
} CFGCONV;

CFGCONV convtbl[] = {
    { "PACKETINT", TEEN_SECTION_PD, TEEN_SUBSECTION_NONE, "100", NULL },
    { "IPADDR", TEEN_SECTION_ETHERNET, TEEN_SUBSECTION_NETIF, "400", NULL },
    { "NETMASK", TEEN_SECTION_ETHERNET, TEEN_SUBSECTION_NETIF, "401", NULL },
    /* { "BROADCAST", TEEN_SECTION_ETHERNET, TEEN_SUBSECTION_NETIF, "402", NULL }, */
    { "GATEWAY", TEEN_SECTION_ETHERNET, TEEN_SUBSECTION_NETIF, "404", NULL },
    { "NAMESERVER", TEEN_SECTION_RESOLVER, TEEN_SUBSECTION_NONE, "300", NULL },
    { NULL, -1, -1, NULL, NULL }
};


char * get_stoken(const char *buf, int *len_token, int use_section)
{
    int tklen = 0;
    char *tk, c;

    tk = skipsp(buf);
    c = *tk;
    if (!nochar(c)) {
        char rblace = '\0';

        if (use_section) {
            if (c == '[') rblace = ']';
            else if (c == '<') rblace = '>';
        }

        while((c = tk[tklen]) != '\0') {
            if (rblace) {
                if (c == rblace) {
                    ++tklen;
                    break;
                }
            }
            else {
                if (is_comment(c) || is_sp(c)) break;
            }
            ++tklen;
        }
    }
    if (len_token) *len_token = tklen;

    return tk;
}

#define get_token(b,l)  get_stoken(b,l,0)
#define teen_token(b,l)  get_stoken(b,l,!0)

char * duptoken(const void *mem, size_t len)
{
    char *s = malloc(len + 1);
    memcpy(s, mem, len);
    s[len] = '\0';
    return s;
}

int cmptoken(const char *mem0, size_t len, const char *s)
{
    if (!mem0 || len == 0 || !s) return -1;
    if (strlen(s) != len) return -1;
    return my_memcasecmp(mem0, s, len);
}


int get_mtcp_cfg(CFGCONV *cnv_tbl, const char *filename)
{
    int entries = 0;
    char buf[LINEBUF_SIZE];
    FILE *fi;
    
    fi = fopen(filename, "rt");
    if (!fi) return -1;
    setvbuf(fi, NULL, _IOFBF, STDIO_VBUF_SIZE);

    while(fgetline(buf, sizeof(buf), fi) != EOF) {
        CFGCONV *cnv;
        int tklen = 0;
        char *tk;

        tk = get_token(buf, &tklen);
        for(cnv = cnv_tbl; cnv->mtcp_entry; ++cnv) {
            int tk2len;
            char *tk2;
            if (cnv->mtcp_value) continue;
            if (cmptoken(tk, tklen, cnv->mtcp_entry) != 0) continue;
            tk2len = 0;
            tk2 = get_token(tk + tklen, &tk2len);
            if (tk2len > 0) {
                cnv->mtcp_value = duptoken(tk2, tk2len);
                if (do_verbose) {
                    fprintf(stderr, "get:%s %s\n", cnv->mtcp_entry, cnv->mtcp_value);
                }
                ++entries;
            }
        }
    }
    fclose(fi);

    return entries;
}


int conv_teen_def(CFGCONV *cnv_tbl, const char *srcdef, const char *dstdef)
{
    int entries = 0;
    char buf[LINEBUF_SIZE];
    FILE *fi, *fo;
    int section = TEEN_SECTION_NONE;
    int subsect = TEEN_SUBSECTION_NONE;

    fi = fopen(srcdef, "rt");
    if (!fi) {
        fprintf(stderr, "error: can't open the file '%s'.\n", srcdef);
        return -1;
    }
    fo = dstdef ? fopen(dstdef, "wt") : stdout;
    if (!fo) {
        fprintf(stderr, "error: can't open the file '%s'.\n", dstdef);
        fclose(fi);
        return -1;
    }
    setvbuf(fi, NULL, _IOFBF, STDIO_VBUF_SIZE);

    while(fgetline(buf, sizeof(buf), fi) != EOF) {
        char *tk;
        int tklen = 0;
        CFGCONV *cfg_conv = NULL;

        tk = teen_token(buf, &tklen);
        if (tk && tklen > 0) {
            CFGCONV *cnv;
            for(cnv = cnv_tbl; cnv->mtcp_entry; ++cnv) {
                if (cnv->teen_section == section && 
                    cnv->teen_subsect == subsect &&
                    cmptoken(tk, tklen, cnv->teen_entry_id) == 0)
                {
                    cfg_conv = cnv;
                    break;
                }
            }
            if (!cfg_conv && tklen >= 2) {
                if (*tk == '[') { /* section */
                    subsect = TEEN_SUBSECTION_NONE;
                    if (cmptoken(tk, tklen, "[ETHERNET]") == 0)
                        section = TEEN_SECTION_ETHERNET;
                    else if (cmptoken(tk, tklen, "[RESOLVER]") == 0)
                        section = TEEN_SECTION_RESOLVER;
                }
                else if (*tk == '<') { /* subsection */
                    if (cmptoken(tk, tklen, "<NETIF>") == 0)
                        subsect = TEEN_SUBSECTION_NETIF;
                    else if (cmptoken(tk, tklen, "<ETHERNET>") == 0)
                        subsect = TEEN_SUBSECTION_ETHERNET;
                }
            }
        }
        if (cfg_conv && cfg_conv->mtcp_value) {
            if (do_verbose) {
                fprintf(stderr, "replace:%s %s\n", cfg_conv->teen_entry_id, cfg_conv->mtcp_value);
            }
            if (cfg_conv->teen_section == TEEN_SECTION_RESOLVER && cfg_conv->teen_subsect == TEEN_SUBSECTION_NONE && strcmp(cfg_conv->teen_entry_id, "300")==0)
                fprintf(fo, "%s\t'%s'\n", cfg_conv->teen_entry_id, cfg_conv->mtcp_value);
            else
                fprintf(fo, "%s\t%s\n", cfg_conv->teen_entry_id, cfg_conv->mtcp_value);
            ++entries;
        }
        else {
            fprintf(fo, "%s\n", buf);
        }
    }
    fclose(fi);
    fclose(fo);

    return entries;
}


int mygetopt(int argc, char *argv[])
{
    int n = 0;
    while(argc) {
        char *s = *argv++;
        if (*s == '/' || *s == '-') {
            char c = toupper(s[1]);
            if (c == '?' || c == 'h') { optHelp = 1; ++n; }
            else if (c == 'V') { ++do_verbose; ++n; }
        }
        --argc;
        ++argv;
    }
    return n;
}

static char *getenv_or_error(const char *envname)
{
    char *e = getenv(envname);
    if (!e) {
        fprintf(stderr, "error: the environment variable %s is not set.\n", envname);
    }
    return e;
}

void usage(void)
{
    printf("MTCP2TD version " VERSION " (built at " __DATE__ " " __TIME__ ")\n");
    printf("Usage: mtcp2td [-v]\n");
    printf("Import DHCP configuration from MTCP.CFG to TEEN.DEF\n");
}


int main(int argc, char *argv[])
{
    int rc;
    char *teendef;
    char *teenbak;
    char *teentmp;
    char *mtcpcfg;

    mygetopt(argc-1, argv+1);
    if (optHelp) {
        usage();
        return 0;
    }

    teendef = getenv_or_error("TEEN");
    mtcpcfg = getenv_or_error("MTCPCFG");
    if (!teendef || !mtcpcfg) {
        return 1;
    }

    teentmp = malloc(strlen(teendef) + 5);
    strcpy(teentmp, teendef);
    strcpy(my_extpos(my_basepos(teentmp)), ".$$$");
    teenbak = malloc(strlen(teendef) + 5);
    strcpy(teenbak, teendef);
    strcpy(my_extpos(my_basepos(teenbak)), ".BAK");

    if (do_verbose) {
        fprintf(stderr, "MTCPCFG: %s\n", mtcpcfg);
        fprintf(stderr, "TEEN   : %s\n", teendef);
    }

    if (do_verbose) {
        fprintf(stderr, "loading configuration:\n");
    }
    rc = get_mtcp_cfg(convtbl, mtcpcfg);
    if (rc < 0) return 1;
    
    if (do_verbose) {
        fprintf(stderr, "copy and replace: %s -> %s\n", teendef, teentmp);
    }
    rc = conv_teen_def(convtbl, teendef, teentmp);
    if (rc < 0) return 1;

    unlink(teenbak);
    rename(teendef, teenbak);
    if (rename(teentmp, teendef) < 0) {
        fprintf(stderr, "error: can't rename %s to %s.\n", teentmp, teendef);
        return 1;
    }

    return 0;
}

