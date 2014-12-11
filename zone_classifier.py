__author__ = 'Clinic'

import cv2
import matplotlib.pyplot as plt
from sklearn.decomposition import PCA

def get_features(image, pt, mean, sd, size=(3,3)):

    high=0
    low=0
    target = image[pt[1]:pt[1]+size[1], pt[0]:pt[0]+size[0]]
    mean, sd = cv2.meanStdDev(target)
    mean = mean[0][0]
    data = []
    for row in target:
        data = data + list(row)

    return (max(data), min(data))

import numpy
def median(lst):
    return numpy.median(numpy.array(lst))

data = []
im = cv2.imread("imtest/1.jpg")
im = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
mean, sd = cv2.meanStdDev(im)
for row in range(0, len(im) - 20, 10):
    print "row ", row
    for pix in range(0, len(im[row]) - 20, 10):
        data.append(get_features(im, (pix, row), mean, sd))


plt.scatter([x[0] for x in data], [x[1] for x in data])
plt.show()