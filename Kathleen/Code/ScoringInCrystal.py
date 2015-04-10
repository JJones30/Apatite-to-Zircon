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
#file = 'Images/Orig/Focused_ScopeStack.jpg'


raw_image = cv2.imread(file, 0)
color_image = cv2.imread(file)




eroded_image = sk.erodeEdges(raw_image, 3, 4.5)
cv2.imwrite("Images/preprocess_eroded_image.jpg", eroded_image)


# Optional time-consuming step to try connect endpoints and find more complete crystals
# Comment out these two lines for faster runs
#denoised_skel = sk.denoiseSkeleton(eroded_image, 25000)
#eroded_image = sk.getConnectedCompontents(denoised_skel, np.copy(color_image))



def connectedComponentCenterDetector(color_image, raw_image, eroded_image):
    """
    Find centers by using connected component algorithm on the image skeleton.
    Generally faster and better results than center map algorithm.
    :param color_image: color image of slide
    :param raw_image: raw image of slide
    :param eroded_image: skeleton of image
    :return: list of center points
    """

    # Use connected components to find crystal bodies and centers
    centers, bodies = icd.inverseConnnected(eroded_image, np.copy(color_image))
    # Filter out crystals that are unlikely to be apatite
    filtered_centers = cc.rankCenters(np.copy(color_image),raw_image,centers,bodies)

    return filtered_centers

def centerMapCenterDetector(color_image, raw_image, eroded_image):
    """
    Find centers using combination of algorithms that create a map
    of potential centers.
    Much slower and generally not as successful as the connected
    component algorithm.
    :param color_image: color image of slide
    :param raw_image: raw image of slide
    :param eroded_image: skeleton of image
    :return: list of center points
    """

    ### form a bunch of images with values between 0-1 for probabilities ###
    dstTrans = icd.make01Values(icd.makeDstTransofrm(color_image, eroded_image, 20, 10))
    dstTrans_center = icd.dstTransJustCenters(dstTrans,.99,15000)
    filled_crysts = icd.make01Values(icd.fillConectedAreas(eroded_image))
    ideal_crystal_rays = icd.raycastWithIdealCrystal(eroded_image,20, (1050,1200))

    all_images = [(ideal_crystal_rays, 2),  (filled_crysts, .5), (dstTrans_center, .25)] # all images and associated weights

    image_dimensions = np.shape(raw_image)
    scoreImg = np.zeros(image_dimensions)
    #### add base image by each probability image ###
    for (im, wt) in all_images:
        scoreImg = np.add(im * wt,scoreImg)


    ### convert probability image into displayable heat map ###
    scoreImg = icd.make01Values(scoreImg)
    scoreImg = scoreImg * 255

    cv2.imwrite('Images/center_map.jpg',scoreImg)

    centers = cc.chooseCenters(scoreImg,color_image, raw_image)
    filtered_centers = cc.rankCenters(np.copy(color_image),raw_image,centers,None)

    return filtered_centers


# Faster algorithm with better results for center detection
centers = connectedComponentCenterDetector(color_image,raw_image,eroded_image)

# Significantly slower algorithm often with somewhat worse results for center detection
#centers = centerMapCenterDetector(color_image,raw_image,eroded_image)


print "Number of crystals found:",
print len(centers)

# Write csv file with center locations
cc.writeToFile(centers)
