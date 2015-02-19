import cv2
import numpy as np
import matplotlib.pyplot as plt
from skimage.filter import (canny, gaussian_filter, threshold_otsu)
from skimage import morphology
from scipy.ndimage import measurements
from pylab import arange
from skimage.draw import line
from scipy.spatial import KDTree


def erodeEdges(raw_image,sig,gaus):
    """
    works a lot better than makeSkeleton
    :param raw_image: a raw image
    :return: a clean skeletonized binary image of the edges
    """
    edges = canny(raw_image, sigma=sig)
    edgesFilt = gaussian_filter(edges, gaus)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh

    im = morphology.erosion(edges,morphology.square(7))

    im2 = morphology.skeletonize(im > 0)

    image = im2.astype(np.int) * 255

    print "made erosion_test image"
    cv2.imwrite("Images/erosion_test.jpg", image)
    return image


def denoiseSkeleton(skeleton, minSize):
    """given a skeleton of an image, return an image of the skeleton with less noise"""

    lw, num = measurements.label(skeleton,[[1,1,1],[1,1,1],[1,1,1]])
    area = measurements.sum(skeleton, lw, index=arange(lw.max() + 1))


    for x in range(len(lw)):
        for y in range(len(lw[x])):
            label = lw[x][y]
            areaSize = area[label]
            if areaSize <= minSize and areaSize != 0:
                skeleton[x][y] = 0

    print "made denoised skeleton image"
    cv2.imwrite("Images/denoised_skeleton.jpg", skeleton)

    return skeleton

def getConnectedCompontents(skeleton, color_image):

    image_dimensions = np.shape(skeleton)
    (maxX, maxY) = image_dimensions



    lw, num = measurements.label(skeleton, [[1,1,1],[1,1,1],[1,1,1]])
    area = measurements.sum(skeleton, lw, index=arange(lw.max() + 1))
    colors = map((lambda x:np.random.rand(3) * 255), area)

    for x in range(len(lw)):
        for y in range(len(lw[x])):
            label = lw[x][y]
            areaSize = area[label]
            color = colors[label]
            if areaSize >= 1000 and areaSize != 0:
                color_image[min(x + 1, maxX - 1)][y] = color
                color_image[max(x - 1, 0)][y] = color
                color_image[x][y] = color
                color_image[x][min(y + 1, maxY - 1)] = color
                color_image[x][max(y - 1, 0)] = color

    endpoints = findCountourEndPoints(lw)

    red = [0,0,255]
    for point in endpoints:
        i,j = point
        color_image[min(i + 1, maxX - 1)][j] = red
        color_image[max(i - 1, 0)][j] = red
        color_image[i][j] = red
        color_image[i][min(j + 1, maxY - 1)] = red
        color_image[i][max(j - 1, 0)] = red

    #newSkel = scoreEndpointLines(endpoints, lw, color_image, skeleton)
    newSkel = scoreEndpointLinesKDTree(endpoints, lw, color_image, skeleton)
    return newSkel


def scoreEndpointLinesKDTree(endpoints, labeled, color_image, skeleton):
    green = [0,255,0]
    canConnect = np.copy(endpoints)
    newSkel = np.copy(skeleton)

    tree = KDTree(canConnect)

    maxVal = 10000

    # loop through each endpoint
    while len(endpoints) > 1:
        point = endpoints[0]
        pointLabel = labeled[point[0]][point[1]] # get label of endpoint's contour
        endpoints = endpoints[1:] #remove ednpoint in use
        distances, possibilities = tree.query(point, k=100, distance_upper_bound=maxVal)

        scores = []
        pts = []
        for i in range(len(possibilities)): # loop through all nearby endpoints

            nxt = tree.data[possibilities[i]]
            score = distances[i]#getDistance(nxt, point)
            if score != 0:
                rr, cc = line(point[0], point[1], nxt[0], nxt[1])
                if labeled[nxt[0]][nxt[1]] != pointLabel: # not the same contour
                    score = score * 1.75
                for i in range(len(rr)): # penalty for crossing over another contour to connect
                    if labeled[rr[i]][cc[i]] != 0:
                        score = score *7
                pts += [nxt]
                scores += [score]

        index = np.argmin(scores)
        connectPoint = pts[index]
        if scores[index] < maxVal:
            rr, cc = line(point[0], point[1], connectPoint[0], connectPoint[1])
            color_image[rr, cc] = green
            newSkel[rr,cc] = 255

    print "made connected_test image"
    cv2.imwrite("Images/connected_test.jpg", color_image)
    print "made connected_test_skeleton image"
    cv2.imwrite("Images/connected_test_skeleton.jpg", newSkel)

    return newSkel


def findCountourEndPoints(labeled):
    """
    :param labeled: image with contours labeled as different number
    :return: list of all the contour endpoints
    """

    image_dimensions = np.shape(labeled)
    (maxX, maxY) = image_dimensions
    end_points = []
    for x in range(len(labeled)):
        for y in range(len(labeled[x])):
            label = labeled[x][y]
            countNearby = 0
            if label != 0:
                for i in range(max(x - 3, 0), min(x + 3, maxX - 1)):
                    for j in range(max(y - 3, 0), min(y + 3, maxY - 1)):
                        nearLabel = labeled[i][j]
                        if label == nearLabel:
                            countNearby += 1
                if countNearby <= 4:
                    end_points += [(x,y)]
    return end_points

def fullSkels(raw_image, sig, gausFilt):
    """
    :param raw_image: the raw image
    :return: a skeletonized binary image of the edges, focusing more on
    providing completely enclosed areas than on clean skeletons
    """
    edges = canny(raw_image, sig)
    edgesFilt = gaussian_filter(edges, gausFilt)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh

    im = morphology.erosion(edges,morphology.square(7))

    im2 = morphology.skeletonize(im > 0)

    image = im2.astype(np.int) * 255

    print "made skeleton_test image"
    cv2.imwrite("Images/skeleton_test.jpg", image)
    return image