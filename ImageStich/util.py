__author__ = 'Clinic'

import numpy as np
import math

def rotatingWhiteDistance(image, location, precision=36):
    """takes in binary image and (x,y), returns array of precision values"""
    retArray = np.zeros(precision)
    xMax = len(image)
    yMax = len(image[0])
    for index in range(precision):
        radian = math.pi*2 * (index / (precision*1.0))
        radius = 0
        while True:
            radius += 1
            x,y = rect(radius, radian)
            x,y = location[0] + int(x), location[1] + int(y)
            if x >= xMax or y >= yMax or x <= 0 or y <= 0:
                radius = xMax + yMax
                break
            pixel = image[x][y]
            if pixel != 0:
                break
            if radius > max(len(image), len(image[0])):
                radius = xMax + yMax
                break
        retArray[index] = radius
    return retArray, np.std(retArray), np.mean(retArray)

def rect(r, w, deg=0):		# radian if deg=0; degree if deg=1
    from math import cos, sin, pi
    if deg:
	    w = pi * w / 180.0
    return r * cos(w), r * sin(w)

def tuple_diff(t1, t2):
    assert len(t1) == len(t2)
    return tuple([t1[i] - t2[i] for i in range(len(t1))])


def tuple_sum(t1, t2):
    assert len(t1) == len(t2)
    return tuple([t1[i] + t2[i] for i in range(len(t1))])


def tuple_ave(tuples):
    first = tuples[0]
    for other_tuple in tuples[1:]:
        first = tuple_sum(first, other_tuple)

    return tuple([x/len(tuples) for x in first])