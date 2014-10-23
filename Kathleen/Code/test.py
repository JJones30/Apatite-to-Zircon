
import numpy as np
import matplotlib.pyplot as plt
import cv2



import math

#apatite = cv2.imread('Capture1.png')

#gray_apatite = cv2.cvtColor(apatite, cv2.COLOR_BGR2GRAY)

#thresh = cv2.adaptiveThreshold(gray_apatite, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY, 9, 10)

#cv2.imwrite('scratches.jpg', thresh)


img = cv2.imread('Capture1.png', 0)
#img = cv2.imread('191052.jpg', 0)
edges_1 = cv2.Canny(img,275,196)
edges_1 = cv2.Canny(img,275,196)
edges_2 = cv2.GaussianBlur(edges_1, (3,3), 3)
flag, edges = cv2.threshold(edges_2, 5, 255, cv2.THRESH_BINARY)

#gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
#edges = cv2.Canny(img,100,200)
#edges = cv2.Canny(gray,100,350,apertureSize = 3)


removed_edges1 = cv2.Canny(img,100,0)
removed_edges2 = cv2.subtract(removed_edges1, edges)

removed_edges3 = cv2.GaussianBlur(removed_edges2, (1,1), 1)
flag, removed_edges4 = cv2.threshold(removed_edges3, 1, 255, cv2.THRESH_BINARY)
#removed_edges4 = cv2.medianBlur(removed_edges4, 3)

cv2.imwrite('edges.jpg',edges)
cv2.imwrite('rem_1.jpg',removed_edges1)
cv2.imwrite('no_edges.jpg',removed_edges4)


# make bw image
im_gray = cv2.imread('Capture1.png', cv2.CV_LOAD_IMAGE_GRAYSCALE)
#im_gray = cv2.imread('191052.jpg', cv2.CV_LOAD_IMAGE_GRAYSCALE)
(thresh, im_bw) = cv2.threshold(im_gray, 128, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)
thresh = 200
im_bw = cv2.threshold(im_gray, thresh, 255, cv2.THRESH_BINARY)[1]
cv2.imwrite('bw_image.jpg', im_bw)

removed_edges_bw_1 = cv2.Canny(im_bw,100,0)
removed_edges_bw_2 = cv2.subtract(removed_edges_bw_1, edges)

removed_edges_bw_3 = cv2.GaussianBlur(removed_edges_bw_2, (1,1), 1)
flag, removed_edges_bw_4 = cv2.threshold(removed_edges_bw_3, 1, 255, cv2.THRESH_BINARY)

cv2.imwrite('rem_bw_1.jpg',removed_edges_bw_1)
cv2.imwrite('no_edges_bw.jpg',removed_edges_bw_4)




lines = cv2.HoughLines(removed_edges_bw_4,50,np.pi/30,200)
print lines
for rho,theta in lines[0]:
    a = np.cos(theta)
    b = np.sin(theta)
    x0 = a*rho
    y0 = b*rho
    x1 = int(x0 + 100*(-b))
    y1 = int(y0 + 100*(a))
    x2 = int(x0 - 100*(-b))
    y2 = int(y0 - 100*(a))
    cv2.line(removed_edges_bw_4,(x1,y1),(x2,y2),(255,0,0),2)

cv2.imwrite('houghlines1.jpg',removed_edges_bw_4)






