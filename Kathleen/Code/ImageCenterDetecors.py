import Skeletonizer as sk
import numpy as np
import matplotlib.pyplot as plt
import cv2
import RayCast as rc
from skimage.filter import (canny, gaussian_filter, threshold_otsu)
from skimage import measure, morphology
from matplotlib import colors

import scipy as sc
from scipy.ndimage import measurements
from pylab import (arange)
import math


from skimage.draw import (line, polygon, circle,
                          circle_perimeter,
                          ellipse, ellipse_perimeter,
                          bezier_curve)

def raycastOnLimitedAreas(chosen_areas, skeleton, numrays, sample_point):
    sample_image = cv2.imread('Images/3396_664.jpg', 0)
    sample_edges = sk.denoiseSkeleton(sk.erodeEdges(sample_image, 2.5, 3.5), 25000)
    sample_rays = rc.rotatingWhiteDistance(sample_edges,sample_point, numrays)[0]


    image_dimensions = np.shape(skeleton)
    newImage = np.zeros(image_dimensions)

    squareSize = 15
    notUsed = []

    for x in range(0,len(skeleton),squareSize):
        for y in range(0,len(skeleton[x]),squareSize):
            if chosen_areas[x][y] > .9:
                point_rays = rc.rotatingWhiteDistance(skeleton, (x,y), numrays)[0]
                val = differenceBetweenRays(sample_rays, point_rays)
                newImage[x][y] = val
            else:
                notUsed.append((x,y))

    print "done casting rays"

    maxVal = np.amax(newImage)
    for val in notUsed:
        x,y = val
        newImage[x][y] = maxVal

    makeSquaresSameValue(newImage, squareSize)

    print "done normalizing values"


    probs =  1 - make01Values(newImage)


    #Display point in image
    #for x in range(sample_point[0] - 5, sample_point[0] + 5):
        #for y in range(sample_point[1] - 5, sample_point[1] + 5):
            #sample_edges[x][y] = 255
    #print "wrote chosen point image"
    #cv2.imwrite('Images/choose_center_point.jpg',sample_edges)


    print "made ideal crystal ray cast image"
    cv2.imwrite('Images/ideal_crystal_cast.jpg',probs* 255)


    return probs




def raycastWithIdealCrystal(skeleton, numrays, sample_point):
    """
    sees how close ray cast from points match ideal crystal
    :param raw_image: the raw image
    :return: an image with the values from ray casting and how close they match the ideal crystal
    """
    #sample_image = cv2.imread('Images/22018.jpg', 0)
    sample_image = cv2.imread('Images/3396_664.jpg', 0)
    sample_edges = sk.denoiseSkeleton(sk.erodeEdges(sample_image, 2.5, 3.5), 25000)
    #sample_point = (800,650)
    sample_rays = rc.rotatingWhiteDistance(sample_edges,sample_point, numrays)[0]


    image_dimensions = np.shape(skeleton)
    newImage = np.zeros(image_dimensions)

    squareSize = 15

    for x in range(0,len(skeleton),squareSize):
        for y in range(0,len(skeleton[x]),squareSize):
            point_rays = rc.rotatingWhiteDistance(skeleton, (x,y), numrays)[0]
            #val = compareRayAverages(sample_rays,point_rays)
            val = differenceBetweenRays(sample_rays, point_rays)
            newImage[x][y] = val

    makeSquaresSameValue(newImage, squareSize)


    probs = 1 - make01Values(newImage)


    #Display point in image
    for x in range(sample_point[0] - 5, sample_point[0] + 5):
        for y in range(sample_point[1] - 5, sample_point[1] + 5):
            sample_edges[x][y] = 255
    print "wrote chosen point image"
    cv2.imwrite('Images/choose_center_point.jpg',sample_edges)


    print "made ideal crystal ray cast image"
    cv2.imwrite('Images/ideal_crystal_cast.jpg',probs* 255)


    return probs




def differenceBetweenRays(ideal, sample):
    """
    Calculates the difference between the array of ideal rays and the array
    of sample rays to return a score. Assumes both arrays are the same size.
    :param ideal: array of ideal rays from ray casting at a center of a crystal
    :param sample: array of rays from a sample point in the image
    :return: a score for the sample point
    """
    # distance form ideal crystal
    totalDif = 0.0
    for i in range(len(ideal)):
        dif = abs(ideal[i] - sample[i])
        totalDif = totalDif + dif

    # are two rays from opposite side the same length?
    totalSidesDif = 0.0
    halfsize = len(sample)/2
    for i in range(halfsize):
        dif = abs(sample[i] - sample[i + halfsize])
        totalSidesDif += dif


    return totalDif + totalSidesDif

def compareRayAverages(ideal, sample):
    """
    Calculates the average of two ray arrays and compares the results
    :param ideal: ideal rays
    :param sample: sample point ays
    :return: score
    """
    avg_ideal = np.average(ideal)
    avg_sample = np.average(sample)
    return abs(avg_ideal - avg_sample)



def rayCasting(image, rays):
    """
    :param image: binary image
    :return: probability image from ray casting algorithm
    """
    image_dimensions = np.shape(image)
    newImage = np.zeros(image_dimensions)

    squareSize = 15

    for x in range(0,len(image),squareSize):
        for y in range(0,len(image[x]),squareSize):
            dist = rc.rotatingWhiteDistance(image, (x,y), rays)[1]
            if dist < .005*image_dimensions[0]:
                dist = 255
            newImage[x][y] = 255 - dist

    makeSquaresSameValue(newImage, squareSize)

    probs = make01Values(newImage)

    return probs

def rcEdges(skeleton):
    """
    :param image: raw image
    :return: map from running ray cast on the image after it has been skeletonized
    """
    #edges = erodeEdges(image)
    #edges = denoiseSkeleton(image)
    rays = rayCasting(skeleton, 20)
    cv2.imwrite('Images/ray.jpg',rays*255)
    return rays



def makeSquaresSameValue(image, numPixels):
    """for functions that only calculate some pixels, use this to make all nearby
    pixels the same value """

    for x in range(0,len(image),numPixels):
        xbound = len(image)
        ybound = len(image[x])
        for y in range(0,len(image[x]),numPixels):
            value = image[x][y]
            for i in range(x - numPixels, x + numPixels):
                for j in range(y - numPixels, y + numPixels):
                    if i > 0 and j > 0 and i < xbound and j < ybound:
                        image[i][j] = value


def make01Values(image):
    """
    normalize values of an image to be between 0 and 1
    """
    image_dimensions = np.shape(image)
    newImage = np.zeros(image_dimensions)
    minPoint = float(np.min(image))
    maxPoint = float(np.max(image)) - minPoint
    for x in range(len(image)):
        for y in range(len(image[x])):
            newImage[x][y] = (image[x][y] - minPoint)/maxPoint

    return newImage






def makeSkeleton(raw_image):
    """
    not working as well as erode edges
    :param raw_image: a raw image
    :return: a clean skeletonized binary image of the edges
    """
    im = cv2.cvtColor(raw_image, cv2.COLOR_BGR2GRAY)
    im = cv2.threshold(im, 0, 255, cv2.THRESH_OTSU)[1]
    cv2.imwrite("Images/skeleton_test_1.jpg", im)

    im2 = morphology.skeletonize(im > 0)

    image = im2.astype(np.int) * 255


    cv2.imwrite("Images/skeleton_test_end.jpg", image)

    return






def makeDstTransofrm(raw_image, skel, ideal, rangeVal):
    """
    :param raw_image: the image as a raw image
    :param bw_image: the grayscaled image
    :return: a map of values weighted by how far the point is to the nearest edge, thresholded to
    optimize center values
    """

    im = cv2.cvtColor(raw_image, cv2.COLOR_BGR2GRAY)

    im = cv2.threshold(im, 0, 255, cv2.THRESH_OTSU)[1]

    for x in range(len(im)):
        for y in range(len(im[x])):
            im[x][y] = skel[x][y]

    im = cv2.bitwise_not(im)

    dstTrans = cv2.distanceTransform(im, distanceType=1, maskSize=5)

    dstTrans = (make01Values(dstTrans) * 255)

    dstTrans = weightToBestValue(dstTrans, ideal, rangeVal)

    print "made distance transform image"
    cv2.imwrite("Images/dstTrans_test_end.jpg", dstTrans)


    return dstTrans

def weightToBestValue(image,value, rng):
    """
    :param image: gray scale image of integer values
    :param value: optimal value
    :param rng: how far a value can be from optimal value
    :return: a gray scale image weighted by how far the values are from the range of optimal values
    """
    image_dimensions = np.shape(image)
    newImage = np.zeros(image_dimensions)
    for x in range(len(image)):
        for y in range(len(image[x])):
            if image[x][y] > value + rng or image[x][y] < value - rng:
                newImage[x][y] = 255 - abs(image[x][y] - value)
            else: newImage[x][y] = 255
    return newImage

def fillConectedAreas(skeleton):
    """
    :param image: raw image
    :return: an attempt to have just the enclosed areas filled in
    """
    fill_holes = sc.ndimage.binary_fill_holes(skeleton).astype(int)
    final = (fill_holes * 255) - skeleton
    final= make01Values(gaussian_filter(final, sigma=20))*255
    print "made fill holes image"
    cv2.imwrite("Images/fill_holes_test.jpg", final)
    return final


def dstTransJustCenters(dstTrans, thresh, maxSize):
    """given the distance transform image, return a binary threshold with the larger components removed"""

    #edges = threshold_otsu(dstTrans, 254)
    #edges = cv2.threshold(dstTrans,dstTrans,255,cv2.THRESH_BINARY)
    #edges = canny((1 - make01Values(dstTrans)) * 255)
    edges = (dstTrans > thresh).astype(np.int)

    lw, num = measurements.label(edges)
    area = measurements.sum(edges, lw, index=arange(lw.max() + 1))

    for x in range(len(lw)):
        for y in range(len(lw[x])):
            label = lw[x][y]
            areaSize = area[label]
            if areaSize >= maxSize and areaSize != 0:
                edges[x][y] = 0

    print "made dst trans centers image"
    cv2.imwrite("Images/test_dst_trans_center.jpg", edges * 255)


    return edges


def inverseConnnected(skeleton, color_image):

    inverse = 1 - skeleton
    for x in range(len(inverse)):
        for y in range(len(inverse[x])):
            if inverse[x][y] != 1:
                inverse[max(x - 1, 0)][y] = 0
                inverse[x][y] = 0
                inverse[x][max(y - 1, 0)] = 0


    cv2.imwrite("Images/inverse_skel.jpg", inverse*255)

    image_dimensions = np.shape(skeleton)
    final = np.zeros(image_dimensions)

    (maxX, maxY) = image_dimensions

    minCirc = .35 #.4
    maxSize = 40000
    minSize = 5000#3000


    lw, num = measurements.label(inverse, [[1,1,1],[1,1,1],[1,1,1]])
    area = measurements.sum(inverse, lw, index=arange(lw.max() + 1))
    filtareas = filter(lambda x: x < maxSize and x > minSize and x != 0, area)

    bodies = getLabeledBodies(lw, lw.max() + 1, area, maxSize, minSize)
    boundings = map(findBounding,bodies)

    #perimeters = calcPerims(lw, lw.max() + 1, bodies)
    perimeters = estimatePerim(boundings)


    circularities = calcCircularity(perimeters, filtareas)




    center_mass_unfilt = measurements.center_of_mass(inverse, lw, index=arange(lw.max() + 1))

    center_mass = []
    for i in range(len(center_mass_unfilt)):
        areaSize = area[i]
        if areaSize < maxSize and areaSize > minSize and areaSize != 0:
            center_mass.append(center_mass_unfilt[i])

    newBodies = []
    newCenters = []
    for i in range(len(bodies)):
        circ = circularities[i]
        #print circ
        if circ > minCirc:

            newBodies.append(bodies[i])
            newCenters.append(center_mass[i])

        else: # see which ones are removed

            for point in bodies[i]:
                color_image[point[0]][point[1]] = [0,0,255]

    bodies = newBodies
    center_mass = newCenters


    colors = map((lambda x:np.random.rand(3) * 255), bodies)

    red = [0, 0, 255]
    centers = []

    for i in range(len(bodies)):
        body = bodies[i]
        color = colors[i]
        for point in body:
            color_image[point[0]][point[1]] = color
            final[point[0]][point[1]] = 1

        center = center_mass[i]
        centers.append(center)


    for point in centers[1:]:
        x = int(point[0])
        y = int(point[1])

        for i in range(x -7, x + 7):
            for j in range(y-7, y+7):
                if i >= 0 and i < maxX and j >= 0 and j < maxY:
                    color_image[i][j] = red


    final= gaussian_filter(final, sigma=20)



    print "made connected inverse"
    cv2.imwrite("Images/inverse_connected.jpg", color_image)
    cv2.imwrite("Images/inverse_connected_map.jpg", final*255)
    return final, centers, bodies

def calcCircularity(perims, areas):
    print len(perims), len(areas)
    circs = []
    for i in range(len(perims)):
        p = perims[i]
        a = areas[i]
        c = (4*math.pi*a)/(p**2)
        circs.append(c)
    return circs


def calcPerims(labels, numLabels, bodies):

    #print numLabels
    perimeters = []
    image_dimensions = np.shape(labels)
    binary = np.zeros(image_dimensions)
    for i in range(len(bodies)):
        binary = np.zeros(image_dimensions)

        for point in bodies[i]:
            binary[point[0]][point[1]] = 1



        edges = canny(binary, sigma=.5)
        edgesFilt = gaussian_filter(edges, 1)
        thresh = threshold_otsu(edgesFilt)
        edges = edgesFilt > thresh

        edges = edges.astype(np.bool)
        binary = binary.astype(np.bool)

        newEdges = np.add(binary,edges)

        newEdges = newEdges.astype(np.int)




        perim = measure.perimeter(newEdges, neighbourhood=4)
        perimeters += [perim]

    return perimeters


def estimatePerim(bounding):
    #print bounding
    perims = []
    for (xmax, xmin, ymax, ymin) in bounding:
        perim = 2*((xmax - xmin) + (ymax - ymin))
        perims.append(perim)
    return perims



def getLabeledBodies(labels, numLabels, areas, maxSize, minSize):
    bodies = []
    for i in range(numLabels):
        bodies.append([])

    for x in range(len(labels)):
        for y in range(len(labels[x])):
            point = (x,y)
            l = labels[x][y]
            areaSize = areas[l]
            if areaSize < maxSize and areaSize > minSize and areaSize != 0:
                bodies[l].append(point)

    bodies = filter(lambda x: x != [], bodies)

    print "done finding bodies"
    return bodies

def findBounding(points):
    xvals = map(lambda x: x[0], points)
    yvals = map(lambda x: x[1], points)

    box =  (max(xvals), min(xvals), max(yvals), min(yvals))

    return box













