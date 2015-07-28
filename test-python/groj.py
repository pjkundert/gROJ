#!/usr/bin/env python3

from gi.repository import Gst

def link(*items):
    print("------------------------------")
    for n,item in enumerate(items): 
        print(n, "\t\033[31m"+item.get_name()+"\33[0m")
        try: item.link(items[n+1])
        except IndexError: pass
    print("------------------------------")

class CAPS:
    signal = "roj/signal"
    frame = "roj/frame"
    stft = "roj/stft"
    tfr = "roj/tfr"
caps = CAPS()


import os

class SVG:
    def __init__(self, slabel, width, height):        
        
        self.slabel = slabel
        self.width = width
        self.height = height

        conf = "__groj/_config.txt"
        frd = open(conf, "r")
        lines = frd.readlines()
        frd.close()

        self.log = True if lines[0][0:-1] == "log" else False
        self.out = True if lines[1][0:-1] == "out" else False

        self.otime = float(lines[2][0:-1].replace(",","."))
        self.etime = float(lines[3][0:-1].replace(",","."))

        self.ofreq = float(lines[4][0:-1].replace(",","."))
        self.efreq = float(lines[5][0:-1].replace(",","."))

        self.oval = float(lines[6][0:-1].replace(",","."))
        self.eval = float(lines[7][0:-1].replace(",","."))

        self.x0 = 40
        self.y0 = self.height+20
        self.separator = 8
        self.content = "<svg xmlns='http://www.w3.org/2000/svg' width='%d' height='%d'>\n" % (self.width+500, self.height+80)


    def print_images(self):

        self.content += "<rect x='%d' y='%d' width='%d' height='%d' stroke='black' stroke-width='2' fill='black'/>\n" % \
              (self.x0, self.y0-self.height, self.width, self.height)
        self.content += "<image x='%d' y='%d' width='%d' height='%d' xlink:href='%s_screen.png' preserveAspectRatio='none'/>\n" % \
              (self.x0, self.y0-self.height, self.width, self.height, self.slabel)

        self.content += "<rect x='%d' y='%d' width='20' height='%d' stroke='black' stroke-width='2' fill='black'/>\n" % \
              (self.x0+self.width+self.separator, self.y0-self.height, self.height)
        self.content += "<image x='%d' y='%d' width='20' height='%d' xlink:href='%s_scale.png' preserveAspectRatio='none'/>\n" % \
              (self.x0+self.width+self.separator, self.y0-self.height, self.height, self.slabel)

        if(not self.out): return

        self.content += "<rect x='%d' y='%d' width='20' height='20' stroke='black' stroke-width='2' fill='black'/>\n" % \
              (self.x0+self.width+self.separator, self.y0+self.separator)
        self.content += "<image x='%d' y='%d' width='20' height='20' xlink:href='%s_out.png' preserveAspectRatio='none'/>\n" % \
              (self.x0+self.width+self.separator, self.y0+self.separator, self.slabel)


    def print_title(self, title):
        self.content += "<text x='%d' y='%d' style='text-anchor:middle;'>%s</text>\n" % \
            (self.x0+self.width/2, self.y0-self.height-self.separator, title)

    def print_labels(self, xlabel="TIME (s)", ylabel="FREQUENCY (Hz)", zlabel="ENERGY DENSITY (dBc)", xscale=1, yscale=1, zscale=1):

        larrow = "<tspan fill='blue'> &#8678; </tspan>"
        rarrow = "<tspan fill='blue'> &#8680; </tspan>"

        xlabel = "<tspan font-weight='bold'>"+larrow+"<tspan fill='red'> "+xlabel+" </tspan>"+rarrow+"</tspan>"
        self.content += "<text x='%d' y='%d' style='text-anchor:middle;'>%g %s %g</text>\n" % \
              (self.x0+self.width/2, self.y0+self.separator+16, self.otime*xscale, xlabel, self.etime*xscale)

        ylabel = "<tspan font-weight='bold'>"+larrow+"<tspan fill='red'> "+ylabel+" </tspan>"+rarrow+"</tspan>"
        self.content += "<text x='%d' y='%d' style='text-anchor:middle;' transform='rotate(-90, %d, %d)'>%g %s %g</text>\n" % \
              (25, self.y0-self.height/2, 25, self.y0-self.height/2, self.ofreq*yscale, ylabel, self.efreq*yscale)

        y = self.y0-self.height/2
        x = self.x0+self.width+4*self.separator+20

        zlabel = "<tspan font-weight='bold'>"+larrow+"<tspan fill='red'> "+zlabel+" </tspan>"+rarrow+"</tspan>"
        self.content += "<text x='%d' y='%d' style='text-anchor:middle;' transform='rotate(-90, %d, %d)'>%g %s %g</text>\n" % \
              (x, y, x, y, self.oval*zscale, zlabel, self.eval*zscale)

        if(not self.out): return
        self.content += "<text  x='%d' y='%d' style='text-anchor:start;'>OUT OF SCALE</text>\n" % \
              (self.x0 + self.width+2*self.separator+20, self.y0+self.separator+16)


    def make(self):
        self.content += "</svg>\n"

        import os
        directory = "draw-"+self.slabel+"/"
        os.system("mkdir "+directory)

        fout = open(directory+"index.html", "w")
        fout.write(self.content)
        fout.close()

        os.system("cp __groj/_screen.png "+directory+self.slabel+"_screen.png")
        os.system("cp __groj/_scale.png "+directory+self.slabel+"_scale.png")
        os.system("cp __groj/_config.txt "+directory+"_config.txt")
        if  self.out:
            os.system("cp __groj/_out.png "+directory+self.slabel+"_out.png")


