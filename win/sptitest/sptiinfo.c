/*

SPTIINFO(ASPIINFO) - Get drive parameters of SCSI(-like) drive

to build for Win32 (with OpenWatcom):
    wcl386 -zq -s -fr sptiinfo.c -bcl=nt

to build for DOS (with OpenWatcom):
    wcl -zq -s -fr -zp1 -os -ml -fe=aspiinfo.exe sptiinfo.c -bcl=dos -k4096


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


#ifndef SCSI_REQUEST_SENSE_MAX
# define SCSI_REQUEST_SENSE_MAX 18
#endif
#ifndef SCSI_TRANSFER_DATA_MAX
# define SCSI_TRANSFER_DATA_MAX 16384
#endif

#define PLINE { fprintf(stderr, "line %u\n", __LINE__); }

#include <limits.h>

#if defined __STDC_VERSION__ || ((__STDC_VERSION__)>=199901L)
# include <stdint.h>
typedef uint8_t   U8;
typedef uint16_t  U16;
typedef uint32_t  U32;
typedef uint64_t  U64;
#else
typedef unsigned char U8;
typedef unsigned short U16;
# if UINT_MAX == 0xffffffffUL
typedef unsigned int U32
# else
typedef unsigned long U32
# endif
# ifdef __GNUC__
typedef unsigned long long U64;
# else
typedef unsigned __int64 U64;
# endif
#endif

/* --------------------------------------------------------- */
#if defined WIN32 || defined _WIN32

# include <windows.h>
# include <winioctl.h>

# if defined _MSC_VER || defined USE_WINDDK
#  include <ntddscsi.h>
# else
#  include <ddk/ntddscsi.h>
# endif

# ifndef WIN32
#  define WIN32 _WIN32
# endif


#endif

#if defined __DOS__
# include <dos.h>
#endif

/* --------------------------------------------------------- */
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    SCSI_NO_TRANSFER = 0,
    SCSI_FROM_DEVICE,
    SCSI_TO_DEVICE
};

typedef struct SCSICMDPKT {
    unsigned cdb_length;
    U8 scsi_status;
    U8 cdb[16];
    unsigned sense_length;
    U8 sense_data[SCSI_REQUEST_SENSE_MAX];
} SCSICMDPKT;

#if UINT_MAX <= 0xffffU
unsigned transfer_buf_max = 2048;
#else
unsigned transfer_buf_max = SCSI_TRANSFER_DATA_MAX;
#endif


/* --------------------------------------------------------- */
#if defined WIN32

typedef struct {
    HANDLE hDrv;
    DWORD last_error;
    int host_id;
    int path_id;
    int target_id;
    int target_lun;
} SCSIDRV_win;

typedef struct {
    SCSI_PASS_THROUGH spt;
    U8 sense_data[SCSI_REQUEST_SENSE_MAX];
    U8 transfer_data[SCSI_TRANSFER_DATA_MAX];
} mySPT;


const char * winErrMsg(DWORD dw)
{
    static char s[256];
    unsigned n;

    ZeroMemory(s, sizeof(s));
    FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)s, sizeof(s)/sizeof(s[0]) - 1, NULL);
    n = strlen(s);
    if (n >= 1 && s[n - 1] == '\n') s[--n] = '\0';
    if (n >= 1 && s[n - 1] == '\r') s[--n] = '\0';

    return s;
}

HANDLE open_drive_a_flag(int drive0, DWORD dw_acc, DWORD dwFlag)
{
    HANDLE h;
    UINT uPrevErrMode;
    DWORD dwErr;
    TCHAR volpn[10];
    int try_count = 3;

    lstrcpy(volpn, TEXT("\\\\.\\?:"));
    volpn[4] = TEXT(( 'A' + drive0));

    dwErr = GetLastError();
    uPrevErrMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    while(try_count-- > 0) {
        h = CreateFile(volpn
                     , dw_acc
                     , FILE_SHARE_READ | FILE_SHARE_WRITE
                     , NULL
                     , OPEN_EXISTING
                     , dwFlag
                     , NULL);
        dwErr = GetLastError();
        if (h != INVALID_HANDLE_VALUE) { dwErr = 0; break; }
        Sleep(1500); /* wait for the drive to get ready */
    }
    if (h != INVALID_HANDLE_VALUE) FlushFileBuffers(h);
    SetErrorMode(uPrevErrMode);
    SetLastError(dwErr);

    return h;
}

HANDLE open_drive_a(int drive0, DWORD dw_acc)
{
    /* not certain about FILE_SHARE_xxx and FILE_FLAG_xxx */
    return open_drive_a_flag(drive0, dw_acc, FILE_FLAG_NO_BUFFERING);
}


DWORD open_drive_win(SCSIDRV_win *drv, const char *winpath)
{
    DWORD dwErr = 0;
    char volpn[8];

    if (isalpha(winpath[0]) && winpath[1] == ':' && (winpath[2] == '\0' || (winpath[2] == '\\' && winpath[3] == '\0'))) {
        strcpy(volpn, "\\\\.\\?:");
        volpn[4] = winpath[0];
        winpath = volpn;
    }
    SetLastError(0);
    drv->hDrv = CreateFileA(winpath
                          , GENERIC_READ | GENERIC_WRITE
                          , FILE_SHARE_READ | FILE_SHARE_WRITE
                          , NULL
                          , OPEN_EXISTING
                          , FILE_FLAG_NO_BUFFERING
                          , NULL);
    if (drv->hDrv != INVALID_HANDLE_VALUE) {
        SCSI_ADDRESS sa = {0};
        DWORD dw = 0;
        if (DeviceIoControl(drv->hDrv, IOCTL_SCSI_GET_ADDRESS, NULL, 0, &sa, sizeof(sa), &dw, NULL)) {
            if (dw > 0 && dw == sa.Length) {
                drv->host_id = sa.PortNumber;
                drv->path_id = sa.PathId;
                drv->target_id = sa.TargetId;
                drv->target_lun = sa.Lun;
            }
        }
    } else {
        dwErr = GetLastError();
    }
    if (dwErr) {
        fprintf(stderr, "can't open '%s' (lasterr=%d %s)\n", winpath, (int)dwErr, winErrMsg(dwErr));
    }
    drv->last_error = dwErr;
    return dwErr;
}

DWORD close_drive_win(SCSIDRV_win *drv)
{
    SetLastError(0);
    CloseHandle(drv->hDrv);
    drv->hDrv = INVALID_HANDLE_VALUE;
    return (drv->last_error = GetLastError());
}



int scsicmd_win(SCSIDRV_win *drv, SCSICMDPKT *pkt, void *mem, int transfer_direction, unsigned transfer_length)
{
    mySPT p;
    BOOL bRc;
    const unsigned off_sense = (unsigned)((char *)&(p.sense_data) - (char *)&p);
    const unsigned off_data = (unsigned)((char *)&(p.transfer_data) - (char *)&p);
    DWORD dwInbuf, dwOutbuf, dwRLen;

    ZeroMemory(&p, off_data);
    p.spt.Length = sizeof(p.spt);
    p.spt.PathId = drv->path_id;
    p.spt.TargetId = drv->target_id;
    p.spt.Lun = drv->target_lun;
    p.spt.CdbLength = pkt->cdb_length;
    p.spt.SenseInfoLength = sizeof(p.sense_data);
    p.spt.DataTransferLength = transfer_length;
    p.spt.TimeOutValue = 10; // 30;
    p.spt.DataBufferOffset = off_data;
    p.spt.SenseInfoOffset = off_sense;
    memcpy(p.spt.Cdb, pkt->cdb, pkt->cdb_length);

    SetLastError(0);
    switch(transfer_direction) {
        case SCSI_FROM_DEVICE:
            p.spt.DataIn = SCSI_IOCTL_DATA_IN;
            dwInbuf = off_sense + p.spt.SenseInfoLength;
            dwOutbuf = off_data + transfer_length;
            break;
        case SCSI_TO_DEVICE:
            p.spt.DataIn = SCSI_IOCTL_DATA_OUT;
            memcpy(p.transfer_data, mem, transfer_length);
            dwInbuf = off_data + p.spt.DataTransferLength;
            dwOutbuf = off_sense + p.spt.SenseInfoLength;
            break;
        case SCSI_NO_TRANSFER:
            transfer_length = 0;
            p.spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
            dwInbuf = dwOutbuf = off_sense + p.spt.SenseInfoLength;
            break;
        default:
            return -1;
    }
    dwRLen = 0;
    if (!DeviceIoControl(drv->hDrv, IOCTL_SCSI_PASS_THROUGH, &p, dwInbuf, &p, dwOutbuf, &dwRLen, NULL)) {
        drv->last_error = GetLastError();
        return -1;
    }
    if (transfer_direction == SCSI_FROM_DEVICE) {
        if (dwRLen > 0 && p.spt.DataTransferLength > 0 && p.spt.DataTransferLength <= transfer_length) {
            memcpy(mem, p.transfer_data, p.spt.DataTransferLength);
        }
    }
    pkt->scsi_status = p.spt.ScsiStatus;
    pkt->sense_length = p.spt.SenseInfoLength;
    if (p.spt.SenseInfoLength) {
        memcpy(pkt->sense_data, p.sense_data, pkt->sense_length);
    }
    else {
        p.sense_data[0] = 0;
    }
    return p.spt.DataTransferLength;
}

# define scsicmd    scsicmd_win
# define SCSIDRV    SCSIDRV_win

#endif /* WIN32 */
/* --------------------------------------------------------- */
#if defined __DOS__ && UINT_MAX <= 0xffffUL


# ifndef offsetof
#  define offsetof(typ,id) ((unsigned)(&((typ *)0)->(id)) - (unsigned)&(typ *)0)
# endif

# if defined LSI_C
typedef void (far *ASPIPROC)(int, int, int, int, void far *);
#  define call_aspi(addr,param)  (*(addr))(0,0,0,0,param)
# else
typedef void (cdecl far *ASPIPROC)(void far *);
#  define call_aspi(addr,param)  (*(addr))(param)
# endif

# if !(defined __TURBOC__ || defined LSI_C)
#  pragma pack(1)
# endif

#define SRB_COMMAND_BASE_ENTRIES_0 \
    U8 SRB_Cmd; \
    volatile U8 SRB_Status; \
    U8 SRB_HaId; \
    U8 SRB_Flags;

#define SRB_COMMAND_BASE_ENTRIES \
    SRB_COMMAND_BASE_ENTRIES_0 \
    U32 SRB_Hdr_Rsvd;

#define SRB_COMMAND_BASE_ENTRIES_1 \
    SRB_COMMAND_BASE_ENTRIES \
    U8 SRB_Target; \
    U8 SRB_Lun;

#define SRB_EXEC_SCSI_CMD_BASE_ENTRIES \
    SRB_COMMAND_BASE_ENTRIES_1 \
    U32 SRB_BufLen; \
    U8 SRB_SenseLen; \
    void far *SRB_BufPointer; \
    void far *SRB_LinkPointer; \
    U8 SRB_CDBLen; \
    U8 SRB_HaStat; \
    U8 SRB_TargStat; \
    void (far *SRB_PostProc)(void); \
    U8 SRB_Rsvd2[34];

struct SRB_GetDeviceType {
    SRB_COMMAND_BASE_ENTRIES_1;
    U8 SRB_DevType;
};

struct SRB_HAInquiry {
    SRB_COMMAND_BASE_ENTRIES_0
    U16 SRB_55AASignature;
    U16 SRB_ExtBufferSize;
    U8 HA_Count;
    U8 HA_SCSI_ID;
    char HA_ManagerId[16];
    char HA_Identifier[16];
    char HA_Unique[16];
    U8 HA_ExtBuffer[2];
    U8 reserved_3C[4];
};

struct SRB_ExecSCSICmd {
    SRB_EXEC_SCSI_CMD_BASE_ENTRIES
    U8 CDB[16 + SCSI_REQUEST_SENSE_MAX];
};

union ASPI_RequestBlock {
    struct SRB_HAInquiry        hainq;
    struct SRB_GetDeviceType    dtype;
    struct SRB_ExecSCSICmd      cmd;
};

# if !(defined __TURBOC__ || defined LSI_C)
#  pragma pack()
# endif

typedef struct {
    int adapter_id;
    int support_residual;
    int target_id;
    int target_lun;
    unsigned last_error;
} SCSIDRV_d16;


static int mydoserr;
static ASPIPROC aspientry;
static union ASPI_RequestBlock aspirb;


int CallASPI(void *rb)
{
    U8 st;

    if (!aspientry) return -1;
    ((struct SRB_ExecSCSICmd *)rb)->SRB_Status = 0;
    call_aspi(aspientry, rb);
    while ((st = ((struct SRB_ExecSCSICmd *)rb)->SRB_Status) == 0) {
        /* ASPIYield(); */
    }
    return (int)(unsigned)st;
}

int scsicmd_d16(SCSIDRV_d16 *drv, SCSICMDPKT *pkt, void *mem, int transfer_direction, unsigned transfer_length)
{
    int rc = -1;
    struct SRB_ExecSCSICmd *cmd = &(aspirb.cmd);
    U8 *psense;
    U8 flag;

    memset(cmd, 0, sizeof(aspirb.cmd) /* offsetof(struct SRB_ExecSCSICmd, CDB) */);
    cmd->SRB_Cmd = 2;
    cmd->SRB_HaId = drv->adapter_id;
    switch(transfer_direction) {
        case SCSI_FROM_DEVICE: flag = 0x08; break;
        case SCSI_TO_DEVICE: flag = 0x10; break;
        case SCSI_NO_TRANSFER: flag = 0x18; transfer_length = 0; break;
        default:
            return -1;
    }
    if (drv->support_residual) flag |= 0x04;
    cmd->SRB_Flags = flag;
    cmd->SRB_Target = drv->target_id;
    cmd->SRB_Lun = drv->target_lun;
    cmd->SRB_BufLen = transfer_length;
    cmd->SRB_SenseLen = SCSI_REQUEST_SENSE_MAX;
    if (transfer_length) cmd->SRB_BufPointer = mem;
    cmd->SRB_CDBLen = pkt->cdb_length;
    memcpy(cmd->CDB, pkt->cdb, pkt->cdb_length);
    psense = (U8 *)&(cmd->CDB[0]) + pkt->cdb_length;
    if (cmd->SRB_SenseLen) memset(psense, 0, cmd->SRB_SenseLen);

    CallASPI(cmd);
    pkt->sense_data[0] = 0;
    if ((pkt->scsi_status = cmd->SRB_TargStat) != 0) {
        pkt->sense_length = cmd->SRB_SenseLen;
        if (pkt->sense_length) memcpy(pkt->sense_data, psense, pkt->sense_length);
    }

    if (cmd->SRB_Status == 1) {
        rc = cmd->SRB_BufLen;
        if (drv->support_residual) rc = transfer_length - rc;
    }
    else {
        drv->last_error = (cmd->SRB_Status);
    }
    return rc;
}


static ASPIPROC getASPIEntry(void)
{
    static const char aspi_devdos[] = "SCSIMGR$";
    ASPIPROC addr = 0L;
    union REGS r;
    struct SREGS sr;

    mydoserr = 0;
    r.x.ax = 0x3d00;
    r.x.dx = FP_OFF(aspi_devdos);
    sr.ds = FP_SEG(aspi_devdos);
    intdosx(&r, &r, &sr);
    if (r.x.cflag) {
        mydoserr = r.x.ax;
    }
    else {
        unsigned h = r.x.ax;
        r.x.ax = 0x4402;
        r.x.bx = h;
        r.x.cx = sizeof(addr);  /* 4 */
        r.x.dx = FP_OFF(&addr);
        sr.ds = FP_SEG(&addr);
        intdosx(&r, &r, &sr);
        if (r.x.cflag) mydoserr = r.x.ax;
        r.h.ah = 0x3e;
        r.x.bx = h;
        intdosx(&r, &r, &sr);
    }

    return addr;
}

int open_aspi_device(SCSIDRV_d16 *drv, int adapter, int id, int lun)
{
    int rc;
    U8 buf[16];
    SCSIDRV_d16 d;

    if (!aspientry) {
        aspientry = getASPIEntry();
        if (!aspientry) return mydoserr;
    }
    memset(&(aspirb.hainq), 0, sizeof(aspirb.hainq));
    /* aspirb.dtype.SRB_Cmd = 0; */
    aspirb.hainq.SRB_55AASignature = 0xaa55;
    aspirb.hainq.SRB_HaId = adapter;
    rc = CallASPI(&(aspirb.hainq));
    if (rc == 1) {
        rc = 0;
        d.adapter_id = adapter;
        d.target_id = id;
        d.target_lun = lun;
        d.support_residual = (aspirb.hainq.HA_ExtBuffer[0] & 0x02) != 0;
        aspirb.dtype.SRB_Cmd = 1;
        aspirb.dtype.SRB_Flags = 0;
        aspirb.dtype.SRB_Target = id;
        aspirb.dtype.SRB_Lun = lun;
        aspirb.dtype.SRB_DevType = 0xff;
        rc = CallASPI(&(aspirb.dtype));
        if (rc == 1) rc = 0;
        if (rc == 0) *drv = d;
    }
    return rc;
}

# define scsicmd    scsicmd_d16
# define SCSIDRV    SCSIDRV_d16

#endif /* __DOS__ */



void *mymalloc(size_t n)
{
    void *p;
    if (n == 0) n=1;
    p = malloc(n);
    if (!p) {
        fprintf(stderr, "FATAL: memory allocation failure\n");
        exit(-1);
    }
    memset(p, 0, n);
    return p;
}

void myfree(void *p)
{
    if (p) free(p);
}


void dumpMemToFile(const void *mem, unsigned len, FILE *fo)
{
#define DUMP_BYTES_PER_LINE 16
    const unsigned char *s = mem;
    unsigned char s_asc[DUMP_BYTES_PER_LINE + 1];
    unsigned n = 0;
    unsigned nline = 0;

    while(n < len) {
        unsigned char c;
        if (nline == 0) {
            fprintf(fo, "%08X", n);
            memset(s_asc, 0, sizeof(s_asc));
        }
        c = s[n];
        s_asc[nline] = (c >= ' ' && c <= 0x7f) ? c : '.';
        fprintf(fo, " %02X", c);
        ++n;
        if (++nline >= DUMP_BYTES_PER_LINE) {
            fprintf(fo, "  %s\n", s_asc);
            nline = 0;
        }
    }
    if (nline != 0) {
        fprintf(fo, "%*c  %s\n", (DUMP_BYTES_PER_LINE - nline) * 3, ' ', s_asc);
    }
    fflush(fo);
}

#define dumpMem(m,l)    dumpMemToFile(m,l,stdout)



int scsi_readdump_extbuf(SCSIDRV *drv, SCSICMDPKT *sc, U8 *buf, unsigned data_length, FILE *fo)
{
    int rc;
    unsigned n;
#if defined WIN32
    DWORD dwRc;
#endif

    if (sc->cdb_length >= 1) fprintf(fo, "CDB:%02X", sc->cdb[0]);
    for(n=1; n<sc->cdb_length; ++n)
        fprintf(fo, " %02X", sc->cdb[n]);
    fprintf(fo, ", transfer_length=%u\n", data_length);
    rc = scsicmd(drv, sc, buf, SCSI_FROM_DEVICE, data_length);
#if defined WIN32
    dwRc = GetLastError();
#endif
    fprintf(fo, "result=%d", rc);
    if (rc >= 0) {
        fprintf(fo, ", scsi_status=0x%02X\n", sc->scsi_status);
        if (rc > 0 && rc <= data_length) dumpMemToFile(buf, rc, fo);
    } else {
#if defined WIN32
        fprintf(fo, " lasterror=%d (%s)", (int)dwRc, winErrMsg(dwRc));
#endif
        fprintf(fo, "\n");
    }
    if (sc->sense_length) {
        fprintf(fo, "*REQUEST SENSE DATA*\n");
        dumpMemToFile(sc->sense_data, sc->sense_length, fo);
    }
    fprintf(fo, "\n");
    return rc;
}


int scsidrvinfo(SCSIDRV *drv, FILE *fo)
{
    SCSICMDPKT sc_null = {0}, sc;
    unsigned bytes_per_sector = 0;
    U8 *buffer = mymalloc(transfer_buf_max);
    int rc;

    fprintf(fo, "[INQUIRY]\n");
    sc = sc_null;
    sc.cdb_length = 6;
    sc.cdb[0] = 0x12;
    sc.cdb[4] = 255;
    rc = scsi_readdump_extbuf(drv, &sc, buffer, 255, fo);
    if (rc <= 0) {
        myfree(buffer);
        return -1;
    }

    fprintf(fo, "[MODE SENSE(6) all pages]\n");
    sc = sc_null;
    sc.cdb_length = 6;
    sc.cdb[0] = 0x1a; /* MODE SENSE(6) */
    sc.cdb[2] = 0x3f;
    sc.cdb[4] = 255;
    rc = scsi_readdump_extbuf(drv, &sc, buffer, 255, fo);

    fprintf(fo, "[MODE SENSE(10) all pages]\n");
    sc = sc_null;
    sc.cdb_length = 10;
    sc.cdb[0] = 0x5a; /* MODE SENSE(10) */
    sc.cdb[2] = 0x3f;
    sc.cdb[7] = transfer_buf_max >> 8;
    sc.cdb[8] = transfer_buf_max & 0xff;
    rc = scsi_readdump_extbuf(drv, &sc, buffer, transfer_buf_max, fo);

    fprintf(fo, "[READ FORMAT CAPACITIES]\n");
    sc = sc_null;
    sc.cdb_length = 10;
    sc.cdb[0] = 0x23; /* READ FORMAT CAPACITIES */
    sc.cdb[7] = transfer_buf_max >> 8;
    sc.cdb[8] = transfer_buf_max & 0xff;
    rc = scsi_readdump_extbuf(drv, &sc, buffer, transfer_buf_max, fo);

    fprintf(fo, "[READ CAPACITY]\n");
    sc = sc_null;
    sc.cdb_length = 10;
    sc.cdb[0] = 0x25; /* READ CAPACITY(10) */
    rc = scsi_readdump_extbuf(drv, &sc, buffer, 8, fo);
    if (rc == 8) {
        U32 lastlba = (U32)(buffer[3]) | ((U32)(buffer[2]) << 8) | ((U32)(buffer[1]) << 16) | ((U32)(buffer[0]) << 24);
        U32 bpblk = (U32)(buffer[7]) | ((U32)(buffer[6]) << 8) | ((U32)(buffer[5]) << 16) | ((U32)(buffer[4]) << 24);
        if ((U64)lastlba >= 0xffffffffUL || (U64)lastlba * bpblk >= (U64)0xffffffffUL * 512U) {
            fprintf(fo, "[READ CAPACITY(16)]\n");   /* SCSI3 SBC-2 */
            sc = sc_null;
            sc.cdb_length = 16;
            sc.cdb[0] = 0x9e; /* READ CAPACITY(16) */
            sc.cdb[1] = 0x10; /* service action */
            rc = scsi_readdump_extbuf(drv, &sc, buffer, 32, fo);
        }
    }

    fprintf(fo, "[READ DEFECT DATA(10) format=7]\n");
    sc = sc_null;
    sc.cdb_length = 10;
    sc.cdb[0] = 0x37; /* READ DEFECT DATA(10) */
    sc.cdb[2] = 0x17; /* Plist/FmtData=1, Format=7 */
    sc.cdb[7] = transfer_buf_max >> 8;
    sc.cdb[8] = transfer_buf_max & 0xff;
    rc = scsi_readdump_extbuf(drv, &sc, buffer, transfer_buf_max, fo);

    fprintf(fo, "[GET CONFIGURATION]\n");    /* SCSI MMC/INF-8090 */
    sc = sc_null;
    sc.cdb_length = 10;
    sc.cdb[0] = 0x46;   /* MECHANISM STATUS */
    sc.cdb[7] = transfer_buf_max >> 8;
    sc.cdb[8] = transfer_buf_max & 0xff;
    rc = scsi_readdump_extbuf(drv, &sc, buffer, transfer_buf_max, fo);

    fprintf(fo, "[MECHANISM STATUS]\n");    /* SCSI MMC/INF-8090 */
    sc = sc_null;
    sc.cdb_length = 12;
    sc.cdb[0] = 0xbd;   /* MECHANISM STATUS */
    sc.cdb[8] = transfer_buf_max >> 8;
    sc.cdb[9] = transfer_buf_max & 0xff;
    rc = scsi_readdump_extbuf(drv, &sc, buffer, transfer_buf_max, fo);

    myfree(buffer);
    return 0;
}




/*

*/
#define MAX_ARGS 8
#define MAX_OPTPARAMS 8
int optV = 1;
int optHelp;
char *myargv[MAX_ARGS];
char *prmF;
char *prmA;
char *prmI;
char *prmL;
char *prmB;
int optI = -1;
int optA = 0;
int optL = 0;

struct {
    char *shortopt;
    char *shortopt2;
    char *longopt;
    char **paramptr;
} paramopt[] = {
    { "-f", "/f", "--format", &prmF },
    { "-a", "/a", "--adaptor", &prmA }, 
    { "-i", "/i", "--id", &prmI },
    { "-l", "/l", "--lun", &prmL },
    { "-b", "/b", "--max-buffer-length", &prmB },
    { NULL, NULL, NULL }
};

static int my_strtoint(const char *s, int*i)
{
    long l;
    if (!s || !i) return -1;
    l = strtol(s, NULL, 0);
    if (l == LONG_MAX || l == LONG_MIN) return -1;
#if (INT_MAX < LONG_MAX)
    if (l > INT_MAX) return -1;
    if (l < INT_MIN) return -1;
#endif
    *i = (int)l;
    return 0;
}

/* strcmp */
static int my_sc(const char *s0, const char *s1)
{
    while(*s0 || *s1) {
        int rc = toupper((int)*(unsigned char *)s0) - toupper((int)*(unsigned char *)s1);
        if (rc != 0) return rc;
        ++s0;
        ++s1;
    }
    return 0;
}

/* strncasecmp */
static int my_snc(const void *m0, const void *m1, size_t len)
{
    int rc = 0;
    const unsigned char *s0 = m0;
    const unsigned char *s1 = m1;
    while(len && (*s0 || *s1)) {
        rc = toupper(*s0) - toupper(*s1);
        if (rc != 0) break;
        ++s0;
        ++s1;
        --len;
    }
    return rc;
}

static int ck_popt(const char *opt, const char *arg)
{
    if (opt) {
        int n = strlen(opt);
        if (my_snc(opt, arg, n) == 0) {
            if (isdigit(arg[n])) return n;
            if ((arg[n] == ':' || arg[n] == '=')) {
                ++n;
                return arg[n] ? n : 0;
            }
            if (arg[n] == '\0') return 0;
        }
    }
    return -1;
}

static int my_optcmp(const char *opt, const char *arg)
{
    if (*opt == '-' && opt[1] == '-') return my_sc(opt, arg);
    if (*arg == '/') return my_sc(opt +1, arg + 1);
    return my_sc(opt, arg);
}

int mygetopt(int argc, char *argv[])
{
    int rc = 0;
    int narg = 0;
    
    while(--argc > 0) {
        char *s = *++argv;
        int iop = 0;
        while(1) {
            int n;
            if (!paramopt[iop].shortopt && !paramopt[iop].longopt) {
                iop = -1;
                break;
            }
            n = ck_popt(paramopt[iop].shortopt, s);
            if (n == -1) n = ck_popt(paramopt[iop].shortopt2, s);
            if (n == -1) n = ck_popt(paramopt[iop].longopt, s);
            if (n > 0) {
                *(paramopt[iop].paramptr) = s + n;
                break;
            }
            if (n == 0) {
                if (argc >= 1) {
                    --argc;
                    *(paramopt[iop].paramptr) = *++argv;
                    break;
                }
            }
            ++iop;
        }

        if (iop < 0) {
            if (my_optcmp("-?", s)==0 || my_optcmp("-h", s)==0 || my_optcmp("--help", s)==0) optHelp = 1;
            else if (my_optcmp("-v", s)==0 || my_optcmp("--verbose", s)==0) ++optV;
            else if (my_optcmp("-q", s)==0 || my_optcmp("--quiet", s)==0) optV=0;
            else if (narg < MAX_ARGS) {
                myargv[narg++] = s;
            }
        }
    }
    my_strtoint(prmA, &optA);
    my_strtoint(prmI, &optI);
    my_strtoint(prmL, &optL);
    if (prmB) {
        long l = strtol(prmB, NULL, 0);
        if (l >= 36 && l <= SCSI_TRANSFER_DATA_MAX) {
            transfer_buf_max = (unsigned)l;
        }
    }
    return rc < 0 ? rc : narg;
}


void usage(void)
{
    const char msg[] =
        "Get SCSI(-like) drive parameters.\n"
#if defined __DOS__
        "usage: ASPIINFO [-A adapter_id] -I device_id [-L lun] [-B max_buffer_length]\n"
#elif defined WIN32
        "usage: SPTIINFO drvve:\n"
#else
# error unsupprted platform
#endif
        ;

    printf("%s", msg);
}

int main(int argc, char *argv[])
{
    int rc;
    SCSIDRV drv;

    rc = mygetopt(argc, argv);
#if defined WIN32
    if (rc < 1 || optHelp) {
        usage();
        return !optHelp;
    }
    if ((rc = (int)open_drive_win(&drv, myargv[0])) == 0) {
        rc = scsidrvinfo(&drv, stdout);
        close_drive_win(&drv);
    }
#elif defined __DOS__
    if (optI < 0 || optI > 7 || optHelp) {
        usage();
        return !optHelp;
    }
    if ((rc = open_aspi_device(&drv, optA, optI, optL)) == 0) {
        rc = scsidrvinfo(&drv, stdout);
    }
#else
# error unsupported platform
#endif
    else {
        fprintf(stderr, "error: can't open the device (rc=%d)\n", rc);
    }

    return rc != 0;
}

