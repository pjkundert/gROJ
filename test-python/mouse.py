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

rojpointer.set_property("rate", 4000.0)
rojpointer.set_property("block", 64)
rojpointer.set_property("mode", "amfm")

#---------------------------------------------------------------------------------
rojfilter = Gst.ElementFactory.make("rojfilter", "filter0")
pipeline.add(rojfilter)

rojfilter.set_property("length", 21)
rojfilter.set_property("recoeff",-0.0203071693467679)
rojfilter.set_property("recoeff",-0.04285720447381696)
rojfilter.set_property("recoeff",0.022617859991399276)
rojfilter.set_property("recoeff",0.0820060408090022)
rojfilter.set_property("recoeff",0.025237203284271408)
rojfilter.set_property("recoeff",-0.0003692969922741838)
rojfilter.set_property("recoeff",0.021969525016504406)
rojfilter.set_property("recoeff",-0.14041219818599066)
rojfilter.set_property("recoeff",-0.24290301344354398)
rojfilter.set_property("recoeff",0.09696153279394837)
rojfilter.set_property("recoeff",0.38677118899627594)
rojfilter.set_property("recoeff",0.09696153279394837)
rojfilter.set_property("recoeff",-0.24290301344354398)
rojfilter.set_property("recoeff",-0.14041219818599066)
rojfilter.set_property("recoeff",0.021969525016504406)
rojfilter.set_property("recoeff",-0.0003692969922741838)
rojfilter.set_property("recoeff",0.025237203284271408)
rojfilter.set_property("recoeff",0.0820060408090022)
rojfilter.set_property("recoeff",0.022617859991399276)
rojfilter.set_property("recoeff",-0.04285720447381696)
rojfilter.set_property("recoeff",-0.0203071693467679)
# fir designed by: http://t-filter.appspot.com/fir/index.html


#---------------------------------------------------------------------------------
rojparcel = Gst.ElementFactory.make("rojparcel", "parcel0")
pipeline.add(rojparcel)

rojparcel.set_property("width", 1000)
rojparcel.set_property("hop", 145)
#rojparcel.set_property("otime", 0.0)
#rojparcel.set_property("etime", 1.5)

#---------------------------------------------------------------------------------
#rojstft = Gst.ElementFactory.make("rojreass", "stft0")
rojstft = Gst.ElementFactory.make("rojstft", "stft0")
pipeline.add(rojstft)

rojstft.set_property("length", 1*1024)
rojstft.set_property("window", "blackman-harris")
#rojstft.set_property("window", "rectangular")
#rojstft.set_property("ofreq", -200.0)
#rojstft.set_property("efreq", 200.0)

#---------------------------------------------------------------------------------
#rojconvert = Gst.ElementFactory.make("rojraster", "convert0")
rojconvert = Gst.ElementFactory.make("rojconvert", "convert0")
pipeline.add(rojconvert)

#rojconvert.set_property("out", "phase")
rojconvert.set_property("out", "energy")
#rojconvert.set_property("out", "both")
#rojconvert.set_property("out", "abs")



 # rojconvert.set_property("dtime", 0.01)
 # rojconvert.set_property("dfreq", 4.0)
 # rojconvert.set_property("titter", 1.2)
 # rojconvert.set_property("fitter", 1.2)
 # rojconvert.set_property("reassign", True)
 # #rojraster.set_property("reraster", True)
 # rojconvert.set_property("latency", 0.005)


#---------------------------------------------------------------------------------
rojscreen = Gst.ElementFactory.make("rojscreen", "screen0")
pipeline.add(rojscreen)

#rojscreen.set_property("mode", "lin")
rojscreen.set_property("mode", "log")
#rojscreen.set_property("wscr", 1100)
rojscreen.set_property("hscr", 1024)
rojscreen.set_property("update", 3)
rojscreen.set_property("channel", 1)
rojscreen.set_property("oval", -30)


#---------------------------------------------------------------------------------
fakesink = Gst.ElementFactory.make("fakesink", "sink0")
pipeline.add(fakesink)

########################################################################################
#  link...

import groj
groj.link(fakesrc, rojpointer, rojfilter, rojparcel, rojstft, rojconvert, rojscreen, fakesink)

########################################################################################


ret = pipeline.set_state(Gst.State.PLAYING)
assert ret != Gst.StateChangeReturn.FAILURE, "error: play"

try:
    import signal
    signal.pause()
except KeyboardInterrupt:
    pipeline.set_state(Gst.State.NULL)
    print("CTRL-C")

