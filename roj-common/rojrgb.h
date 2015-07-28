#ifndef _GST_ROJRGB_H_
#define _GST_ROJRGB_H_

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PALETTE_NUMBER 7

void groj_palette_gray(double, unsigned char *, char);
void groj_palette_igray(double, unsigned char *, char);
void groj_palette_warm(double, unsigned char *, char);
void groj_palette_phase(double, unsigned char *, char);
void groj_palette_abc(double, unsigned char *, char);
void groj_palette_rgb(double, unsigned char *, char);
void groj_palette_old(double, unsigned char *, char);

#endif
