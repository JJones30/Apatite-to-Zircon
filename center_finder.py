__author__ = 'Clinic'

import numpy as np
import matplotlib.pyplot as plt
import cv2
from Stitch.raycast import cast_rays as cast
import ellipse

diag = 1.414/2
def pixel_to_pts(pixel, center):
    print "pixel_to_pts center:", center
    print "pixel_to_pts pixel:", pixel
    x = center[0]
    y = center[1]
    pts = [(x                  , y -        pixel[0]),
           (x + diag * pixel[1], y - diag * pixel[1]),
           (x + pixel[2]       , y                  ),
           (x + diag * pixel[3], y + diag * pixel[3]),
           (x                  , y +        pixel[4]),
           (x - diag * pixel[5], y + diag * pixel[5]),
           (x -        pixel[6], y                  ),
           (x - diag * pixel[7], y - diag * pixel[7])]
    pts = [(int(x[0]), int(x[1])) for x in pts]
    print "pixel_to_pts pts:", pts
    return pts


image = cv2.imread("C:\Users\Clinic\PycharmProjects\Apatite-to-Zircon\\test_images\\1x1 skeleton denoised connected.jpg")
ray_array = cast(image, blur=True, resolution=7)
ray_im = cv2.merge(ray_array)



centers = []
i=0
for row_ind in range(len(ray_im)):
    row = ray_im[row_ind]
    print i
    i += 1
    for pixel_ind in range(len(row)):
        pixel = row[pixel_ind]
        pts = pixel_to_pts(pixel, (pixel_ind, row_ind))
        x, y = np.array([x[0] for x in pts]), np.array([x[1] for x in pts])
        print "x:", x
        print "y:", y
        ellipse_data = ellipse.fit_ellipse(x, y)
        ellipse_center = ellipse.ellipse_center(ellipse_data)
        centers.append(ellipse_center)

implot = plt.imshow(image)
plt.plot(centers)
plt.show()