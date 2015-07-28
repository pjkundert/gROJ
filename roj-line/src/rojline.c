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


#include "rojline.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojline_debug);
#define GST_CAT_DEFAULT gst_rojline_debug


enum
{
  PROP_NULL,
  PROP_GAIN,
  PROP_FREQUENCY
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

#define gst_rojline_parent_class parent_class
G_DEFINE_TYPE (Gstrojline, gst_rojline, GST_TYPE_ELEMENT);

static void gst_rojline_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojline_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojline_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojline_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojline's class */
static void
gst_rojline_class_init (GstrojlineClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojline_set_property;
  gobject_class->get_property = gst_rojline_get_property;

  g_object_class_install_property (gobject_class, PROP_GAIN, 
	g_param_spec_double ("gain", "gain", "gain", 
		0.0, G_MAXDOUBLE, 1.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FREQUENCY, 
	g_param_spec_double ("frequency", "frequency", "frequency", 
		-G_MAXDOUBLE, G_MAXDOUBLE, 1.0, G_PARAM_READWRITE));


  gst_element_class_set_details_simple(gstelement_class,
    "rojline", "GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojline_init (Gstrojline * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojline_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojline_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->gain = 1.0;
  groj_init_tags(filter->tags);
}

static void
gst_rojline_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojline *filter = GST_ROJLINE (object);

  switch (prop_id) {
    case PROP_GAIN:
      filter->gain = g_value_get_double (value);
      break;
    case PROP_FREQUENCY:
      filter->frequency = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rojline_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojline *filter = GST_ROJLINE (object);

  switch (prop_id) {
    case PROP_GAIN:
      g_value_set_double (value, filter->gain);
      break;
    case PROP_FREQUENCY:
      g_value_set_double (value, filter->frequency);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojline_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojline *filter = GST_ROJLINE (parent);
  g_print ("* %s: event %s\n",
	   gst_element_get_name(parent), gst_event_type_get_name(GST_EVENT_TYPE (event))); 

  gboolean ret;
  switch (GST_EVENT_TYPE (event)) {
  case GST_EVENT_CAPS:
    {
      GstEvent *ovent = groj_caps_event(parent, event, GROJ_CAPS_SIGNAL, GROJ_CAPS_SIGNAL);
      ret = gst_pad_event_default (pad, parent, ovent);      
      break;
    }
  case GST_EVENT_TAG:
    {
      ret = gst_pad_event_default (pad, parent, event);

      GstTagList *taglist;
      gst_event_parse_tag(event, &taglist);

      groj_tag_catch_double(taglist, "rate", gst_element_get_name(parent), &filter->tags.rate);
      break;
    }
  case GST_EVENT_SEGMENT:
    {
      ret = gst_pad_event_default (pad, parent, event);
      if (!filter->tags.rate.flag)
	groj_error(gst_element_get_name(parent), "need tags (rate)");
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
gst_rojline_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojline *filter = GST_ROJLINE (parent);

  GstMemory *cmem = gst_buffer_get_memory (buffer, 0);
  double time = groj_config_get_time(cmem);

  GstMapInfo info;
  GstMemory *mem = gst_buffer_get_memory (buffer, 1);
  gst_memory_map (mem, &info, GST_MAP_READWRITE);
  int n, size = info.size/sizeof(fftw_complex);

  if (filter->gain!=1.0){
    for(n=0; n<size; n++)
      ((fftw_complex *)info.data)[n] *= filter->gain;
  }

  double period = 1.0/filter->tags.rate.val;
  double factor = 2.0*M_PI*filter->frequency;
  if (filter->frequency!=0.0){
    for(n=0; n<size; n++){
      ((fftw_complex *)info.data)[n] *= cexp(I * factor * time);
      time += period;	  
    }
  }
  
  gst_memory_unmap (mem, &info);
  return gst_pad_push (filter->srcpad, buffer);
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojline_init (GstPlugin * rojline)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojline' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojline_debug, "rojline", 0, "Template rojline");
  return gst_element_register (rojline, "rojline", GST_RANK_NONE, GST_TYPE_ROJLINE);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojline"
#endif

/* gstreamer looks for this structure to register rojlines
 * exchange the string 'Template rojline' with your rojline description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojline,
    "rojline",
    rojline_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
