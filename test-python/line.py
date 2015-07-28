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

#-------------
queue = Gst.ElementFactory.make("queue", "queue0")
pipeline.add(queue)

queue.set_property("leaky", True)
queue.set_property("max-size-buffers", 10)

#---------------------------------------------------------------------------------
rojgener = Gst.ElementFactory.make("rojgener", "gener0")
pipeline.add(rojgener)

rojgener.set_property("duration", -1.0)
rojgener.set_property("width", 1.0)
rojgener.set_property("pause", 0.1)

rojgener.set_property("signal", "awgn")
rojgener.set_property("rate", 4000.0)

#---------------------------------------------------------------------------------
rojfilter0 = Gst.ElementFactory.make("rojfilter", "filter0")
pipeline.add(rojfilter0)

rojfilter0.set_property("length",21)
rojfilter0.set_property("recoeff",-0.0203071693467679)
rojfilter0.set_property("recoeff",-0.04285720447381696)
rojfilter0.set_property("recoeff",0.022617859991399276)
rojfilter0.set_property("recoeff",0.0820060408090022)
rojfilter0.set_property("recoeff",0.025237203284271408)
rojfilter0.set_property("recoeff",-0.0003692969922741838)
rojfilter0.set_property("recoeff",0.021969525016504406)
rojfilter0.set_property("recoeff",-0.14041219818599066)
rojfilter0.set_property("recoeff",-0.24290301344354398)
rojfilter0.set_property("recoeff",0.09696153279394837)
rojfilter0.set_property("recoeff",0.38677118899627594)
rojfilter0.set_property("recoeff",0.09696153279394837)
rojfilter0.set_property("recoeff",-0.24290301344354398)
rojfilter0.set_property("recoeff",-0.14041219818599066)
rojfilter0.set_property("recoeff",0.021969525016504406)
rojfilter0.set_property("recoeff",-0.0003692969922741838)
rojfilter0.set_property("recoeff",0.025237203284271408)
rojfilter0.set_property("recoeff",0.0820060408090022)
rojfilter0.set_property("recoeff",0.022617859991399276)
rojfilter0.set_property("recoeff",-0.04285720447381696)
rojfilter0.set_property("recoeff",-0.0203071693467679)
# fir designed by: http://t-filter.appspot.com/fir/index.html

#---------------------------------------------------------------------------------
rojline = Gst.ElementFactory.make("rojline", "rojline0")
pipeline.add(rojline)

rojline.set_property("frequency", 500.0)

#---------------------------------------------------------------------------------
rojfilter1 = Gst.ElementFactory.make("rojfilter", "filter1")
pipeline.add(rojfilter1)

rojfilter1.set_property("length",21)
rojfilter1.set_property("recoeff",-0.0203071693467679)
rojfilter1.set_property("recoeff",-0.04285720447381696)
rojfilter1.set_property("recoeff",0.022617859991399276)
rojfilter1.set_property("recoeff",0.0820060408090022)
rojfilter1.set_property("recoeff",0.025237203284271408)
rojfilter1.set_property("recoeff",-0.0003692969922741838)
rojfilter1.set_property("recoeff",0.021969525016504406)
rojfilter1.set_property("recoeff",-0.14041219818599066)
rojfilter1.set_property("recoeff",-0.24290301344354398)
rojfilter1.set_property("recoeff",0.09696153279394837)
rojfilter1.set_property("recoeff",0.38677118899627594)
rojfilter1.set_property("recoeff",0.09696153279394837)
rojfilter1.set_property("recoeff",-0.24290301344354398)
rojfilter1.set_property("recoeff",-0.14041219818599066)
rojfilter1.set_property("recoeff",0.021969525016504406)
rojfilter1.set_property("recoeff",-0.0003692969922741838)
rojfilter1.set_property("recoeff",0.025237203284271408)
rojfilter1.set_property("recoeff",0.0820060408090022)
rojfilter1.set_property("recoeff",0.022617859991399276)
rojfilter1.set_property("recoeff",-0.04285720447381696)
rojfilter1.set_property("recoeff",-0.0203071693467679)
# fir designed by: http://t-filter.appspot.com/fir/index.html

#---------------------------------------------------------------------------------
rojparcel = Gst.ElementFactory.make("rojparcel", "parcel0")
pipeline.add(rojparcel)

rojparcel.set_property("width", 1000)
rojparcel.set_property("hop", 25)
#rojparcel.set_property("otime", 0.0)
#rojparcel.set_property("etime", 1.5)

#---------------------------------------------------------------------------------
rojstft = Gst.ElementFactory.make("rojstft", "stft0")
pipeline.add(rojstft)

rojstft.set_property("length", 1024)
rojstft.set_property("window", "blackman-harris")
#rojstft.set_property("window", "rectangular")
rojstft.set_property("ofreq", 0.0)
#rojstft.set_property("efreq", 1300.0)

#---------------------------------------------------------------------------------
rojconvert = Gst.ElementFactory.make("rojconvert", "convert0")
pipeline.add(rojconvert)

#rojconvert.set_property("out", "phase")
rojconvert.set_property("out", "energy")
#rojconvert.set_property("out", "both")
#rojconvert.set_property("out", "abs")

#---------------------------------------------------------------------------------
rojscreen = Gst.ElementFactory.make("rojscreen", "screen0")
pipeline.add(rojscreen)

#rojscreen.set_property("mode", "lin")
rojscreen.set_property("mode", "log")
rojscreen.set_property("wscr", 1100)
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
groj.link(fakesrc, queue, rojgener, rojfilter0, rojline, rojfilter1, rojparcel, rojstft, rojconvert, rojscreen, fakesink)

########################################################################################


ret = pipeline.set_state(Gst.State.PLAYING)
assert ret != Gst.StateChangeReturn.FAILURE, "error: play"

try:
    import signal
    signal.pause()
except KeyboardInterrupt:
    pipeline.set_state(Gst.State.NULL)
    print("CTRL-C")

