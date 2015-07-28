#!/usr/bin/env python3

import os, sys
path = "/usr/local/lib/gstreamer-1.0"
os.environ["GST_PLUGIN_PATH"] = path

from gi.repository import Gst
Gst.init(None)


import groj
pipeline = Gst.Pipeline.new("test-pipeline")

#---------------------------------------------------------------------------------
fakesrc = Gst.ElementFactory.make("fakesrc", "src0")
pipeline.add(fakesrc)

#---------------------------------------------------------------------------------
rojload = Gst.ElementFactory.make("rojload", "load0")
pipeline.add(rojload)

rojload.set_property("block", 512)
rojload.set_property("location", "test.txt")
rojload.set_property("start", 0.0)
rojload.set_property("rate", 4000.0)
rojload.set_property("complex", True)


#---------------------------------------------------------------------------------
rojparcel = Gst.ElementFactory.make("rojparcel", "parcel0")
pipeline.add(rojparcel)

rojparcel.set_property("width", 300)
rojparcel.set_property("hop", 1)
#rojparcel.set_property("otime", 0.39)
#rojparcel.set_property("etime", 0.48)

#---------------------------------------------------------------------------------
#rojtransform = Gst.ElementFactory.make("rojstft", "stft0")
rojtransform = Gst.ElementFactory.make("rojreass", "reass0")
pipeline.add(rojtransform)

rojtransform.set_property("length", 1024)
rojtransform.set_property("window", "blackman-harris")
rojtransform.set_property("ofreq", 0.0)
rojtransform.set_property("efreq", 1120.0)

#---------------------------------------------------------------------------------
rojconvert = Gst.ElementFactory.make("rojraster", "raster0")
#rojconvert = Gst.ElementFactory.make("rojconvert", "convert0")
pipeline.add(rojconvert)

rojconvert.set_property("reassign", True)
rojconvert.set_property("dtime", 0.01)
rojconvert.set_property("dfreq", 0.5)
rojconvert.set_property("titter", 50)
#rojconvert.set_property("out", "energy")

#---------------------------------------------------------------------------------
rojscreen = Gst.ElementFactory.make("rojscreen", "screen0")
pipeline.add(rojscreen)

rojscreen.set_property("mode", "log")
rojscreen.set_property("wscr", 987)
rojscreen.set_property("hscr", 900)
rojscreen.set_property("update", 10)
rojscreen.set_property("channel", 1)
rojscreen.set_property("oval", -30)

#---------------------------------------------------------------------------------
fakesink = Gst.ElementFactory.make("fakesink", "sink0")
pipeline.add(fakesink)

########################################################################################
#  link...

import groj
groj.link(fakesrc, rojload, rojparcel, rojtransform, rojconvert, rojscreen, fakesink)

########################################################################################



ret = pipeline.set_state(Gst.State.PLAYING)
assert ret != Gst.StateChangeReturn.FAILURE, "error: play"

try:
    import signal
    signal.pause()
except KeyboardInterrupt:
    pipeline.set_state(Gst.State.NULL)
    print("CTRL-C")

