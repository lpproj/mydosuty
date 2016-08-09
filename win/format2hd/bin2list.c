/*
  The Public Domain:
  You can use, modify and/or redistribute freely BUT NO WARRANTY.
*/

#include <stdio.h>
#include <string.h>

static void
usage(void)
{
    printf("usage: bin2list binfile [destfile]\n");
}

int
main(int argc, char *argv[])
{
    const int bytes_per_line = 16;
    char *src, *dst;
    FILE *fi, *fo;
    int c;
    unsigned long cnt;
    
    if (argc <= 1) {
        usage();
        return 0;
    }
    
    src = dst = NULL;
    fo = stdout;
    while(--argc > 0) {
        char *s;
        s = *++argv;
        if (strcmp(s,"-h")==0 || strcmp(s,"-?")==0 || strcmp(s,"--help")==0) {
            usage();
            return 0;
        }
        if (strcmp(s, "--version")==0) {
            printf("bin2list (build at " __DATE__ " " __TIME__ ")\n");
            return 0;
        }
        if (src) {
            if (!dst) dst = s;
        } else {
            src = s;
        }
    }
    
    fi = fopen(src, "rb");
    if (!fi) {
        fprintf(stderr, "can't open %s\n", src);
        return 1;
    }
    if (dst) {
        fo = fopen(dst, "wt");
        if (!fo) {
            fprintf(stderr, "can't open %s\n", dst);
            return 1;
        }
    }
    
    for(cnt = 0; (c = fgetc(fi)) != EOF; ++cnt) {
        if ((cnt % bytes_per_line) != 0)
            fprintf(fo, ", ");
        fprintf(fo, "0x%02x", (unsigned)c);
        if ((cnt % bytes_per_line) == bytes_per_line - 1)
            fprintf(fo, ",\n");
    }
    if ((cnt % bytes_per_line) != bytes_per_line - 1)
        fprintf(fo, "\n");
    
    if (dst) fclose(fo);
    fclose(fi);
    
    return 0;
}
