__author__ = 'Clinic'
import numpy as np
import cv2
import matplotlib.pyplot as plt
import matplotlib.cm as cm
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
    print image.shape
    output = np.zeros((image.shape[0], image.shape[1]))
    for row_index in range(len(image)):
        print row_index
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

image = cv2.imread("C:\Users\Clinic\PycharmProjects\Apatite-to-Zircon\\test_images\\1x1 skeleton denoised connected.jpg")
image = cv2.threshold(image, thresh=128, maxval=255, type=cv2.THRESH_BINARY)[1]
image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
start = time.time()
ray_im = raycast(image)
end = time.time()
print "total runtime:", end-start
plt.subplot(121)
plt.imshow(ray_im)
plt.subplot(122)
plt.imshow(image, cmap=cm.gray)
plt.show()