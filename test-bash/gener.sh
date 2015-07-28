#!/bin/bash

export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0

OBJECT=rojgener
LA_FILE=$GST_PLUGIN_PATH/lib$OBJECT.la
SO_FILE=$GST_PLUGIN_PATH/lib$OBJECT.so

if [[ -f $LA_FILE ]];
then
   echo "File $LA_FILE exists."
else
   echo "File $LA_FILE does not exist."
   echo "enter: make && make install"
fi

if [[ -f $SO_FILE ]];
then
   echo "File $SO_FILE exists."
else
   echo "File $SO_FILE does not exist."
   echo "enter: make && make install"
fi

# info about plugin:
#-----------------------------------

gst-inspect-1.0 rojgener

# use:
#-----------------------------------

#gst-launch-1.0 fakesrc ! rojgener ! fakesink

#gst-launch-1.0 fakesrc ! rojgener duration=3 ! fakesink
#gst-launch-1.0 fakesrc ! rojgener block=300 ! fakesink
#gst-launch-1.0 fakesrc ! rojgener block=134217727 ! fakesink

gst-launch-1.0 fakesrc ! rojgener duration=1 rate=64 signal=time block=128 ! fakesink

