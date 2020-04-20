/*
rdver: check version and revision of Datalight ROM-DOS


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
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int disp_romdosver(void)
{
    union REGS r;
    struct SREGS sr;

    r.x.ax = 0x30db;
    r.x.si = 0xb2d2;
    r.x.di = 3;
    r.x.bx = r.x.cx = r.x.dx = 0;
    sr.es = 0;
    intdosx(&r, &r, &sr);
    if (r.x.cflag || r.x.cx != 0xb2d2) return 0;

    printf("ROM-DOS version %d.%02d\n", r.h.dh, r.h.dl);
    printf("Kernel revision %x.%02x\n", r.h.ah, r.h.al);
    if (sr.es != 0 && r.x.bx != 0) {
        const char far *s = MK_FP(sr.es, r.x.bx);
        int s_max = 256;
        printf("Kernel revision string \"");
        while(*s && s_max-- > 0) printf("%c", *s++);
        printf("\"\n");
    }
    return r.x.dx;
}


int main(int argc, char *argv[])
{
    int rc;

    rc = disp_romdosver();
    if (rc == 0) {
        fprintf(stderr, "Not (or unknown) ROM-DOS!!\n");
    }
    return rc != 0;
}

