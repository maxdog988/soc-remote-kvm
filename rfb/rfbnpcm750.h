#ifndef RFBNPCM750_H
#define RFBNPCM750_H

/*
 * rfbnpcm750.h
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


#include <rfb/rfb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define RAWFB_MMAP 1
#define RAWFB_FILE 2
#define RAWFB_SHM  3

struct ioctl_cmd {
    unsigned int framebuf;
    unsigned int gap_len;
    char *buf;
    int len;
    int x;
    int y;
    int w;
    int h;
    int LP;
};

#define IOC_MAGIC 'k'
#define COPYRD _IOW(IOC_MAGIC, 1 , struct ioctl_cmd)
#define GETED _IOR(IOC_MAGIC, 2 , struct ioctl_cmd)
#define CPGET _IOWR(IOC_MAGIC,3 , struct ioctl_cmd)
#define SETFB _IOW(IOC_MAGIC, 4 , struct ioctl_cmd)
#define GETFB _IOR(IOC_MAGIC, 5 , struct ioctl_cmd)
#define SETLP _IOW(IOC_MAGIC, 6 , struct ioctl_cmd)
#define PDGET _IOW(IOC_MAGIC, 7 , struct ioctl_cmd)
#define IOC_MAXNR 7

struct vcd_info {
    unsigned int vcd_fb;
    unsigned int pixelClock;
    unsigned int line_pitch;
    int hdisp;
    int hfrontporch;
    int hsync;
    int hbackporch;
    int vdisp;
    int vfrontporch;
    int vsync;
    int vbackporch;
    int refresh_rate;
    int hpositive;
    int vpositive;
    int bpp;
    int r_max;
    int g_max;
    int b_max;
    int r_shift;
    int g_shift;
    int b_shift;
};

struct vcd_diff {
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
};

struct nu_rfb {
    struct vcd_info vcd_info;
    struct vcd_diff *diff_table;
    char *fake_fb;
    char *raw_fb;
    char *raw_fb_addr;
    char *raw_ece_addr;
    int raw_fb_offset;
    int raw_fb_shm;
    int raw_fb_mmap;
    int raw_ece_mmap;
    int raw_fb_seek;
    int raw_fb_fd;
    int hextile_fd;
    unsigned int vcd_fb;
    unsigned int line_pitch;
    unsigned int diff_cnt;
    int res_changed;
    int is_hid_init;
    int last_mode;
    int cl_cnt;
};


#define VCD_IOC_MAGIC     'v'
#define VCD_IOCGETINFO  _IOR(VCD_IOC_MAGIC,  1, struct vcd_info)
#define VCD_IOCSENDCMD  _IOW(VCD_IOC_MAGIC,  2, int)
#define VCD_IOCCHKRES   _IOR(VCD_IOC_MAGIC,  3, int)
#define VCD_IOCGETDIFF  _IOR(VCD_IOC_MAGIC,  4, struct vcd_diff)
#define VCD_IOCDIFFCNT  _IOR(VCD_IOC_MAGIC,  5, int)
#define VCD_IOCCHKDRES	_IOW(VCD_IOC_MAGIC,  6, int)
#define VCD_IOC_MAXNR     6

int get_update(rfbClientRec *cl);
char *get_fb_adr(rfbClientRec *cl);
int get_diff_cnt(rfbClientRec *cl);
int get_diff_table(rfbClientRec *cl, struct vcd_diff *diff, int i);
int set_vcd_cmd(struct nu_rfb *nurfb, int cmd);
int chk_vcd_res(rfbClientRec *cl);
int init_vcd_res(struct nu_rfb *nurfb);
struct nu_rfb *init_nurfb(void);
int get_vcd_info(struct nu_rfb *nurfb, struct vcd_info *info);
int init_vcd(struct nu_rfb *nurfb);
void clear_nurfb(struct nu_rfb *nurfb);
int hid_init(void);
void hid_close(void);
void keyboard(rfbBool down, rfbKeySym keysym, rfbClientPtr client);
void pointer_event(int mask, int x, int y, rfbClientPtr client);
int auth_pam_ldap(char *username, char *passwd, char *ip);
#endif
