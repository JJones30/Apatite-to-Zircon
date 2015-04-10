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

    # Find edges using Canny Edge Detector
    edges = canny(raw_image, sigma=sig)
    edgesFilt = gaussian_filter(edges, gaus)
    thresh = threshold_otsu(edgesFilt)
    edges = edgesFilt > thresh

    # Erode edges to single-width pixels
    im = morphology.erosion(edges,morphology.square(7))
    im2 = morphology.skeletonize(im > 0)

    image = im2.astype(np.int) * 255

    print "made erosion_test image"
    cv2.imwrite("Images/erosion_test.jpg", image)
    return image


def denoiseSkeleton(skeleton, minSize):
    """
    given a skeleton of an image, return an image of the skeleton with less noise
    :param skeleton: eroded skeleton of slide
    :param minSize: minimum size of edges before they are filtered out as noise
    :return: new skeleton with small points filtered out
    """

    # Find connected components for skeleton
    lw, num = measurements.label(skeleton,[[1,1,1],[1,1,1],[1,1,1]])
    area = measurements.sum(skeleton, lw, index=arange(lw.max() + 1))

    # Loop through every point and if is comes from a label with too small of a size, remove it
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
    """
    Given a skeleton, use connected components to find separate edges and attempt
    to connect endpoints
    :param skeleton: eroded skeleton of crystals
    :param color_image: color image of slide
    :return: new skeleton with endpoints connected
    """

    image_dimensions = np.shape(skeleton)
    (maxX, maxY) = image_dimensions

    # Use connected components to find separate contours
    lw, num = measurements.label(skeleton, [[1,1,1],[1,1,1],[1,1,1]])
    area = measurements.sum(skeleton, lw, index=arange(lw.max() + 1))

    # color in separate contours to visualize results
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

    # Find the endpoints of all contours
    endpoints = findCountourEndPoints(lw)

    # color all endpoints red to visualize results
    red = [0,0,255]
    for point in endpoints:
        i,j = point
        color_image[min(i + 1, maxX - 1)][j] = red
        color_image[max(i - 1, 0)][j] = red
        color_image[i][j] = red
        color_image[i][min(j + 1, maxY - 1)] = red
        color_image[i][max(j - 1, 0)] = red

    # Create a new skeleton by attempting to connect endpoints
    newSkel = scoreEndpointLinesKDTree(endpoints, lw, color_image, skeleton)

    return newSkel


def scoreEndpointLinesKDTree(endpoints, labeled, color_image, skeleton):
    """
    Use a KDTTree to do a local search of other endpoints for each given endpoint and
    connect the best matches
    :param endpoints: list of endpoints in the image
    :param labeled: labeled image of the separate contours from connected components
    :param color_image: color image of slide
    :param skeleton: eroded skeleton of slide
    :return: new skeleton with endpoints connected
    """
    maxVal = 10000 # Max score allowed to allow endpoint connection
    dif_contour_penalty = 1.75 # multiplicative penalty for connecting two different contours
    cross_edge_penalty = 7 # multiplicative penalty for crossing over an edge to connect endpoints

    green = [0,255,0]
    canConnect = np.copy(endpoints)
    newSkel = np.copy(skeleton)

    tree = KDTree(canConnect) # make KDTree with endpoint for localized searches

    # loop through each endpoint and connect them
    # with the endpoint that receives the best score
    while len(endpoints) > 1:
        point = endpoints[0]
        pointLabel = labeled[point[0]][point[1]] # get label of endpoint's contour
        endpoints = endpoints[1:] # remove endpoint in use

        # Get possible endpoints and the distances from current endpoint
        distances, possibilities = tree.query(point, k=100, distance_upper_bound=maxVal)

        scores = []
        pts = []
        # loop through all nearby endpoints and calculate their scores
        for i in range(len(possibilities)):
            nxt = tree.data[possibilities[i]]
            score = distances[i]
            if score != 0:
                rr, cc = line(point[0], point[1], nxt[0], nxt[1])
                if labeled[nxt[0]][nxt[1]] != pointLabel: # not the same contour
                    score = score * dif_contour_penalty
                for i in range(len(rr)): # penalty for crossing over another contour to connect
                    if labeled[rr[i]][cc[i]] != 0:
                        score = score * cross_edge_penalty
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
    Locate the endpoints on a labeled skeleton image of a slide
    :param labeled: image with contours labeled as different number
    :return: list of all the contour endpoints
    """

    image_dimensions = np.shape(labeled)
    (maxX, maxY) = image_dimensions
    end_points = []

    # Loop through all points and check if point is an endpoint
    for x in range(len(labeled)):
        for y in range(len(labeled[x])):
            label = labeled[x][y]
            countNearby = 0
            if label != 0: # if it is a contour
                for i in range(max(x - 3, 0), min(x + 3, maxX - 1)): # check all nearby points and see how many
                    for j in range(max(y - 3, 0), min(y + 3, maxY - 1)): # are the same label
                        nearLabel = labeled[i][j]
                        if label == nearLabel:
                            countNearby += 1
                if countNearby <= 4: # if four or fewer nearby points share the label, it is an endpoint
                    end_points += [(x,y)]
    return end_points
