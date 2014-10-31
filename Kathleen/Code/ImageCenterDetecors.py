
import numpy as np
import matplotlib.pyplot as plt
import cv2
import RayCast as rc
from skimage.filter import (canny, gaussian_filter, threshold_otsu)
from skimage import measure, morphology
from matplotlib import colors

import scipy as sc

from skimage.draw import line

from skimage.draw import (line, polygon, circle,
                          circle_perimeter,
                          ellipse, ellipse_perimeter,
                          bezier_curve)


def randomImage(image):
    """
    :param image: raw image
    :return: np array of image dimension with random probabilities as values
    """
    image_dimensions = np.shape(image)
    return np.random.uniform(0,1, size=image_dimensions)

def rayCasting(image):
    """
    :param image: binary image
    :return: probability image from ray casting algorithm
    """
    image_dimensions = np.shape(image)
    newImage = np.zeros(image_dimensions)

    squareSize = 15

    for x in range(0,len(image),squareSize):
        for y in range(0,len(image[x]),squareSize):
            dist = rc.rotatingWhiteDistance(image, (x,y), 20)[1]
            if dist < .005*image_dimensions[0]:
                dist = 255
            newImage[x][y] = 255 - dist

    makeSquaresSameValue(newImage, squareSize)

    probs = make01Values(newImage)

    return probs

def rcEdges(image):
    #edges = makeEdges(image)
    edges = erodeEdges(image)
    return rayCasting(edges)

def rcAllLines(image):
    lines = makeAllLines(image)
    return rayCasting(lines)

def makeSquaresSameValue(image, numPixels):
    """for functions that only calculate some pixels, use this to make all nearby
    pixels the same value """

    for x in range(0,len(image),numPixels):
        xbound = len(image)
        ybound = len(image[x])
        for y in range(0,len(image[x]),numPixels):
            point = (x,y)
            value = image[x][y]
            for i in range(x - numPixels, x + numPixels):
                for j in range(y - numPixels, y + numPixels):
                    if i > 0 and j > 0 and i < xbound and j < ybound:
                        image[i][j] = value


def make01Values(image):
    image_dimensions = np.shape(image)
    newImage = np.zeros(image_dimensions)
    maxPoint = float(np.max(image))
    for x in range(len(image)):
        for y in range(len(image[x])):
            newImage[x][y] = image[x][y]/maxPoint
    return newImage


def makeEdges(raw_image):
    edges = canny(raw_image, sigma=2.5)
    allLines = canny(raw_image, sigma=.25)
    edgesFilt = gaussian_filter(edges, 2)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh
    edges = edges.astype(np.int) *255

    cv2.imwrite('Images/edge_testing.jpg',edges)
    return edges

def makeAllLines(raw_image):
    edges = canny(raw_image, sigma=1.5)
    allLines = canny(raw_image, sigma=.25)
    edgesFilt = gaussian_filter(edges, 2)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh
    edges = edges.astype(np.int) *255

    cv2.imwrite('Images/alllines_testing.jpg',edges)
    return edges


def colorEdges(image, color_image):


    image_dimensions = np.shape(image)
    (maxX, maxY) = image_dimensions
    newImage = np.zeros(image_dimensions)
    colors.rgb_to_hsv(color_image)

    #fig, (ax1) = plt.subplots(ncols=1, figsize=(9, 4))
    img = makeAllLines(image)
    contours = measure.find_contours(img, 0)
    for i in range(len(contours)):
        body = contours[i]
        #coords = measure.approximate_polygon(body, tolerance=20.5)
        #ax1.plot(coords[:, 0], coords[:, 1], '-r', linewidth=2)
        (x1,y1) = body[0]
        (x2,y2) = body[-1]
        rr, cc = line(int(x1),int(y1),int(x2),int(y2))
        color = np.random.rand(3) * 255
        image[rr,cc] = 255
        for (x,y) in body:
            color_image[min(x + 1, maxX - 1)][y] = color
            color_image[max(x - 1, 0)][y] = color
            color_image[x][y] = color
            color_image[x][min(y + 1, maxY - 1)] = color
            color_image[x][max(y - 1, 0)] = color
    cv2.imwrite('Images/contour_detect.jpg',color_image)



def makeSkeleton(raw_image):
    im = cv2.cvtColor(raw_image, cv2.COLOR_BGR2GRAY)
    im = cv2.threshold(im, 0, 255, cv2.THRESH_OTSU)[1]
    cv2.imwrite("Images/skeleton_test_1.jpg", im)

    im2 = morphology.skeletonize(im > 0)

    image = im2.astype(np.int) * 255


    cv2.imwrite("Images/skeleton_test_end.jpg", image)

    return

def erodeEdges(raw_image):

    edges = canny(raw_image, sigma=2.5)
    allLines = canny(raw_image, sigma=.25)
    edgesFilt = gaussian_filter(edges, 3.5)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh

    im = morphology.erosion(edges,morphology.square(7))

    im2 = morphology.skeletonize(im > 0)

    image = im2.astype(np.int) * 255


    cv2.imwrite("Images/erosion_test.jpg", image)
    return image



def makeDstTransofrm(raw_image, bw_image):
    skel = erodeEdges(bw_image)

    im = cv2.cvtColor(raw_image, cv2.COLOR_BGR2GRAY)

    im = cv2.threshold(im, 0, 255, cv2.THRESH_OTSU)[1]


    edg = cv2.bitwise_not(im)

    for x in range(len(im)):
        for y in range(len(im[x])):
            im[x][y] = skel[x][y]

    im = cv2.bitwise_not(im)
    cv2.imwrite("Images/skeleton_test_1.jpg", im)

    dstTrans = cv2.distanceTransform(im, distanceType=1, maskSize=5)

    dstTrans = (make01Values(dstTrans) * 255)

    dstTrans = weightToBestValue(dstTrans, 150, 20)

    cv2.imwrite("Images/dstTrans_test_end.jpg", dstTrans)


    return dstTrans

def weightToBestValue(image,value, rng):
    image_dimensions = np.shape(image)
    newImage = np.zeros(image_dimensions)
    for x in range(len(image)):
        for y in range(len(image[x])):
            if image[x][y] > value + rng or image[x][y] < value - rng:
                newImage[x][y] = 255 - abs(image[x][y] - value)
            else: newImage[x][y] = 255
    return newImage

def fillConectedAreas(image):
    skeleton = erodeEdges(image)

    fill_holes = sc.ndimage.binary_fill_holes(skeleton).astype(int)
    cv2.imwrite("Images/fill_holes_test.jpg", fill_holes * 255)



