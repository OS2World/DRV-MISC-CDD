#define    INCL_BASE
#include   <os2.h>

#include   <string.h>

/* forward references */
void far start(void);
void _saveregs near realstart(int i);
void initproc(void);

/* structure of an INIT device packet */
typedef struct
{
   char    PktLen;             /* Lenght in bytes of packet            */
   char    PktUnit;            /* Subunit number of block device       */
   char    PktCmd;             /* Command code                         */
   int     PktStatus;          /* Status word                          */
   long    PktDOSLink;         /* Reserved                             */
   long    PktDevLink;         /* Device multiple-request link         */
   char    PktData[1];         /* Data pertaining to specific packet   */
   int     codesize;           /* code size on INIT return             */
   int     datasize;           /* data size on INIT return             */
} PKT;

/* definitions of device driver return status */
#define done   0x0100          /* Word representing the code for DONE  */
#define error  0x8000          /* error return                         */
#define general_failure 0xc    /* aaargh!                              */

/* structure of device header */
typedef struct
{
   PSZ  ptr_to_next;           /* pointer to next device driver        */
                               /* set to -1 OS/2 will fill it in       */
   int  attributes;            /* attributes of device driver          */
   PFN  strategy;              /* pointer to strategy routine          */
                               /* + the ignored SEG part of strategy!  */
   char name[8];               /* name of the device                   */
   int rsvd[4];                /* used by OS/2                         */
} DEVHDR;

/* definitions of some of the bits in the attribute word */
#define devlev_1       0x0080  /* Bits 7-9 OS/2 1.x                    */
#define dev_char_dev   0x8000  /* Bit 15: Device is a character device */
#define dev_open       0x0800  /* Bit 11: Accepts Open/Close/Removable */

/* define this device */
static DEVHDR devhdr =
{
   (PSZ) -1L,
   devlev_1 | dev_char_dev | dev_open,
   (PFN) start,
   "CDD$    "
};

/* definitions of the command values used */
#define INIT   0

/* define _acrtused to prevent the C runtime startup code being linked in */
int _acrtused = 0;

/* device driver entry point */
/* ES:BX points to the device driver request packet */
void far start(void)
{
   /* call the real entry point */
   realstart(0);
}

/* the 'real' device driver code - work around for the C5.1 bug */
void _saveregs near realstart(int dummy)
{
   PKT far  *pkt;
   int      *regptr;

   #define ES -11      /* offset for ES in saved registers */
   #define BX -5       /* offset for BX in saved registers */

   regptr = &dummy;    /* get address of top of saved registers */

   pkt = MAKEP( regptr[ES], regptr[BX]);

   /* if not init command, return failure! */
   if (pkt->PktCmd != INIT)
   {
       pkt->PktStatus = done + error + general_failure;
       return;
   }

   /* perform the initialisation function */
   initproc();

   /* save code and data size to initialise as a device driver */
   pkt->codesize  = (int)initproc;
   pkt->datasize  = sizeof(DEVHDR);
   pkt->PktStatus = done;

} /* realstart */

void initproc(void)
{
   static  KBDKEYINFO kbci;
   RESULTCODES        res;
   int                i;

   /* initial read to kick things off - seems needed to wake up KBD subsystem */
   KbdPeek(&kbci, 0);

   /* tell the world we're ready */
   VioWrtTTY("Press ESC for command prompt...", 31, 0);

   /* wait up to 5 seconds for a keypress then carry on */
   for ( i=0; i<10; i++)
   {
       DosSleep(500L);

       /* drop out on key press */
       if ((KbdCharIn(&kbci, IO_NOWAIT, 0) == 0) && (kbci.fbStatus & 0x40))
           break;
   } /* for */

   /* check for ESC, which kicks off CMD.EXE */
   if (kbci.chChar == '\033')
   {
       DosExecPgm(NULL,
                  0,
                  EXEC_SYNC,
                  "cmd\0",                 /* no arguments            */
                  NULL,                    /* inherit the environment */
                  &res,                    /* resultcode              */
                  "\\os2\\cmd.exe");       /* program name            */
   }

} /* initproc */
