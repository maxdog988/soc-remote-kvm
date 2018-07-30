/*
 * rfbnpcm750.c
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
#include <sys/mman.h>
#include <errno.h>

void clear_nurfb(struct nu_rfb *nurfb) {
    if (nurfb->fake_fb) {
        free(nurfb->fake_fb);
        nurfb->fake_fb = NULL;
    } else if (nurfb->raw_fb_addr) {
        munmap(nurfb->raw_fb_addr, nurfb->raw_fb_mmap);
        nurfb->raw_fb_mmap = 0;
        nurfb->raw_fb_addr = NULL;
    }

    if (nurfb->raw_fb_fd > -1) {
        close(nurfb->raw_fb_fd);
        nurfb->raw_fb_fd = -1;
    }

    if (nurfb->raw_ece_addr) {
        munmap(nurfb->raw_ece_addr, nurfb->raw_ece_mmap);
        nurfb->raw_ece_mmap = 0;
        nurfb->raw_ece_addr = NULL;
    }

    if (nurfb->hextile_fd > -1) {
        close(nurfb->hextile_fd);
        nurfb->hextile_fd = -1;
    }

    free(nurfb);
    nurfb = NULL;

    hid_close();
}

struct nu_rfb *init_nurfb(void) {
    struct nu_rfb *nurfb = NULL;

    nurfb = malloc(sizeof(struct nu_rfb));
    if (!nurfb)
        return NULL;

    memset(nurfb, 0 , sizeof(struct nu_rfb));

    if (init_vcd(nurfb) < 0)
        return NULL;

    return nurfb;
}

int init_vcd(struct nu_rfb *nurfb) {
    struct vcd_info *vcd_info = &nurfb->vcd_info;
    struct ioctl_cmd cmd;
    int size;

    if (nurfb->last_mode == RAWFB_MMAP) {
        if (!nurfb->fake_fb) {
            munmap(nurfb->raw_fb_addr, nurfb->raw_fb_mmap);
        } else {
            free(nurfb->fake_fb);
            nurfb->fake_fb = NULL;
        }

        close(nurfb->raw_fb_fd);
        munmap(nurfb->raw_ece_addr, nurfb->raw_ece_mmap);
        close(nurfb->hextile_fd);
        nurfb->last_mode = 0;
    }

    if (nurfb->last_mode == 0) {
        nurfb->raw_fb_fd = -1;
        nurfb->hextile_fd = -1;
    }

    nurfb->raw_fb_fd = open("/dev/vcd", O_RDWR);
    if (nurfb->raw_fb_fd < 0) {
        rfbLog("failed to open /dev/vcd \n");
        goto error;
    }

    if (init_vcd_res(nurfb) < 0)
        goto error;

    if (get_vcd_info(nurfb, vcd_info) < 0)
        goto error;

    size = vcd_info->hdisp *  vcd_info->vdisp * (16 / 8);

    nurfb->hextile_fd = open("/dev/hextile", O_RDWR);
    if (nurfb->hextile_fd < 0) {
        rfbErr("failed to open /dev/hextile \n");
        goto error;
    }

    cmd.framebuf= vcd_info->vcd_fb;
    if (ioctl(nurfb->hextile_fd, SETFB , &cmd) < 0) {
        rfbErr("ece SETFB failed \n");
        goto error;
    }

    if (vcd_info->hdisp == 0
        || vcd_info->vdisp == 0
        || vcd_info->line_pitch == 0) {
        /* grapich is off, fake a FB */
        vcd_info->hdisp = 320;
        vcd_info->vdisp = 240;
        vcd_info->line_pitch = 1024;
        nurfb->fake_fb = malloc(vcd_info->hdisp  * vcd_info->vdisp  * 2);
        if (!nurfb->fake_fb)
            goto error;
    }

    cmd.w = vcd_info->hdisp;
    cmd.h = vcd_info->vdisp;
    cmd.LP = vcd_info->line_pitch;
    if (ioctl(nurfb->hextile_fd, SETLP , &cmd) < 0) {
        rfbErr("ece SETLP failed \n");
        goto error;
    }

    nurfb->raw_ece_mmap = vcd_info->hdisp * vcd_info->vdisp * 2;
    nurfb->raw_ece_addr = mmap(0, nurfb->raw_ece_mmap, PROT_READ,
        MAP_SHARED, nurfb->hextile_fd, 0);
    if (!nurfb->raw_ece_addr) {
        rfbErr("mmap raw_ece_addr failed \n");
        goto error;
    }

    if (!nurfb->is_hid_init) {
        hid_init();
        nurfb->is_hid_init = 1;
    }

    if (!nurfb->fake_fb) {
        nurfb->raw_fb_addr = mmap(0, size, PROT_READ, MAP_SHARED,
            nurfb->raw_fb_fd, 0);
        if (!nurfb->raw_fb_addr) {
            rfbErr("mmap raw_fb_addr failed \n");
            goto error;
        } else {
            nurfb->raw_fb_mmap = size;
            rfbLog("   w: %d h: %d b: %d addr: %p sz: %d\n", vcd_info->hdisp, vcd_info->vdisp,
                16, nurfb->raw_fb_addr, size);
        }
    } else
        nurfb->raw_fb_addr = nurfb->fake_fb;

    nurfb->last_mode = RAWFB_MMAP;
    return 0;

error:
    clear_nurfb(nurfb);
    return -1;
}

int get_update(rfbClientRec *cl)
{
    struct nu_rfb *nurfb = (struct nu_rfb *)cl->clientData;

    if (chk_vcd_res(cl)) {
        if (cl->id == 1) {
            printf("*********res is changed **********\n");
            usleep(1000 * 600);
            set_vcd_cmd(nurfb, 0);
            init_vcd(nurfb);
            rfbNewFramebuffer(cl->screen, nurfb->raw_fb_addr, nurfb->vcd_info.hdisp, nurfb->vcd_info.vdisp, 5, 1, 2);
            cl->screen->serverFormat.redMax = 31;
            cl->screen->serverFormat.greenMax = 63;
            cl->screen->serverFormat.blueMax = 31;
            cl->screen->serverFormat.redShift = 11;
            cl->screen->serverFormat.greenShift = 5;
            cl->screen->serverFormat.blueShift = 0;
        }

        LOCK(cl->updateMutex);
        cl->useNewFBSize = 1;
        cl->newFBSizePending = 1;
        UNLOCK(cl->updateMutex);
        cl->update_cnt = 30;
        return 1;
    }

    if (cl->update_cnt)
        cl->update_cnt--;

    if (!cl->incremental || cl->update_cnt > 0) {
        if (cl->id == 1)
            set_vcd_cmd(nurfb, 0);
        return 1;
    } else {
        if (cl->id == 1)
            set_vcd_cmd(nurfb, 2);
        return get_diff_cnt(cl);
    }
}

char *get_fb_adr(rfbClientRec *cl)
{
    struct nu_rfb *nurfb = (struct nu_rfb *)cl->clientData;
    return nurfb->raw_fb_addr;
}

int get_diff_cnt(rfbClientRec *cl)
{
    struct nu_rfb *nurfb = (struct nu_rfb *)cl->clientData;
    if (cl->id == 1) {
        if (ioctl(nurfb->raw_fb_fd, VCD_IOCDIFFCNT, &nurfb->diff_cnt) < 0) {
            printf("get diff cnt failed\n");
            return -1;
        }

        if (nurfb->diff_table) {
            free(nurfb->diff_table);
            nurfb->diff_table = NULL;
        }

        nurfb->diff_table = (struct vcd_diff *)malloc(sizeof(struct vcd_diff) * (nurfb->diff_cnt + 1));
    }

    return nurfb->diff_cnt;
}

int get_diff_table(rfbClientRec *cl, struct vcd_diff *diff, int i)
{
    struct nu_rfb *nurfb = (struct nu_rfb *)cl->clientData;
    struct vcd_diff *diff_table = nurfb->diff_table;

    if (cl->id == 1) {
        if (ioctl(nurfb->raw_fb_fd, VCD_IOCGETDIFF, diff) < 0) {
            printf("get diff table failed\n");
            return -1;
        }
        diff_table[i].x = diff->x;
        diff_table[i].y = diff->y;
        diff_table[i].w = diff->w;
        diff_table[i].h = diff->h;
    } else {
        if (!cl->incremental || cl->update_cnt > 0) {
            diff->x = 0;
            diff->y = 0;
            diff->w = cl->screen->width;
            diff->h = cl->screen->height;
        } else {
            diff->x = diff_table[i].x;
            diff->y = diff_table[i].y;
            diff->w = diff_table[i].w;
            diff->h = diff_table[i].h;
        }
    }
    return 0;
}

int chk_vcd_res(rfbClientRec *cl)
{
    struct nu_rfb *nurfb = (struct nu_rfb *)cl->clientData;
    if (cl->id == 1) {
        if (ioctl(nurfb->raw_fb_fd, VCD_IOCCHKRES, &nurfb->res_changed) < 0) {
            printf("VCD_IOCCHKRES failed\n");
            return -1;
        }
    }
    return nurfb->res_changed;
}

int init_vcd_res(struct nu_rfb *nurfb)
{
    int changed = 0;
    if (ioctl(nurfb->raw_fb_fd, VCD_IOCCHKRES, &changed) < 0) {
        printf("VCD_IOCCHKRES failed\n");
        return -1;
    }
    return 0;
}

int get_vcd_info(struct nu_rfb *nurfb, struct vcd_info *info) {
    if (ioctl(nurfb->raw_fb_fd, VCD_IOCGETINFO, info) < 0) {
        printf("get info failed\n");
        return -1;
    }
    return 0;
}

int set_vcd_cmd(struct nu_rfb *nurfb, int cmd)
{
    if (ioctl(nurfb->raw_fb_fd, VCD_IOCSENDCMD, &cmd) < 0) {
        printf("set cmd failed\n");
        return -1;
    }
    return 0;
}
