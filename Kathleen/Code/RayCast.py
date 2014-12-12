__author__ = 'Clinic'

import numpy as np
import math
import cv2
from math import cos, sin, pi

def rotatingWhiteDistance(image, location, precision=36):
    """takes in binary image and (x,y), returns array of precision values"""
    retArray = np.zeros(precision)
    xMax = len(image)
    yMax = len(image[0])
    max_radius = max(len(image), len(image[0]))
    for index in range(precision):
        radian = math.pi*2 * (index / (precision*1.0))
        radius = 0
        while True:
            radius += 1
            x,y = rect(radius, radian)
            x,y = location[0] + int(x), location[1] + int(y)
            if x >= xMax or y >= yMax or x <= 0 or y <= 0:
                """ do we want to count image borders as edges? half the way to an edge? will never find an edge? """
                #radius = xMax + yMax
                #radius = radius * 2
                break
            pixel = image[x][y]
            if pixel != 0:
                break
            if radius > max_radius:
                #radius = xMax + yMax
                break
        retArray[index] = radius

    return retArray, np.std(retArray), np.mean(retArray)

def rotatingWhiteDistance_b(image, location, precision=36):
    """takes in binary image and (x,y), returns array of precision values"""
    retArray = np.zeros(precision)
    xMax = len(image)
    yMax = len(image[0])
    max_radius = max(len(image), len(image[0]))

    for index in range(precision):
        radian = math.pi*2 * (index / (precision*1.0))
        cos_radian = cos(radian)
        sin_radian = sin(radian)
        for radius in range(0, max_radius):
            x,y = radius*cos_radian, radius*sin_radian
            x,y = location[0] + int(x), location[1] + int(y)
            if x >= xMax or y >= yMax or x <= 0 or y <= 0:
                break
            pixel = image[x][y]
            if pixel != 0:
                break

        retArray[index] = radius

    return retArray, np.std(retArray), np.mean(retArray)



def rect(r, w, deg=0):		# radian if deg=0; degree if deg=1
    if deg:
	    w = pi * w / 180.0
    return r * cos(w), r * sin(w)

def rect_b(r,w):
    return r * cos(w), r * sin(w)