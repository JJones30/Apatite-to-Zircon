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

image =  io.imread("Capture1.png", as_grey=True)
#image =  io.imread("19848.jpg", as_grey=True)

#edges = canny(image, sigma=.25)
edges = image
#allLines = canny(image, sigma=.25)

#edgesFilt = gaussian_filter(edges, 1)
edgesFilt = edges
thresh = threshold_otsu(edgesFilt)
edges = edgesFilt > thresh
"""

allFilt = gaussian_filter(allLines, 2)
allThresh = threshold_otsu(allFilt)
allLines = allFilt > allThresh

no_edges = allLines - edges
no_edges_filt = gaussian_filter(no_edges, 2)
no_edge_thresh = threshold_otsu(no_edges_filt)
no_edges = no_edges_filt > no_edge_thresh
"""


test1 = label(edges, neighbors=8, background=0, return_num=False)

#test1 = test1.astype(np.float)/ max(np.max(test1), 1)


io.imsave('label.jpg',test1)
io.imsave('edges.jpg',image)







