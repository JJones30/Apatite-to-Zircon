import numpy as np
import matplotlib.pyplot as plt
import cv2

from skimage import io

from skimage.transform import (hough_line, hough_line_peaks,
                               probabilistic_hough_line, warp, AffineTransform)
from skimage.filter import (canny, gaussian_filter, threshold_otsu)
from skimage.viewer import ImageViewer
from skimage.draw import (line, ellipse, circle)
from skimage.measure import label

from matplotlib import pyplot as plt

from skimage.feature import corner_harris, corner_subpix, corner_peaks, corner_moravec, corner_shi_tomasi


raw_image =  io.imread("19848.jpg", as_grey=True)


edges = canny(raw_image, sigma=.25)

allLines = canny(raw_image, sigma=.25)
edges = canny(raw_image, sigma=3)
edgesFilt = gaussian_filter(edges, 2)
thresh = threshold_otsu(edgesFilt)
edges = edgesFilt > thresh

print edges
newEdges = edges.astype(np.int)

for x in range(len(edges)):
    for y in range(len(edges[x])):
        if edges[x][y]:
            newEdges[x][y] = 255
        else:
            newEdges[x][y] = 1

print newEdges



coords = corner_peaks(corner_shi_tomasi(edges, .5), min_distance=50)
#coords_subpix = corner_subpix(raw_image, coords, window_size=13)

#print coords_subpix

for coord in coords:
    rr, cc = circle(coord[0], coord[1], 7)
    raw_image[rr, cc] = 255


io.imsave('test_edges.jpg',newEdges)
io.imsave('test_corners.jpg',raw_image)







