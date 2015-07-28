#!/usr/bin/env python3

import os, sys
path = "/usr/local/lib/gstreamer-1.0"
os.environ["GST_PLUGIN_PATH"] = path

from gi.repository import Gst

Gst.init(None)
if Gst.init_check(None):
    print("Wersja:", Gst.version())

