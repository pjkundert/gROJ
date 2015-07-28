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


#include "rojkadr.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojkadr_debug);
#define GST_CAT_DEFAULT gst_rojkadr_debug

enum
{
  PROP_NULL,
  PROP_OTIME,
  PROP_ETIME,
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

#define gst_rojkadr_parent_class parent_class
G_DEFINE_TYPE (Gstrojkadr, gst_rojkadr, GST_TYPE_ELEMENT);

static void gst_rojkadr_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojkadr_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojkadr_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojkadr_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojkadr's class */
static void
gst_rojkadr_class_init (GstrojkadrClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojkadr_set_property;
  gobject_class->get_property = gst_rojkadr_get_property;

  g_object_class_install_property (gobject_class, PROP_OTIME, 
	g_param_spec_double ("otime", "set otime", "set otime", 
   	        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ETIME, 
	g_param_spec_double ("etime", "set etime", "set etime", 
   	        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_OFREQ, 
	g_param_spec_double ("ofreq", "set ofreq", "set ofreq", 
   	        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_EFREQ, 
	g_param_spec_double ("efreq", "set efreq", "set efreq", 
   	        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class, "rojkadr",
    "GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}


/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojkadr_init (Gstrojkadr * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojkadr_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojkadr_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->otime.flag = FALSE;
  filter->etime.flag = FALSE;
  filter->ofreq.flag = FALSE;
  filter->efreq.flag = FALSE;

  groj_init_tags(filter->tags);
}

static void
gst_rojkadr_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojkadr *filter = GST_ROJKADR (object);

  switch (prop_id) {
  case PROP_OTIME:
    filter->otime.val = g_value_get_double (value);
    filter->otime.flag = TRUE;
    break;
  case PROP_ETIME:
    filter->etime.val = g_value_get_double (value);
    filter->etime.flag = TRUE;
    break;
  case PROP_OFREQ:
    filter->ofreq.val = g_value_get_double (value);
    filter->ofreq.flag = TRUE;
    break;
  case PROP_EFREQ:
    filter->efreq.val = g_value_get_double (value);
    filter->efreq.flag = TRUE;
    break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rojkadr_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojkadr *filter = GST_ROJKADR (object);

  switch (prop_id) {
  case PROP_OTIME:
    g_value_set_double (value, filter->otime.val);
    break;
  case PROP_ETIME:
    g_value_set_double (value, filter->etime.val);
    break;
  case PROP_OFREQ:
    g_value_set_double (value, filter->otime.val);
    break;
  case PROP_EFREQ:
    g_value_set_double (value, filter->etime.val);
    break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojkadr_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojkadr *filter = GST_ROJKADR (parent);
  g_print ("* %s: event %s\n",
	   gst_element_get_name(parent), gst_event_type_get_name(GST_EVENT_TYPE (event))); 

  gboolean ret;
  switch (GST_EVENT_TYPE (event)) {
  case GST_EVENT_CAPS:
    {
      GstEvent *ovent = groj_caps_event(parent, event, GROJ_CAPS_TFR, GROJ_CAPS_TFR);
      ret = gst_pad_event_default (pad, parent, ovent);      
      break;
    }

  case GST_EVENT_TAG:
    {
      gboolean forward = TRUE;
      GstTagList *taglist;
      gst_event_parse_tag(event, &taglist);

      if (groj_tag_catch_double(taglist, "ofreq", gst_element_get_name(parent), &filter->tags.ofreq))
	forward = FALSE;
      
      if (forward)
	ret = gst_pad_event_default (pad, parent, event);
      else
	ret = TRUE;
      
      groj_tag_catch_double(taglist, "leap", gst_element_get_name(parent), &filter->tags.leap);

     if (filter->tags.ofreq.flag && filter->tags.leap.flag && !filter->ofreq_tag_is_send){
	if (!filter->ofreq.flag)
	  filter->ofreq.val = filter->tags.ofreq.val;
	else{
	  if(filter->ofreq.val<=filter->tags.ofreq.val){
	    filter->ofreq.val=filter->tags.ofreq.val;
	    filter->ofreq.flag=FALSE;
	  }
	  else
	    filter->ofreq.val = groj_ceil(filter->tags.ofreq.val, filter->tags.leap.val, filter->ofreq.val);
	}
	
	if(filter->efreq.flag && filter->efreq.val <= filter->ofreq.val)
	  groj_error(gst_element_get_name(parent), "ofreq >= efreq");

       	GstTagList * taglistout = gst_tag_list_new ("ofreq", filter->ofreq.val, NULL);
	GstEvent * event = gst_event_new_tag (taglistout);
	gst_pad_push_event (filter->srcpad, event);
	filter->ofreq_tag_is_send = TRUE;
     }

     break;
    }

  case GST_EVENT_SEGMENT:
    {
      ret = gst_pad_event_default (pad, parent, event);
      if (!filter->tags.ofreq.flag || !filter->tags.leap.flag)      
	groj_error(gst_element_get_name(parent), "need tags");
      if (!filter->ofreq_tag_is_send)
	groj_error(gst_element_get_name(parent), "important tag(s) are not emited");
      break;
    }

    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}

/* CHAIN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
static GstFlowReturn
gst_rojkadr_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojkadr *filter = GST_ROJKADR (parent);

  GstMemory *cmem = gst_buffer_get_memory (buffer, 0);
  double time = groj_config_get_time(cmem);

  if (filter->otime.flag && time<filter->otime.val){
    gst_buffer_unref (buffer);
    return 0;
  } 

  if (filter->etime.flag && time>filter->etime.val){
    gst_buffer_unref (buffer);
    return GST_FLOW_EOS;
  } 


  if (filter->ofreq.flag || filter->efreq.flag){
    
    int c, channels = gst_buffer_n_memory (buffer);
    GstBuffer * outbuffer = gst_buffer_new ();
    gst_buffer_append_memory (outbuffer, cmem);

    for(c=1; c<channels; c++){
      GstMemory *mem = gst_buffer_get_memory (buffer, c);
      GstMapInfo info;

      gst_memory_map (mem, &info, GST_MAP_READ);
      int samples = info.size / sizeof(double);
      
      int initial = 0;
      if (filter->ofreq.flag)
	initial = (filter->ofreq.val-filter->tags.ofreq.val) / filter->tags.leap.val;

      int last = samples;
      if (filter->efreq.flag){

	last = (filter->efreq.val - filter->tags.ofreq.val) / filter->tags.leap.val;
	if (last > samples)
	  last = samples;
      }

      int newlength = last - initial;      
      GstMemory * outmem = gst_allocator_alloc (NULL, newlength * sizeof(double), NULL);
      GstMapInfo outinfo;
      gst_memory_map (outmem, &outinfo, GST_MAP_WRITE);
      
      memcpy(outinfo.data, &((double *)info.data)[initial], newlength *sizeof(double));

      gst_memory_unmap (outmem, &outinfo);
      gst_buffer_append_memory (outbuffer, outmem);
      gst_memory_unmap (mem, &info);
    }

    gst_buffer_unref (buffer);
    buffer = outbuffer;
  }
    
  return gst_pad_push (filter->srcpad, buffer);
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojkadr_init (GstPlugin * rojkadr)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojkadr' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojkadr_debug, "rojkadr", 0, "Template rojkadr");
  return gst_element_register (rojkadr, "rojkadr", GST_RANK_NONE, GST_TYPE_ROJKADR);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojkadr"
#endif

/* gstreamer looks for this structure to register rojkadrs
 * exchange the string 'Template rojkadr' with your rojkadr description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojkadr,
    "rojkadr",
    rojkadr_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
