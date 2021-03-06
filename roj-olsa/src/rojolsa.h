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

#ifndef __GST_ROJOLSA_H__
#define __GST_ROJOLSA_H__

#include <../../roj-common/rojtypes.h>
#include <../../roj-common/rojvar.h>

#include <gst/gst.h>
#include <string.h>
#include <stdlib.h>

#include <complex.h>
#include <fftw3.h>
#include <math.h>

#include <alsa/asoundlib.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_ROJOLSA (gst_rojolsa_get_type())

#define GST_ROJOLSA(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ROJOLSA,Gstrojolsa))
#define GST_ROJOLSA_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ROJOLSA,GstrojolsaClass))

#define GST_IS_ROJOLSA(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ROJOLSA))
#define GST_IS_ROJOLSA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ROJOLSA))

typedef struct _Gstrojolsa      Gstrojolsa;
typedef struct _GstrojolsaClass GstrojolsaClass;

struct _Gstrojolsa
{
  GstElement element;
  GstPad *sinkpad, *srcpad;
 
  snd_pcm_t *pcm;
  snd_pcm_hw_params_t *params;

  struct RojTags tags;
};

struct _GstrojolsaClass 
{
  GstElementClass parent_class;
};

GType gst_rojolsa_get_type (void);

G_END_DECLS

#endif /* __GST_ROJOLSA_H__ */
