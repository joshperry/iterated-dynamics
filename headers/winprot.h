#ifndef WINPROT_H
#define WINPROT_H

/* This file contains prototypes for win specific functions. */

/*  calmanp5 -- assembler file prototypes */

extern long calcmandfpasm_p5(void);
extern void calcmandfpasmstart_p5(void);

/* added for Win32 port */
extern void scroll_center(int, int);
extern void scroll_relative(int, int);

extern int strncasecmp(const char *, const char *, int);

#endif
