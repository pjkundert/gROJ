#!/usr/bin/env python3

import sys
try: slabel = str(sys.argv[1])
except: slabel = "test"
try: wscr = int(sys.argv[2])
except: wscr = 800
try: hscr = int(sys.argv[3])
except: hscr = 600


import groj
svg = groj.SVG(slabel, wscr, hscr)
svg.print_images()
svg.print_labels()
svg.make()
