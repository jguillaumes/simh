/*  iEPROM.c: Intel EPROM simulator for 8-bit SBCs

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

    NOTES:

        These functions support a simulated ROM devices on an iSBC-80/XX SBCs.
        This allows the attachment of the device to a binary file containing the EPROM
        code image.  Unit will support a single 2708, 2716, 2732, or 2764 type EPROM.
        These functions also support bit 0x80 of 8255 number 0, port C, to enable/
        disable the onboard ROM.
*/

#include "system_defs.h"

/* function prototypes */

t_stat EPROM_cfg(uint16 base, uint16 size);
t_stat EPROM_attach (UNIT *uptr, CONST char *cptr);
t_stat EPROM_reset (DEVICE *dptr);
uint8 EPROM_get_mbyte(uint16 addr);

/* external function prototypes */

/* external globals */

extern uint8 xack;                   /* XACK signal */
extern uint8 i8255_C[4];                    //port C byte I/O

/* globals */

/* SIMH EPROM Standard I/O Data Structures */

UNIT EPROM_unit = {
    UDATA (NULL, UNIT_ATTABLE+UNIT_BINK+UNIT_ROABLE+UNIT_RO+UNIT_BUFABLE+UNIT_MUSTBUF, 0), 0
};

DEBTAB EPROM_debug[] = {
    { "ALL", DEBUG_all },
    { "FLOW", DEBUG_flow },
    { "READ", DEBUG_read },
    { "WRITE", DEBUG_write },
    { "XACK", DEBUG_xack },
    { "LEV1", DEBUG_level1 },
    { "LEV2", DEBUG_level2 },
    { NULL }
};

DEVICE EPROM_dev = {
    "EPROM",            //name
    &EPROM_unit,        //units
    NULL,               //registers
    NULL,               //modifiers
    1,                  //numunits
    16,                 //aradix
    16,                 //awidth
    1,                  //aincr
    16,                 //dradix
    8,                  //dwidth
    NULL,               //examine
    NULL,               //deposit
    EPROM_reset,        //reset
    NULL,               //boot
    &EPROM_attach,      //attach
    NULL,               //detach
    NULL,               //ctxt
    DEV_DEBUG,          //flags
    0,                  //dctrl
    EPROM_debug,        //debflags
    NULL,               //msize
    NULL                //lname
};

/* EPROM functions */

// EPROM configuration

t_stat EPROM_cfg(uint16 base, uint16 size)
{
    EPROM_unit.capac = size;        /* set EPROM size */
    EPROM_unit.u3 = base & 0xFFFF;  /* set EPROM base */
    sim_printf("    EPROM: 0%04XH bytes at base 0%04XH\n",
        EPROM_unit.capac, EPROM_unit.u3);
    return SCPE_OK;
}

/* EPROM reset */

t_stat EPROM_reset (DEVICE *dptr)
{
    return SCPE_OK;
}

/* EPROM attach  */

t_stat EPROM_attach (UNIT *uptr, CONST char *cptr) 
{
    t_stat r;

    if ((r = attach_unit (uptr, cptr)) != SCPE_OK) {
        sim_printf ("EPROM_attach: Error %d\n", r);
        return r;
    }
    return SCPE_OK;
}

/*  get a byte from memory */ 

uint8 EPROM_get_mbyte(uint16 addr)
{
    uint8 val;

    if ((addr >= EPROM_unit.u3) && ((uint16) addr <= (EPROM_unit.u3 + EPROM_unit.capac))) {
        SET_XACK(1);                /* good memory address */
        val = *((uint8 *)EPROM_unit.filebuf + (addr - EPROM_unit.u3));
        val &= 0xFF;
        return val;
    } else {
        SET_XACK(0);                /* bad memory address */
        sim_printf("EPROM:  Out of range\n");
    }
    SET_XACK(0);                    /* bad memory address */
    return 0;
}

/* end of iEPROM.c */
