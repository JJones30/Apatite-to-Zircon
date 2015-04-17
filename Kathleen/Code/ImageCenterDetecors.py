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






def raycastWithIdealCrystal(skeleton, numrays, sample_point):
    """
    sees how close ray cast from points match ideal crystal
    :param raw_image: the raw image
    :return: an image with the values from ray casting and how close they match the ideal crystal
    """

    sample_image = cv2.imread('Images/Orig/3396_664.jpg', 0)
    sample_edges = sk.denoiseSkeleton(sk.erodeEdges(sample_image, 2.5, 3.5), 25000)
    sample_rays = rc.rotatingWhiteDistance(sample_edges,sample_point, numrays)[0]


    image_dimensions = np.shape(skeleton)
    newImage = np.zeros(image_dimensions)

    squareSize = 15

    for x in range(0,len(skeleton),squareSize):
        for y in range(0,len(skeleton[x]),squareSize):
            point_rays = rc.rotatingWhiteDistance(skeleton, (x,y), numrays)[0]
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


def inverseConnnected(skeleton, color_image, minCirc=.35, maxSize=200000, minSize=5000):
    """
    Find bodies of crystals and centers by running connected components on the
    inverted skeleton
    :param skeleton: skeleton of image
    :param color_image: color image of slide
    :param minCirc: minimum circularity for body to be considered a crystal
    :param maxSize: maximum size for body to be considered a crystal
    :param minSize: minimum size for body to be considered a crystal
    :return: list of centers of crystals and list of bodies of crystals
    """

    #Invert and dilate the skeleton to use for connected components
    inverse = 1 - skeleton
    for x in range(len(inverse)):
        for y in range(len(inverse[x])):
            if inverse[x][y] != 1:
                inverse[max(x - 1, 0)][y] = 0
                inverse[x][y] = 0
                inverse[x][max(y - 1, 0)] = 0

    print "created inverted skeleton"
    cv2.imwrite("Images/inverse_skel.jpg", inverse*255)

    image_dimensions = np.shape(skeleton)
    final = np.zeros(image_dimensions)
    (maxX, maxY) = image_dimensions

    #Find connected components
    lw, num = measurements.label(inverse, [[1,1,1],[1,1,1],[1,1,1]])
    area = measurements.sum(inverse, lw, index=arange(lw.max() + 1))


    #Filter out areas where the crystal size is too large or too small
    filtareas = filter(lambda x: x < maxSize and x > minSize and x != 0, area)
    # Get list of bodies with their associated points from the labeled image
    # Filters out bodies that are too large or small
    bodies = getLabeledBodies(lw, lw.max() + 1, area, maxSize, minSize)
    # For each body, calculate the bounding box
    boundings = map(findBounding,bodies)
    # Use the bounding boxes to estimate the crystals' perimeters
    perimeters = estimatePerim(boundings)
    # Use the estimated perimeters and ares to estimate the crystals' circularities
    circularities = calcCircularity(perimeters, filtareas)

    # Find the centers of masses for all bodies in the labeled image
    index=  arange(lw.max())
    index[0] = 1
    center_mass_unfilt = measurements.center_of_mass(inverse, lw, index)
    # Filter out the centers of bodies that were too large or too small
    center_mass = []
    for i in range(len(center_mass_unfilt)):
        areaSize = area[i]
        if areaSize < maxSize and areaSize > minSize and areaSize != 0:
            center_mass.append(center_mass_unfilt[i])

    # Remove all bodies and centers from crystals that do not meet the minimum circularity requirement
    newBodies = []
    newCenters = []
    for i in range(len(bodies)):
        circ = circularities[i]
        if circ > minCirc:
            newBodies.append(bodies[i])
            newCenters.append(center_mass[i])

        else: # see which ones are removed
            for point in bodies[i]:
                color_image[point[0]][point[1]] = [0,0,255]

    bodies = newBodies
    center_mass = newCenters


    # Create image to visualize
    colors = map((lambda x:np.random.rand(3) * 255), bodies)
    red = [0, 0, 255]
    centers = []

    # Color in all crystal bodies different colors
    for i in range(len(bodies)):
        body = bodies[i]
        color = colors[i]
        for point in body:
            color_image[point[0]][point[1]] = color
            final[point[0]][point[1]] = 1

        center = center_mass[i]
        centers.append(center)

    # Make a point in each body for the center
    for point in centers:
        x = int(point[0])
        y = int(point[1])

        for i in range(x -7, x + 7):
            for j in range(y-7, y+7):
                if i >= 0 and i < maxX and j >= 0 and j < maxY:
                    color_image[i][j] = red

    # "Final" ia binary image where the crystals are 1s and the rest is 0s if later implementations
    # want to use the algorithm in conjunction with other algorithms that give results in that form
    # Not used in this implementation

    #final= gaussian_filter(final, sigma=20)



    print "made connected inverse"
    cv2.imwrite("Images/inverse_connected.jpg", color_image)
    cv2.imwrite("Images/inverse_connected_map.jpg", final*255)
    return centers, bodies

def calcCircularity(perims, areas):
    """
    Calculate the circularity of the bodies based on their areas and and perimeters
    :param perims: list of bodies' estimated perimeters
    :param areas: list of bodies' areas
    :return: list of bodies' circularities
    """
    circs = []
    for i in range(len(perims)):
        p = perims[i]
        a = areas[i]
        c = (4*math.pi*a)/(p**2)
        circs.append(c)
    return circs


def calcPerims(labels, numLabels, bodies):
    """
    Calculate the actual perimeters of bodies. Not used currently due to long run time.
    Also highly sensitive to features within the crystal that cause gaps in the body
    :param labels: labeled image
    :param numLabels: number of different labels in image
    :param bodies: list of bodies
    :return: list of perimeters for each body
    """
    perimeters = []
    image_dimensions = np.shape(labels)

    # For each body, create a new image that is just that body
    # Run edge detector on that body and calculate the rsulting perimeter of that body
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
    """
    Estimate perimeters of crystals based on their bounding boxes
    :param bounding: list of bounding boxes for crystals
    :return: list of estimated perimeters
    """
    perims = []
    for (xmax, xmin, ymax, ymin) in bounding:
        perim = 2*((xmax - xmin) + (ymax - ymin))
        perims.append(perim)
    return perims



def getLabeledBodies(labels, numLabels, areas, maxSize, minSize):
    """
    Format a labeled image into a list of separate bodies, filtering out
    bodies that are too large or small
    :param labels: labeld image of bodies
    :param numLabels: number of labels
    :param areas: list of bodies' areas
    :param maxSize: maximum allowed size for a crystal
    :param minSize: minimum allowed size for a crystal
    :return: list of bodies, which are lists of points in the body
             large and small bodies are filtered out of the list
    """
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

    # Remove empty bodies which were bodies that were too large or small to include
    bodies = filter(lambda x: x != [], bodies)

    print "done finding bodies"
    return bodies

def findBounding(points):
    """
    Calculate the bounding box for points in a body
    :param points: list of points in a single body
    :return:the bounding box of the body, as a tuple of the max x, min x, max y, and min y values
    """
    xvals = map(lambda x: x[0], points)
    yvals = map(lambda x: x[1], points)

    box =  (max(xvals), min(xvals), max(yvals), min(yvals))

    return box













