
import numpy as np
import matplotlib.pyplot as plt
import cv2
import RayCast as rc
from skimage.filter import (canny, gaussian_filter, threshold_otsu, inverse)
from skimage import measure, morphology
from matplotlib import colors

import scipy as sc
from scipy.ndimage import measurements
from pylab import (arange)

from skimage.draw import line

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
    #sample_image = cv2.imread('Images/22018.jpg', 0)
    sample_image = cv2.imread('Images/3396_664.jpg', 0)
    sample_edges = denoiseSkeleton(erodeEdges(sample_image, 2.5, 3.5), 25000)
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


    #UNCOMMENT TO DISPLAY POINT IN IMAGE
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



def randomImage(image):
    """
    :param image: raw image
    :return: np array of image dimension with random probabilities as values
    """
    image_dimensions = np.shape(image)
    return np.random.uniform(0,1, size=image_dimensions)

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


def makeEdges(raw_image):
    """
    :param raw_image: raw image
    :return: binary image after running canny edge detection and bluring and thresholding
    """
    edges = canny(raw_image, sigma=2.5)
    #allLines = canny(raw_image, sigma=.25)
    edgesFilt = gaussian_filter(edges, 2)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh
    edges = edges.astype(np.int) *255

    cv2.imwrite('Images/edge_testing.jpg',edges)
    return edges

def makeAllLines(raw_image):
    """
    Canny edge detection optimized to extract all lines (including etch marks and
    fission tracks) as well as the edges of crystals
    :param raw_image: the raw image
    :return: binary image of edges and features
    """
    edges = canny(raw_image, sigma=1.5)
    #allLines = canny(raw_image, sigma=.25)
    edgesFilt = gaussian_filter(edges, 2)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh
    edges = edges.astype(np.int) *255

    cv2.imwrite('Images/alllines_testing.jpg',edges)
    return edges


def colorEdges(image, color_image):
    """
    :param image: raw image
    :param color_image: raw color image
    :return: color image with all edges from contour detection colored in
    """

    image_dimensions = np.shape(image)
    (maxX, maxY) = image_dimensions
    #newImage = np.zeros(image_dimensions)
    colors.rgb_to_hsv(color_image)


    #img = makeAllLines(image)
    img = fullSkels(image, 2, 7)
    contours = measure.find_contours(img, 0, fully_connected="high")
    for i in range(len(contours)):
        body = contours[i]

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

def erodeEdges(raw_image,sig,gaus):
    """
    works a lot better than makeSkeleton
    :param raw_image: a raw image
    :return: a clean skeletonized binary image of the edges
    """
    edges = canny(raw_image, sigma=sig)
    #allLines = canny(raw_image, sigma=.25)
    edgesFilt = gaussian_filter(edges, gaus)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh

    im = morphology.erosion(edges,morphology.square(7))

    im2 = morphology.skeletonize(im > 0)

    image = im2.astype(np.int) * 255

    print "made erosion_test image"
    cv2.imwrite("Images/erosion_test.jpg", image)
    return image

def fullSkels(raw_image, sig, gausFilt):
    """
    :param raw_image: the raw image
    :return: a skeletonized binary image of the edges, focusing more on
    providing completely enclosed areas than on clean skeletons
    """
    edges = canny(raw_image, sig)
    #allLines = canny(raw_image, sigma=.25)
    edgesFilt = gaussian_filter(edges, gausFilt)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh

    im = morphology.erosion(edges,morphology.square(7))

    im2 = morphology.skeletonize(im > 0)

    image = im2.astype(np.int) * 255

    print "made skeleton_test image"
    cv2.imwrite("Images/skeleton_test.jpg", image)
    return image



def makeDstTransofrm(raw_image, skel, ideal, rangeVal):
    """
    :param raw_image: the image as a raw image
    :param bw_image: the grayscaled image
    :return: a map of values weighted by how far the point is to the nearest edge, thresholded to
    optimize center values
    """
    #skel = erodeEdges(bw_image)

    im = cv2.cvtColor(raw_image, cv2.COLOR_BGR2GRAY)

    im = cv2.threshold(im, 0, 255, cv2.THRESH_OTSU)[1]


    edg = cv2.bitwise_not(im)

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
    cv2.imwrite("Images/fill_holes_test.jpg", final)
    return final


def getConnectedCompontents(raw_image, color_image):

    image_dimensions = np.shape(raw_image)
    newImage = np.zeros(image_dimensions)

    thresh = threshold_otsu(raw_image)
    binary = raw_image < thresh

    image = binary.astype(np.int)

    #image = canny(raw_image, 1)
    #image = erodeEdges(raw_image)


    lw, num = measurements.label(image)
    area = measurements.sum(image, lw, index=arange(lw.max() + 1))
    colors = map((lambda x:np.random.rand(3) * 255), area)


    for x in range(len(lw)):
        for y in range(len(lw[x])):
            label = lw[x][y]
            areaSize = area[label]
            color = colors[label]
            if areaSize >= 1000 and areaSize != 0:
                color_image[x][y] = color

    print "made connected_test image"
    cv2.imwrite("Images/connected_test.jpg", color_image)


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


def matchTemplate(raw_image):
    """
    :param raw_image: grayscale image
    :return: map of where the image matches a template of a crystal
    """
    img = raw_image
    img2 = img.copy()
    template = cv2.imread('Images/Focused_ScopeStack.jpg',0)
    sizex = 50
    sizey = 50
    template = cv2.getRectSubPix(template, (sizex,sizey), (1400,270))

    #template = cv2.blur(template,(5,5))

    print "made template image"
    cv2.imwrite("Images/template_for_matching.jpg", template)

    # All the 6 methods for comparison in a list
    methods = ['cv2.TM_CCOEFF', 'cv2.TM_CCOEFF_NORMED', 'cv2.TM_CCORR',
            'cv2.TM_CCORR_NORMED', 'cv2.TM_SQDIFF', 'cv2.TM_SQDIFF_NORMED']
    meth =  'cv2.TM_SQDIFF_NORMED'

    for meth in methods:
        img = img2.copy()
        method = eval(meth)

        # Apply template Matching
        res = cv2.matchTemplate(img,template,method)


        image_dimensions = np.shape(raw_image)
        newImage = np.zeros(image_dimensions)

        res = make01Values(res)

        """
        for i in range(len(newImage)):
            for j in range(len(newImage[i])):
                if i < len(res) and j < len(res[0]):
                    newImage[i][j] = res[i][j]
                else:
                    newImage[i][j] = .25
        """

        x = sizex/2
        y = sizey/2
        newImage[x:x+res.shape[0], y:y+res.shape[1]] = res


        cv2.imwrite("Images/" + meth[4:] + "_matching.jpg", newImage * 255)
        print "made " + meth[4:] + " image"
    return newImage
