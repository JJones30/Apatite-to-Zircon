__author__ = 'Clinic'
import numpy as np
import cv2
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import Skeletonizer as sk
import ImageCenterDetecors as icd
import time

def raycast(image, rays=4):
    """
    :param image: must be grayscale
    :param rays: number of rays to cast. must be 4 or 8. 4 gives cardinal directions, 8 adds diagonal at 45 degrees.
    :return: four or eight channel image of ray lengths, clockwise from up
    """
    lr_ray_im = left_right_ray(image)
    image = cv2.flip(image, 1)
    rl_ray_im = cv2.flip(left_right_ray(image), 1)
    image = cv2.transpose(image)
    down_ray_im = cv2.transpose(cv2.flip(left_right_ray(image), 1))
    image = cv2.flip(image, 1)
    up_ray_im = cv2.flip(cv2.transpose(cv2.flip(left_right_ray(image), 1)), 1)
    merged_image = cv2.merge([up_ray_im, lr_ray_im, down_ray_im, rl_ray_im])
    return merged_image

def left_right_ray(image):
    #print image.shape
    output = np.zeros((image.shape[0], image.shape[1]))
    for row_index in range(len(image)):
        #print row_index
        row = image[row_index]
        dist=0
        for pix_index in range(len(row)):
            if image[row_index][pix_index] == 255:
                dist=0
                continue
            else:
                dist += 1
                output[row_index][pix_index] = dist
    return output


def rayCastCenterMap(skeleton, center_point, numRays=4 ):

    center_map = np.copy(skeleton)

    rc_output = raycast(skeleton, numRays)

    print "made ray_cast_v2 image"
    cv2.imwrite("Images/ray_cast_v2.jpg", rc_output)



    sample_image = cv2.imread('Images/3396_664.jpg', 0)
    sample_edges = sk.denoiseSkeleton(sk.erodeEdges(sample_image, 2.5, 3.5), 25000)
    sample_rays = raycast(sample_edges, numRays)
    sample_ray_point = sample_rays[center_point[0]][center_point[1]]
    print "made sample_point_rays image"
    cv2.imwrite("Images/sample_point_rays.jpg", sample_rays)

    for x in range(len(skeleton)):
        for y in range(len(skeleton[0])):
            center_map[x][y] = scorePoint(rc_output[x][y], sample_ray_point)

    center_map = 1 - icd.make01Values(center_map)

    print "made ray_cast_v2_map image"
    cv2.imwrite("Images/ray_cast_v2_map.jpg", center_map*255)

    return center_map

def scorePoint(test_point, sample_point):
    totalDif = 0.0
    for i in range(len(test_point)):
        totalDif += abs(test_point[i] - sample_point[i])


    # are two rays from opposite side the same length?
    totalSidesDif = 0.0
    halfsize = len(test_point)/2
    for i in range(halfsize):
        dif = abs(test_point[i] - test_point[i + halfsize])
        totalSidesDif += dif



    return totalDif + totalSidesDif

