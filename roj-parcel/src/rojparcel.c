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


#include "rojparcel.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojparcel_debug);
#define GST_CAT_DEFAULT gst_rojparcel_debug

enum
{
  PROP_NULL,
  PROP_WIDTH,
  PROP_HOP,
  PROP_OTIME,
  PROP_ETIME
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

#define gst_rojparcel_parent_class parent_class
G_DEFINE_TYPE (Gstrojparcel, gst_rojparcel, GST_TYPE_ELEMENT);

static void gst_rojparcel_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojparcel_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

//static gboolean gst_rojparcel_src_event (GstPad * pad, GstObject * parent, GstEvent * event);
static gboolean gst_rojparcel_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojparcel_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojparcel's class */
static void
gst_rojparcel_class_init (GstrojparcelClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojparcel_set_property;
  gobject_class->get_property = gst_rojparcel_get_property;

  g_object_class_install_property (gobject_class, PROP_WIDTH,
      g_param_spec_int ("width", "width in samples", "width in samples",
          1, G_MAXINT, 1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HOP,
      g_param_spec_int ("hop", "hopsize in samples", "hopsize in samples",
          1, G_MAXINT, 1, G_PARAM_READWRITE));


  g_object_class_install_property (gobject_class, PROP_OTIME, 
	g_param_spec_double ("otime", "set otime", "set otime", 
   	        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ETIME, 
	g_param_spec_double ("etime", "set etime", "set etime", 
   	        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
      "rojparcel", "GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));

  gst_tag_register ("width", GST_TAG_FLAG_META, G_TYPE_INT, "width", "width in samples", NULL);
  gst_tag_register ("hop", GST_TAG_FLAG_META, G_TYPE_INT, "hop", "hop in samples", NULL);
}


/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojparcel_init (Gstrojparcel * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojparcel_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojparcel_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  //  gst_pad_set_event_function (filter->srcpad, GST_DEBUG_FUNCPTR(gst_rojparcel_src_event));
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->current = NULL;
  filter->previous = NULL;
  
  filter->otime.flag = FALSE;
  filter->etime.flag = FALSE;
  filter->width.flag = FALSE;
  filter->hop = 1;

  groj_init_tags(filter->tags);
}

void 
gst_rojparcel_allocate_memory (Gstrojparcel * filter)
{
  if (!filter->width.flag)
    groj_error(gst_element_get_name(filter), "first set width");

  if (filter->current!=NULL)
    free(filter->current);
  if (filter->previous!=NULL)
    free(filter->previous);

  filter->current = malloc((2+filter->width.val)*sizeof(fftw_complex));
  filter->previous = malloc((2+filter->width.val)*sizeof(fftw_complex));
  if (filter->current==NULL || filter->previous==NULL) 
    groj_error(gst_element_get_name(filter), "cannot allocate memory");
}

static void
gst_rojparcel_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojparcel *filter = GST_ROJPARCEL (object);

  switch (prop_id) {
  case PROP_WIDTH:
    {
      filter->width.val = g_value_get_int (value);
      filter->width.flag = TRUE;
      
      gst_rojparcel_allocate_memory (filter);    
      filter->necessary = 2+filter->width.val;
      filter->loaded = 0;
      
      groj_tag_emit_int(gst_element_get_name(filter), filter->srcpad, "width", filter->width.val);
    }
    break;

  case PROP_HOP:
    filter->hop = g_value_get_int (value);
    groj_tag_emit_int(gst_element_get_name(filter), filter->srcpad, "hop", filter->hop);
    break;
    
  case PROP_OTIME:
    filter->otime.val = g_value_get_double (value);
    filter->otime.flag = TRUE;
    break;
  case PROP_ETIME:
    filter->etime.val = g_value_get_double (value);
    filter->etime.flag = TRUE;
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
gst_rojparcel_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojparcel *filter = GST_ROJPARCEL (object);
  
  switch (prop_id) {
  case PROP_HOP:
    g_value_set_int (value, filter->hop);
    break;
  case PROP_WIDTH:
    g_value_set_int (value, filter->width.val);
    break;
  case PROP_OTIME:
    g_value_set_double (value, filter->otime.val);
    break;
  case PROP_ETIME:
    g_value_set_double (value, filter->etime.val);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

/*
static gboolean
gst_rojparcel_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  
  Gstrojparcel *filter = GST_ROJPARCEL (gst_pad_get_parent (pad));
  g_print ("* %s: (up)event  %s\n",
	   gst_element_get_name(filter), gst_event_type_get_name(GST_EVENT_TYPE (event))); 
  gboolean  res = gst_pad_event_default (pad, parent, event);
  return res;
}
*/

/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojparcel_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojparcel *filter = GST_ROJPARCEL (parent);
  g_print ("* %s: event %s\n",
	   gst_element_get_name(parent), gst_event_type_get_name(GST_EVENT_TYPE (event))); 
  
  gboolean ret;
  switch (GST_EVENT_TYPE (event)) {
  case GST_EVENT_CAPS:
    {
      GstEvent *ovent = groj_caps_event(parent, event, GROJ_CAPS_SIGNAL, GROJ_CAPS_FRAME);
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

  case GST_EVENT_STREAM_START:
    {
      ret = gst_pad_event_default (pad, parent, event);
      groj_tag_emit_int(gst_element_get_name(filter), filter->srcpad, "width", filter->width.val);
      groj_tag_emit_int(gst_element_get_name(filter), filter->srcpad, "hop", filter->hop);
      break;
    }

  case GST_EVENT_SEGMENT:
    {
      ret = gst_pad_event_default (pad, parent, event);
      if (!filter->width.flag)
      	groj_error(gst_element_get_name(parent), "set width");
      break;
    }

  default:
    ret = gst_pad_event_default (pad, parent, event);
  }
  return ret;
}

/* CHAIN >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
static GstFlowReturn
gst_rojparcel_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojparcel *filter = GST_ROJPARCEL (parent);

  GstMemory *cmem = gst_buffer_get_memory (buffer, 0);
  double time = groj_config_get_time(cmem);
  double half = 0.5*(double)(filter->width.val+2) / filter->tags.rate.val;

  /* sprawdzanie czy początek przychodzącego buffora jest większy of etime */
  if(filter->etime.flag && time>filter->etime.val+half){
    gst_buffer_unref (buffer);
    return GST_FLOW_EOS;
  }

  GstMapInfo info;
  GstMemory *mem = gst_buffer_get_memory (buffer, 1);
  gst_memory_map (mem, &info, GST_MAP_READ);

  int available = info.size / sizeof(fftw_complex);
  double last = time + available / filter->tags.rate.val;  /* czas ostatniej próbki w przychodzącym bloku */

  /* sprawdzanie czy koniec przychodzącego buffora jest miejszy od otime */
  if(filter->otime.flag && last<filter->otime.val-half){
    gst_memory_unmap (mem, &info);
    gst_buffer_unref (buffer);    
    return 0;
  }
  
  int next2send = 0;
  while(available>0){ 

    if(available>=filter->necessary){ /* <--------------------------------------------------------------------------------------------- */
      memcpy(
	     &filter->current[filter->loaded],
	     &((fftw_complex*)info.data)[next2send],
	     filter->necessary*sizeof(fftw_complex)
	     );
      
      available -= filter->necessary;
      next2send += filter->necessary;
      
      filter->loaded += filter->necessary;
      filter->necessary = 0;

      /* wyslanie */

      GstBuffer * outbuffer = gst_buffer_new ();
      GstMemory * outmem = gst_allocator_alloc (NULL, (2+filter->width.val)*sizeof(fftw_complex), NULL);
      GstMemory *outcmem = groj_config_new();

      double outtime = time + (double)next2send / filter->tags.rate.val - half; 
      groj_config_set_time(outcmem, outtime);
      gst_buffer_append_memory (outbuffer,outcmem);
      gst_buffer_append_memory (outbuffer,outmem);


      GstMapInfo outinfo;
      gst_memory_map (outmem, &outinfo, GST_MAP_WRITE);
      memcpy(outinfo.data, filter->current, (2+filter->width.val)*sizeof(fftw_complex));
      gst_memory_unmap (outmem, &outinfo);
      
      if(outtime<filter->otime.val){
	gst_buffer_unref (outbuffer);
      }
      else{
	if (outtime>filter->etime.val && filter->etime.flag){
	  gst_buffer_unref (outbuffer);
	  return GST_FLOW_EOS;
	}
	gst_pad_push (filter->srcpad, outbuffer);
      }
          
      fftw_complex * tmp = filter->previous; 
      filter->previous = filter->current;
      filter->current = tmp;
      
      if(filter->hop<(2+filter->width.val)){ 
	memcpy(
	       &filter->current[0],
	       &filter->previous[filter->hop],
	       (2+filter->width.val-filter->hop)*sizeof(fftw_complex)
	       );

	filter->loaded = 2+filter->width.val-filter->hop;
	filter->necessary = filter->hop;
      }
      else{
	filter->loaded = 0;
	filter->necessary = 2+filter->width.val;
      }

    }
    else{ /* <--------------------------------------------------------------------------------------------- */

      memcpy(
	     &filter->current[filter->loaded],
	     &((fftw_complex*)info.data)[next2send],
	     available*sizeof(fftw_complex)
	     );

      filter->loaded += available;
      filter->necessary -= available;

      available = 0;
      next2send = 0;
    } /* <--------------------------------------------------------------------------------------------- */

  }

  gst_memory_unmap (mem, &info);
  gst_buffer_unref (buffer);
  return 0; 
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojparcel_init (GstPlugin * rojparcel)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojparcel' with your description
   */

  GST_DEBUG_CATEGORY_INIT (gst_rojparcel_debug, "rojparcel", 0, "rojparcel");
  return gst_element_register (rojparcel, "rojparcel", GST_RANK_NONE, GST_TYPE_ROJPARCEL);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojparcel"
#endif

/* gstreamer looks for this structure to register rojparcels
 * exchange the string 'Template rojparcel' with your rojparcel description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojparcel,
    "rojparcel",
    rojparcel_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
