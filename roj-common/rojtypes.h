#ifndef _GST_ROJTYPES_H_
#define _GST_ROJTYPES_H_

#include <gst/gst.h>

#include <complex.h>
#include <fftw3.h>
#include <math.h>

#define GROJ_CAPS_SIGNAL "roj/signal"
#define GROJ_CAPS_FRAME "roj/frame"
#define GROJ_CAPS_STFT "roj/stft"
#define GROJ_CAPS_TFR "roj/tfr"

struct RojConfig{
  double time;
  unsigned long number;
}RojConfig;

struct RojBuf{
  double time;
  GstMemory *config;
  GstMemory **mems;
  GstMapInfo *infos;
} RojBuf;

struct RojDouble{
  gdouble val;
  gboolean flag;
};

struct RojInt{
  gint val;
  gboolean flag;
};

struct RojTags
{
  struct RojDouble rate;
  struct RojDouble start;

  struct RojInt width;
  struct RojInt length;

  struct RojDouble ofreq;
  struct RojDouble otime;

  struct RojDouble hop;
  struct RojDouble leap;
};

#endif
