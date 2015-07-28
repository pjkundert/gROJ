#include "rojrgb.h"

void groj_palette_gray(double nval, unsigned char *rgb, char diffout){

  if (nval<0.0 || nval>1.0){
    if (diffout){
      rgb[0] = 0;
      rgb[1] = 255;
      rgb[2] = 0; 
      return;
    }
    else{
      if (nval<0.0) nval = 0.0;
      else nval = 1.0;
    }
  }

  unsigned char c = floor(255.99*nval);
  rgb[0] = c;
  rgb[1] = c;
  rgb[2] = c; 
}


void groj_palette_igray(double nval, unsigned char *rgb, char diffout){

  if (nval<0.0 || nval>1.0){
    if (diffout){
      rgb[0] = 0;
      rgb[1] = 255;
      rgb[2] = 0; 
      return;
    }
    else{
      if (nval<0.0) nval = 0.0;
      else nval = 1.0;
    }
  }

  unsigned char c = floor(255.99*(1.0-nval));
  rgb[0] = c;
  rgb[1] = c;
  rgb[2] = c; 
}



void groj_palette_warm(double nval, unsigned char *rgb, char diffout){

  if (nval<0.0 || nval>1.0){
    if (diffout){
      rgb[0] = 0;
      rgb[1] = 255;
      rgb[2] = 0; 
      return;
    }
    else{
      if (nval<0.0) nval = 0.0;
      else nval = 1.0;
    }
  }

  if (nval<0.2){
    double v = 5*nval; 
    rgb[0] = 0;
    rgb[1] = 0;
    rgb[2] = floor(255.99*v);
    return;
  }
  if (nval>=0.2 && nval<0.4){
    double v = 5*(nval-0.2);
    rgb[0] = floor(255.99*v);
    rgb[1] = 0;
    rgb[2] = 255;
    return;
  }
  if (nval>=0.4 && nval<0.6){
    double v = 5*(nval-0.4);
    rgb[0] = 255;
    rgb[1] = 0;
    rgb[2] = 255-floor(255.99*v);
    return;
  }
  if (nval>=0.6 && nval<0.8){
    double v = 5*(nval-0.6);
    rgb[0] = 255;
    rgb[1] = floor(255.99*v);
    rgb[2] = 0;
    return;
  }
  double v = 5*(nval-0.8);
  rgb[0] = 255;
  rgb[1] = 255;
  rgb[2] = floor(255.99*v); 
}


void groj_palette_phase(double nval, unsigned char *rgb, char diffout){

  if (nval<0.0 || nval>1.0){
    if (diffout){
      rgb[0] = 0;
      rgb[1] = 255;
      rgb[2] = 0; 
      return;
    }
    else{
      if (nval<0.0) nval = 0.0;
      else nval = 1.0;
    }
  }

  if (nval<0.25){
    double v = 4*nval;
    rgb[0] = floor(255.99*v);
    rgb[1] = 0;
    rgb[2] = 0;
    return;
  }
  if (nval>=0.25 && nval<0.5){
    double v = 4*(nval-0.25);
    rgb[0] = 255;
    rgb[1] = floor(255.99*v);
    rgb[2] = floor(255.99*v);
    return;
  }
  if (nval>=0.5 && nval<0.75){
    double v = 4*(nval-0.5);
    rgb[0] = 255-floor(255.99*v);
    rgb[1] = 255-floor(255.99*v);
    rgb[2] = 255;
    return;
  }
  double v = 4*(nval-0.75);
  rgb[0] = 0;
  rgb[1] = 0;
  rgb[2] = 255-floor(255.99*v);
}


void groj_palette_abc(double nval, unsigned char *rgb, char diffout){
  
  if (nval<0.0 || nval>1.0){
    if (diffout){
      rgb[0] = 128;
      rgb[1] = 128;
      rgb[2] = 128; 
      return;
    }
    else{
      if (nval<0.0) nval = 0.0;
      else nval = 1.0;
    }
  }

  if (nval<0.2){
    double v = 2.5*nval;
    rgb[0] = 255-floor(255.99*v);
    rgb[1] = 255;
    rgb[2] = 255-floor(255.99*v);
    return;
  }
 if (nval<0.4){
   double v = 2.5*(nval-0.2);
   rgb[0] = 128+floor(255.99*v);
   rgb[1] = 255;
   rgb[2] = 128;
   return;
 }
 if (nval<0.6){
   double v = 2.5*(nval-0.4);
   rgb[0] = 255;
   rgb[1] = 255-floor(255.99*v);
   rgb[2] = 128-floor(255.99*v);
   return;
 }
if (nval<0.7){
  double v = 5*(nval-0.6);
  rgb[0] = 255;
  rgb[1] = 128-floor(255.99*v);
  rgb[2] = 0;
  return;
}
if (nval<0.8){
  double v = 5*(nval-0.7);
  rgb[0] = 255-floor(255.99*v);
  rgb[1] = 0;
  rgb[2] = floor(0.5*255.99*v);
  return;
}
if (nval<0.9){
  double v = 2.5*(nval-0.8);
  rgb[0] = 128-floor(2*255.99*v);
  rgb[1] = 0;
  rgb[2] = 64+floor(255.99*v);
  return;
}
    
double v = 5*(nval-0.9);
rgb[0] = 0;
rgb[1] = 0;
rgb[2] = 128-floor(255.99*v);
}

void groj_palette_rgb(double nval, unsigned char *rgb, char diffout){
  
  if (nval<0.0 || nval>1.0){
    if (diffout){
      rgb[0] = 255;
      rgb[1] = 255;
      rgb[2] = 128; 
      return;
    }
    else{
      if (nval<0.0) nval = 0.0;
      else nval = 1.0;
    }
  }

  if (nval<0.25){
    double v = 4*nval;
    rgb[0] = 0;
    rgb[1] = 0;
    rgb[2] = floor(255.99*v);
    return;
  }
  if (nval<0.5){
    double v = 4*(nval-0.25);
    rgb[0] = 0;
    rgb[1] = floor(255.99*v);
    rgb[2] = 255-floor(255.99*v);
    return;
  }
  if (nval<0.75){
    double v = 4*(nval-0.5);
    rgb[0] = floor(255.99*v);
    rgb[1] = 255-floor(255.99*v);
    rgb[2] = 0;
    return;
  }
    double v = 4*(nval-0.75);
    rgb[0] = 255;
    rgb[1] = floor(255.99*v);
    rgb[2] = floor(255.99*v);
}

void groj_palette_old(double nval, unsigned char *rgb, char diffout){
  
  if (nval<0.0 || nval>1.0){
    if (diffout){
      rgb[0] = 0;
      rgb[1] = 0;
      rgb[2] = 0; 
      return;
    }
    else{
      if (nval<0.0) nval = 0.0;
      else nval = 1.0;
    }
  }

  if (nval<0.25){
    double v = 4*nval;
    rgb[0] = 0;
    rgb[1] = floor(255.99*v);
    rgb[2] = 255-floor(255.99*v);
    return;
  }
  if (nval<0.5){
    double v = 4*(nval-0.25);
    rgb[0] = floor(255.99*v);
    rgb[1] = 255;
    rgb[2] = floor(255.99*v);
    return;
  }
  if (nval<0.75){
    double v = 4*(nval-0.5);
    rgb[0] = 255;
    rgb[1] = 255;
    rgb[2] = 255-floor(255.99*v);
    return;
  }
    double v = 4*(nval-0.75);
    rgb[0] = 255;
    rgb[1] = 255-floor(255.99*v);
    rgb[2] = 0;
}
