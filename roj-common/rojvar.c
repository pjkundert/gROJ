#include "rojvar.h"

void
groj_error(char *name, char *info)
{
  if(name==NULL)fprintf(stderr, "ERROR (GROJ): %s\n", info);
  else fprintf(stderr, "ERROR (%s): %s\n", name, info);
  exit(-1);
}

/* **************************** */
/* CONFIG */

GstMemory *
groj_config_new(void)
{
  GstMemory *cmem = gst_allocator_alloc (NULL, sizeof(RojConfig), NULL);
  return cmem;
}

void
groj_config_set_time(GstMemory *cmem, double time){
  GstMapInfo info;
  gst_memory_map (cmem, &info, GST_MAP_WRITE);
  ((struct RojConfig *)info.data)->time = time;
  gst_memory_unmap (cmem, &info);    
}

double
groj_config_get_time(GstMemory *cmem)
{
  GstMapInfo info;
  gst_memory_map (cmem, &info, GST_MAP_READ);
  double time = ((struct RojConfig *)info.data)->time;
  gst_memory_unmap (cmem, &info);    
  return time;
}


void
groj_config_set_number(GstMemory *cmem, unsigned long number){
  GstMapInfo info;
  gst_memory_map (cmem, &info, GST_MAP_WRITE);
  ((struct RojConfig *)info.data)->number = number;
  gst_memory_unmap (cmem, &info);    
}

unsigned long
groj_config_get_number(GstMemory *cmem)
{
  GstMapInfo info;
  gst_memory_map (cmem, &info, GST_MAP_READ);
  unsigned long number = ((struct RojConfig *)info.data)->number;
  gst_memory_unmap (cmem, &info);    
  return number;
}

/* **************************** */

double
groj_ceil(double start, double delta, double otime)
{
  if (otime<start) 
    return start;

  double tmp = (otime - start) / delta;
  tmp = 1.0 + (long)tmp  - tmp;
  return otime + delta * tmp;
}

void
groj_init_tags(struct RojTags tags)
{
  tags.width.flag = FALSE;
  tags.length.flag = FALSE;
  tags.start.flag = FALSE;
  tags.otime.flag = FALSE;
  tags.ofreq.flag = FALSE;
  tags.rate.flag = FALSE;
  tags.leap.flag = FALSE;
  tags.hop.flag = FALSE;
}

gboolean
groj_tag_catch_double(GstTagList *taglist, char *tagname, char *plugin, struct RojDouble *tag)
{
  int n, size = gst_tag_list_n_tags(taglist);
  for (n=0; n<size; ++n) {
    if (!strcmp(gst_tag_list_nth_tag_name(taglist, n), tagname)){
      gst_tag_list_get_double (taglist, gst_tag_list_nth_tag_name(taglist, n), &tag->val);
      g_print("\t\e[1;32m(assign in %s to %s): %g\e[0m\n", plugin, tagname, tag->val);
      tag->flag = TRUE;
      return TRUE;
    }
  }
  
  return FALSE;
}

gboolean
groj_tag_catch_int(GstTagList *taglist, char *tagname, char *plugin, struct RojInt *tag)
{
  int n, size = gst_tag_list_n_tags(taglist);
  for (n=0; n<size; ++n) {
    if (!strcmp(gst_tag_list_nth_tag_name(taglist, n), tagname)){
      gst_tag_list_get_int (taglist, gst_tag_list_nth_tag_name(taglist, n), &tag->val);
      g_print("\t\e[1;32m(assign in %s to %s): %d\e[0m\n", plugin, tagname, tag->val);
      tag->flag = TRUE;
      return TRUE;
    }
  }
  
  return FALSE;
}

gboolean
groj_tag_emit_int(char *plugin, GstPad *srcpad, char *tagname, int val)
{
  GstTagList * taglist = gst_tag_list_new (tagname, val, NULL);
  GstEvent * event = gst_event_new_tag (taglist);

  //gboolean ret = gst_pad_send_event (srcpad, event);
  gboolean ret = gst_pad_push_event (srcpad, event);
  if (ret) g_print("\t\e[1;35m(emit in %s as %s): %d\e[0m\n", plugin, tagname, val);
  
  return ret;
}

gboolean
groj_tag_emit_double(char *plugin, GstPad *srcpad, char *tagname, double val)
{
  GstTagList * taglist = gst_tag_list_new (tagname, val, NULL);
  GstEvent * event = gst_event_new_tag (taglist);

  //gboolean ret = gst_pad_send_event (srcpad, event);
  gboolean ret = gst_pad_push_event (srcpad, event);
  if (ret) g_print("\t\e[1;35m(emit in %s as %s): %g\e[0m\n", plugin, tagname, val);
  
  return ret;
}


float groj_gauss(double sigma) 
{ 
  double u1 = (double)rand() / RAND_MAX;
  if (u1 > 0.99999999999)
    u1 = 0.99999999999;
  double u2 = sigma * sqrt( 2.0 * log( 1.0 / (1.0 - u1) ) );
  
  u1 = (double)rand() / RAND_MAX;
  if (u1 > 0.99999999999) 
    u1 = 0.99999999999;
  
  return (double) (u2 * cos(2 * M_PI * u1));
}



GstEvent* groj_caps_event(GstObject * parent, GstEvent * event, gchar *input, gchar *output)
{
  GstCaps * caps;
  gst_event_parse_caps (event, &caps);
  gboolean ret = strcmp(input, gst_caps_to_string (caps));
  if (ret) groj_error(gst_element_get_name(parent), "caps");

  GstCaps *ocaps = gst_caps_from_string (output);
  GstEvent *ovent = gst_event_new_caps (ocaps);

  g_print("\t\e[1;33m(filter in %s): %s -> %s\e[0m\n", gst_element_get_name(parent), gst_caps_to_string(caps), gst_caps_to_string(ocaps));
  gst_caps_unref (caps);  
  return ovent;
}
