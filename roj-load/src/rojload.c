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


#include "rojload.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojload_debug);
#define GST_CAT_DEFAULT gst_rojload_debug

enum
{
  FORMAT_UKNOWN,
  FORMAT_WAV,
  FORMAT_TXT
};

enum
{
  PROP_NULL,
  PROP_BLOCK,
  PROP_CHANNEL,
  PROP_LOCATION,
  PROP_START,
  PROP_RATE,
  PROP_COMPLEX
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

#define gst_rojload_parent_class parent_class
G_DEFINE_TYPE (Gstrojload, gst_rojload, GST_TYPE_ELEMENT);

static void gst_rojload_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojload_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojload_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojload_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojload's class */
static void
gst_rojload_class_init (GstrojloadClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojload_set_property;
  gobject_class->get_property = gst_rojload_get_property;


  g_object_class_install_property (gobject_class, PROP_COMPLEX,
      g_param_spec_boolean ("complex", "complex", "complex stream",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LOCATION, 
	g_param_spec_string ("location", "file name", "file name", 
		NULL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHANNEL, 
	g_param_spec_int ("channel", "channel", "channel",
		0, 1023, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BLOCK, 
	g_param_spec_int ("block", "data block", "number of samples in single block",
		1, 134217727, 1024, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RATE, 
	g_param_spec_double ("rate", "sampling rate", "sampling rate",
		0.0, G_MAXDOUBLE, 1000.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_START, 
	g_param_spec_double ("start", "instant start", "signal start", 
		-G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));


  gst_element_class_set_details_simple(gstelement_class,
      "rojload", "GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));

  gst_tag_register ("rate", GST_TAG_FLAG_META, G_TYPE_DOUBLE, "rate", "sampling rate", NULL);
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojload_init (Gstrojload * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojload_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojload_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->location = NULL;
  filter->channel = 0;
  filter->block= 1024;

  filter->time = 0.0;  
  filter->start = 0.0;  
  filter->cmplx = FALSE;
  filter->rate = 1000.0;

  filter->number = 0;
}

static void
gst_rojload_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojload *filter = GST_ROJLOAD (object);

  switch (prop_id) {
  case PROP_COMPLEX:
    filter->cmplx = g_value_get_boolean (value);
    break;
    
  case PROP_START:
    filter->start = g_value_get_double (value);
    filter->time = g_value_get_double (value);
    break;

  case PROP_RATE:
    filter->rate = g_value_get_double (value);
    groj_tag_emit_double(gst_element_get_name(filter), filter->srcpad, "rate", filter->rate);
    break;

  case PROP_BLOCK:
    filter->block = g_value_get_int (value);
    break;

  case PROP_CHANNEL:
    filter->channel = g_value_get_int (value);
    break;

  case PROP_LOCATION:
    {
      int len = strlen((char *) g_value_get_string (value));	
      filter->location = malloc(len*sizeof(char));
      sprintf(filter->location, "%s", (char *) g_value_get_string (value));
      
      filter->format = FORMAT_UKNOWN;
      if(strcmp(&filter->location[len-4], ".wav")==0 || strcmp(&filter->location[len-4], ".WAV")==0)
	filter->format = FORMAT_WAV;
      if(strcmp(&filter->location[len-4], ".txt")==0 || strcmp(&filter->location[len-4], ".TXT")==0)
	filter->format = FORMAT_TXT;

      if(filter->format == FORMAT_UKNOWN)
	groj_error(gst_element_get_name(object), "not known file type");      
      break;
    }
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
gst_rojload_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojload *filter = GST_ROJLOAD (object);

  switch (prop_id) {
  case PROP_COMPLEX:
    g_value_set_boolean (value, filter->cmplx);
    break;

  case PROP_RATE:
    g_value_set_double (value, filter->rate);
    break;

  case PROP_START:
    g_value_set_double (value, filter->start);
    break;

  case PROP_BLOCK:
    g_value_set_int (value, filter->block);
    break;

  case PROP_CHANNEL:
    g_value_set_int (value, filter->channel);
    break;

  case PROP_LOCATION:
    g_value_set_string (value, filter->location);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojload_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojload *filter = GST_ROJLOAD (parent);
  g_print ("* %s: event %s\n",
	   gst_element_get_name(filter), gst_event_type_get_name(GST_EVENT_TYPE (event))); 

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

      /* rest of start */

      switch(filter->format){
      case FORMAT_WAV:
	{
	  SF_INFO info;
	  filter->sndfile = sf_open(filter->location, SFM_READ, &info);
	  if (filter->sndfile == NULL) 
	    groj_error(gst_element_get_name(parent), (char *)sf_strerror(filter->sndfile));
      
	  filter->rate = info.samplerate;
	  filter->channels = info.channels;
	  filter->samples = info.frames;
	  if (filter->block>filter->samples)
	    filter->block = filter->samples;
            
	  filter->buffer = malloc(filter->block * filter->channels * sizeof(double));
	  if (filter->buffer == NULL)
	    groj_error(gst_element_get_name(parent), "cannot allocate memory");
      
	  if(filter->channel>=info.channels)
	    groj_error(gst_element_get_name(parent), "wrong channel");

	  break;
	}
      case FORMAT_TXT:
	{
	  putenv("LC_ALL=C");  
	  filter->txtfile = fopen(filter->location, "r");
	  if (filter->txtfile == NULL) 
	    groj_error(gst_element_get_name(parent), "cannot open txt file");
	  break;
	}
      }
      
      /* emit rate tag */

      groj_tag_emit_double(gst_element_get_name(filter), filter->srcpad, "rate", filter->rate);

      break;
    }

  case GST_EVENT_EOS:
    {
      ret = gst_pad_event_default (pad, parent, event);

      switch(filter->format){
      case FORMAT_WAV:
	sf_close(filter->sndfile);
	break;
      case FORMAT_TXT:
	fclose(filter->txtfile);
	break;
      }

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
gst_rojload_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojload *filter = GST_ROJLOAD (parent);

  GstMemory *mem = gst_buffer_get_all_memory (buffer);
  if (mem!=NULL) g_print("Warning memory are not empty\n");
  gst_buffer_unref(buffer);

  GstMapInfo info;
  GstBuffer * outbuffer = gst_buffer_new ();

  /* W pierwszym slocie wysyłany jest config */
  GstMemory *cmem = groj_config_new();
  groj_config_set_time(cmem, filter->time);
  groj_config_set_number(cmem, filter->number);
  filter->number++;
  gst_buffer_append_memory (outbuffer, cmem);

  int n, number=0;
  switch(filter->format){
  case FORMAT_WAV:
    {
      number = sf_readf_double(filter->sndfile, filter->buffer, filter->block);
      if (number==0)
	return GST_FLOW_EOS;

      mem = gst_allocator_alloc (NULL, number*sizeof(fftw_complex), NULL);
      gst_memory_map (mem, &info, GST_MAP_WRITE);
      memset(info.data, 0x0, number*sizeof(fftw_complex));

      for(n=0; n<number; n++)
	((fftw_complex *)info.data)[n] = filter->buffer[n*filter->channels+filter->channel]+0.0*I;

      gst_memory_unmap (mem, &info);    
      gst_buffer_append_memory (outbuffer, mem);
      break;
    }
    
  case FORMAT_TXT:
    {
      fflush(filter->txtfile);
      if (feof(filter->txtfile)){
	gst_buffer_unref(outbuffer);
	return GST_FLOW_EOS;
      }
      
      gdouble re, im;
      fftw_complex *tmp = malloc(filter->block * sizeof(fftw_complex));
      if (tmp == NULL) groj_error(gst_element_get_name(parent), "mem");      
      
      number = 0;
      for(n=0; n<filter->block; n++){
	number++;
	
	fscanf(filter->txtfile, " %lf ", &re);
	if(filter->cmplx && feof(filter->txtfile)==0)
	  fscanf(filter->txtfile, " %lf ", &im);	
	else im = 0.0;

	tmp[n] = re + I*im;
	if (feof(filter->txtfile)) break;  	  
      }
      
      mem = gst_allocator_alloc (NULL, number*sizeof(fftw_complex), NULL);
      gst_memory_map (mem, &info, GST_MAP_WRITE);
      memcpy(info.data, tmp, number*sizeof(fftw_complex));
      free(tmp);

      gst_memory_unmap (mem, &info);    
      gst_buffer_append_memory (outbuffer, mem);
      break;
    }

  default:
    groj_error(gst_element_get_name(parent), "not known file type");      
  }

  filter->time += (float)number/filter->rate;
  return gst_pad_push (filter->srcpad, outbuffer);
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojload_init (GstPlugin * rojload)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojload' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojload_debug, "rojload", 0, "Template rojload");
  return gst_element_register (rojload, "rojload", GST_RANK_NONE, GST_TYPE_ROJLOAD);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojload"
#endif

/* gstreamer looks for this structure to register rojloads
 * exchange the string 'Template rojload' with your rojload description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojload,
    "rojload",
    rojload_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
