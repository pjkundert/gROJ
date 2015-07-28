#ifndef _GST_ROJFFT_H_
#define _GST_ROJFFT_H_

#include <stdlib.h>
#include <string.h>

#include <complex.h>
#include <fftw3.h>
#include <math.h>

#include "rojvar.h"

void groj_fftshift (fftw_complex *, int); 
void groj_fft(fftw_complex *, double *, fftw_complex *, fftw_complex *, int, int , int, int);

enum
{
  WINDOW_RECTANGULAR,
  WINDOW_BLACKMAN_HARRIS
};

 double* groj_create_window (int, int);


#endif
