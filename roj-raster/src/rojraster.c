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


#include "rojraster.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojraster_debug);
#define GST_CAT_DEFAULT gst_rojraster_debug

enum
{
  PROP_NULL,
  PROP_DTIME,
  PROP_DFREQ,
  PROP_LATENCY,
  PROP_TITTER,
  PROP_FITTER,
  PROP_REASSIGN,
  PROP_RERASTER
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

#define gst_rojraster_parent_class parent_class
G_DEFINE_TYPE (Gstrojraster, gst_rojraster, GST_TYPE_ELEMENT);

static void gst_rojraster_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojraster_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojraster_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojraster_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojraster's class */
static void
gst_rojraster_class_init (GstrojrasterClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojraster_set_property;
  gobject_class->get_property = gst_rojraster_get_property;


  g_object_class_install_property (gobject_class, PROP_DTIME, 
	g_param_spec_double ("dtime", "set dtime", "set dtime", 
   	        0.0, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DFREQ, 
	g_param_spec_double ("dfreq", "set dfreq", "set dfreq", 
		0.0, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LATENCY, 
        g_param_spec_double ("latency", "set latency", "set latency", 
		0.0, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TITTER, 
	g_param_spec_double ("titter", "set jitter", "set jitter", 
		0.0, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FITTER, 
	g_param_spec_double ("fitter", "set jitter", "set jitter", 
		0.0, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_REASSIGN,
      g_param_spec_boolean ("reassign", "on reassign", "on reassign",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RERASTER,
      g_param_spec_boolean ("reraster", "on reraster", "on reraster",
          FALSE, G_PARAM_READWRITE));


  gst_element_class_set_details_simple(gstelement_class, "rojraster",
	"GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojraster_init (Gstrojraster * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojraster_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojraster_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->dtime.flag = FALSE;
  filter->dfreq.flag = FALSE;
  filter->latency = 0.0;

  filter->reassign = FALSE;
  filter->reraster = FALSE;
  filter->titter = 0.0;
  filter->fitter = 0.0;

  groj_init_tags(filter->tags);
  filter->tags_are_send = FALSE;
}

static void
gst_rojraster_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojraster *filter = GST_ROJRASTER (object);

  switch (prop_id) {
  case PROP_DTIME:
    filter->dtime.val = g_value_get_double (value);
    filter->dtime.flag = TRUE;
    break;

  case PROP_DFREQ:
    filter->dfreq.val = g_value_get_double (value);
    filter->dfreq.flag = TRUE;
    break;

  case PROP_LATENCY:
    filter->latency = g_value_get_double (value);
    break;

  case PROP_TITTER:
    filter->titter = g_value_get_double (value);
    break;
  case PROP_FITTER:
    filter->fitter = g_value_get_double (value);
    break;
  case PROP_REASSIGN:
    filter->reassign = g_value_get_boolean (value);
    break;
  case PROP_RERASTER:
    filter->reraster = g_value_get_boolean (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
gst_rojraster_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojraster *filter = GST_ROJRASTER (object);

  switch (prop_id) {
  case PROP_DTIME:
    g_value_set_double (value, filter->dtime.val);
    break;
  case PROP_DFREQ:
    g_value_set_double (value, filter->dfreq.val);
    break;
  case PROP_LATENCY:
    g_value_set_double (value, filter->latency);
    break;
  case PROP_TITTER:
    g_value_set_double (value, filter->titter);
    break;
  case PROP_FITTER:
    g_value_set_double (value, filter->fitter);
    break;
  case PROP_REASSIGN:
    g_value_set_boolean (value, filter->reassign);
    break;
  case PROP_RERASTER:
    g_value_set_boolean (value, filter->reraster);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
groj_rojraster_init(GstObject * parent)
{
  /* do it only onec */
  static gboolean done = FALSE;
  if (done) return;
  done = TRUE;

  Gstrojraster *filter = GST_ROJRASTER (parent);
      
  filter->clushalf = 1+(int)floor(filter->latency / filter->dtime.val);
  filter->cluster = malloc((2*filter->clushalf+1)*sizeof(struct RojBuf *));
  memset(filter->cluster, 0x0, (2*filter->clushalf+1)*sizeof(struct RojBuf *));

  if (filter->cluster==NULL)
    groj_error(gst_element_get_name(parent), "mem");
}

/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojraster_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojraster *filter = GST_ROJRASTER (parent);
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
      
      //      groj_tag_catch_double(taglist, "rate", gst_element_get_name(parent), &filter->tags.rate);	
      //      groj_tag_catch_int(taglist, "width", gst_element_get_name(parent), &filter->tags.width);	
   
      groj_tag_catch_double(taglist, "ofreq", gst_element_get_name(parent), &filter->tags.ofreq);	
      //      groj_tag_catch_double(taglist, "otime", gst_element_get_name(parent), &filter->tags.otime);
      
      if (groj_tag_catch_double(taglist, "leap", gst_element_get_name(parent), &filter->tags.leap))
	forward = FALSE;

      //     if (groj_tag_catch_double(taglist, "hop", gst_element_get_name(parent), &filter->tags.hop))
      //	forward = FALSE;
    
      if (forward) ret = gst_pad_event_default (pad, parent, event);
      else ret = TRUE;

      if (filter->tags.leap.flag && !filter->tags_are_send){
	filter->tags_are_send = TRUE;

	GstTagList * taglist;
	GstEvent * event;
	
	taglist = gst_tag_list_new ("leap", filter->dfreq.val, NULL);	
	event = gst_event_new_tag (taglist);
	gst_pad_push_event (filter->srcpad, event);	
      }

      if (filter->tags.leap.flag)
	groj_rojraster_init(parent);
      break;
    }

  case GST_EVENT_SEGMENT:
    {      
      ret = gst_pad_event_default (pad, parent, event);      
      if (filter->dtime.flag==FALSE)
	groj_error(gst_element_get_name(parent), "no dtime (set property)");
      if (filter->dfreq.flag==FALSE)
	groj_error(gst_element_get_name(parent), "no dfreq (set property)");

      if (filter->reassign==FALSE && filter->reraster==FALSE)
	groj_error(gst_element_get_name(parent), "no output (set reraster or reassign)");
      break;
    }
    
  default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}


struct RojBuf * 
groj_buffer_new(Gstrojraster *filter, int channels, int length, double time)
{
  /* only data channels */

  struct RojBuf *buffer = malloc(sizeof(RojBuf));
  buffer->time = time;
  buffer->mems = malloc(channels*sizeof(GstMemory *));
  buffer->infos = malloc(channels*sizeof(GstMapInfo));
  if (buffer->mems==NULL || buffer->infos==NULL)
    groj_error(gst_element_get_name(filter), "mem");

  int c;
  for(c=0; c<channels; c++){    
    buffer->mems[c] = gst_allocator_alloc (NULL, length*sizeof(double), NULL);
    gst_memory_map (buffer->mems[c], &buffer->infos[c], GST_MAP_READWRITE);
    memset(buffer->infos[c].data, 0x0, length*sizeof(double));
  }

  return buffer;
}

GstBuffer * 
groj_buffer_to_send(struct RojBuf *buffer, int channels)
{  
  GstBuffer *outbuffer = gst_buffer_new ();
  GstMemory *outcmem = groj_config_new();
  groj_config_set_time(outcmem, buffer->time);
  gst_buffer_append_memory (outbuffer, outcmem);

  int c;
  for(c=0; c<channels; c++){    
    gst_memory_unmap (buffer->mems[c], &buffer->infos[c]);      
    gst_buffer_append_memory (outbuffer, buffer->mems[c]);
  }

  return outbuffer;
}


/* chain function this function does the actual processing */
static GstFlowReturn
gst_rojraster_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  gboolean ret = TRUE;
  Gstrojraster *filter = GST_ROJRASTER (parent);

  char channels = gst_buffer_n_memory (buffer);
  char outchannels = 1 + (filter->reraster ? channels-1 : 0)+(filter->reassign ? 1 : 0);

  GstMapInfo info;
  GstMemory *mem = gst_buffer_get_memory (buffer, 1);
  gst_memory_map (mem, &info, GST_MAP_READ);
  int samples = info.size / sizeof(double);
  gst_memory_unmap(mem, &info);

  double efreq = filter->tags.ofreq.val + samples * filter->tags.leap.val;
  int outlen = (efreq - filter->tags.ofreq.val) / filter->dfreq.val;

  GstMemory *cmem = gst_buffer_get_memory (buffer, 0);
  filter->intime = groj_config_get_time(cmem);

  int k, t, f, c;


  int n = 0;
  double lasttime = filter->intime + filter->clushalf*filter->dtime.val;
  while (filter->cluster[n]==NULL && n<2*filter->clushalf+1)
    {      
      filter->cluster[n] = groj_buffer_new(filter, outchannels-1, outlen, lasttime-n*filter->dtime.val);
      n++;
    }
  
  if(filter->reassign){
    if (channels<4)
      groj_error(gst_element_get_name(parent), "too few channels");
    if (channels>=15)
      groj_error(gst_element_get_name(parent), "too many channels");
    
    GstMemory *enmem = gst_buffer_get_memory (buffer, 1);
    GstMemory *lgdmem = gst_buffer_get_memory (buffer, 2);
    GstMemory *cifmem = gst_buffer_get_memory (buffer, 3);
    
    GstMapInfo eninfo, lgdinfo, cifinfo;
    gst_memory_map (enmem, &eninfo, GST_MAP_READ);
    gst_memory_map (lgdmem, &lgdinfo, GST_MAP_READ);
    gst_memory_map (cifmem, &cifinfo, GST_MAP_READ);
    
    c = filter->reraster ? channels-1 : 0;
    for(k=0;k<samples;k++){
      t = round(((double *)lgdinfo.data)[k] / filter->dtime.val + groj_gauss(filter->titter));
      f = round((((double *)cifinfo.data)[k] - filter->tags.ofreq.val) / filter->dfreq.val + groj_gauss(filter->fitter));
      if (t<-filter->clushalf || t>filter->clushalf) continue;
      if (f<0 || f>=outlen) continue;      
      ((double *)filter->cluster[filter->clushalf+t]->infos[c].data)[f] += ((double *)eninfo.data)[k];
    }
    
    gst_memory_unmap (enmem, &eninfo);
    gst_memory_unmap (lgdmem, &lgdinfo);
    gst_memory_unmap (cifmem, &cifinfo);    
  }

  
  /* rest reraster */
  if(filter->reraster){
    for(c=1; c<channels;c++){
      mem = gst_buffer_get_memory (buffer, c);
      gst_memory_map (mem, &info, GST_MAP_READ);

      for(k=0;k<samples;k++){
	t = round(groj_gauss(filter->titter));
	f = round((filter->tags.leap.val*k ) / filter->dfreq.val + groj_gauss(filter->fitter));
	if (t<-filter->clushalf || t>filter->clushalf) continue;
	if (f<0 || f>=outlen) continue;      
	((double *)filter->cluster[filter->clushalf+t]->infos[c-1].data)[f] += ((double *)info.data)[k];

      }

      gst_memory_unmap (mem, &info);    	  
    }
  }

  
  /* reload */
  while(filter->intime-filter->cluster[filter->clushalf]->time > filter->dtime.val/2){
    GstBuffer *outbuffer = groj_buffer_to_send(filter->cluster[2*filter->clushalf], outchannels-1);   
    ret = gst_pad_push (filter->srcpad, outbuffer);

    for(n=2*filter->clushalf-1;n>=0;n--)
      filter->cluster[n+1] = filter->cluster[n];
    filter->cluster[0] = NULL; 
    
    if (filter->cluster[filter->clushalf] == NULL)
      break;
  }
  
  gst_buffer_unref (buffer);
  return ret;
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojraster_init (GstPlugin * rojraster)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojraster' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojraster_debug, "rojraster", 0, "Template rojraster");
  return gst_element_register (rojraster, "rojraster", GST_RANK_NONE, GST_TYPE_ROJRASTER);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojraster"
#endif

/* gstreamer looks for this structure to register rojrasters
 * exchange the string 'Template rojraster' with your rojraster description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojraster,
    "rojraster",
    rojraster_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
