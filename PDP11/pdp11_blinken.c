/* pdp11_blinken.c: BlinkenLights for the PDP11
 ------------------------------------------------------------------------------
 Copyright (c) 2013, Jordi Guillaumes Pons
 
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
 THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 Except as contained in this notice, the name of the author shall not be
 used in advertising or otherwise to promote the sale, use or other dealings
 in this Software without prior written authorization from the author.
 
 ------------------------------------------------------------------------------
 */

#include <time.h>
#include <ctype.h>
#include <string.h>

#include "sim_defs.h"
#include "blinkenclient.h"
#include "pdp11_blinken.h"

extern int32 R[8];                          /* Working registers */
extern int32 REGFILE[6][2];                 /* Saved registers   */
extern int32 STACKFILE[4];                  /* SP, four modes */
extern int32 cm;                            /*   current mode */
extern int32 rs;                            /*   register set */
extern t_addr cpu_memsize;                  /* last mem addr */
extern uint16 *M;                           /* Physical memory */
extern t_stat (*iodispR[IOPAGESIZE >> 1])(int32 *dat, int32 ad, int32 md);

int32 relocC (int32 va, int32 sw);          /* From pdp11_cpu.c */

DEBTAB blnk_debug[] = {
    {"TRACE",  DBG_TRC},
    {"WARN",   DBG_WRN},
    {"INFO",   DBG_INF},
    {"DATA",   DBG_DAT},
    {"DATASUM",DBG_DTS},
    {"SOCKET", DBG_SOK},
    {0}
};


static dataSelectValues    dataSel = dataPaths;
static displaySelectValues dispSel = consPhy;
static uint32              switchReg = 0;
static uint32              dispAddress = 017777700;
static blnkContextStruct   blnk_context = {NULL, BLNK_DEFAULT_RATE,0};

REG blnk_regs [] = {
    {ORDATA(DATASEL, dataSel, 8),PV_RSPC},
    {ORDATA(DISPSEL, dispSel, 8),PV_RSPC},
    {ORDATA(DISPADD, dispAddress, 32),PV_RZRO},
    NULL
};

MTAB blnk_mods[] = {
    { MTAB_XTD | MTAB_VDV , 0,
        "RATE",   "RATE=pollrate",
        &blnk_setrate, &blnk_showrate, NULL },
    { MTAB_XTD | MTAB_VDV, 0,
        NULL, "REFRESH"
        ,&blnk_setrefresh, NULL, NULL },
    { 0 },
};

/*
 * Device information block (DIB)
 * Since this is not a real UNIBUS device, we won't let it to autoconfigure.
 * The switch register appears in the IOSPACE of the models which have it (ie 11/70)
 */

#define IOLN_BLNK    02      // Footprint = 2 bytes

DIB blnk_dib = {
                0,          // Base addr
                0,          // Length (a single word)
                &blnk_rd,   // Read routine
                NULL,       // Write routine
                0,          // Vectors: number
                0,          // Locator
                0,          // value
                NULL        // Ack routines
};

UNIT blnk_units[] = {
    {NULL,
        &blnk_action,   // Action routine
        NULL,           // Filename
        NULL,           // File reference
        NULL,           // Memory buffer ptr
        0,              // Highwater mark
        1000000,        // Timeout
        UNIT_ATTABLE,   // Flags
        0,              // Capacity
        0,              // File position
        0,              // Buffer
        0,              // Wait
        0,              // Device Specific 3
        0,              // Device Specific 4
        0,              // Device Specific 5
        0               // Device Specific 6
    }
};


DEVICE blnk_dev = {
    "BLNK",          // Device name
    blnk_units,     // Units array (just one!)
    blnk_regs,      // Register array
    blnk_mods,      // Modifiers
    BLNK_UNITSPERDEVICE,  //Number of units
    BLNK_RADIX,     // Address radix
    16,             // Address width
    2,              // Address increment
    BLNK_RADIX,     // Data radix
    16,             // Data width
    NULL,           // Examine routine
    NULL,           // Deposit routine
    &blnk_reset,    // Reset routine
    NULL,           // Boot routine
    &blnk_attach,   // Attach routine
    &blnk_detach,   // Detach routine
    &blnk_dib,      // Private context
    DEV_DISABLE | DEV_DIS,
    0, 0, NULL, NULL
};


t_stat blnk_action(UNIT *uptr) {
    clock_t now = clock();
    clock_t intvl = now - blnk_context.last_clock;
    int ratio;

    ratio = intvl*blnk_context.rate/CLOCKS_PER_SEC;
    if (ratio > 1) {
        blnk_send_lites(&blnk_dev);
        blnk_context.last_clock = now;
    }
    // printf("Intvl: %d, ratio: %d\n", intvl, ratio);

    return sim_activate_after(uptr, BLNK_DEFAULT_POLL);
}

t_stat blnk_reset(DEVICE *dptr) {
    t_stat rc=SCPE_OK;
    
    auto_config(0,0);
    if (dptr->units[0].flags & UNIT_ATT) {
        rc = sim_activate_after(&blnk_units[0], BLNK_DEFAULT_POLL);
    }
  
    if (rc != SCPE_OK) return rc;
    
    if (!(dptr->flags & DEV_DIS))
    {
        rc = auto_config ("BLNK", 1);
    }
    return rc;
}

t_stat blnk_refresh (UNIT* uptr, int32 val, char* cptr, void* desc) {
    blnk_send_lites(&blnk_dev);
    return SCPE_OK;
}

void blnk_send_lites(DEVICE *dptr) {
    int rc=0;
    int nreg=0;
    int32 idx=0;
    int resync = 0;
    int error = 0;
    int32 sw = 0;
    int32 pa = 0;
    int32 val = 0;
    WORD lites = 0;
    
    if ((dptr->units[0].flags & UNIT_ATT) != 0) {
        
        switch(dataSel) {
            case dataPaths:
                if (SEL_MEM_REG(dispAddress)) {
                    nreg = dispAddress - 017777700;
                    if (nreg < 6) {
                        if (rs == 0) {
                            lites = R[nreg];
                        } else {
                            lites = REGFILE[nreg][0];
                        }
                    } else if (nreg == 6) {
                        if (cm == 0) {
                            lites = R[6];
                        } else {
                            lites = STACKFILE[0];
                        }
                    } else if (nreg == 7) {
                        lites = R[7];       // <- FIX THIS!
                    } else if (nreg < 13 ) {
                        if (rs == 1) {
                            lites = R[nreg];
                        } else {
                            lites = REGFILE[nreg-8][1];
                        }
                    } else if (nreg == 14) {
                        if (cm == 1) {
                            lites = R[6];
                        } else {
                            lites = STACKFILE[1];
                        }
                    } else if (nreg == 15 ) {
                        if (cm == 3) {
                            lites = R[6];
                        } else {
                            lites = STACKFILE[3];
                        }
                    }
                }
                else if (SEL_MEM_PHYS(dispSel)) {
                    switch(dispSel) {
                        case consPhy:
                            if (ADDR_IS_MEM(dispAddress)) {
                                lites = M[(dispAddress & 0x003FFFFF) >> 1];
                            } else {
                                if (dispAddress < IOPAGEBASE) {
                                    lites = 0;
                                    error = 1;
                                } else {
                                    idx = (dispAddress & IOPAGEMASK) >> 1;
                                    if (iodispR[idx]) {
                                        rc = iodispR[idx] (&val, dispAddress, READ);
                                        if (rc == SCPE_OK) lites = (WORD) val;
                                    } else {
                                        rc = SCPE_NXM;
                                    }
                                    if (rc != SCPE_OK) {
                                        error = 1;
                                    }
                                }
                            }
                            break;
                        case progPhy:
                        default:
                            lites = 0;
                            error = 1;
                            break;
                    }
                } else {
                    switch(dispSel) {
                        case userI:
                        case userD:
                            sw = SWMASK('U');
                            break;
                        case superI:
                        case superD:
                            sw = SWMASK('S');
                            break;
                        case kernelI:
                        case kernelD:
                            sw = SWMASK('K');
                            break;
                        default:
                            sw = SWMASK('U');
                            break;
                    }
                    if (dispSel == userD ||
                        dispSel == superD ||
                        dispSel == kernelD) {
                        sw = sw | SWMASK('D');
                    }
                    pa = relocC(dispAddress, sw);
                    if (pa != MAXMEMSIZE) {
                        lites = M[pa>>1];
                    } else {
                        lites = 0;
                        error = 1;
                    }
                }
                break;
            case busReg:
                lites = 0;
                error = 1;
                break;
            case microAddress:
                lites = 0;
                error = 1;
                break;
            default:
                lites = 0;
                error = 1;
                break;
        }
        if (error==1) {
            blk_sendError(blnk_context.bstat,resync);
        } else {
            blk_sendWord((PBLINKENSTATUS) blnk_context.bstat , lites, resync);
        }
    }
}

t_stat blnk_attach (UNIT* uptr, char *attString) {
    PBLINKENSTATUS pstat = NULL;
    int lstring=0;
    t_stat status = SCPE_OK;

    if (uptr->flags & UNIT_ATT) blnk_detach(uptr);

    pstat = blk_open(attString);
    
    if (pstat != NULL) {
        lstring = strlen(attString);
        uptr->filename = malloc(lstring+1);
        memset(uptr->filename, 0, lstring+1);
        strncpy(uptr->filename, attString, lstring);
        uptr->flags |= UNIT_ATT;
        if(blnk_context.bstat != NULL) {
            blk_close(blnk_context.bstat);
        }
        blnk_context.bstat = pstat;
        status = sim_activate_after(uptr, BLNK_DEFAULT_POLL);
    } else {
        status = SCPE_OPENERR;
    }
    return status;
}

t_stat blnk_detach(UNIT *uptr) {
    t_stat status = SCPE_OK;

    sim_cancel(uptr);

    if (uptr->flags & UNIT_ATT) {
        if (blnk_context.bstat != NULL) {
            blk_close(blnk_context.bstat);
            blnk_context.bstat = NULL;
        }
        free(uptr->filename);
        uptr->filename = NULL;
        uptr->flags &= ~UNIT_ATT;
    }
    return status;
}

t_stat blnk_setrefresh (UNIT* uptr, int32 val, char* cptr, void* desc) {
    blnk_send_lites(&blnk_dev);
    return SCPE_OK;
}

t_stat blnk_setrate (UNIT* uptr, int32 val, char* cptr, void* desc) {
    int32 newrate;

    char *endptr = NULL;
    if (cptr == NULL) return SCPE_ARG;
    if (*cptr == '\0') return SCPE_ARG;
    newrate = strtol(cptr, &endptr, 10);
    if (*endptr != '\0') return SCPE_ARG;
    blnk_context.rate = newrate;
    sim_cancel(uptr);
    return sim_activate_after(uptr, BLNK_DEFAULT_POLL);
}

t_stat blnk_showrate (FILE *st, UNIT *uptr, int value, void *desc) {
    fprintf(st, "rate=%d", blnk_context.rate);
    return SCPE_OK;
}

t_stat blnk_rd(int32 *data, int32 PA, int32 access)
{
    sim_debug(DBG_TRC, &blnk_dev, "blnk_rd(), addr=0x%x access=%d\n", PA, access);
    *data = switchReg;
    
    return SCPE_OK;
}
