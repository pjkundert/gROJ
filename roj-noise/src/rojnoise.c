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


#include "rojnoise.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojnoise_debug);
#define GST_CAT_DEFAULT gst_rojnoise_debug

enum
{
  NOISE_WHITE
};

enum
{
  PROP_NULL,
  PROP_SNR,
  PROP_TYPE,
  PROP_BLOCKS
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

#define gst_rojnoise_parent_class parent_class
G_DEFINE_TYPE (Gstrojnoise, gst_rojnoise, GST_TYPE_ELEMENT);

static void gst_rojnoise_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojnoise_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojnoise_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojnoise_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojnoise's class */
static void
gst_rojnoise_class_init (GstrojnoiseClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojnoise_set_property;
  gobject_class->get_property = gst_rojnoise_get_property;

  g_object_class_install_property (gobject_class, PROP_TYPE, 
	g_param_spec_string ("type", "noise type", "noise type", 
		"white", G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SNR, 
	g_param_spec_double ("snr", "signal-to-noise ratio", "signal-to-noise ratio", 
		-G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BLOCKS, 
	g_param_spec_int ("blocks", "blocks of signal", "blocks of signal",
		1, 16*4096, 1, G_PARAM_READWRITE));


  gst_element_class_set_details_simple(gstelement_class,
      "rojnoise", "GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojnoise_init (Gstrojnoise * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojnoise_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojnoise_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->type = NOISE_WHITE;
  filter->blocks = 1;
  filter->count = 0;
  filter->snr = 0.0;

  filter->energies = malloc(filter->blocks*sizeof(double));
  if(filter->energies==NULL)
    groj_error(gst_element_get_name(filter), "mem");
  filter->energies[0] = -1;  
}

static void
gst_rojnoise_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojnoise *filter = GST_ROJNOISE (object);

  switch (prop_id) {
    case PROP_SNR:
      filter->snr = g_value_get_double (value);
      break;

  case PROP_BLOCKS:
    {
      filter->blocks = g_value_get_int (value);

      if (filter->energies!=NULL )
	free(filter->energies);
    
      filter->energies = malloc(filter->blocks*sizeof(double));
      if(filter->energies==NULL)
	groj_error(gst_element_get_name(object), "mem");

      int n;
      for(n=0; n<filter->blocks; n++)
	filter->energies[n] = -1.0;

      break;
    }  
  
  case PROP_TYPE:    
    if (!strcmp(g_value_get_string (value), "white")){
      filter->type = NOISE_WHITE;
      break;
    }
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    groj_error(gst_element_get_name(object), "not known noise");

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
gst_rojnoise_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojnoise *filter = GST_ROJNOISE (object);

  switch (prop_id) {
  case PROP_SNR:
    g_value_set_double (value, filter->snr);
    break;
    
  case PROP_BLOCKS:
    g_value_set_double (value, filter->blocks);
    break;
    
  case PROP_TYPE:      
    switch(filter->type){
    case NOISE_WHITE:
      g_value_set_string (value, "white");
      break;
    default:
      groj_error(gst_element_get_name(object), "not known noise");
    }
    break;
    
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojnoise_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{

  Gstrojnoise *filter = GST_ROJNOISE (parent);
  g_print ("* %s: event %s\n",
	   gst_element_get_name(filter), gst_event_type_get_name(GST_EVENT_TYPE (event))); 


  gboolean ret;
  switch (GST_EVENT_TYPE (event)) {
  case GST_EVENT_CAPS:
    {
      GstEvent *ovent = groj_caps_event(parent, event, GROJ_CAPS_SIGNAL, GROJ_CAPS_SIGNAL);
      ret = gst_pad_event_default (pad, parent, ovent);      
      break;
    }
        
  default:
    ret = gst_pad_event_default (pad, parent, event);
  }

  return ret;
}




/* CHAIN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
static GstFlowReturn
gst_rojnoise_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojnoise *filter = GST_ROJNOISE (parent);

  GstMapInfo info;
  GstMemory *mem = gst_buffer_get_memory (buffer, 1);
  gst_memory_map (mem, &info, GST_MAP_READWRITE);
  int n, size = info.size/sizeof(fftw_complex);

  double energy = 0.0;
  for(n=0; n<size; n++){
    gdouble re = creal(((fftw_complex *)info.data)[n]);
    gdouble im = cimag(((fftw_complex *)info.data)[n]);
    energy += pow(re, 2.0) + pow(im, 2.0);
  }
  energy /= (double)size;
  filter->count++;
  filter->count%=filter->blocks;
  filter->energies[filter->count] = energy;

  energy = 0.0;
  int count = 0;
  for(n=0; n<filter->blocks; n++){
    if (filter->energies[n]>=0){
      energy += filter->energies[n];
      count++;
    }
  }
  energy /= count; 
  

  double sigma = energy * pow(10.0, -filter->snr/10.0);
  sigma = sqrt(sigma/2.0);

  for(n=0; n<size; n++){
    gdouble re = creal(((fftw_complex *)info.data)[n]) + groj_gauss(sigma);
    gdouble im = cimag(((fftw_complex *)info.data)[n]) + groj_gauss(sigma);
    ((fftw_complex *)info.data)[n] = re + im*I;
  }

  gst_memory_unmap (mem, &info);    
  return gst_pad_push (filter->srcpad, buffer);
}

/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojnoise_init (GstPlugin * rojnoise)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojnoise' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojnoise_debug, "rojnoise", 0, "rojnoise");
  return gst_element_register (rojnoise, "rojnoise", GST_RANK_NONE, GST_TYPE_ROJNOISE);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojnoise"
#endif

/* gstreamer looks for this structure to register rojnoises
 * exchange the string 'Template rojnoise' with your rojnoise description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojnoise,
    "rojnoise",
    rojnoise_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
