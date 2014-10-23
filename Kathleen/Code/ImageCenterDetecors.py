
import numpy as np
import matplotlib.pyplot as plt
import cv2
import RayCast as rc
from skimage.filter import (canny, gaussian_filter, threshold_otsu)
from skimage import measure
from matplotlib import colors

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
            dist = rc.rotatingWhiteDistance(image, (x,y), 5)[1]
            if dist < .01*image_dimensions[0]:
                dist = 255
            newImage[x][y] = 255 - dist

    makeSquaresSameValue(newImage, squareSize)

    probs = make01Values(newImage)

    return probs

def rcEdges(image):
    edges = makeEdges(image)
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

    cv2.imwrite('edge_testing.jpg',edges)
    return edges

def makeAllLines(raw_image):
    edges = canny(raw_image, sigma=1.5)
    allLines = canny(raw_image, sigma=.25)
    edgesFilt = gaussian_filter(edges, 2)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh
    edges = edges.astype(np.int) *255

    cv2.imwrite('alllines_testing.jpg',edges)
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
    cv2.imwrite('contour_detect.jpg',color_image)
    print "done"
