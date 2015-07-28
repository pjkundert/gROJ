#!/usr/bin/python

import numpy as np

import locale
locale.setlocale(locale.LC_ALL, "")



fds = open("data", "r")
lines = fds.readlines()
tab = map(locale.atof, lines)
fds.close()



hist, bin_edges = np.histogram(tab, range=(-0.55, 0.55), bins=1000)
xaxis = []
for i,x in enumerate(bin_edges):
    xaxis.append(x)
    if i==0: continue 
    if i==len(bin_edges)-1: continue 
    xaxis.append(x)
yaxis = []
for y in hist:
    yaxis.append(y)
    yaxis.append(y)

import matplotlib.pyplot as plt
plt.plot(xaxis, yaxis)
plt.grid()
plt.show()
