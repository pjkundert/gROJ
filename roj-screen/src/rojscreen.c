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


#include "rojscreen.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GST_DEBUG_CATEGORY_STATIC (gst_rojscreen_debug);
#define GST_CAT_DEFAULT gst_rojscreen_debug

enum
{
  MODE_LIN,
  MODE_LOG
};

enum
{
  PROP_NULL,
  PROP_MODE,
  PROP_WSCR,
  PROP_HSCR,
  PROP_CHANNEL,
  PROP_UPDATE,
  PROP_OVAL,
  PROP_EVAL
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

#define gst_rojscreen_parent_class parent_class
G_DEFINE_TYPE (Gstrojscreen, gst_rojscreen, GST_TYPE_ELEMENT);

static void gst_rojscreen_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_rojscreen_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_rojscreen_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_rojscreen_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */
/* initialize the rojscreen's class */
static void
gst_rojscreen_class_init (GstrojscreenClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rojscreen_set_property;
  gobject_class->get_property = gst_rojscreen_get_property;

  g_object_class_install_property (gobject_class, PROP_OVAL, 
	g_param_spec_double ("oval", "oval", "oval",
		-G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_EVAL, 
	g_param_spec_double ("eval", "eval", "eval",
		-G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MODE, 
	g_param_spec_string ("mode", "mode", "mode", 
		"lin", G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHANNEL, 
	g_param_spec_int ("channel", "channel", "channel to display",
		0, 15, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_UPDATE, 
	g_param_spec_int ("update", "update", "update",
		0, G_MAXINT, 10, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_WSCR, 
	g_param_spec_int ("wscr", "wscr", "width of screen",
		1, 8192, 800, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HSCR, 
	g_param_spec_int ("hscr", "hscr", "height of screen",
		1, 8192, 800, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
      "rojscreen", "GSTroj:Generic", "GSTroj:Generic", "Krzysztof Czarnecki <czarnecki.krzysiek@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

void gst_rojscreen_memory_free(Gstrojscreen *filter)
{
  int w;
  if(filter->rgbdata!=NULL){
    free(filter->rgbdata);
    filter->rgbdata = NULL;
  }

  if(filter->data!=NULL){
    for(w=0; w<filter->wscr; w++)
      if (filter->data[w]!=NULL){
	free(filter->data[w]);
	filter->data[w] = NULL;
      }
    free(filter->data);
    filter->data = NULL;
  }

  if(filter->maxima!=NULL){
    free(filter->maxima);
    filter->maxima = NULL;
  }
  if(filter->minima!=NULL){
    free(filter->minima);
    filter->minima = NULL;
  }
  if(filter->times!=NULL){
    free(filter->times);
    filter->times = NULL;
  }
}

void gst_rojscreen_memory_allocation(Gstrojscreen *filter)
{
  int w;

  filter->data = malloc(filter->wscr*sizeof(double *));
  if(filter->data==NULL) groj_error(gst_element_get_name(filter), "mem");      
  memset(filter->data, 0x0, filter->wscr*sizeof(double *));
  filter->maxima = malloc(filter->wscr*sizeof(double));
  if(filter->maxima==NULL) groj_error(gst_element_get_name(filter), "mem");      
  memset(filter->maxima, 0x0, filter->wscr*sizeof(double));
  filter->minima = malloc(filter->wscr*sizeof(double));
  if(filter->minima==NULL) groj_error(gst_element_get_name(filter), "mem");      
  memset(filter->minima, 0x0, filter->wscr*sizeof(double));
  filter->times = malloc(filter->wscr*sizeof(double));
  if(filter->times==NULL) groj_error(gst_element_get_name(filter), "mem");      
  memset(filter->times, 0x0, filter->wscr*sizeof(double));

  for(w=0; w<filter->wscr; w++){
    filter->data[w] = malloc(filter->hscr*sizeof(double));
    if(filter->data[w]==NULL) groj_error(gst_element_get_name(filter), "mem");      
    memset(filter->data[w], 0x0, filter->hscr*sizeof(double));
  }
  int size = 3*filter->wscr*filter->hscr;
  filter->rgbdata = malloc(size*sizeof(unsigned char));
  memset(filter->rgbdata, 0x88, size*sizeof(unsigned char));

  int x,y; /* startowy screen */
  for(x=0; x<filter->wscr; x++){
    for(y=0; y<filter->hscr; y++){
	  
      filter->rgbdata[(x+y*filter->wscr)*3] = (x+y)%256;
      filter->rgbdata[(x+y*filter->wscr)*3+1] = (x+2*y)%256;
      filter->rgbdata[(x+y*filter->wscr)*3+2] = (2*x+y)%256;  
    }
  }
}

void
gst_rojscreen_refresh(Gstrojscreen * filter, char *save)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data((const guchar *)filter->rgbdata, GDK_COLORSPACE_RGB,
					       FALSE, 8, filter->wscr, filter->hscr, 3*filter->wscr, NULL, NULL);
  gtk_image_set_from_pixbuf (GTK_IMAGE(filter->image), pixbuf);  
  if(save!=NULL) gdk_pixbuf_savev (pixbuf, save, "png", NULL, NULL, NULL);
  g_object_unref(G_OBJECT(pixbuf));    
  while (gtk_events_pending ())
    gtk_main_iteration ();  
}

void
gst_screen_color_lin(Gstrojscreen *filter)
{
  double min = filter->min;
  if (filter->oval.flag) 
    min = filter->oval.val;

  double max = filter->max;
  if (filter->eval.flag) 
    max = filter->eval.val;

  int xi, x, y, e;
  int init = filter->wscr - filter->stored;
  int current = filter->current-1-filter->stored+2*filter->wscr;
  
  double val;
  for(x=0; x<filter->stored; x++){
    xi = (current+x+2)%filter->wscr;
    for(y=0; y<filter->hscr; y++){
      e = (init + x + (filter->hscr-y-1)*filter->wscr)*3;
      val = (filter->data[xi][y]-min)/(max-min);
      filter->palette(val, &filter->rgbdata[e], filter->diffout);
    }    
  }
}

void
gst_screen_color_log(Gstrojscreen *filter)
{
  double max = filter->max;
  if (filter->max<0)
    groj_error(gst_element_get_name(filter), "log scale can not be negative");      
  double logmax = 10*log10(max);
  
  double min = 10*log10(filter->min);
  if (filter->oval.flag) 
    min = filter->oval.val;
  
  int xi, x, y, e;
  int init = filter->wscr - filter->stored;
  int current = filter->current-1-filter->stored+2*filter->wscr;

  for(x=0; x<filter->stored; x++){
    xi = (current+x+2)%filter->wscr;
    for(y=0; y<filter->hscr; y++){
      e = (init + x + (filter->hscr-y-1)*filter->wscr)*3;
            
      double val = filter->data[xi][y]<0 ? -filter->data[xi][y] : filter->data[xi][y];
      val = 10*log10(val) - logmax - min;
      val = -val / min;
      if (isnan(val))
	val = -1;
      filter->palette(val, &filter->rgbdata[e], filter->diffout);
    }
  }
}

void
gst_rojscreen_update(Gstrojscreen *filter)
{
  int n;
  filter->min = filter->minima[0];
  filter->max = filter->maxima[0];
  for(n=0;n<filter->stored;n++){    
    if(filter->min>filter->minima[n]) filter->min=filter->minima[n];
    if(filter->max<filter->maxima[n]) filter->max=filter->maxima[n];
  }
  
  switch(filter->mode){
  case MODE_LIN:
    gst_screen_color_lin(filter);
    break;
  case MODE_LOG:
    gst_screen_color_log(filter);
    break;
  default:
    groj_error(gst_element_get_name(filter), "not known mode");      
  }
}

void
gst_rojscreen_destroy(GtkWidget *ignored, GstObject *parent)
{
  Gstrojscreen *filter = GST_ROJSCREEN (parent);
  if(filter->eof)
    gtk_main_quit ();
  filter->eof = TRUE;  
}

void
gst_rojscreen_export(Gstrojscreen *filter)
{
  mkdir("__groj", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  int n;
  unsigned char *rgbscale = malloc(768*sizeof(char));
  for(n=0; n<256; n++) filter->palette((float)(255-n)/256.0, &rgbscale[3*n], 0);

  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data((const guchar *)rgbscale, GDK_COLORSPACE_RGB, FALSE, 8, 1, 256, 3, NULL, NULL);
  gdk_pixbuf_savev (pixbuf, "__groj/_scale.png", "png", NULL, NULL, NULL);
  g_object_unref(G_OBJECT(pixbuf));

  filter->palette(-1.0, rgbscale, 1);
  pixbuf = gdk_pixbuf_new_from_data((const guchar *)rgbscale, GDK_COLORSPACE_RGB, FALSE, 8, 1, 1, 3, NULL, NULL);
  gdk_pixbuf_savev (pixbuf, "__groj/_out.png", "png", NULL, NULL, NULL);
  g_object_unref(G_OBJECT(pixbuf));
  free(rgbscale);

  FILE *fds = fopen("__groj/_config.txt", "w");
  switch(filter->mode){
  case MODE_LIN: fprintf(fds, "lin\n"); break;
  case MODE_LOG: fprintf(fds, "log\n"); break;
  default: fprintf(fds, "err\n");
  }
  if(filter->diffout)
    fprintf(fds, "out\n");
  else
    fprintf(fds, "noout\n");  
	  
  int xi = (filter->current-filter->stored+2*filter->wscr)%filter->wscr;
  int xii = (filter->current-1+filter->wscr)%filter->wscr;

  double otime = filter->times[xi];
  double etime = filter->times[xii];

  fprintf(fds, "%g\n", otime);
  fprintf(fds, "%g\n", etime);

  double ofreq = filter->tags.ofreq.val;
  double efreq = filter->tags.ofreq.val + ((double)filter->hscr - 1.0)*filter->tags.leap.val;
  fprintf(fds, "%g\n", ofreq);
  fprintf(fds, "%g\n", efreq);

  double min = filter->min; 
  double max = filter->max;
  if (filter->mode == MODE_LOG){
    min = filter->oval.val;
    max = 0.0;
  }
  if (filter->mode == MODE_LIN){
    if (filter->oval.flag) min = filter->oval.val;
    if (filter->eval.flag) max = filter->eval.val;
  }
  fprintf(fds, "%g\n", min);
  fprintf(fds, "%g\n", max);
  fclose(fds);
}

gboolean
gst_rojscreen_press(GtkWidget *widget, GdkEventKey *kevent, GstObject *parent)
{
  if (kevent->type == GDK_KEY_PRESS){
    Gstrojscreen *filter = GST_ROJSCREEN (parent);
    gboolean refresh = FALSE;
 
    switch(kevent->keyval){
    case '/':
      g_print("diffout\n");
      refresh = TRUE;
      filter->diffout++;
      filter->diffout%=2;
      break;

    case '0':
      g_print("reset\n");
      refresh = TRUE;
      if (filter->mode == MODE_LIN) filter->jump = 0.1;
      else filter->jump = 30.0;      
      filter->oval.flag = FALSE;
      filter->eval.flag = FALSE;
      break;

    case '1':
      refresh = TRUE;
      filter->oval.flag = TRUE;
      filter->oval.val -= filter->jump;
      g_print("low lim -  <%g %g>\n", filter->oval.val, filter->eval.val);
      break;
    case '2':
      refresh = TRUE;
      filter->oval.flag = TRUE;
      filter->oval.val += filter->jump;
      refresh = TRUE;      
      if (filter->mode == MODE_LOG && filter->oval.val>=-filter->jump){
	filter->oval.val -= filter->jump;
	refresh = FALSE;
      }
      if (filter->mode == MODE_LIN && filter->oval.val>=filter->eval.val-filter->jump){
	filter->oval.val -= filter->jump;
	refresh = FALSE;
      }
      g_print("low lim +  <%g %g>\n", filter->oval.val, filter->eval.val);
      break;

    case '4':
      filter->jump /= 10;
      g_print("jump: %g\n", filter->jump);
      break;
    case '5':
      filter->jump *= 10;
      g_print("jump: %g\n", filter->jump);
      break;

    case '7':
      refresh = TRUE;
      filter->eval.flag = TRUE;
      filter->eval.val -= filter->jump;
      if (filter->mode == MODE_LIN && filter->oval.val>=filter->eval.val-filter->jump){
	filter->eval.val += filter->jump;
	refresh = FALSE;
      }
      g_print("high lim -   <%g %g>\n", filter->oval.val, filter->eval.val);
      break;
    case '8':
      refresh = TRUE;
      filter->eval.flag = TRUE;
      filter->eval.val += filter->jump;
      g_print("high lim +   <%g %g>\n", filter->oval.val, filter->eval.val);
      break;

    case 'c':
      g_print("color\n");
      refresh = TRUE;
      filter->color++;
      filter->color%=PALETTE_NUMBER;

	switch(filter->color){
	case 0: filter->palette = &groj_palette_rgb; break;
	case 1: filter->palette = &groj_palette_old; break;
	case 2: filter->palette = &groj_palette_warm; break;
	case 3: filter->palette = &groj_palette_gray; break;
	case 4: filter->palette = &groj_palette_igray; break;
	case 5: filter->palette = &groj_palette_phase; break;
	case 6: filter->palette = &groj_palette_abc; break;
	}

      break;

    case ' ':
      {
	g_print("save\n");
	gst_rojscreen_export(filter);
	gst_rojscreen_refresh(filter, "__groj/_screen.png");
      }
    }   

    if(refresh){
      gst_rojscreen_update(filter);
      gst_rojscreen_refresh(filter, NULL);
    }
  }
  
  return FALSE;
}

gboolean gst_rojscreen_push(GtkWidget *widget, GdkEventButton * event, Gstrojscreen *filter)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      int begin = filter->wscr - filter->stored;
      int first = (filter->current-filter->stored+filter->wscr)%filter->wscr;
      int xi = ((int)event->x-begin+first+filter->wscr)%filter->wscr;

      double time = filter->times[xi];
      double freq = filter->tags.ofreq.val + ((double)filter->hscr - event->y - 1.0)*filter->tags.leap.val;
      
      g_print("clk (%d)>> %g (s)  x  %g (Hz)  =  %g\n", event->button, time, freq, filter->data[xi][filter->hscr - (int)event->y - 1]);
    }
  
  return FALSE;
}

/* initialize the new element instantiate pads and add them to element
 * set pad calback functions initialize instance structure
 */
static void
gst_rojscreen_init (Gstrojscreen * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojscreen_sink_event));
  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_rojscreen_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->channel = 1;
  filter->mode = MODE_LIN;

  filter->wscr = 800;
  filter->hscr = 500;
  filter->update = 10;

  filter->data=NULL;
  filter->times=NULL;
  filter->maxima=NULL;
  filter->minima=NULL;
  filter->rgbdata=NULL;

  gst_rojscreen_memory_allocation(filter);

  filter->oval.flag = FALSE;
  filter->eval.flag = FALSE;

  filter->eof = FALSE;
  filter->jump = 0.1;

  filter->stored = 0;
  filter->current = 0;
  filter->allreceived = 0;

  filter->color = 1;
  filter->diffout = 0;
  filter->palette = &groj_palette_old;  

  groj_init_tags(filter->tags);
  gtk_init(0, NULL);

  filter->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (filter->window), "GROJ >>");
  g_signal_connect(G_OBJECT(filter->window), "destroy", G_CALLBACK(gst_rojscreen_destroy), filter);
  g_signal_connect(G_OBJECT(filter->window), "key-press-event", G_CALLBACK(gst_rojscreen_press), filter);

  filter->frame = gtk_fixed_new();
  gtk_container_add(GTK_CONTAINER(filter->window), filter->frame);

  filter->image = gtk_image_new ();
  GtkWidget *  eventbox = gtk_event_box_new();
  gtk_container_add (GTK_CONTAINER (eventbox), filter->image);
  gtk_fixed_put(GTK_FIXED(filter->frame), eventbox, 0, 0);
  gtk_event_box_set_above_child(GTK_EVENT_BOX(eventbox), FALSE);
  g_signal_connect (G_OBJECT (eventbox), "button-press-event", G_CALLBACK(gst_rojscreen_push), filter);
  gtk_widget_set_size_request(filter->image, filter->wscr, filter->hscr);
  
  gtk_widget_show_all (filter->window);
  gst_rojscreen_refresh(filter, NULL);
}


static void
gst_rojscreen_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  Gstrojscreen *filter = GST_ROJSCREEN (object);

  switch (prop_id) {
  case PROP_OVAL: 
    filter->oval.val = g_value_get_double (value);
    filter->oval.flag = TRUE;
    break;
  case PROP_EVAL: 
    filter->eval.val = g_value_get_double (value);
    filter->eval.flag = TRUE;
    break;
  case PROP_UPDATE: 
    filter->update = g_value_get_int (value);
    if (filter->update>filter->wscr || filter->update<=0)
      filter->update=filter->wscr;
    break;
  case PROP_CHANNEL: 
    filter->channel = g_value_get_int (value);
    break;
  case PROP_WSCR:
    { 
      gst_rojscreen_memory_free(filter);

      filter->wscr = g_value_get_int (value);
      filter->allreceived = 0;
      filter->current = 0;
      filter->stored = 0;

      if (filter->update>filter->wscr)
	filter->update=filter->wscr;
      
      /* alokowanie pamięci dla danych */
      gst_rojscreen_memory_allocation(filter);

      gtk_window_resize (GTK_WINDOW (filter->window), filter->wscr, filter->hscr);
      gtk_window_set_default_size(GTK_WINDOW (filter->window), filter->wscr, filter->hscr);
      gtk_widget_set_size_request(filter->image, filter->wscr, filter->hscr);

      break;
    }
  case PROP_HSCR: 
    {
      gst_rojscreen_memory_free(filter);

      filter->hscr = g_value_get_int (value);
      filter->allreceived = 0;
      filter->current = 0;
      filter->stored = 0;
 
      /* alokowanie pamięci dla danych */
      gst_rojscreen_memory_allocation(filter);

      gtk_window_resize (GTK_WINDOW (filter->window), filter->wscr, filter->hscr);
      gtk_window_set_default_size(GTK_WINDOW (filter->window), filter->wscr, filter->hscr);
      gtk_widget_set_size_request(filter->image, filter->wscr, filter->hscr);

      break;
    }

  case PROP_MODE:    
    if (!strcmp(g_value_get_string (value), "lin")){
      filter->mode = MODE_LIN;
      filter->jump = 1.0;
      break;
    }
    if (!strcmp(g_value_get_string (value), "log")){
      filter->mode = MODE_LOG;
      filter->jump = 10.0;
      break;
    }
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    groj_error(gst_element_get_name(object), "not known mode");      
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rojscreen_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstrojscreen *filter = GST_ROJSCREEN (object);

  switch (prop_id) {
  case PROP_OVAL: 
    g_value_set_double (value, filter->oval.val);	
    break;
  case PROP_EVAL: 
    g_value_set_double (value, filter->eval.val);	
    break;
  case PROP_UPDATE: 
    g_value_set_int (value, filter->update);	
    break;
  case PROP_CHANNEL: 
    g_value_set_int (value, filter->channel);	
    break;
  case PROP_WSCR: 
    g_value_set_int (value, filter->wscr);	
    break;
  case PROP_HSCR: 
    g_value_set_int (value, filter->hscr);
    break;
     
  case PROP_MODE:      
    switch(filter->mode){
    case MODE_LIN:
      g_value_set_string (value, "lin");
      break;
    case MODE_LOG:
      g_value_set_string (value, "log");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      groj_error(gst_element_get_name(object), "not known mode");      
    }
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}




/* GstElement vmethod implementations */
/* this function handles sink events */
static gboolean
gst_rojscreen_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstrojscreen *filter = GST_ROJSCREEN (parent);
  g_print ("* %s: event %s\n",
	   gst_element_get_name(filter), gst_event_type_get_name(GST_EVENT_TYPE (event))); 

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
      ret = gst_pad_event_default (pad, parent, event);

      GstTagList *taglist;
      gst_event_parse_tag(event, &taglist);

      groj_tag_catch_double(taglist, "ofreq", gst_element_get_name(parent), &filter->tags.ofreq);
      groj_tag_catch_double(taglist, "leap", gst_element_get_name(parent), &filter->tags.leap);
      break;
    }

  case GST_EVENT_SEGMENT:
    {
      ret = gst_pad_event_default (pad, parent, event);
      if (!filter->tags.ofreq.flag || !filter->tags.leap.flag)      
	groj_error(gst_element_get_name(parent), "need tags");

      break;
    }

  case GST_EVENT_EOS:
    {
      ret = gst_pad_event_default (pad, parent, event);

      if (!filter->eof){
	filter->eof = TRUE;
	if (GTK_IS_WINDOW (filter->window)){
	  gst_rojscreen_update(filter);
	  gst_rojscreen_refresh(filter, NULL);
	  gtk_window_set_title (GTK_WINDOW (filter->window), "GROJ ##");
	}
	g_print("* screen0: %d/%d, %d\n",   filter->allreceived, filter->wscr, filter->hscr);
	gtk_main ();
      }
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
gst_rojscreen_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  Gstrojscreen *filter = GST_ROJSCREEN (parent);  
  if (filter->eof)
    return GST_FLOW_EOS;

  if (filter->channel==0 || filter->channel>=gst_buffer_n_memory(buffer))
    groj_error(gst_element_get_name(parent), "too large channel or zero");
  
  GstMemory *cmem = gst_buffer_get_memory (buffer, 0);
  GstMemory *mem = gst_buffer_get_memory (buffer, filter->channel);
  GstMapInfo info;

  gst_memory_map (mem, &info, GST_MAP_READ);
  int samples = info.size / sizeof(double);

  if (samples<filter->hscr){
    filter->hscr = samples;

    gtk_window_resize (GTK_WINDOW (filter->window), filter->wscr, filter->hscr);
    gtk_window_set_default_size(GTK_WINDOW (filter->window), filter->wscr, filter->hscr);
    gtk_widget_set_size_request(filter->image, filter->wscr, filter->hscr);
  }

  memcpy(filter->data[filter->current], info.data, filter->hscr*sizeof(double));
  gst_memory_unmap (mem, &info);

  int n;
  filter->times[filter->current] = groj_config_get_time(cmem);
  filter->maxima[filter->current] = filter->data[filter->current][0];
  filter->minima[filter->current] = filter->data[filter->current][0];
  for(n=1;n<filter->hscr;n++){
    if(filter->maxima[filter->current] < filter->data[filter->current][n]) filter->maxima[filter->current] = filter->data[filter->current][n];
    if(filter->minima[filter->current] > filter->data[filter->current][n]) filter->minima[filter->current] = filter->data[filter->current][n];
  }

  if(filter->allreceived%filter->update==0){
    gst_rojscreen_update(filter);
    gst_rojscreen_refresh(filter, NULL);
  }
  
  if(filter->stored<filter->wscr)
    filter->stored++;
  filter->allreceived++;

  filter->current++;
  filter->current = filter->current % filter->wscr;

  return gst_pad_push (filter->srcpad, buffer);
}


/* entry point to initialize the plug-in initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
rojscreen_init (GstPlugin * rojscreen)
{
  /* debug category for fltering log messages
   * exchange the string 'Template rojscreen' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_rojscreen_debug, "rojscreen", 0, "rojscreen");
  return gst_element_register (rojscreen, "rojscreen", GST_RANK_NONE, GST_TYPE_ROJSCREEN);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "rojscreen"
#endif

/* gstreamer looks for this structure to register rojscreens
 * exchange the string 'Template rojscreen' with your rojscreen description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rojscreen,
    "rojscreen",
    rojscreen_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
