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


#include "rojfilter.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojfilter_debug);
#define GST_CAT_DEFAULT gst_rojfilter_debug

enum
{
  PROP_NULL,
  PROP_RECOEFF,
  PROP_IMCOEFF,
  PROP_LENGTH
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

#define gst_rojfilter_parent_class parent_class
G_DEFINE_TYPE (Gstrojfilter, gst_rojfilter, GST_TYPE_ELEMENT);

static void gst_rojfilter_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojfilter_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojfilter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojfilter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojfilter's class */
static void
gst_rojfilter_class_init (GstrojfilterClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojfilter_set_property;
  gobject_class->get_property = gst_rojfilter_get_property;

  g_object_class_install_property (gobject_class, PROP_LENGTH, 
	g_param_spec_int ("length", "length", "filter length and reset", 
   	        1, G_MAXINT, 1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RECOEFF, 
	g_param_spec_double ("recoeff", "recoeff", "filter real coeff", 
   	        -G_MAXDOUBLE, G_MAXDOUBLE, 1.0, G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_IMCOEFF, 
	g_param_spec_double ("imcoeff", "imcoeff", "filter imag coeff", 
   	        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_WRITABLE));

  gst_element_class_set_details_simple(gstelement_class, "rojfilter",
     "GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojfilter_init (Gstrojfilter * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojfilter_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojfilter_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->fir = malloc(sizeof(fftw_complex));
  filter->buf = malloc(sizeof(fftw_complex));
  if(filter->fir==NULL || filter->buf==NULL)
    groj_error(gst_element_get_name(filter), "mem");
  
  filter->fir[0] = 1.0 + 0.0*I;
  filter->length = 1;
  filter->recount = 1;
  filter->imcount = 1;
  filter->swap = FALSE;
}

static void
gst_rojfilter_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojfilter *filter = GST_ROJFILTER (object);
  
  switch (prop_id) {
  case PROP_LENGTH:
    filter->newlength = g_value_get_int (value);
    filter->tmp = malloc(filter->newlength * sizeof(fftw_complex));
    if (filter->tmp==NULL || filter->newlength<=0)
      groj_error(gst_element_get_name(filter), "len mem");
    memset(filter->tmp, 0x0, filter->newlength * sizeof(fftw_complex));

    filter->recount = 0;
    filter->imcount = 0;
    break;

  case PROP_RECOEFF:
    if(filter->recount>=filter->newlength){
      g_print("re overload\n");
      break;
    }

    filter->tmp[filter->recount] += g_value_get_double (value) + I*0.0;
    filter->recount++;

    if(filter->recount==filter->newlength)
      filter->swap = TRUE;    
    break;
    
  case PROP_IMCOEFF:
    if(filter->imcount>=filter->newlength){
      g_print("im overload\n");
      break;
    }

    filter->tmp[filter->imcount] += 0.0 + I*g_value_get_double (value);
    filter->imcount++;      
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
gst_rojfilter_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojfilter *filter = GST_ROJFILTER (object);

  switch (prop_id) {
  case PROP_LENGTH:
    g_value_set_int (value, filter->length);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojfilter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojfilter * filter = GST_ROJFILTER (parent);
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
    break;
  }
  return ret;
}

/* chain function this function does the actual processing */
static GstFlowReturn
gst_rojfilter_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojfilter *filter = GST_ROJFILTER (parent);

  GstMapInfo info;
  GstMemory *cmem = gst_buffer_get_memory (buffer, 0);
  GstMemory *mem = gst_buffer_get_memory (buffer, 1);
  gst_memory_map (mem, &info, GST_MAP_READ);
  int n, size = info.size / sizeof(fftw_complex);

  GstBuffer * outbuffer = gst_buffer_new ();
  gst_buffer_append_memory (outbuffer, cmem);
  
  GstMapInfo outinfo;
  GstMemory * outmem = gst_allocator_alloc (NULL, size*sizeof(fftw_complex), NULL);
  gst_buffer_append_memory (outbuffer, outmem);
  gst_memory_map (outmem, &outinfo, GST_MAP_WRITE);

  if (filter->swap){
    free(filter->fir);
    filter->fir = filter->tmp;

    filter->length = filter->newlength;

    free(filter->buf);
    filter->buf = malloc(filter->length * sizeof(fftw_complex));
    if (filter->buf==NULL)
      groj_error(gst_element_get_name(filter), "mem");
    memset(filter->buf, 0x0, filter->length * sizeof(fftw_complex));

    filter->swap = FALSE;
    g_print("* %s: log: new filter\n", gst_element_get_name(filter));
    /*
    for(n=0; n<filter->length; n++)
      g_print("%g %g\n", creal(filter->fir[n]), cimag(filter->fir[n]));
    */
  }

  int k;
  for(n=0; n<size; n++){
    
    memcpy(filter->buf, &filter->buf[1], (filter->length-1) * sizeof(fftw_complex));
    filter->buf[filter->length-1] = ((fftw_complex *)info.data)[n];

    fftw_complex val = 0.0 + 0.0*I;
    for(k=0; k<filter->length;k++)
      val += filter->fir[k]*filter->buf[k];
    ((fftw_complex *)outinfo.data)[n] = val;

  }

  gst_buffer_unref (buffer);
  gst_memory_unmap (mem, &info);
  gst_memory_unmap (outmem, &outinfo);	
  return gst_pad_push (filter->srcpad, outbuffer);
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojfilter_init (GstPlugin * rojfilter)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojfilter' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojfilter_debug, "rojfilter", 0, "Template rojfilter");
  return gst_element_register (rojfilter, "rojfilter", GST_RANK_NONE, GST_TYPE_ROJFILTER);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojfilter"
#endif

/* gstreamer looks for this structure to register rojfilters
 * exchange the string 'Template rojfilter' with your rojfilter description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojfilter,
    "rojfilter",
    rojfilter_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
