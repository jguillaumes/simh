/* pdp11_blinken.h: BlinkenLights for the PDP11
 ------------------------------------------------------------------------------
 
 Copyright (c) 2012, Jordi Guillaumes Pons
 
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
 
 ---------------------------------------------------------------------------*/

// Notes
//

#ifndef _PDP11_BLINKEN_H
#define _PDP11_BLINKEN_H

#include "pdp11_defs.h"
#include "blinkenclient.h"

#define BLNK_NUMDEVICE 1            /* Just one device      */
#define BLNK_UNITSPERDEVICE 1       /* Just one unit        */
#define BLNK_RADIX 8                /* Not used anywhere... */

/* debugging bitmaps */
#define DBG_TRC  0x0001             /* trace routine calls */
#define DBG_WRN  0x0004             /* display warnings */
#define DBG_INF  0x0008             /* display informational messages (high level trace) */
#define DBG_DAT  0x0010             /* display data buffer contents */
#define DBG_DTS  0x0020             /* display data summary */
#define DBG_SOK  0x0040             /* display socket open/close */

#define BLNK_DEFAULT_PORT   11696
#define BLNK_DEFAULT_BITS   16
#define BLNK_DEFAULT_HOST   "127.0.0.1"

#define BLNK_DEFAULT_RATE 100
#define BLNK_DEFAULT_POLL 10000

#define SWREG_ADDRESS   017777570

typedef enum {
    dataPaths=0,
    displayRegister=1,
    busReg=2,
    microAddress=3
} dataSelectValues;

typedef enum {
    consPhy=0,
    progPhy=1,
    userI=2,
    userD=3,
    superI=4,
    superD=5,
    kernelI=6,
    kernelD=7
} displaySelectValues;

#define SEL_MEM_PHYS(d)     (((d)==consPhy)||((d)==progPhy))
#define SEL_MEM_VIRT(d)     (!SEL_MEM_PHYS((d)))
#define SEL_MEM_REG(d)      (((d) >= 017777700) && ((d) <= 017777717))

typedef struct {
    PBLINKENSTATUS bstat;
    int32 rate;
    clock_t last_clock;
} blnkContextStruct;

t_stat blnk_action(UNIT *);
t_stat blnk_reset(DEVICE *);
t_stat blnk_attach (UNIT* uptr, char* cptr);
t_stat blnk_detach (UNIT *uptr);
t_stat blnk_setrate (UNIT* uptr, int32 val, char* cptr, void* desc);
t_stat blnk_showrate (FILE *st, UNIT *uptr, int value, void *desc);
t_stat blnk_setrefresh (UNIT* uptr, int32 val, char* cptr, void* desc);
void blnk_send_lites(DEVICE *);
t_stat blnk_rd(int32 *data, int32 PA, int32 access);

#endif                               /* _PDP11_BLINKEN_H */
