__author__ = 'Clinic'
import numpy as np
import cv2
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import time
import Skeletonizer as sk
import ImageCenterDetecors as icd

def raycast(image, rays=4, resolution=1, ret_type="image", dist_delta=1):
    """
    :param image: must be grayscale
    :param rays: number of rays to cast. must be 2, 4 or 8. 2 is left-right, 4 gives cardinal directions, 8 adds diagonal at 45 degrees.
    :param ret_type: "image" for all of the things composited, "array" for a list of separate frames
    :return: four or eight channel image of ray lengths, clockwise from up
    """
    if not ret_type in ["image", "array"]:
        print "invalid return type in raycast call"
        return
    init_im = image
    lr_ray_im = left_right_ray(image, resolution, dist_delta)
    image = cv2.flip(image, 1)
    rl_ray_im = cv2.flip(left_right_ray(image, resolution), 1, dist_delta)
    if rays in [2, 4, 8]:
        if ret_type == "image":
            ret = cv2.merge([lr_ray_im, rl_ray_im])
        elif ret_type == "array":
            ret = [lr_ray_im, rl_ray_im]

    if rays in [4, 8]:
        image = cv2.transpose(image)
        down_ray_im = cv2.transpose(left_right_ray(image, resolution, dist_delta), 1)
        image = cv2.flip(image, 1)
        up_ray_im = cv2.flip(cv2.transpose(cv2.flip(left_right_ray(image, resolution, dist_delta), 1)), 1)
        if ret_type == "image":
            ret = cv2.merge([up_ray_im, lr_ray_im, down_ray_im, rl_ray_im])
        elif ret_type == "array":
            ret = [up_ray_im, lr_ray_im, down_ray_im, rl_ray_im]

    if rays == 8:
        skew_1, deskew_1 = skew(init_im)
        sca_ne_sw = [deskew(x, reverse_affine=deskew_1) for x in raycast(skew_1, rays=2, ret_type="array", resolution=resolution, dist_delta=1.414)]
        skew_2, deskew_2 = skew(cv2.flip(init_im, 0))
        sca_se_nw = [cv2.flip(deskew(x, reverse_affine=deskew_2), 0) for x in raycast(skew_2, rays=2, ret_type="array", resolution=resolution, dist_delta=1.414)]
        array = [up_ray_im, sca_ne_sw[0], lr_ray_im, sca_se_nw[0], down_ray_im, sca_ne_sw[1], rl_ray_im, sca_se_nw[1]]
        if ret_type == "image":
            ret = cv2.merge(array)
        elif ret_type == "array":
            ret = array
    return ret



def left_right_ray(image, resolution, dist_delta=1):
    #print image.shape
    output = np.zeros((image.shape[0], image.shape[1]))
    for row_index in range(0, len(image), resolution):
        #print row_index
        row = image[row_index]
        dist=0
        for pix_index in range(0, len(row), resolution):
            if image[row_index][pix_index] == 255:
                dist=0
                continue
            else:
                dist += dist_delta
                output[row_index][pix_index] = dist
    return output

def skew(image):
    im_x = image.shape[1]
    im_y = image.shape[0]
    src = np.array([(0,0), (im_x, 0), (im_x, im_y), (0, im_y)], np.float32)
    dst = np.array([(0,0), (im_x, im_y), (im_x,2*im_y), (0, im_y)], np.float32)
    affine = cv2.getPerspectiveTransform(src, dst)
    reverse_affine = cv2.getPerspectiveTransform(dst, src)
    image = cv2.warpPerspective(image, affine, (im_x, 2*im_y), borderMode=cv2.BORDER_CONSTANT, borderValue=255, flags=cv2.INTER_NEAREST)
    return image, reverse_affine

def deskew(image, reverse_affine):
    im_x = image.shape[1]
    im_y = image.shape[0]
    image = cv2.warpPerspective(image, reverse_affine, (im_x, int(im_y)), borderMode=cv2.BORDER_CONSTANT, borderValue=255, flags=cv2.INTER_NEAREST)
    return image[0:image.shape[0]/2, 0:image.shape[1]]

def plot_8(array):
    names=["up", "ne", "lr", "se", "dn", "sw", "rl", "nw"]
    for n in range(1,9):
        print "n is:", n
        print "sum is:", cv2.sumElems(array[n-1])
        plt.subplot("24"+str(n))
        plt.imshow(array[n-1], interpolation="none")
        plt.title(names[n-1])
    plt.show()


def rayCastCenterMap(skeleton, center_point, numRays=4, res=1 ):

    center_map = np.copy(skeleton)

    rc_output = raycast(skeleton, numRays, res)

    #print "made ray_cast_v2 image"
    #cv2.imwrite("Images/ray_cast_v2.jpg", rc_output)



    #sample_image = cv2.imread('Images/3396_664.jpg', 0)
    #sample_edges = sk.denoiseSkeleton(sk.erodeEdges(sample_image, 2.5, 3.5), 25000)
    #sample_rays = raycast(sample_edges, numRays, res)
    #sample_ray_point = sample_rays[center_point[0]][center_point[1]]
    #print "made sample_point_rays image"
    #cv2.imwrite("Images/sample_point_rays.jpg", sample_rays)

    for x in range(0,len(skeleton)):
        for y in range(0,len(skeleton[0])):
                center_map[x][y] = scorePoint(rc_output[x][y])#, sample_ray_point)
    #print skeleton
    center_map = 1 - icd.make01Values(center_map) - (.25*icd.make01Values(skeleton))

    print "made ray_cast_v3_map image"
    cv2.imwrite("Images/v3_ray_cast_map.jpg", center_map*255)

    return center_map

def scorePoint(test_point, sample_point=None):
    #totalDif = 0.0
   #for i in range(len(test_point)):
        #totalDif += abs(test_point[i] - sample_point[i])


    # are two rays from opposite side the same length?
    totalSidesDif = 0.0
    halfsize = len(test_point)/2
    for i in range(halfsize):
        dif = abs(test_point[i] - test_point[i + halfsize])
        totalSidesDif += dif



    return totalSidesDif**.5 #(totalDif + totalSidesDif)**.5