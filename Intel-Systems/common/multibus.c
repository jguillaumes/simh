/*  multibus.c: Multibus I simulator

    Copyright (c) 2010, William A. Beech

        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included in
        all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
        WILLIAM A. BEECH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
        IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
        CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

        Except as contained in this notice, the name of William A. Beech shall not be
        used in advertising or otherwise to promote the sale, use or other dealings
        in this Software without prior written authorization from William A. Beech.

    MODIFICATIONS:

        ?? ??? 10 - Original file.
        16 Dec 12 - Modified to use isbc_80_10.cfg file to set base and size.
        24 Apr 15 -- Modified to use simh_debug

    NOTES:

        This software was written by Bill Beech, Dec 2010, to allow emulation of Multibus
        Computer Systems.

*/

#include "system_defs.h"

#define SET_XACK(VAL)       (xack = VAL)

int32   mbirq = 0;              /* set no multibus interrupts */

/* function prototypes */

t_stat multibus_svc(UNIT *uptr);
t_stat multibus_reset(DEVICE *dptr);
void set_irq(int32 int_num);
void clr_irq(int32 int_num);
int32 nulldev(int32 io, int32 data);
int32 reg_dev(int32 (*routine)(int32, int32), int32 port);
t_stat multibus_reset (DEVICE *dptr);
int32 multibus_get_mbyte(int32 addr);
int32 multibus_get_mword(int32 addr);
void multibus_put_mbyte(int32 addr, int32 val);
void multibus_put_mword(int32 addr, int32 val);

/* external function prototypes */

extern t_stat SBC_reset(DEVICE *dptr);      /* reset the iSBC80/10 emulator */
extern int32 isbc064_get_mbyte(int32 addr);
extern void isbc064_put_mbyte(int32 addr, int32 val);
extern void set_cpuint(int32 int_num);
extern t_stat SBC_reset (DEVICE *dptr);
extern t_stat isbc064_reset (DEVICE *dptr);
extern t_stat isbc208_reset (DEVICE *dptr);

/* external globals */

extern uint32 xack;                          /* XACK signal */
extern int32 int_req;                       /* i8080 INT signal */

/* multibus Standard SIMH Device Data Structures */

UNIT multibus_unit = { 
    UDATA (&multibus_svc, 0, 0), 20 
};

REG multibus_reg[] = { 
    { HRDATA (MBIRQ, mbirq, 32) }, 
    { HRDATA (XACK, xack, 8) }
};

DEBTAB multibus_debug[] = {
    { "ALL", DEBUG_all },
    { "FLOW", DEBUG_flow },
    { "READ", DEBUG_read },
    { "WRITE", DEBUG_write },
    { "LEV1", DEBUG_level1 },
    { "LEV2", DEBUG_level2 },
    { NULL }
};

DEVICE multibus_dev = {
    "MBIRQ",                    //name 
    &multibus_unit,             //units 
    multibus_reg,               //registers 
    NULL,                       //modifiers
    1,                          //numunits 
    16,                         //aradix  
    32,                         //awidth  
    1,                          //aincr  
    16,                         //dradix  
    8,                          //dwidth
    NULL,                       //examine  
    NULL,                       //deposit  
    &multibus_reset,            //reset 
    NULL,                       //boot
    NULL,                       //attach  
    NULL,                       //detach
    NULL,                       //ctxt     
    DEV_DEBUG,                  //flags 
    0,                          //dctrl 
    multibus_debug,             //debflags
    NULL,                       //msize
    NULL                        //lname
};

/* Service routines to handle simulator functions */

/* service routine - actually does the simulated interrupts */

t_stat multibus_svc(UNIT *uptr)
{
    switch (mbirq) {
        case INT_1:
            set_cpuint(INT_R);
            clr_irq(SBC208_INT);    /***** bad, bad, bad! */
//            sim_printf("multibus_svc: mbirq=%04X int_req=%04X\n", mbirq, int_req);
            break;
        default:
//            sim_printf("multibus_svc: default mbirq=%04X\n", mbirq);
            break;
    }
    sim_activate (&multibus_unit, multibus_unit.wait); /* continue poll */
    return SCPE_OK;
}

/* Reset routine */

t_stat multibus_reset(DEVICE *dptr)
{
    SBC_reset(NULL); 
    isbc064_reset(NULL); 
    isbc208_reset(NULL); 
    sim_printf("   Multibus: Reset\n");
    sim_activate (&multibus_unit, multibus_unit.wait); /* activate unit */
    return SCPE_OK;
}

void set_irq(int32 int_num)
{
    mbirq |= int_num;
//    sim_printf("set_irq: int_num=%04X mbirq=%04X\n", int_num, mbirq);
}

void clr_irq(int32 int_num)
{
    mbirq &= ~int_num;
//    sim_printf("clr_irq: int_num=%04X mbirq=%04X\n", int_num, mbirq);
}

/* This is the I/O configuration table.  There are 256 possible
device addresses, if a device is plugged to a port it's routine
address is here, 'nulldev' means no device is available
*/
struct idev {
    int32 (*routine)(int32, int32);
};

struct idev dev_table[256] = {
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 000H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 004H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 008H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 00CH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 010H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 014H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 018H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 01CH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 020H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 024H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 028H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 02CH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 030H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 034H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 038H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 03CH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 040H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 044H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 048H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 04CH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 050H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 054H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 058H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 05CH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 060H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 064H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 068H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 06CH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 070H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 074H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 078H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 07CH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 080H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 084H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 088H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 08CH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 090H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 094H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 098H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 09CH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0A0H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0A4H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0A8H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0A0H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0B0H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0B4H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0B8H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0B0H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0C0H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0C4H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0C8H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0CCH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0D0H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0D4H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0D8H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0DCH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0E0H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0E4H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0E8H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0ECH */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0F0H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0F4H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev},         /* 0F8H */
{&nulldev}, {&nulldev}, {&nulldev}, {&nulldev}          /* 0FCH */
};

int32 nulldev(int32 flag, int32 data)
{
    SET_XACK(0);                        /* set no XACK */
    if (flag == 0)                      /* if we got here, no valid I/O device */
        return (0xFF);
    return 0;
}

int32 reg_dev(int32 (*routine)(int32, int32), int32 port)
{
    if (dev_table[port].routine != &nulldev) {  /* port already assigned */
//        sim_printf("Multibus: I/O Port %02X is already assigned\n", port);
    } else {
//        sim_printf("Port %02X is assigned\n", port);
        dev_table[port].routine = routine;
    }
	return 0;
}

/*  get a byte from memory */

int32 multibus_get_mbyte(int32 addr)
{
    SET_XACK(0);                        /* set no XACK */
//    sim_printf("multibus_get_mbyte: Cleared XACK for %04X\n", addr); 
    return isbc064_get_mbyte(addr);
}

/*  get a word from memory */

int32 multibus_get_mword(int32 addr)
{
    int32 val;

    val = multibus_get_mbyte(addr);
    val |= (multibus_get_mbyte(addr+1) << 8);
    return val;
}

/*  put a byte to memory */

void multibus_put_mbyte(int32 addr, int32 val)
{
    SET_XACK(0);                        /* set no XACK */
//    sim_printf("multibus_put_mbyte: Cleared XACK for %04X\n", addr); 
    isbc064_put_mbyte(addr, val);
//    sim_printf("multibus_put_mbyte: Done XACK=%dX\n", XACK); 
}

/*  put a word to memory */

void multibus_put_mword(int32 addr, int32 val)
{
    multibus_put_mbyte(addr, val);
    multibus_put_mbyte(addr+1, val << 8);
}

/* end of multibus.c */

