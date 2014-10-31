import numpy as np
import matplotlib.pyplot as plt
import cv2

from skimage import io

from skimage.transform import (hough_line, hough_line_peaks,
                               probabilistic_hough_line)
from skimage.filter import (canny, gaussian_filter, threshold_otsu)
from skimage.viewer import ImageViewer

from skimage.draw import line

from skimage.measure import label

from skimage import data



# Skimage things here

# Construct test image

#image =  io.imread("Capture1.png", as_grey=True)
image =  io.imread("Images/19848.jpg", as_grey=True)

# Line finding, using the Probabilistic Hough Transform
edges = canny(image, sigma=.5)

allLines = canny(image, sigma=.25)
edges = canny(image, sigma=3)
edgesFilt = gaussian_filter(edges, 2)
thresh = threshold_otsu(edgesFilt)
edges = edgesFilt > thresh

allFilt = gaussian_filter(allLines, 2)
allThresh = threshold_otsu(allFilt)
allLines = allFilt > allThresh

no_edges = allLines - edges
no_edges_filt = gaussian_filter(no_edges, 2)
no_edge_thresh = threshold_otsu(no_edges_filt)
no_edges = no_edges_filt > no_edge_thresh

io.imsave("Images/skimageEdges.jpg", edges)
viewer = ImageViewer(no_edges)


lines = probabilistic_hough_line(no_edges, threshold=2, line_length=75, line_gap=0)


print lines

no_edges = no_edges.astype(np.int) * 0
for line in lines:
    p0, p1 = line
    cv2.line(no_edges,p0,p1,(255,255,255),2)


cv2.imwrite('Images/test_lines.jpg',no_edges)





