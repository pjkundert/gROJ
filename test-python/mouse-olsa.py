#!/usr/bin/env python3

import os, sys
path = "/usr/local/lib/gstreamer-1.0"
os.environ["GST_PLUGIN_PATH"] = path

from gi.repository import Gst
Gst.init(None)


pipeline = Gst.Pipeline.new("test-pipeline")

#---------------------------------------------------------------------------------
fakesrc = Gst.ElementFactory.make("fakesrc", "src0")
pipeline.add(fakesrc)


#---------------------------------------------------------------------------------
rojpointer = Gst.ElementFactory.make("rojpointer", "pointer0")
pipeline.add(rojpointer)

rojpointer.set_property("rate", 1000.0)
rojpointer.set_property("block", 32)
rojpointer.set_property("mode", "xy")


#---------------------------------------------------------------------------------
rojline = Gst.ElementFactory.make("rojline", "line0")
pipeline.add(rojline)

rojline.set_property("gain", 12000)

#---------------------------------------------------------------------------------
rojolsa = Gst.ElementFactory.make("rojolsa", "olsa0")
pipeline.add(rojolsa)

#---------------------------------------------------------------------------------
fakesink = Gst.ElementFactory.make("fakesink", "sink0")
pipeline.add(fakesink)

########################################################################################
#  link...

import groj
groj.link(fakesrc, rojpointer, rojline, rojolsa, fakesink)

########################################################################################


ret = pipeline.set_state(Gst.State.PLAYING)
assert ret != Gst.StateChangeReturn.FAILURE, "error: play"

try:
    import signal
    signal.pause()
except KeyboardInterrupt:
    pipeline.set_state(Gst.State.NULL)
    print("CTRL-C")

