/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2015 Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include "rojreass.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojreass_debug);
#define GST_CAT_DEFAULT gst_rojreass_debug


enum
{
  PROP_NULL,
  PROP_LENGTH,
  PROP_WINDOW,
  PROP_OFREQ,
  PROP_EFREQ
};

/* the capabilities of the inputs and outputs.
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

#define gst_rojreass_parent_class parent_class
G_DEFINE_TYPE (Gstrojreass, gst_rojreass, GST_TYPE_ELEMENT);

static void gst_rojreass_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojreass_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojreass_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojreass_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojreass's class */
static void
gst_rojreass_class_init (GstrojreassClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojreass_set_property;
  gobject_class->get_property = gst_rojreass_get_property;


  g_object_class_install_property (gobject_class, PROP_WINDOW, 
      g_param_spec_string ("window", "window type", "set window type.", 
	  "blackman-harris", G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LENGTH,
      g_param_spec_int ("length", "length in samples", "length in samples",
          1, G_MAXINT, 1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_OFREQ, 
				   g_param_spec_double ("ofreq", "set ofreq", "set ofreq", 
   	        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_EFREQ, 
	g_param_spec_double ("efreq", "set efreq", "set efreq", 
   	        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class, "rojreass",
    "GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));

  gst_tag_register ("ofreq", GST_TAG_FLAG_META, G_TYPE_DOUBLE, "ofreq", "ofreq in hz", NULL);
  gst_tag_register ("length", GST_TAG_FLAG_META, G_TYPE_INT, "length", "length in sa", NULL);
  gst_tag_register ("leap", GST_TAG_FLAG_META, G_TYPE_DOUBLE, "leap", "leap in hz", NULL);
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojreass_init (Gstrojreass * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojreass_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojreass_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);


  filter->window = WINDOW_BLACKMAN_HARRIS;  
  filter->winbuf = NULL;
  filter->length = 0;

  filter->inbuf = NULL;
  filter->outbuf = NULL;
  filter->outprev = NULL;
  filter->outnext = NULL;
  filter->outpit = NULL;
  filter->outtop = NULL;

  filter->dfreq = 0.0001;

  filter->ofreq.flag = FALSE;
  filter->efreq.flag = FALSE;

  groj_init_tags(filter->tags);
}

void
groj_rojreass_allocate_memory(Gstrojreass *filter)
{      
  if (filter->winbuf!=NULL) free(filter->winbuf);
  if (filter->inbuf!=NULL) free(filter->inbuf);
  if (filter->outbuf!=NULL) free(filter->outbuf);
    
  filter->winbuf = groj_create_window (filter->window, filter->tags.width.val);
  if(filter->winbuf==NULL) groj_error(gst_element_get_name(filter), "need window allocate");

  filter->inbuf = malloc(filter->length*sizeof(fftw_complex));
  if (filter->inbuf==NULL) groj_error(gst_element_get_name(filter), "canot allocate memory");

  filter->outbuf = malloc(filter->length*sizeof(fftw_complex));
  if(filter->outbuf==NULL) groj_error(gst_element_get_name(filter), "canot allocate memory");

  if(filter->outprev!=NULL) free(filter->outprev);
  if(filter->outnext!=NULL) free(filter->outnext);
  if(filter->outtop!=NULL) free(filter->outtop);
  if(filter->outpit!=NULL) free(filter->outpit);

  filter->outprev = malloc(filter->length*sizeof(fftw_complex));
  if(filter->outprev==NULL) groj_error(gst_element_get_name(filter), "canot allocate memory");

  filter->outnext = malloc(filter->length*sizeof(fftw_complex));
  if(filter->outnext==NULL) groj_error(gst_element_get_name(filter), "canot allocate memory");

  filter->outtop = malloc(filter->length*sizeof(fftw_complex));
  if(filter->outtop==NULL) groj_error(gst_element_get_name(filter), "canot allocate memory");

  filter->outpit = malloc(filter->length*sizeof(fftw_complex));
  if(filter->outpit==NULL) groj_error(gst_element_get_name(filter), "canot allocate memory");
}

static void
gst_rojreass_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojreass *filter = GST_ROJREASS (object);

  switch (prop_id) {
  case PROP_LENGTH:
    filter->length = g_value_get_int (value);

    groj_tag_emit_int(gst_element_get_name(filter), filter->srcpad, "length", filter->length);
    if (filter->tags.rate.flag)
      groj_tag_emit_double(gst_element_get_name(filter), filter->srcpad, "leap", filter->tags.rate.val / filter->length);	
    groj_rojreass_allocate_memory(filter);
    break;
    
  case PROP_OFREQ:
    filter->ofreq.val = g_value_get_double (value);
    filter->ofreq.flag = TRUE;
    
    groj_tag_emit_double(gst_element_get_name(filter), filter->srcpad, "ofreq", filter->ofreq.val);    
    break;
  case PROP_EFREQ:
    filter->efreq.val = g_value_get_double (value);
    filter->efreq.flag = TRUE;
    break;

  case PROP_WINDOW:
    if (!strcmp(g_value_get_string (value), "rectangular")){
      filter->window = WINDOW_RECTANGULAR;
      break;
    }
    if (!strcmp(g_value_get_string (value), "blackman-harris")){
      filter->window = WINDOW_BLACKMAN_HARRIS;
      break;
    }
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    groj_error(gst_element_get_name(object), "not known window");

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
gst_rojreass_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojreass *filter = GST_ROJREASS (object);

  switch (prop_id) {
    case PROP_LENGTH:
      g_value_set_int (value, filter->length);
      break;

  case PROP_OFREQ:
    g_value_set_double (value, filter->ofreq.val);
    break;
  case PROP_EFREQ:
    g_value_set_double (value, filter->efreq.val);
    break;
    
  case PROP_WINDOW:
    switch(filter->window){
    
    case WINDOW_RECTANGULAR:
      g_value_set_string (value, "rectangular");
      break;
    
    case WINDOW_BLACKMAN_HARRIS:
      g_value_set_string (value, "blackman-harris");
      break;      

    default:
      groj_error(gst_element_get_name(object), "not known window");
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}



/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojreass_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojreass *filter = GST_ROJREASS (parent);
  g_print ("* %s: event %s\n",
	   gst_element_get_name(parent), gst_event_type_get_name(GST_EVENT_TYPE (event))); 

  gboolean ret;
  switch (GST_EVENT_TYPE (event)) {
  case GST_EVENT_CAPS:
    {
      GstEvent *ovent = groj_caps_event(parent, event, GROJ_CAPS_FRAME, GROJ_CAPS_TFR);
      ret = gst_pad_event_default (pad, parent, ovent);      
      break;
    }
    
  case GST_EVENT_TAG:
    {
      ret = gst_pad_event_default (pad, parent, event);

      GstTagList *taglist;
      gst_event_parse_tag(event, &taglist);

      if (groj_tag_catch_double(taglist, "rate", gst_element_get_name(parent), &filter->tags.rate)){
	if (filter->ofreq.flag)	
	  filter->ofreq.val = groj_ceil(-filter->tags.rate.val/2, filter->tags.rate.val/filter->length, filter->ofreq.val);
	
	if(filter->ofreq.flag && filter->ofreq.val>=filter->tags.rate.val/2)
	  groj_error(gst_element_get_name(parent), "bad ofreq");	
	if(filter->efreq.flag && filter->efreq.val<=-filter->tags.rate.val/2)
	  groj_error(gst_element_get_name(parent), "bad oereq");
	
	if (filter->ofreq.val<-filter->tags.rate.val/2 || !filter->ofreq.flag)
	  filter->ofreq.val=-filter->tags.rate.val/2;
	
	if (filter->efreq.val>filter->tags.rate.val/2 || !filter->efreq.flag)
	  filter->efreq.val=filter->tags.rate.val/2;

	groj_tag_emit_int(gst_element_get_name(filter), filter->srcpad, "length", filter->length);
	groj_tag_emit_double(gst_element_get_name(filter), filter->srcpad, "ofreq", filter->ofreq.val);
	groj_tag_emit_double(gst_element_get_name(filter), filter->srcpad, "leap", filter->tags.rate.val / filter->length);	
      }

      if (groj_tag_catch_int(taglist, "width", gst_element_get_name(parent), &filter->tags.width)){
	if (filter->length<filter->tags.width.val)
	  filter->length = filter->tags.width.val;
	
	groj_rojreass_allocate_memory(filter);
      }
      
      break;
    }

  case GST_EVENT_SEGMENT:
    {
      ret = gst_pad_event_default (pad, parent, event);
      if (!filter->tags.width.flag || !filter->tags.rate.flag)
	groj_error(gst_element_get_name(parent), "need tags (width, rate)");
      break;
    }

  default:
    ret = gst_pad_event_default (pad, parent, event);
  }
  return ret;
}

/* chain function this function does the actual processing */
static GstFlowReturn
gst_rojreass_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojreass *filter = GST_ROJREASS (parent);

  if(filter->ofreq.flag && filter->efreq.flag && filter->efreq.val<=filter->ofreq.val)
    groj_error(gst_element_get_name(parent), "bad ofreq or efreq");

  GstMapInfo info;
  GstMemory *cmem = gst_buffer_get_memory (buffer, 0);
  GstMemory *mem = gst_buffer_get_memory (buffer, 1);
  gst_memory_map (mem, &info, GST_MAP_READ);

  groj_fft(&((fftw_complex*)info.data)[0], filter->winbuf, filter->inbuf, filter->outprev, filter->tags.width.val, filter->length, 0, 1);
  groj_fft(&((fftw_complex*)info.data)[1], filter->winbuf, filter->inbuf, filter->outbuf, filter->tags.width.val, filter->length, 0, 1);
  groj_fft(&((fftw_complex*)info.data)[2], filter->winbuf, filter->inbuf, filter->outnext, filter->tags.width.val, filter->length, 0, 1);

  int initial = filter->length * (filter->ofreq.val + filter->tags.rate.val/2) / filter->tags.rate.val;
  int n, coeffs = filter->length * (filter->efreq.val - filter->ofreq.val) / filter->tags.rate.val;

  double dtf = filter->dfreq/filter->tags.rate.val;

  memcpy(filter->inbuf,  &((fftw_complex*)info.data)[1], filter->tags.width.val*sizeof(fftw_complex));
  for(n=0;n<filter->tags.width.val;n++)
    filter->inbuf[n] = filter->inbuf[n] * cexp(I*dtf*n);
  groj_fft(NULL, filter->winbuf, filter->inbuf, filter->outpit, filter->tags.width.val, filter->length, 0, 1);

  memcpy(filter->inbuf,  &((fftw_complex*)info.data)[1], filter->tags.width.val*sizeof(fftw_complex));
  for(n=0;n<filter->tags.width.val;n++)
    filter->inbuf[n] = filter->inbuf[n] * cexp(-I*dtf*n);
  groj_fft(NULL, filter->winbuf, filter->inbuf, filter->outtop, filter->tags.width.val, filter->length, 0, 1);

  GstBuffer * outbuffer = gst_buffer_new ();
  GstMemory * memenergy = gst_allocator_alloc (NULL, (coeffs)*sizeof(double), NULL);
  GstMemory * memcif = gst_allocator_alloc (NULL, (coeffs)*sizeof(double), NULL);
  GstMemory * memlgd = gst_allocator_alloc (NULL, (coeffs)*sizeof(double), NULL);
  GstMapInfo outenergy, outcif, outlgd;
  gst_memory_map (memenergy, &outenergy, GST_MAP_READWRITE);
  gst_memory_map (memcif, &outcif, GST_MAP_READWRITE);
  gst_memory_map (memlgd, &outlgd, GST_MAP_READWRITE);

  double c1, c2, d1, d2;
  double dcorr = (double)filter->tags.width.val/(2.0*filter->tags.rate.val);

  int k;
  for (k=0; k<coeffs; k++){
    ((double *)outenergy.data)[k] = pow(creal(filter->outbuf[initial+k]), 2.0) + pow(cimag(filter->outbuf[initial+k]), 2.0);

    d1 = carg(filter->outtop[initial+k]*conj(filter->outbuf[initial+k]));	      
    d2 = carg(filter->outbuf[initial+k]*conj(filter->outpit[initial+k]));
    ((double *)outlgd.data)[k] = (d1+d2) / (2.0 * filter->dfreq) +dcorr;    

    c1 = carg(filter->outbuf[initial+k]*conj(filter->outprev[initial+k]));
    c2 = carg(filter->outnext[initial+k]*conj(filter->outbuf[initial+k]));
    ((double *)outcif.data)[k] = (c1+c2) * filter->tags.rate.val / (4.0 * M_PI);
  }

  gst_memory_unmap (memenergy, &outenergy);
  gst_memory_unmap (memcif, &outcif);
  gst_memory_unmap (memlgd, &outlgd);

  gst_buffer_append_memory (outbuffer, cmem);
  gst_buffer_append_memory (outbuffer, memenergy);
  gst_buffer_append_memory (outbuffer, memlgd);
  gst_buffer_append_memory (outbuffer, memcif);

  gst_memory_unmap (mem, &info);
  gst_buffer_unref (buffer);

  return gst_pad_push (filter->srcpad, outbuffer);
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojreass_init (GstPlugin * rojreass)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojreass' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojreass_debug, "rojreass", 0, "Template rojreass");
  return gst_element_register (rojreass, "rojreass", GST_RANK_NONE, GST_TYPE_ROJREASS);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojreass"
#endif

/* gstreamer looks for this structure to register rojreasss
 * exchange the string 'Template rojreass' with your rojreass description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojreass,
    "rojreass",
    rojreass_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
