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


#include "rojconvert.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojconvert_debug);
#define GST_CAT_DEFAULT gst_rojconvert_debug

enum
{
  CONVERT_ENERGY,
  CONVERT_PHASE,
  CONVERT_BOTH,
  CONVERT_ABS
};

enum
{
  PROP_NULL,
  PROP_OUT
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

#define gst_rojconvert_parent_class parent_class
G_DEFINE_TYPE (Gstrojconvert, gst_rojconvert, GST_TYPE_ELEMENT);

static void gst_rojconvert_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojconvert_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojconvert_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojconvert_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojconvert's class */
static void
gst_rojconvert_class_init (GstrojconvertClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojconvert_set_property;
  gobject_class->get_property = gst_rojconvert_get_property;

  g_object_class_install_property (gobject_class, PROP_OUT, 
	g_param_spec_string ("out", "output", "output", 
		"time", G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class, "rojconvert",
    "GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojconvert_init (Gstrojconvert * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojconvert_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojconvert_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->out = CONVERT_ENERGY;
}

static void
gst_rojconvert_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojconvert *filter = GST_ROJCONVERT (object);
  
  switch (prop_id) {
  case PROP_OUT:
    if (!strcmp(g_value_get_string (value), "energy")){
      filter->out = CONVERT_ENERGY;
      break;
    }
    if (!strcmp(g_value_get_string (value), "phase")){
      filter->out = CONVERT_PHASE;
      break;
    }
    if (!strcmp(g_value_get_string (value), "abs")){
      filter->out = CONVERT_ABS;
      break;
    }
    if (!strcmp(g_value_get_string (value), "both")){
      filter->out = CONVERT_BOTH;
      break;
    }

    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    groj_error(gst_element_get_name(object), "not known output");
    
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
gst_rojconvert_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojconvert *filter = GST_ROJCONVERT (object);

  switch (prop_id) {
  case PROP_OUT:
    switch(filter->out){
    case CONVERT_ENERGY:
      g_value_set_string (value, "energy");
      break;
    case CONVERT_PHASE:
      g_value_set_string (value, "phase");
      break;
    case CONVERT_ABS:
      g_value_set_string (value, "abs");
      break;
    case CONVERT_BOTH:
      g_value_set_string (value, "both");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      groj_error(gst_element_get_name(object), "not known output");
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
gst_rojconvert_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  g_print ("* %s: event %s\n",
	   gst_element_get_name(parent), gst_event_type_get_name(GST_EVENT_TYPE (event))); 

  gboolean ret;  
  switch (GST_EVENT_TYPE (event)) {
  case GST_EVENT_CAPS:
    {
      GstEvent *ovent = groj_caps_event(parent, event, GROJ_CAPS_STFT, GROJ_CAPS_TFR);
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
gst_rojconvert_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojconvert * filter = GST_ROJCONVERT (parent);

  GstMapInfo ininfo;
  GstMemory *cmem = gst_buffer_get_memory (buffer, 0);
  GstMemory *mem = gst_buffer_get_memory (buffer, 1);
  gst_memory_map (mem, &ininfo, GST_MAP_READ);
  gint len = ininfo.size/sizeof(fftw_complex);  

  GstBuffer * outbuffer = gst_buffer_new ();
  gint n;
  switch(filter->out){
  case CONVERT_ENERGY:
    {
      GstMemory * mem = gst_allocator_alloc (NULL, len*sizeof(double), NULL);
      GstMapInfo outinfo;
      gst_memory_map (mem, &outinfo, GST_MAP_WRITE);

      for(n=0; n<len; n++){
	gdouble re = creal(((fftw_complex *)ininfo.data)[n]);
	gdouble im = cimag(((fftw_complex *)ininfo.data)[n]);
	((double *)outinfo.data)[n] = pow(re ,2.0) + pow(im ,2.0);
      }

      gst_memory_unmap (mem, &outinfo);
      gst_buffer_append_memory (outbuffer, cmem);
      gst_buffer_append_memory (outbuffer, mem);
      break;
    }
  case CONVERT_PHASE:
    {
      GstMemory * mem = gst_allocator_alloc (NULL, len*sizeof(double), NULL);
      GstMapInfo outinfo;
      gst_memory_map (mem, &outinfo, GST_MAP_WRITE);
      
      for(n=0; n<len; n++){
	((double *)outinfo.data)[n] = carg(((fftw_complex *)ininfo.data)[n]);
      }

      gst_memory_unmap (mem, &outinfo);
      gst_buffer_append_memory (outbuffer, cmem);
      gst_buffer_append_memory (outbuffer, mem);
      break;
    }    
  case CONVERT_ABS:
    {
      GstMemory * mem = gst_allocator_alloc (NULL, len*sizeof(double), NULL);
      GstMapInfo outinfo;
      gst_memory_map (mem, &outinfo, GST_MAP_WRITE);

      for(n=0; n<len; n++){
	gdouble re = creal(((fftw_complex *)ininfo.data)[n]);
	gdouble im = cimag(((fftw_complex *)ininfo.data)[n]);
	((double *)outinfo.data)[n] = sqrt(pow(re, 2.0) + pow(im, 2.0));
      }

      gst_memory_unmap (mem, &outinfo);
      gst_buffer_append_memory (outbuffer, cmem);
      gst_buffer_append_memory (outbuffer, mem);     
      break;
    }    

  case CONVERT_BOTH:
    {
      GstMemory * mem1 = gst_allocator_alloc (NULL, len*sizeof(double), NULL);
      GstMemory * mem2 = gst_allocator_alloc (NULL, len*sizeof(double), NULL);
      GstMapInfo outinfo1, outinfo2;
      gst_memory_map (mem1, &outinfo1, GST_MAP_WRITE);
      gst_memory_map (mem2, &outinfo2, GST_MAP_WRITE);

      for(n=0; n<len; n++){
	gdouble re = creal(((fftw_complex *)ininfo.data)[n]);
	gdouble im = cimag(((fftw_complex *)ininfo.data)[n]);
	((double *)outinfo1.data)[n] = pow(re ,2.0) + pow(im ,2.0);
      }

      for(n=0; n<len; n++)
	((double *)outinfo2.data)[n] = carg(((fftw_complex *)ininfo.data)[n]);

      gst_memory_unmap (mem1, &outinfo1);
      gst_memory_unmap (mem2, &outinfo2);
      gst_buffer_append_memory (outbuffer, cmem);
      gst_buffer_append_memory (outbuffer, mem1);
      gst_buffer_append_memory (outbuffer, mem2);
    } 
    break;
  }

  
  gst_memory_unmap (mem, &ininfo);
  gst_buffer_unref (buffer);

  return gst_pad_push (filter->srcpad, outbuffer);
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojconvert_init (GstPlugin * rojconvert)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojconvert' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojconvert_debug, "rojconvert", 0, "rojconvert");
  return gst_element_register (rojconvert, "rojconvert", GST_RANK_NONE, GST_TYPE_ROJCONVERT);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojconvert"
#endif

/* gstreamer looks for this structure to register rojconverts
 * exchange the string 'Template rojconvert' with your rojconvert description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojconvert,
    "rojconvert",
    rojconvert_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
