#ifndef _GST_ROJVAR_H_
#define _GST_ROJVAR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <complex.h>
#include <fftw3.h>

#include "rojtypes.h"

void 
groj_error(char *, char *);


GstMemory* 
groj_config_new (void);

void
groj_config_set_time(GstMemory *, double);

double 
groj_config_get_time(GstMemory *);

void
groj_config_set_number(GstMemory *, unsigned long);

unsigned long 
groj_config_get_number(GstMemory *);

void 
groj_init_tags(struct RojTags);

gboolean 
groj_tag_catch_int(GstTagList *, char *, char *, struct RojInt *);
gboolean 
groj_tag_catch_double(GstTagList *, char *, char *, struct RojDouble *);

gboolean 
groj_tag_emit_int(char *, GstPad *, char *, int);
gboolean 
groj_tag_emit_double(char *, GstPad *, char *, double);

double 
groj_ceil(double, double, double);
float 
groj_gauss(double); 

GstEvent* 
groj_caps_event(GstObject *, GstEvent *, gchar *, gchar *);

#endif
