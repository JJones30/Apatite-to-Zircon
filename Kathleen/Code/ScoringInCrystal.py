import numpy as np
import matplotlib.pyplot as plt
import cv2
import ImageCenterDetecors as icd


### create image of all 1s to store scores ###
#raw_image = cv2.imread('Images/19848.jpg', 0)
raw_image = cv2.imread('Images/Capture1.png', 0)
image_dimensions = np.shape(raw_image)
scoreImg = np.zeros(image_dimensions)
#color_image = cv2.imread('Images/19848.jpg')
color_image = cv2.imread('Images/Capture1.png')


icd.colorEdges(raw_image, color_image)
#icd.makeSkeleton(color_image)
icd.erodeEdges(raw_image)
icd.fillConectedAreas(raw_image)


### form a bunch of images with values between 0-1 for probabilities ###

# testing map generated randomly
random_image = icd.randomImage(raw_image)
ray_edges = icd.rcEdges(raw_image)
dstTrans = icd.make01Values(icd.makeDstTransofrm(color_image, raw_image))
#ray_lines = icd.rcAllLines(raw_image)
all_images = [(random_image, 0), (ray_edges, 1), (dstTrans, 1)] # all images and associated weights

#### add base image by each probability image ###
#scoreImg = np.add(scoreImg,ray_image)
for (im, wt) in all_images:
    scoreImg = np.add(im * wt,scoreImg)

### convert probability image into displayable heat map ###
scoreImg = icd.make01Values(scoreImg)
scoreImg = scoreImg * 255

cv2.imwrite('Images/center_map2.jpg',scoreImg)
