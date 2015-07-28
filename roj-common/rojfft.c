#include "rojfft.h"

void 
groj_fftshift (fftw_complex *data, int size) 
{
  fftw_complex *buf = malloc(sizeof(fftw_complex)*(size/2+1));
  memset(buf,0,sizeof(fftw_complex)*(size/2+1));

  if(size%2==0){
    memcpy(buf,data,sizeof(fftw_complex)*size/2);
    memcpy(data,&data[size/2],sizeof(fftw_complex)*size/2);
    memcpy(&data[size/2],buf,sizeof(fftw_complex)*size/2);
  }
  else{
    memcpy(buf,&data[size/2],sizeof(fftw_complex)*(size/2+1));
    memcpy(&data[size/2+1],data,sizeof(fftw_complex)*(size/2));
    memcpy(data,buf,sizeof(fftw_complex)*(size/2+1));
  }

  free(buf);
} 

void 
groj_fft(fftw_complex *signal, double *window, fftw_complex *inbuf, fftw_complex *outbuf, int width, int length, int fftshift_before, int fftshift_after)
{
  memset(outbuf, 0, length*sizeof(fftw_complex));
  if (signal!=NULL){
    memset(inbuf, 0, length*sizeof(fftw_complex));
    memcpy(inbuf, signal, width*sizeof(fftw_complex)); /* Nie symetryczny zeropadding - należy to skompensować!!!*/
  }
  else
    memset(&inbuf[width], 0, (length-width)*sizeof(fftw_complex));

  if (window != NULL){
    int n;
    for(n=0; n<width; n++)
      inbuf[n] *= window[n];
  }

  if(fftshift_before){
    /* TO DO
            end\............../init
    */
  }
  
  fftw_plan plan = fftw_plan_dft_1d(length, inbuf, outbuf, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(plan);
  fftw_destroy_plan(plan);

  if(fftshift_after) 
    groj_fftshift(outbuf, length);

  int k;
  for(k=0; k<length; k++)
    outbuf[k] /= length;
}


double *
groj_create_window (int window, int width)
{

  double *winbuf = malloc(width*sizeof(double));
  if (winbuf==NULL)
    groj_error("stft / reass", "canot allocate memory");
  

  switch(window){	
  case WINDOW_RECTANGULAR:
    {
      int n;
      for (n=0;n<width;n++)
	winbuf[n] = 1.0/width;
      return winbuf;
    }
	
  case WINDOW_BLACKMAN_HARRIS:
    {
      double a0 = 0.3232153788877343;
      double a1 = -0.4714921439576260;
      double a2 = 0.175534129901972;
      double a3 = -0.02849699010614994;
      double a4 = 0.001261357088292677;
      
      int n;
      double sum = 0.0;
      for(n=0; n<width; n++){
	winbuf[n] = a0
	  +a1*cos(2.0*M_PI*(double)n/(double)(width-1))
	  +a2*cos(4.0*M_PI*(double)n/(double)(width-1))
	  +a3*cos(6.0*M_PI*(double)n/(double)(width-1))
	  +a4*cos(8.0*M_PI*(double)n/(double)(width-1));  
	sum += winbuf[n]>0 ? -winbuf[n] : winbuf[n];
      }    
      
      for (n=0; n<width; n++)
	winbuf[n] /= sum;
      return winbuf;
    }

  default:
    groj_error("stft / reass", "not known window");
  }  

  return NULL;
}

