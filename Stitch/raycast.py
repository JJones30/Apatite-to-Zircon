__author__ = 'Clinic'
import numpy as np
import cv2
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import time
from skimage import morphology
import math

def raycast(image, rays=4, resolution=1, ret_type="image", dist_delta=1, blur=False):
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

    if blur:
        box_size = (int(math.ceil(resolution*2-1)), int(math.ceil(resolution*2-1)))
        if ret_type == "array":
            print "printing ret for investigative purposes:"
            print ret
            print "len(ret)", len(ret)
            print "ret[0]", type(ret[0]), ret[0]
            print "ret[1]", type(ret[1]), ret[1]

            ret = [cv2.boxFilter(x, -1, box_size) for x in ret]
        if ret_type == "image":
            ret = cv2.boxFilter(x, -1, box_size)
    return ret



def left_right_ray(image, resolution, dist_delta=1):
    print image.shape
    output = np.zeros((image.shape[0], image.shape[1]))
    for row_index in range(0, len(image), resolution):
        print row_index
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

def pix_score(pix, dims=8):
    sym_score=0
    for i in range(dims/2):
        x1 = pix[i]
        x2 = pix[i-(dims/2)]
        if x1 == 0 or x2 == 0:
            continue
        diff = abs(x1-x2)
        ave = (x1+x2)/2

        subscore = -(diff/ave)
        sym_score += subscore

    stdev_score = np.std(pix)
    stdev_score = 1/(stdev_score+0.1)

    mean_score = abs(0.1-np.mean(pix))
    mean_score = 1/(mean_score+0.1)

    return sym_score*stdev_score*mean_score

def symmetry(image, downscale=4):
    np_sh = image.shape
    cv_sh = (np_sh[1], np_sh[0])
    cv_out_size = (cv_sh[0]/downscale, cv_sh[1]/downscale)
    np_out_size = (cv_out_size[1], cv_out_size[0])
    small_im = cv2.resize(image, cv_out_size)
    out_im = np.zeros(np_out_size)
    sm_sh = out_im.shape
    for y in range(sm_sh[0]):
        for x in range(sm_sh[1]):
            score = pix_score(small_im[y][x], 8)
            out_im[y][x] = score

    return cv2.resize(out_im, (cv_sh[0], cv_sh[1]))

# perform and clock basic raycast, 8-ray
if True:
    image = cv2.imread("C:\Users\Clinic\PycharmProjects\Apatite-to-Zircon\\test_images\\1x1 skeleton denoised connected.jpg")
    image = cv2.threshold(image, thresh=128, maxval=255, type=cv2.THRESH_BINARY)[1]
    image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    image = morphology.dilation(image, morphology.diamond(3))
    #plt.imshow(image)
    #plt.show()
    start = time.time()
    global_res = 8
    ray_array = raycast(image, rays=8, ret_type="array", resolution=global_res, blur=True)
    end = time.time()
    print "total runtime:", end-start
    #plot_8(ray_array)
    im8 = cv2.merge(ray_array)
    for ray in ray_array:
        shape = ray.shape
        print cv2.sumElems(ray)[0]/(shape[0]*shape[1])
    symage = symmetry(im8, global_res)
    plt.subplot(121)
    plt.imshow(image)
    plt.subplot(122)
    plt.imshow(symage)
    plt.show()


