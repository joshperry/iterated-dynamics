#ifndef WINPROT_H
#define WINPROT_H

/* This file contains prototypes for win specific functions. */

/*  calmanp5 -- assembler file prototypes */

extern long calcmandfpasm_p5(void);
extern void calcmandfpasmstart_p5(void);

/*  dialog -- C file prototypes */

extern void PrintFile(void);
extern int Win_OpenFile(unsigned char *);
extern void Win_SaveFile(unsigned char *);

/*  prompts1 -- C file prototypes */

extern void set_default_parms(void);

/*  windos -- C file prototypes */

extern void debugmessage(char *, char *);
extern int  farread(int, VOIDPTR, unsigned);
extern int  farwrite(int, VOIDPTR, unsigned);
extern void far_memcpy(void *, void *, int);
extern void far_memset(void *, int , int);
extern int getcolor(int, int);
extern int out_line(BYTE *, int);
extern void putcolor_a(int, int, int);
extern void buzzer(int);
extern void CalibrateDelay(void);
extern void start_wait(void);
extern void end_wait(void);

extern int get_video_mode(struct fractal_info *,struct ext_blk_3 *);
extern int check_vidmode_keyname(char *);
extern void vidmode_keyname(int, char *);
extern int check_vidmode_key(int, int);
extern void restoredac(void);
extern void reset_zoom_corners(void);
extern void flush_screen(void);

extern int win_load(void);
extern void win_save(void);
extern void win_cycle(void);

extern void winfract_help(void);

/*  windos2 -- C file prototypes */

extern void putstring(int, int, int, unsigned char *);
extern int  input_field(int, int, char *, int, int, int, int (*)(int));
extern void helptitle(void);
extern void stackscreen(void);
extern void unstackscreen(void);
extern void discardscreen(void);
extern void discardgraphics(void);
extern int load_palette(void);
extern void save_palette(void);
extern void fractint_help(void);
extern int win_make_batch_file(void);
extern int fractint_getkeypress(int);
extern int savegraphics(void);
extern int restoregraphics(void);

extern int main_menu(int);

/*  winfract -- C file prototypes */

extern void win_set_title_text(void);
extern void win_savedac(void);

/*  winstubs -- C file prototypes */

extern void rotate(int);
extern void cleartempmsg(void);
extern void freetempmsg(void);
extern int FromMemDisk(long, int, void *);
extern int ToMemDisk(long, int, void *);
extern int common_startdisk(long, long, int);
extern long normalize(char *);
extern void drawbox(int);

extern void farmessage(unsigned char *);
extern void setvideomode(int, int, int, int);
extern int fromvideotable(void);
extern void home(void);

extern int intro_overlay(void);
extern int rotate_overlay(void);
extern int printer_overlay(void);
extern int pot_startdisk(void);
//extern void SetTgaColors(void);
extern int startdisk(void);
extern void enddisk(void);
extern int targa_startdisk(FILE *,int);
extern void targa_writedisk(unsigned int,unsigned int,BYTE,BYTE,BYTE);
extern void targa_readdisk(unsigned int,unsigned int,BYTE *,BYTE *,BYTE *);
extern int SetColorPaletteName(char *);
extern BYTE *findfont(int);
extern void EndTGA(void);

extern int key_count(int);

extern void dispbox(void);
extern void clearbox(void);
extern void addbox(struct coords);
extern void drawlines(struct coords, struct coords, int, int);
extern int showvidlength(void);

extern int soundon(int);
extern void soundoff(void);
extern int initfm(void);
extern void mute(void);

extern int tovideotable(void);
extern void TranspPerPixel(void);
extern void stopslideshow(void);
extern void aspectratio_crop(float, float);
extern void setvideotext(void);

/* added for Win32 port */
extern void gettruecolor(int, int, int*, int*, int*);
extern void puttruecolor(int, int, int, int, int);
extern void scroll_center(int, int);
extern void scroll_relative(int, int);
extern void scroll_state(int);
extern void delay(int);

#endif
