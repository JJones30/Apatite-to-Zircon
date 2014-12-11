import util

__author__ = 'Clinic'

import numpy as np
import matplotlib.pyplot as plt
import random
import copy
import cv2
import math
import cmath

def getEnclosedRegions(canny_apatite, i):
    print "working image " + str(i) + ":"
    contours=cv2.findContours(canny_apatite, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)[0]

    canny_apatite=cv2.cvtColor(canny_apatite, cv2.COLOR_GRAY2RGB)
    for c in range(len(contours)):
        print len(contours[c])
        if len(contours[c]) > 100 and len(contours[c]) < 1000:

            print "drawing " + str(c)
            cv2.drawContours(canny_apatite, contours, c, (255, 0, 0), thickness=-1)

    color_apatite=cv2.cvtColor(canny_apatite, cv2.COLOR_GRAY2RGB)

    cv2.imwrite('contour' + str(i) + '.jpg', canny_apatite)






def dotplot(image, heuristic, spacing):
    m = len(image)
    n = len(image[0])

    points = []

    for i in range(0, m, spacing):
        for j in range(0, n, spacing):
            points.append((i,j))

    radii = heuristic.evaluateAll(image, points)

    for key in radii.keys():
        sd = radii[key]
        rad= int(100/sd)
        cv2.circle(gray3, key, rad, 255, thickness=-1)

def findCircliestPoint(image, radiusBounds):
    min=999999
    minPos = (-1, -1)
    sdDict={}
    for i in range(0, len(image), 20):
        for j in range(0, len(image[0]), 20):
            array, sd, median = util.rotatingWhiteDistance(bw1, (i,j))
            print median, sd
            if radiusBounds[0] < median < radiusBounds[1]:
                sdDict[(j,i)] = sd
            if sd < min and radiusBounds[0] < median < radiusBounds[1]:
                min=sd
                minPos = (i,j)
                print "minpos!"

    print min
    print minPos
    return minPos, sdDict

def smoothness(radialArray):
    radiansBetween = math.pi * 2 / len(radialArray)

    for i in range(len(radialArray)):
        diff = abs(radialArray[i] - radialArray[i-1])


sdDict=[]
minPos, sdDict = findCircliestPoint(bw1, [10, 200])
print minPos
for key in sdDict.keys():

    sd = sdDict[key]
    rad= int(100/sd)
    cv2.circle(gray3, key, rad, 255, thickness=-1)
cv2.imwrite("minSD2.jpg", gray3)
print len(gray3)
print len(gray3[0])

