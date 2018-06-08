import sys
import os
import glob
import re
import csv
import math
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

path = sys.argv[1]
filestring = os.path.join(path, "lb_*.out")

outprefix = ""
if len(sys.argv) == 3:
  outprefix = sys.argv[2]

files = glob.glob(filestring)

traces = []
names = []

for f in files:
  trace = []
  for line in open(f):
    l = []
    for t in line.split():
      try:
        l.append(int(t))
      except ValueError:
        try:
          l.append(float(t))
        except ValueError:
          pass
    if len(l) == 3:
      if l[0] == len(trace):
        trace.append([])
      if l[1] == len(names):
        names.append(" ".join(line.split()[3:]))
      trace[l[0]].append(l[2])
  traces.append(trace)

for i in range(len(traces[0][0])):
  name = 'metric_' + str(i) + '.csv'
  name = os.path.join(outprefix, name)
  file = open(name,'w')
  file.write(names[i] + '\n')
  for j in range(len(traces[0])):
    for k in range(len(traces)):
      file.write(str(traces[k][j][i]) + ' ')
    file.write('\n')
  file.close()

files = glob.glob(os.path.join(outprefix,"metric_*.csv"))

for filename in files:
  file = open(filename)
  outfile = os.path.splitext(os.path.basename(filename))[0] + ".eps"
  outfile = os.path.join(outprefix, outfile)
  data = []
  name = file.readline().strip()

  print "Parsing", filename, "for", name, "and putting results into", outfile

  for row in file:
    data.append(map(float, row.split()))

  rows = len(data)

  
  fig,ax = plt.subplots(rows, sharex=True)

  for a, d in zip(ax, data):
    avg = sum(d) / float(len(d))
    m = max(d)
    a.bar(np.arange(len(d)),d,1,color='r',linewidth=1)
    a.axhline(y=avg, linewidth=1, color='k')
    a.set_xlim([0,len(d)])
    if avg > 0:
      a.set_title("Max: " + str(m) + " Avg: " + str(avg) + " Ratio: " + str(m / avg))

  fig.suptitle(name)
  fig.savefig(outfile,format='eps',dpi=1000)
  plt.clf()
