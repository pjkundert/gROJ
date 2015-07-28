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

rojload.set_property("block", 1024)
rojload.set_property("location", "flute-C6.wav")
rojload.set_property("channel", 0)

#---------------------------------------------------------------------------------
rojparcel = Gst.ElementFactory.make("rojparcel", "parcel0")
pipeline.add(rojparcel)

rojparcel.set_property("width", 2200)
rojparcel.set_property("hop", 29)
#rojparcel.set_property("otime", 0.5)
#rojparcel.set_property("etime", 1.5)

#---------------------------------------------------------------------------------
rojstft = Gst.ElementFactory.make("rojstft", "stft0")
pipeline.add(rojstft)

rojstft.set_property("length", 1024*8*8)
rojstft.set_property("window", "blackman-harris")
#rojstft.set_property("window", "rectangular")
rojstft.set_property("ofreq", 800.0)
rojstft.set_property("efreq", 1300.0)

#---------------------------------------------------------------------------------
rojconvert = Gst.ElementFactory.make("rojconvert", "convert0")
pipeline.add(rojconvert)

#rojconvert.set_property("out", "phase")
rojconvert.set_property("out", "energy")
#rojconvert.set_property("out", "both")
#rojconvert.set_property("out", "abs")

#---------------------------------------------------------------------------------
rojkadr = Gst.ElementFactory.make("rojkadr", "kadr0")
pipeline.add(rojkadr)

#rojkadr.set_property("otime", 0.7)
#rojkadr.set_property("etime", 0.5)
rojkadr.set_property("ofreq", 1020.0)
rojkadr.set_property("efreq", 1100.0)

#---------------------------------------------------------------------------------
rojscreen = Gst.ElementFactory.make("rojscreen", "screen0")
pipeline.add(rojscreen)

#rojscreen.set_property("mode", "lin")
rojscreen.set_property("mode", "log")
rojscreen.set_property("wscr", 1192)
rojscreen.set_property("hscr", 600)
rojscreen.set_property("update", 10)
rojscreen.set_property("channel", 1)
rojscreen.set_property("oval", -30)

#---------------------------------------------------------------------------------
fakesink = Gst.ElementFactory.make("fakesink", "sink0")
pipeline.add(fakesink)

########################################################################################
#  link...

import groj
groj.link(fakesrc, rojload, rojparcel, rojstft, rojconvert, rojkadr, rojscreen, fakesink)

########################################################################################



ret = pipeline.set_state(Gst.State.PLAYING)
assert ret != Gst.StateChangeReturn.FAILURE, "error: play"



try:
    import signal
    signal.pause()
except KeyboardInterrupt:
    pipeline.set_state(Gst.State.NULL)
    print("CTRL-C")

