#!/bin/bash

export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0

OBJECT=rojparcel
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

gst-inspect-1.0 rojparcel


# use together with another plugins
#-----------------------------------

#gst-launch-1.0 --gst-debug-level=4 fakesrc ! rojgener ! rojparcel ! fakesink
#gst-launch-1.0 fakesrc ! rojgener ! rojparcel width=16 ! fakesink

gst-launch-1.0 fakesrc ! \
    rojgener block=10 start=1.5 rate=20 duration=-2.0 ! rojparcel otime=1.1 etime=2.2 width=16 hop=2 ! \
    rojdebug pack=TRUE tags=TRUE data=TRUE ! fakesink
                                          
