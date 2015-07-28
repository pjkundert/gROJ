/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2015 Dorian Dabrowski <dorian.dabrowski@gmail.com >
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


#include "rojolsa.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojolsa_debug);
#define GST_CAT_DEFAULT gst_rojolsa_debug

#define PCM_DEVICE "default"

enum
{
  PROP_NULL
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

#define gst_rojolsa_parent_class parent_class
G_DEFINE_TYPE (Gstrojolsa, gst_rojolsa, GST_TYPE_ELEMENT);

static void gst_rojolsa_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojolsa_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojolsa_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojolsa_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojolsa's class */
static void
gst_rojolsa_class_init (GstrojolsaClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojolsa_set_property;
  gobject_class->get_property = gst_rojolsa_get_property;

  gst_element_class_set_details_simple(gstelement_class,
    "rojolsa", "GSTroj:Generic", "GSTroj:Generic",
    "Krzysztof Czarnecki & Dorian Dabrowski");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojolsa_init (Gstrojolsa * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojolsa_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojolsa_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);
  
  filter->pcm = NULL;
  groj_init_tags(filter->tags);
}

static void
gst_rojolsa_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  //  Gstrojolsa *filter = GST_ROJOLSA (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rojolsa_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  //  Gstrojolsa *filter = GST_ROJOLSA (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojolsa_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojolsa *filter = GST_ROJOLSA (parent);
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
  case GST_EVENT_TAG:
    {
      ret = gst_pad_event_default (pad, parent, event);
      
      GstTagList *taglist;
      gst_event_parse_tag(event, &taglist);
      if(groj_tag_catch_double(taglist, "rate", gst_element_get_name(parent), &filter->tags.rate)){

	if (filter->pcm!=NULL){
	  snd_pcm_drain(filter->pcm);
	  snd_pcm_close(filter->pcm); 
	}

	int ret = snd_pcm_open(&filter->pcm, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0); // 
	if (ret<0) groj_error(gst_element_get_name(filter), (char *)snd_strerror(ret));  
	
	snd_pcm_hw_params_alloca(&filter->params); 
	snd_pcm_hw_params_any(filter->pcm, filter->params);
	
	ret = snd_pcm_hw_params_set_access(filter->pcm, filter->params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (ret<0) groj_error(gst_element_get_name(filter), (char *)snd_strerror(ret));  
	
	ret = snd_pcm_hw_params_set_format(filter->pcm, filter->params, SND_PCM_FORMAT_S16_LE);
	if (ret<0) groj_error(gst_element_get_name(filter), (char *)snd_strerror(ret));  
	
	ret = snd_pcm_hw_params_set_channels(filter->pcm, filter->params, 2);
	if (ret<0) groj_error(gst_element_get_name(filter), (char *)snd_strerror(ret));  
	
	int dir;
	unsigned int rate = filter->tags.rate.val;
	ret = snd_pcm_hw_params_set_rate_near(filter->pcm, filter->params, &rate, &dir);
	//ret = snd_pcm_hw_params_set_rate_resample (filter->pcm, filter->params, rate);
	if (ret<0) groj_error(gst_element_get_name(filter), (char *)snd_strerror(ret));  
	
	ret = snd_pcm_hw_params(filter->pcm, filter->params);
	if (ret<0) groj_error(gst_element_get_name(filter), (char *)snd_strerror(ret));  
	
//	  snd_pcm_uframes_t frames;
//	  snd_pcm_hw_params_get_period_size(filter->params, &frames, 0);
//	  printf("OK %d\n", frames);
//
//	  filter->size = frames * channels * 2 /* 2 -> sample size */;
//	  filter->buff = (char *) malloc(buff_size); 
//	  if (filter->buff==NULL) groj_error(gst_element_get_name(filter), (char *)snd_strerror(ret));  
      }
      
      break;
    }
  case GST_EVENT_EOS:
    {
      ret = gst_pad_event_default (pad, parent, event);

      snd_pcm_drain(filter->pcm);
      snd_pcm_close(filter->pcm); 
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
gst_rojolsa_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojolsa * filter = GST_ROJOLSA (parent);

  GstMapInfo info;
  GstMemory *mem = gst_buffer_get_memory (buffer, 1);
  gst_memory_map (mem, &info, GST_MAP_READ);
  int n, size = info.size / sizeof(fftw_complex);

  short *alsabuff = malloc(size*2*sizeof(short));
  if (alsabuff==NULL) groj_error(gst_element_get_name(filter), "mem");  
 
  for (n=0;n<size;n++){
    alsabuff[2*n] = creal(((fftw_complex *)info.data)[n]);
    alsabuff[2*n+1] = cimag(((fftw_complex *)info.data)[n]);
  }


//  if (pcm = read(0, buff, buff_size) == 0) {
//    printf("Early end of file.\n");
//    return 0;
//  }
 
  int ret;
  ret = snd_pcm_writei(filter->pcm, alsabuff, size);
  //  fprintf(stderr, "%d \n", ret);

  if (ret  == -EPIPE) {
    printf("XRUN.\n");
    snd_pcm_prepare(filter->pcm);
  } else 
    if (ret<0) groj_error(gst_element_get_name(filter), (char *)snd_strerror(ret));  

  free(alsabuff);
  gst_memory_unmap (mem, &info);
  return gst_pad_push (filter->srcpad, buffer);
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojolsa_init (GstPlugin * rojolsa)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojolsa' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojolsa_debug, "rojolsa", 0, "Template rojolsa");
  return gst_element_register (rojolsa, "rojolsa", GST_RANK_NONE, GST_TYPE_ROJOLSA);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojolsa"
#endif

/* gstreamer looks for this structure to register rojolsas
 * exchange the string 'Template rojolsa' with your rojolsa description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojolsa,
    "rojolsa",
    rojolsa_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
