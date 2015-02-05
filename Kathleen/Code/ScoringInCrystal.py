import numpy as np
import cv2
import ImageCenterDetecors as icd
import choose_centers as cc
import Skeletonizer as sk


#file = 'Images/1987_708.jpg'
file = 'Images/Focused_ScopeStack.jpg'
#file = 'Images/5x5_1_composite.jpg'
#file = 'Images/5x5_2_composite.jpg'
#file = 'Images/Focused_0_0.jpg'
#file = 'Images/stitched_1.jpg'
#file = 'Images/3396_664.jpg'
### create image of all 1s to store scores ###
#raw_image = cv2.imread('Images/19848.jpg', 0)
#file = 'Images/3x3_1_composite_new_slide.jpg'
#file = 'Images/LightChange2/Focused_0_0(6).jpg'
raw_image = cv2.imread(file, 0)
color_image = cv2.imread(file)


#raw_image = cv2.imread('Images/22018.jpg', 0)
#raw_image = cv2.imread('Images/0_0.jpg', 0)
#raw_image = cv2.imread('Images/Capture1.png', 0)
#raw_image = cv2.imread('Images/191052.jpg', 0)
#color_image = cv2.imread('Images/19848.jpg')
#color_image = cv2.imread('Images/914_480.jpg')
#color_image = cv2.imread('Images/22018.jpg')
#color_image = cv2.imread('Images/0_0.jpg')
#color_image = cv2.imread('Images/Capture1.png')


image_dimensions = np.shape(raw_image)
scoreImg = np.zeros(image_dimensions)


eroded_image = sk.erodeEdges(raw_image, 3, 4.5)
cv2.imwrite("Images/preprocess_eroded_image.jpg", eroded_image)
denoised_skel = sk.denoiseSkeleton(eroded_image, 25000)

"""
full_skeleton = icd.fullSkels(raw_image, 1.5, 9)
cv2.imwrite("Images/preprocess_full_skeleton.jpg", full_skeleton)
denoised_full_skel = icd.denoiseSkeleton(full_skeleton, 25000)
cv2.imwrite("Images/preprocess_denoised_full_skel.jpg", denoised_full_skel)
"""


connectedSkel = sk.getConnectedCompontents(denoised_skel, np.copy(color_image))


#icd.denoiseSkeleton(raw_image)
#icd.makeEdges(raw_image)

#icd.fullSkels(raw_image)
#icd.colorEdges(raw_image, np.copy(color_image))
#icd.makeSkeleton(color_image)
#icd.erodeEdges(raw_image)



### form a bunch of images with values between 0-1 for probabilities ###

# testing map generated randomly
#random_image = icd.randomImage(raw_image)


dstTrans = icd.make01Values(icd.makeDstTransofrm(color_image, connectedSkel, 20, 10))
#dstTrans_basic = icd.basicDstTrans(color_image, connectedSkel)

dstTrans_center = icd.dstTransJustCenters(dstTrans,.99,15000)


filled_crysts = icd.make01Values(icd.fillConectedAreas(connectedSkel))

ideal_crystal_rays = icd.raycastWithIdealCrystal(connectedSkel,20, (1050,1200))
#ideal_crystal_rays = icd.raycastOnLimitedAreas(filled_crysts,connectedSkel,20, (1050,1200))


#ray_edges = icd.rcEdges(denoised_skel)



#ray_lines = icd.rcAllLines(raw_image)
all_images = [(ideal_crystal_rays, 2),  (filled_crysts, .5), (dstTrans_center, .25)] # all images and associated weights

#### add base image by each probability image ###
for (im, wt) in all_images:
    scoreImg = np.add(im * wt,scoreImg)



### convert probability image into displayable heat map ###
scoreImg = icd.make01Values(scoreImg)
scoreImg = scoreImg * 255

cv2.imwrite('Images/center_map2.jpg',scoreImg)


centers = cc.chooseCenters(scoreImg,color_image)
print "Number of crystals found:",
print len(centers)