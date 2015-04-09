import numpy as np
import cv2
import ImageCenterDetecors as icd
import choose_centers as cc
import Skeletonizer as sk
import RayCast_v2 as rc2
import RayCast_v3 as rc3


#file = 'Images/SlidesToTest/3x3_slide2.jpg'
file = 'Images/SlidesToTest/3x3_slide3.jpg'
#file = 'Images/SlidesToTest/5x5_slide1.jpg'
#file = 'Images/SlidesToTest/1129-1E2.jpg'
#file = 'Images/SlidesToTest/1293-7.jpg'
#file = 'Images/SlidesToTest/1293-9.jpg'
#file = 'Images/SlidesToTest/1293-12.jpg'
#file = 'Images/SlidesToTest/1479-4.jpg'
#file = 'Images/SlidesToTest/1479-6.jpg'
#file = 'Images/SlidesToTest/Slide_on_Slide.jpg'
#file = 'Images/SlidesToTest/Slide_on_Slide_no_edges.jpg'

#file = 'Images/SlidesToTest/10x10_1_composite.jpg'

#file = 'Images/SlidesToTest/BearForceTwo_randfeather.jpg'

#file = 'Images/Orig/Focused_ScopeStack.jpg'

#file = 'Images/1987_708.jpg'
#file = 'Images/Focused_ScopeStack.jpg'
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





# testing for center finder w/o calculating map
scoreImg = cv2.imread('Images/center_map.jpg', 0)
#centers = cc.chooseCenters(scoreImg,np.copy(color_image), raw_image)

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
#denoised_skel = sk.denoiseSkeleton(eroded_image, 25000)



centers, bodies = icd.inverseConnnected(eroded_image, np.copy(color_image))
filtered_centers = cc.rankCenters(np.copy(color_image),raw_image,centers,bodies)


#full_skeleton = icd.fullSkels(raw_image, 1.5, 9)
#cv2.imwrite("Images/preprocess_full_skeleton.jpg", full_skeleton)
#denoised_full_skel = icd.denoiseSkeleton(full_skeleton, 25000)
#cv2.imwrite("Images/preprocess_denoised_full_skel.jpg", denoised_full_skel)


#connectedSkel = eroded_image
#connectedSkel = sk.getConnectedCompontents(denoised_skel, np.copy(color_image))


#icd.denoiseSkeleton(raw_image)
#icd.makeEdges(raw_image)

#icd.fullSkels(raw_image)
#icd.colorEdges(raw_image, np.copy(color_image))
#icd.makeSkeleton(color_image)
#icd.erodeEdges(raw_image)



### form a bunch of images with values between 0-1 for probabilities ###

# testing map generated randomly
#random_image = icd.randomImage(raw_image)

#icd.inverseConnnected(connectedSkel, np.copy(color_image))


#ray_cast_2 = rc2.rayCastCenterMap(connectedSkel, (1040,1200), 4)
#ray_cast_v3 = rc3.rayCastCenterMap(connectedSkel,(1040,1200),8, 1)

#dstTrans = icd.make01Values(icd.makeDstTransofrm(color_image, connectedSkel, 20, 10))
#dstTrans_basic = icd.basicDstTrans(color_image, connectedSkel)

#dstTrans_center = icd.dstTransJustCenters(dstTrans,.99,15000)


#filled_crysts = icd.make01Values(icd.fillConectedAreas(connectedSkel))
#filled_crysts = icd.inverseConnnected(connectedSkel, np.copy(color_image))

#ideal_crystal_rays = icd.raycastWithIdealCrystal(connectedSkel,20, (1050,1200))
#ideal_crystal_rays = icd.raycastOnLimitedAreas(filled_crysts,connectedSkel,20, (1050,1200))


#ray_edges = icd.rcEdges(denoised_skel)



#ray_lines = icd.rcAllLines(raw_image)
#all_images = [(ray_cast_v3, 1),  (filled_crysts, .5)]
#all_images = [(ray_cast_2, 1),  (filled_crysts, .5), (dstTrans_center, .25)]
#all_images = [(ideal_crystal_rays, 2),  (filled_crysts, .5), (dstTrans_center, .25)] # all images and associated weights

#### add base image by each probability image ###
#for (im, wt) in all_images:
    #scoreImg = np.add(im * wt,scoreImg)

#scoreImg = filled_crysts

### convert probability image into displayable heat map ###
#scoreImg = icd.make01Values(scoreImg)
#scoreImg = scoreImg * 255

#cv2.imwrite('Images/center_map.jpg',scoreImg)


#centers = cc.chooseCenters(scoreImg,color_image, raw_image)
print "Number of crystals found:",
print len(filtered_centers)

cc.writeToFile(filtered_centers)