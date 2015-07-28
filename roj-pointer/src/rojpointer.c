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


#include "rojpointer.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojpointer_debug);
#define GST_CAT_DEFAULT gst_rojpointer_debug

enum
{
  PROP_NULL,
  PROP_RATE,
  PROP_START,
  PROP_BLOCK,
  PROP_MODE
};

enum
{
  MODE_AMFM,
  MODE_2FM,
  MODE_XY
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

#define gst_rojpointer_parent_class parent_class
G_DEFINE_TYPE (Gstrojpointer, gst_rojpointer, GST_TYPE_ELEMENT);

static void gst_rojpointer_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojpointer_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojpointer_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojpointer_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojpointer's class */
static void
gst_rojpointer_class_init (GstrojpointerClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojpointer_set_property;
  gobject_class->get_property = gst_rojpointer_get_property;

  g_object_class_install_property (gobject_class, PROP_BLOCK, 
	g_param_spec_int ("block", "data block", "number of samples in single block",
		1, 134217727, 1024, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RATE, 
	g_param_spec_double ("rate", "sampling rate", "sampling rate",
		0.0, G_MAXDOUBLE, 1000.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_START, 
	g_param_spec_double ("start", "instant start", "signal start", 
		-G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MODE, 
	g_param_spec_string ("mode", "mode", "mode", 
		"mode", G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "rojpointer", "GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));

  gst_tag_register ("rate", GST_TAG_FLAG_META, G_TYPE_DOUBLE, "rate", "sampling rate", NULL);
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
#define MOUSEFILE "/dev/input/event2"

static void
gst_rojpointer_init (Gstrojpointer * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojpointer_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojpointer_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->block = 1024;  
  filter->start = 0.0;  
  filter->rate = 1000.0;
  filter->cycle = 500000.0 / filter->rate;
  filter->period = 1.0 / filter->rate;

  filter->number = 0;

  filter->display = XOpenDisplay(NULL); 
  filter->wscr = DisplayWidth(filter->display, DefaultScreen(filter->display))/2;
  filter->hscr = DisplayHeight(filter->display, DefaultScreen(filter->display))/2;
}

static void
gst_rojpointer_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojpointer *filter = GST_ROJPOINTER (object);

  switch (prop_id) {
  case PROP_MODE:    
    if (!strcmp(g_value_get_string (value), "amfm")){
      filter->mode = MODE_AMFM;
      break;
    }
    if (!strcmp(g_value_get_string (value), "2fm")){
      filter->mode = MODE_2FM;
      break;
    }
    if (!strcmp(g_value_get_string (value), "xy")){
      filter->mode = MODE_XY;
      break;
    }

    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    groj_error(gst_element_get_name(object), "not known mode");
    
  case PROP_START:
    filter->start = g_value_get_double (value);
    filter->time = g_value_get_double (value);
    break;

  case PROP_RATE:
    filter->rate = g_value_get_double (value);
    filter->cycle = 500000.0 / filter->rate;
    filter->period = 1.0 / filter->rate;
    groj_tag_emit_double(gst_element_get_name(filter), filter->srcpad, "rate", filter->rate);
    break;

  case PROP_BLOCK:
    filter->block = g_value_get_int (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
gst_rojpointer_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojpointer *filter = GST_ROJPOINTER (object);

  switch (prop_id) {

   case PROP_RATE:
    g_value_set_double (value, filter->rate);
    break;

  case PROP_START:
    g_value_set_double (value, filter->start);
    break;

  case PROP_BLOCK:
    g_value_set_int (value, filter->block);
    break;

  case PROP_MODE:      
    switch(filter->mode){
    case MODE_AMFM:
      g_value_set_string (value, "amfm");
      break;
    case MODE_2FM:
      g_value_set_string (value, "2fm");
      break;
    case MODE_XY:
      g_value_set_string (value, "xy");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      groj_error(gst_element_get_name(object), "not known mode");
    }
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojpointer_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojpointer *filter = GST_ROJPOINTER (parent);

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

      groj_tag_emit_double(gst_element_get_name(filter), filter->srcpad, "rate", filter->rate);
      
      break;
    }
  case GST_EVENT_EOS:
    {
      ret = gst_pad_event_default (pad, parent, event);
      XCloseDisplay(filter->display); 
      break;
    }

  default:
    ret = gst_pad_event_default (pad, parent, event);
    break;
  }
 
  return ret;
}

/* chain function this function does the actual processing */
static GstFlowReturn
gst_rojpointer_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojpointer *filter = GST_ROJPOINTER (parent);
  
  GstMemory *mem = gst_buffer_get_all_memory (buffer);
  if (mem!=NULL) g_print("Warning memory are not empty\n");
  gst_buffer_unref(buffer);
  
  GstMapInfo info;
  GstBuffer * outbuffer = gst_buffer_new ();
  
  /* W pierwszym slocie wysyłany jest config */
  GstMemory *cmem = groj_config_new();
  groj_config_set_time(cmem, filter->time);
  groj_config_set_number(cmem, filter->number);
  gst_buffer_append_memory (outbuffer, cmem);
  filter->number++;
  
  mem = gst_allocator_alloc (NULL, filter->block*sizeof(fftw_complex), NULL);
  gst_memory_map (mem, &info, GST_MAP_WRITE);
  memset(info.data, 0x0, filter->block*sizeof(fftw_complex));

  int rootx, rooty; 
  int childx, childy;
  Window root, child;
  unsigned int mask; 


  int n;
  for(n=0;n<filter->block;n++){

    struct timespec current={0,0};
    clock_gettime(CLOCK_MONOTONIC, &current);
    XQueryPointer(filter->display, DefaultRootWindow(filter->display),  &root, &child, &rootx, &rooty, &childx, &childy, &mask);

    switch(filter->mode){
    case MODE_AMFM:
      {
	double x = (double)rootx/filter->wscr;
	double y = 0.5*filter->rate*(double)(rooty-filter->hscr)/filter->hscr;
   
	((fftw_complex *)info.data)[n] = 
	  x * cexp(-2.0*M_PI*I*y* (filter->time + n*filter->period) );
      }
      break;
    case MODE_2FM:
      {
	double x = 0.5*filter->rate*(double)(rootx-filter->wscr)/filter->wscr;
	double y = 0.5*filter->rate*(double)(rooty-filter->hscr)/filter->hscr;
   
	((fftw_complex *)info.data)[n] = 
	  cexp(-2.0*M_PI*I*x* (filter->time + n*filter->period) ) + cexp(-2.0*M_PI*I*y* (filter->time + n*filter->period) );
      }
      break;
    case MODE_XY:
      {
	((fftw_complex *)info.data)[n] = 
	  (double)rootx+I*(double)rooty;
      }
      break;
    }

    usleep(filter->cycle); // TO DO: interpolacja....
  }

  gst_memory_unmap (mem, &info);    
  gst_buffer_append_memory (outbuffer, mem);

  filter->time += (float)filter->block*filter->period;
  return gst_pad_push (filter->srcpad, outbuffer);
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojpointer_init (GstPlugin * rojpointer)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojpointer' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojpointer_debug, "rojpointer", 0, "Template rojpointer");
  return gst_element_register (rojpointer, "rojpointer", GST_RANK_NONE, GST_TYPE_ROJPOINTER);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojpointer"
#endif

/* gstreamer looks for this structure to register rojpointers
 * exchange the string 'Template rojpointer' with your rojpointer description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojpointer,
    "rojpointer",
    rojpointer_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
