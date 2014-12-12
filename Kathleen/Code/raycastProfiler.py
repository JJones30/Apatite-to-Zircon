__author__ = 'Clinic'

import time
from RayCast import rotatingWhiteDistance, rotatingWhiteDistance_b
import cv2
import numpy as np
import matplotlib.pyplot as plt

im_base = cv2.imread("Images/1.jpg")
im = cv2.threshold(im_base, thresh=150, maxval=255, type=cv2.THRESH_BINARY)[1]
im = im[600:700, 700:800]
im = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
im = cv2.Canny(im, 128, 200)

end_times = []
rwd = np.zeros((100,100))
for j in range(0, 100, 3):
    for i in range(0, 100, 3):
        print (i,j)
        start_time = time.time()

        new = rotatingWhiteDistance_b(im, (i,j))
        thing= np.std(new[0])
        rwd[i][j]=thing

        end_times.append(time.time() - start_time)

print "mean time:", np.mean(end_times)
print "stdev time:", np.std(end_times)
plt.subplot(2,2,(1,2))
plt.hist(end_times, bins=25)
plt.subplot(223)
plt.imshow(rwd)
plt.subplot(224)
plt.imshow(im)
plt.show()
print end_times