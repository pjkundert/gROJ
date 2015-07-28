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

rojparcel.set_property("width", 300)
rojparcel.set_property("hop", 1)
rojparcel.set_property("etime", 0.8)

#---------------------------------------------------------------------------------
rojreass = Gst.ElementFactory.make("rojreass", "reass0")
pipeline.add(rojreass)

rojreass.set_property("length", 1024*2)
rojreass.set_property("window", "blackman-harris")
rojreass.set_property("ofreq", 800.0)
rojreass.set_property("efreq", 1320.0)

#---------------------------------------------------------------------------------
rojraster = Gst.ElementFactory.make("rojraster", "raster0")
pipeline.add(rojraster)

rojraster.set_property("dtime", 0.0005)
rojraster.set_property("dfreq", 1.2)
#rojraster.set_property("titter", 1.5)
#rojraster.set_property("fitter", 1.5)
rojraster.set_property("reassign", True)
#rojraster.set_property("reraster", True)
rojraster.set_property("latency", 0.005)

#---------------------------------------------------------------------------------
rojkadr = Gst.ElementFactory.make("rojkadr", "kadr0")
pipeline.add(rojkadr)

rojkadr.set_property("otime", 0.1)
rojkadr.set_property("etime", 0.5)
rojkadr.set_property("ofreq", 800.0)
rojkadr.set_property("efreq", 1320.0)

#---------------------------------------------------------------------------------
rojscreen = Gst.ElementFactory.make("rojscreen", "screen0")
pipeline.add(rojscreen)

#rojscreen.set_property("mode", "lin")
rojscreen.set_property("mode", "log")
rojscreen.set_property("wscr", 888)
rojscreen.set_property("hscr", 800)
rojscreen.set_property("update", 10)
rojscreen.set_property("channel", 1)
rojscreen.set_property("oval", -30)

#---------------------------------------------------------------------------------
fakesink = Gst.ElementFactory.make("fakesink", "sink0")
pipeline.add(fakesink)

########################################################################################
#  link...

import groj
groj.link(fakesrc, rojload, rojparcel, rojreass, rojraster, rojkadr, rojscreen, fakesink)

########################################################################################

ret = pipeline.set_state(Gst.State.PLAYING)
assert ret != Gst.StateChangeReturn.FAILURE, "error: play"

try:
    import signal
    signal.pause()
except KeyboardInterrupt:
    pipeline.set_state(Gst.State.NULL)
    print("CTRL-C")

