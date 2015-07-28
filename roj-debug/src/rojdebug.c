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


#include "rojdebug.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojdebug_debug);
#define GST_CAT_DEFAULT gst_rojdebug_debug

enum
{
  PROP_NULL,
  PROP_TAGS,
  PROP_DATA,
  PROP_PACK
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

#define gst_rojdebug_parent_class parent_class
G_DEFINE_TYPE (Gstrojdebug, gst_rojdebug, GST_TYPE_ELEMENT);

static void gst_rojdebug_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojdebug_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojdebug_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojdebug_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojdebug's class */
static void
gst_rojdebug_class_init (GstrojdebugClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojdebug_set_property;
  gobject_class->get_property = gst_rojdebug_get_property;

  g_object_class_install_property (gobject_class, PROP_PACK,
				   g_param_spec_boolean ("pack", "pack flag", "pack flag",
							 FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DATA,
				   g_param_spec_boolean ("data", "data flag", "data flag",
							 FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TAGS,
				   g_param_spec_boolean ("tags", "tags flag", "tags flag",
							 FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
				       "rojdebug", "GSTroj:Generic", "GSTroj:Generic",
				       "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");
  
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojdebug_init (Gstrojdebug * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojdebug_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojdebug_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->tags = FALSE;
  filter->pack = FALSE;
  filter->data = FALSE;
  filter->real = FALSE;

  filter->counter = 0;
}

static void
gst_rojdebug_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojdebug *filter = GST_ROJDEBUG (object);

  switch (prop_id) {
    case PROP_PACK:
      filter->pack = g_value_get_boolean (value);
      break;
    case PROP_TAGS:
      filter->tags = g_value_get_boolean (value);
      break;
    case PROP_DATA:
      filter->data = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rojdebug_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojdebug *filter = GST_ROJDEBUG (object);

  switch (prop_id) {
    case PROP_DATA:
      g_value_set_boolean (value, filter->data);
      break;
    case PROP_PACK:
      g_value_set_boolean (value, filter->pack);
      break;
    case PROP_TAGS:
      g_value_set_boolean (value, filter->tags);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojdebug_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojdebug *filter = GST_ROJDEBUG (parent);
  g_print ("* %s event %s\n",
	   gst_element_get_name(parent), gst_event_type_get_name(GST_EVENT_TYPE (event))); 

  gboolean ret;
  switch (GST_EVENT_TYPE (event)) {
  case GST_EVENT_CAPS:
    {
      ret = gst_pad_event_default (pad, parent, event);      

      GstCaps * caps;
      gst_event_parse_caps (event, &caps);
      gboolean signal_flag = !(strcmp(GROJ_CAPS_SIGNAL, gst_caps_to_string (caps)));
      if (signal_flag) filter->real = FALSE;

      gboolean frame_flag = !(strcmp(GROJ_CAPS_FRAME, gst_caps_to_string (caps)));
      if (frame_flag) filter->real = FALSE;

      gboolean stft_flag = !(strcmp(GROJ_CAPS_STFT, gst_caps_to_string (caps)));
      if (stft_flag) filter->real = FALSE;

      gboolean tfr_flag = !(strcmp(GROJ_CAPS_TFR, gst_caps_to_string (caps)));
      if (tfr_flag) filter->real = TRUE;
      

      if (!(tfr_flag || stft_flag || frame_flag || signal_flag))
	groj_error(gst_element_get_name(parent), "caps");

      break;
    }
  case GST_EVENT_TAG:
    {
      if (filter->tags){
	GstTagList *taglist;
	gst_event_parse_tag (event, &taglist);
	g_print ("   \e[1;33mtag not forward: %s\e[0m\n", gst_tag_list_to_string(taglist)); 
      }		
	   
      /* Bez przesyłania tagów dalej w ostatniej wtyczce: */
      ret = TRUE;
      break;
    }
  default:
    ret = gst_pad_event_default (pad, parent, event);
  }
  
  return ret;
}
 
/* CHAIN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
static GstFlowReturn
gst_rojdebug_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojdebug *filter = GST_ROJDEBUG (parent);
  filter->counter++;

  if (filter->pack == TRUE){
    g_print ("%d buffer:\n", filter->counter); 
    g_print ("\tmemory slots %d/%d\n", gst_buffer_n_memory (buffer), gst_buffer_get_max_memory ());
    g_print ("\tsize: %d B\n", (int)gst_buffer_get_size (buffer));
  }

  if (filter->data == TRUE && gst_buffer_n_memory (buffer)){

    GstMapInfo info;
    GstMemory *cmem = gst_buffer_get_memory (buffer, 0);
    GstMemory *mem = gst_buffer_get_memory (buffer, 1);
    gst_memory_map (mem, &info, GST_MAP_READ);
    double time = groj_config_get_time(cmem);

    g_print("time: %g\n", time);
    if (filter->real == FALSE){
      int n, size = info.size/sizeof(fftw_complex);
      for (n=0; n<size; n++)
	g_print ("%f %fj\n", creal(((fftw_complex *)info.data)[n]), cimag(((fftw_complex *)info.data)[n]));
    }
    else{
      int n, size = info.size/sizeof(double);
      for (n=0; n<size; n++)
	g_print ("%f\n", ((double *)info.data)[n]);
    }

    gst_memory_unmap (mem, &info);
  }

  return gst_pad_push (filter->srcpad, buffer);
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojdebug_init (GstPlugin * rojdebug)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojdebug' with your description
   */

  GST_DEBUG_CATEGORY_INIT (gst_rojdebug_debug, "rojdebug", 0, "rojdebug");
  return gst_element_register (rojdebug, "rojdebug", GST_RANK_NONE, GST_TYPE_ROJDEBUG);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojdebug"
#endif

/* gstreamer looks for this structure to register rojdebugs
 * exchange the string 'Template rojdebug' with your rojdebug description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojdebug,
    "rojdebug",
    rojdebug_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
