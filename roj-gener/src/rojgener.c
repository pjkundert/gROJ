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

#include "rojgener.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojgener_debug);
#define GST_CAT_DEFAULT gst_rojgener_debug

enum
{
  SIGNAL_TIME,
  SIGNAL_SINUS,
  SIGNAL_XSINUS,
  SIGNAL_CHIRP,
  SIGNAL_XCHIRP,
  SIGNAL_AWGN
};

enum
{
  PROP_NULL,
  PROP_RATE,
  PROP_SIGNAL,
  PROP_BLOCK,
  PROP_START,
  PROP_DURATION,
  PROP_FREQUENCY,
  PROP_CHIRPRATE,
  PROP_WIDTH,
  PROP_PAUSE
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

#define gst_rojgener_parent_class parent_class
G_DEFINE_TYPE (Gstrojgener, gst_rojgener, GST_TYPE_ELEMENT);

static void gst_rojgener_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojgener_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojgener_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojgener_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojgener's class */
static void
gst_rojgener_class_init (GstrojgenerClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojgener_set_property;
  gobject_class->get_property = gst_rojgener_get_property;


  g_object_class_install_property (gobject_class, PROP_WIDTH, 
	g_param_spec_double ("width", "width", "pulse width", 
		0.0, G_MAXDOUBLE, 1.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PAUSE, 
	g_param_spec_double ("pause", "pause", "pause", 
		0.0, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHIRPRATE, 
	g_param_spec_double ("chirprate", "chirprate", "chirprate", 
		-G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DURATION, 
	g_param_spec_double ("duration", "signal duration", "signal duration", 
		-G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FREQUENCY, 
	g_param_spec_double ("frequency", "signal frequency", "signal frequency",
		-G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RATE, 
	g_param_spec_double ("rate", "sampling rate", "sampling rate",
		0.0, G_MAXDOUBLE, 1000.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_START, 
	g_param_spec_double ("start", "instant start", "signal start", 
		-G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SIGNAL, 
	g_param_spec_string ("signal", "signal type", "signal type", 
		"time", G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BLOCK, 
	g_param_spec_int ("block", "data block", "number of samples in single block",
		1, 134217727, 1024, G_PARAM_READWRITE));
  
  gst_element_class_set_details_simple(gstelement_class, 
	"rojgener", "ROJgener:Generic", "ROJgener:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));

  gst_tag_register ("rate", GST_TAG_FLAG_META, G_TYPE_DOUBLE, "rate", "sampling rate", NULL);
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojgener_init (Gstrojgener * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojgener_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojgener_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->block = 1024;
  filter->type = SIGNAL_TIME;
  filter->duration = 0.0;
  filter->start = 0.0;
  
  filter->rate = 1000.0;
  filter->period = 1.0/filter->rate;

  filter->block = 1024;
  filter->type = SIGNAL_TIME;

  filter->frequency = 0.0;

  filter->chirprate = 0.0;
  filter->width = 1.0;
  filter->pause = 0.0;
  filter->number = 0;
}


static void
gst_rojgener_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojgener *filter = GST_ROJGENER (object);

  switch (prop_id) {
  case PROP_RATE:
    filter->rate = g_value_get_double (value);
    filter->period = 1.0/filter->rate;
    groj_tag_emit_double(gst_element_get_name(filter), filter->srcpad, "rate", filter->rate);
    break;

  case PROP_CHIRPRATE:
    filter->chirprate = g_value_get_double (value);
    break;

  case PROP_WIDTH:
    filter->width = g_value_get_double (value);
    break;
  case PROP_PAUSE:
    filter->pause = g_value_get_double (value);
    break;

  case PROP_DURATION:
    filter->duration = g_value_get_double (value);
    break;

  case PROP_FREQUENCY:
    filter->frequency = g_value_get_double (value);
    break;

  case PROP_START:
    filter->start = g_value_get_double (value);
    filter->time = filter->start;
    break;
    
  case PROP_BLOCK:
    filter->block = g_value_get_int (value);
    break;
    
  case PROP_SIGNAL:    
    if (!strcmp(g_value_get_string (value), "time")){
      filter->type = SIGNAL_TIME;
      break;
    }
    if (!strcmp(g_value_get_string (value), "sinus")){
      filter->type = SIGNAL_SINUS;
      break;
    }
    if (!strcmp(g_value_get_string (value), "xsinus")){
      filter->type = SIGNAL_XSINUS;
      break;
    }
    if (!strcmp(g_value_get_string (value), "chirp")){
      filter->type = SIGNAL_CHIRP;
      break;
    }
    if (!strcmp(g_value_get_string (value), "xchirp")){
      filter->type = SIGNAL_XCHIRP;
      break;
    }
    if (!strcmp(g_value_get_string (value), "awgn")){
      filter->type = SIGNAL_AWGN;
      break;
    }
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    groj_error(gst_element_get_name(object), "not known signal");
    
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
gst_rojgener_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojgener *filter = GST_ROJGENER (object);
 
  switch (prop_id) {
  case PROP_RATE:
    g_value_set_double (value, filter->rate);
    break;
    
  case PROP_CHIRPRATE:
    g_value_set_double (value, filter->chirprate);
    break;

  case PROP_WIDTH:
    g_value_set_double (value, filter->width);
    break;
  case PROP_PAUSE:
    g_value_set_double (value, filter->pause);
    break;

  case PROP_DURATION:
    g_value_set_double (value, filter->duration);
    break;

  case PROP_FREQUENCY:
    g_value_set_double (value, filter->frequency);
    break;

  case PROP_START:
    g_value_set_double (value, filter->start);
    break;
    
  case PROP_BLOCK:
    g_value_set_int (value, filter->block);
    break;
    
  case PROP_SIGNAL:      
    switch(filter->type){
    case SIGNAL_TIME:
      g_value_set_string (value, "time");
      break;
    case SIGNAL_SINUS:
      g_value_set_string (value, "sinus");
      break;
    case SIGNAL_XSINUS:
      g_value_set_string (value, "xsinus");
      break;
    case SIGNAL_CHIRP: 
      g_value_set_string (value, "chirp");
      break;
    case SIGNAL_XCHIRP: 
      g_value_set_string (value, "xchirp");
      break;
    case SIGNAL_AWGN: 
      g_value_set_string (value, "awgn");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      groj_error(gst_element_get_name(object), "not known signal");
    }
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojgener_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojgener *filter = GST_ROJGENER (parent);
  g_print ("* %s: event %s\n",
	   gst_element_get_name(parent), gst_event_type_get_name(GST_EVENT_TYPE (event))); 
  
  gboolean ret;
  switch (GST_EVENT_TYPE (event)) {
  case GST_EVENT_STREAM_START:
    {
      ret = gst_pad_event_default (pad, parent, event);

      /* init caps */

      GstCaps *ocaps = gst_caps_from_string (GROJ_CAPS_SIGNAL);
      GstEvent *ovent = gst_event_new_caps (ocaps);
      ret = gst_pad_event_default (pad, parent, ovent);      
      g_print("\t\e[1;33m(filter in %s): gst/null -> %s\e[0m\n", gst_element_get_name(parent), gst_caps_to_string(ocaps)); 
      
      /* emit rate tag */

      groj_tag_emit_double(gst_element_get_name(filter), filter->srcpad, "rate", filter->rate);

      break;
    }

  default:
    ret = gst_pad_event_default (pad, parent, event);
  }

  return ret;
}


/* CHAIN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
static GstFlowReturn
gst_rojgener_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojgener *filter = GST_ROJGENER (parent);

  /* czas twania sygnału może być dłuższy niż duration */
  /* z dokładnością do rozmiaru pojedynczego bloku */
  if (filter->time>=filter->duration+filter->start && filter->duration>0.0)
    return GST_FLOW_EOS;

  /* W drugim porcja danych z sygnałem */
  GstMemory *mem = gst_buffer_get_all_memory (buffer);
  if (mem!=NULL) g_print("Warning memory are not empty\n");
  mem = gst_allocator_alloc (NULL, filter->block*sizeof(fftw_complex), NULL);

  /* W pierwszym slocie wysyłany jest config */
  GstMemory *cmem = groj_config_new();
  groj_config_set_time(cmem, filter->time);
  groj_config_set_number(cmem, filter->number);
  filter->number++;
  gst_buffer_append_memory (buffer, cmem);
 

  
  GstMapInfo info;
  gst_memory_map (mem, &info, GST_MAP_WRITE);
  memset(info.data, 0x0, filter->block*sizeof(fftw_complex));

  gint n;
  switch(filter->type){
  case SIGNAL_TIME:
    {
      /* rampa */
      for (n=0; n<filter->block; n++){
	((fftw_complex *)info.data)[n] = filter->time+0.0*I;
	filter->time += filter->period; 
      }
      break;
    }
  case SIGNAL_SINUS:
    {
      /* rzeczywista sinusoida */
      for (n=0; n<filter->block; n++){
	double normtime = filter->time/(filter->width+filter->pause);
	double fractime = normtime - (long)normtime;
	double intertime = fractime * (filter->width+filter->pause); 
	if(intertime<filter->width)
	  ((fftw_complex *)info.data)[n] = sin(2.0*M_PI*filter->frequency*filter->time) + 0.0*I;	
	else
	  ((fftw_complex *)info.data)[n] = 0.0 + 0.0*I;
	filter->time += filter->period; 
      }
      break;
    }
  case SIGNAL_XSINUS:
    {
      /* zespolona sinusoida */
      for (n=0; n<filter->block; n++){
	double normtime = filter->time/(filter->width+filter->pause);
	double fractime = normtime - (long)normtime;
	double intertime = fractime * (filter->width+filter->pause); 
	if(intertime<filter->width){
	  gdouble re = cos(2.0*M_PI*filter->frequency*filter->time);
	  gdouble im = sin(2.0*M_PI*filter->frequency*filter->time);
	  ((fftw_complex *)info.data)[n] = re + im*I;
	}
	else
	  ((fftw_complex *)info.data)[n] = 0.0 + 0.0*I;
	filter->time += filter->period; 
      }
      break;
    }
  case SIGNAL_CHIRP: 
    {
      /* rzeczywisty świergot */
      for (n=0; n<filter->block; n++){
	double normtime = filter->time/(filter->width+filter->pause);
	double fractime = normtime - (long)normtime;
	double intertime = fractime * (filter->width+filter->pause); 
	if(intertime<filter->width){
	  double arg = 2.0*M_PI*filter->frequency*filter->time + M_PI*filter->chirprate*pow(intertime, 2.0); 
	  ((fftw_complex *)info.data)[n] = sin(arg) + 0.0*I;
	}
	else
	  ((fftw_complex *)info.data)[n] = 0.0 + 0.0*I;
	filter->time += filter->period; 
      }
      break;
    }
  case SIGNAL_XCHIRP: 
    {
      /*zespolony świergot*/
      for (n=0; n<filter->block; n++){
	double normtime = filter->time/(filter->width+filter->pause);
	double fractime = normtime - (long)normtime;
	double intertime = fractime * (filter->width+filter->pause); 
	if(intertime<filter->width){
	  double arg = 2.0*M_PI*filter->frequency*filter->time + M_PI*filter->chirprate*pow(intertime, 2.0); 
	  ((fftw_complex *)info.data)[n] = cos(arg) + sin(arg)*I;
	}
	else
	  ((fftw_complex *)info.data)[n] = 0.0 + 0.0*I;
      filter->time += filter->period; 
      }
      break;
    }
  case SIGNAL_AWGN: 
    {    
      /* szum biały */
      for (n=0; n<filter->block; n++){
	double normtime = filter->time/(filter->width+filter->pause);
	double fractime = normtime - (long)normtime;
	double intertime = fractime * (filter->width+filter->pause); 
	if(intertime<filter->width)
	  ((fftw_complex *)info.data)[n] = groj_gauss(1.0)+groj_gauss(1.0)*I;	
	else
	  ((fftw_complex *)info.data)[n] = 0.0 + 0.0*I;
	filter->time += filter->period; 
      }
      break;
    }
  default:
    groj_error(gst_element_get_name(parent), "not known signal");
  }
  
  gst_memory_unmap (mem, &info);    
  gst_buffer_append_memory (buffer, mem);

  return gst_pad_push (filter->srcpad, buffer);   
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojgener_init (GstPlugin * rojgener)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojgener' with your description
   */

  GST_DEBUG_CATEGORY_INIT (gst_rojgener_debug, "rojgener", 0, "rojgener");
  return gst_element_register (rojgener, "rojgener", GST_RANK_NONE, GST_TYPE_ROJGENER);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojgener"
#endif

/* gstreamer looks for this structure to register rojgeners
 * exchange the string 'Template rojgener' with your rojgener description
 */

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojgener,
    "ROJgener",
    rojgener_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
    )

