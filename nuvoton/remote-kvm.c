/*
 * remote-kvm.c
 *
 * Copyright (C) 2018 NUVOTON
 *
 * KW Liu <kwliu@nuvoton.com>
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; If not, see <http://www.gnu.org/licenses/>
 */

#include <rfb/rfbnpcm750.h>
#include <rfb/usb_hid.h>

struct nu_rfb *nurfb = NULL;

static void clientgone(rfbClientPtr cl)
{
    nurfb->cl_cnt--;
    cl->clientData = NULL;
}

static enum rfbNewClientAction newclient(rfbClientPtr cl)
{
    cl->clientData = nurfb;
    cl->id = ++nurfb->cl_cnt;

    if (cl->id > 1)
        cl->viewOnly = 1;

    cl->update_cnt = 30;
    cl->clientGoneHook = clientgone;
    cl->screen->serverFormat.redMax = 31;
    cl->screen->serverFormat.greenMax = 63;
    cl->screen->serverFormat.blueMax = 31;
    cl->screen->serverFormat.redShift = 11;
    cl->screen->serverFormat.greenShift = 5;
    cl->screen->serverFormat.blueShift = 0;
    return RFB_CLIENT_ACCEPT;
}

int main(int argc,char** argv)
{
    nurfb = init_nurfb();
    if (!nurfb)
        return 0;

    rfbScreenInfoPtr rfbScreen = rfbGetScreen(&argc, argv, nurfb->vcd_info.hdisp, nurfb->vcd_info.vdisp, 5, 1, 2);
    if(!rfbScreen)
        return 0;

    rfbScreen->desktopName = "Remote KVM";
    rfbScreen->frameBuffer = nurfb->raw_fb_addr;
    rfbScreen->alwaysShared = TRUE;
    rfbScreen->ptrAddEvent = pointer_event;
    rfbScreen->kbdAddEvent = keyboard;
    rfbScreen->newClientHook = newclient;

  /* initialize the server */
    rfbInitServer(rfbScreen);
    rfbRunEventLoop(rfbScreen,40000,FALSE);

    clear_nurfb(nurfb);
    rfbScreenCleanup(rfbScreen);
    return(0);
}
