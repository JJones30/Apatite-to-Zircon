import numpy as np
from skimage.filter import (canny, gaussian_filter, threshold_otsu)
import cv2
import math as math
from skimage.draw import line
import ImageCenterDetecors as icd
from skimage import measure, morphology
from matplotlib import colors
import Skeletonizer as sk

def randomImage(image):
    """
    :param image: raw image
    :return: np array of image dimension with random probabilities as values
    """
    image_dimensions = np.shape(image)
    return np.random.uniform(0,1, size=image_dimensions)

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


def scoreEndpointLines(endpoints, labeled, color_image, skeleton):
    green = [0,255,0]
    canConnect = np.copy(endpoints)
    newSkel = np.copy(skeleton)

    # loop through each endpoint
    while len(endpoints) > 1:
        point = endpoints[0]
        pointLabel = labeled[point[0]][point[1]] # get label of endpoint's contour
        endpoints = endpoints[1:]

        scores = []
        for next in canConnect: # loop through all remaining endpoints

            score = getDistance(next, point)
            if score == 0: # same point
                score = float("inf")
                scores += [score]
            else:
                rr, cc = line(point[0], point[1], next[0], next[1])
                if labeled[next[0]][next[1]] != pointLabel: # not the same contour
                    score = score * 1.75
                for i in range(len(rr)): # penalty for crossing over another contour to connect
                    if labeled[rr[i]][cc[i]] != 0:
                        score = score *7
                scores += [score]

        index = np.argmin(scores)
        ##print scores[index]
        #connectPoint = endpoints.pop(index)
        connectPoint = canConnect[index]
        #connectPointLabel = labeled[connectPoint[0]][connectPoint[1]]
        #if pointLabel == connectPointLabel:
        if scores[index] < 10000:
            rr, cc = line(point[0], point[1], connectPoint[0], connectPoint[1])
            color_image[rr, cc] = green
            newSkel[rr,cc] = 255

    print "made connected_test image"
    cv2.imwrite("Images/connected_test.jpg", color_image)
    print "made connected_test_skeleton image"
    cv2.imwrite("Images/connected_test_skeleton.jpg", newSkel)

    return newSkel

def getDistance(p1,p2):
    """get Euclidean distance between two points"""
    return math.sqrt((p1[0] - p2[0])**2 +(p1[1] - p2[1])**2)

def endpointSearch(unaltered_image, endpoints, labeled, searchRange, pointsBefore):
    contours = []
    while len(endpoints) > 1:
        toSearchFor = endpoints.pop()
        unaltered_image = whiteOutLabeled(toSearchFor, unaltered_image, labeled)
        contourFill = []
        endpointLabel = labeled[toSearchFor[0]][toSearchFor[1]]
        nextPoint = (-1,-1)
        while nextPoint not in endpoints and len(contourFill) < 500:
            nextPoint = findOtherPoints(toSearchFor, unaltered_image, endpoints, labeled, 1, pointsBefore)
            unaltered_image[nextPoint[0]][nextPoint[1]][0] = -1
            contourFill += [nextPoint]
        print contourFill
        contours += [contourFill]
    return contours

def findOtherPoints(toSearchFor, unaltered_image, endpoints, labeled, searchRange, pointsBefore):
    image_dimensions = np.shape(labeled)
    (maxX, maxY) = image_dimensions
    (x,y) = toSearchFor
    pointBefore = pointsBefore[toSearchFor]

    maxVal = -float("inf")
    point = (0,0)
    for i in range(max(x - searchRange, 0), min(x + searchRange, maxX - 1)):
        for j in range(max(y - searchRange, 0), min(y + searchRange, maxY - 1)):
            dist = getDistance(pointBefore, (i,j))
            value = unaltered_image[i][j][0] * dist

            if value > maxVal:
                point = (i,j)
                maxVal = value

    return point

def whiteOutLabeled(point, image, labeled):
    image_dimensions = np.shape(labeled)
    (maxX, maxY) = image_dimensions
    (x,y) = point
    pointLabel = labeled[point[0]][point[1]]
    for i in range(max(x - 10, 0), min(x + 10, maxX - 1)):
        for j in range(max(y - 10, 0), min(y + 10, maxY - 1)):
            if labeled[i][j] == pointLabel:
                image[i][j][0] = -1
    return image

def findPointsBefore(endpoints, labeled):
    image_dimensions = np.shape(labeled)
    (maxX, maxY) = image_dimensions
    pointsBefore = {}
    for point in endpoints:
        (x,y) = point
        pointLabel = labeled[x][y]
        for i in range(max(x - 1, 0), min(x + 1, maxX - 1)):
            for j in range(max(y - 1, 0), min(y + 1, maxY - 1)):
                if pointLabel == labeled[i][j]:
                    print pointLabel, i, j
                    pointsBefore[point] = (i,j)
    return pointsBefore


def connectPointsByNearest(color_image, endpoints, lw):
    green = [0,255,0]
    while len(endpoints) > 1:
        point = endpoints[0]
        pointLabel = lw[point[0]][point[1]]
        endpoints = endpoints[1:]
        dists = map(lambda x: getDistance(x, point), endpoints)
        index = np.argmin(dists)
        #connectPoint = endpoints.pop(index)
        connectPoint = endpoints[index]
        connectPointLabel = lw[connectPoint[0]][connectPoint[1]]
        print dists[index], connectPoint, point
        if pointLabel == connectPointLabel:
            rr, cc = line(point[0], point[1], connectPoint[0], connectPoint[1])
            color_image[rr, cc] = green
    return color_image

def basicDstTrans(raw_image, skel):
    """ run out-of-box distance transform
    """

    im = cv2.cvtColor(raw_image, cv2.COLOR_BGR2GRAY)

    im = cv2.threshold(im, 0, 255, cv2.THRESH_OTSU)[1]


    edg = cv2.bitwise_not(im)

    for x in range(len(im)):
        for y in range(len(im[x])):
            im[x][y] = skel[x][y]

    im = cv2.bitwise_not(im)

    dstTrans = cv2.distanceTransform(im, distanceType=1, maskSize=5)

    dstTrans = icd.make01Values(dstTrans)


    print "made distance transform basic image"
    cv2.imwrite("Images/dstTrans_basic.jpg", dstTrans*255)

    return dstTrans

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

        res = icd.make01Values(res)

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
    img = sk.fullSkels(image, 2, 7)
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
