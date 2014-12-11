__author__ = 'Clinic'

import matplotlib.pyplot as plt
import cv2
import numpy as np
import main
import time

#fig = plt.figure()
im = cv2.imread("thresh.jpg")
im = cv2.threshold(im, 128, 255, cv2.THRESH_BINARY)[1]
im = cv2.cvtColor(im, cv2.COLOR_RGB2GRAY)
#plt.imshow(im)
#def onclick(event):
#    print 'button=%d, x=%d, y=%d, xdata=%f, ydata=%f'%(
#        event.button, event.x, event.y, event.xdata, event.ydata)
#plt.show()
fig = plt.figure()
ax = fig.add_subplot(121)
ax.imshow(im)
ax2 = fig.add_subplot(122)

def onclick(event):
    print 'button=%d, x=%d, y=%d, xdata=%f, ydata=%f'%(
        event.button, event.x, event.y, event.xdata, event.ydata)

    array, stDev, mean = main.rotatingWhiteDistance(im, (int(event.xdata), int(event.ydata)))
    ax2.clear()
    ax2.plot(array)
    print array
    print stDev/mean




cid = fig.canvas.mpl_connect('button_press_event', onclick)
plt.show()
