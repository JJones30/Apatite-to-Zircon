__author__ = 'Clinic'
import cv2
import numpy as np
import math
import matplotlib.pyplot as plt
import matplotlib.cm as cm

def makeline(angle = math.pi/4, blurry=3):
    print "call"
    size=25
    im = cv2.imread("imtest/blank.png")
    im = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
    hs = size/2.0
    p1 = (int(hs - size*math.sin(angle)), int(hs - size*math.cos(angle)))
    p2 = (int(hs + size*math.sin(angle)), int(hs + size*math.cos(angle)))
    cv2.line(im, p1, p2, color=0, thickness=3)
    cv2.blur(im, (blurry,blurry))
    return im

def frange(x, y, jump):
  while x < y:
    yield x
    x += jump

test_im = cv2.imread("imtest/1.jpg")
test_im = cv2.cvtColor(test_im, cv2.COLOR_BGR2GRAY)
test_im = cv2.blur(test_im, (5,5))

lines = [makeline(angle=x, blurry=3) for x in frange(0, math.pi, math.pi/20.0)] + [makeline(angle=x, blurry=5) for x in frange(0, math.pi, math.pi/20.0)]

res_accum = []
print "test_im.shape ", test_im.shape
print test_im[0]
for line in lines:
    res = cv2.matchTemplate(test_im, line, method=cv2.TM_CCORR_NORMED)

    if len(res_accum) == 0:
        res_accum = res
    else:
        res_accum = cv2.max(res_accum, res)

mean, sd = cv2.meanStdDev(res_accum)
res_accum = cv2.threshold(res_accum, thresh=mean+sd, maxval=1, type=cv2.THRESH_BINARY)[1]
print res_accum

plt.imshow(res_accum, cmap=cm.Greys)
print res_accum
plt.show()