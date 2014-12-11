import numpy as np
import cv2

def chooseCenters(center_map, color_image):

    (x,y) = np.unravel_index(center_map.argmax(), center_map.shape)

    xbound = len(center_map)
    ybound = len(center_map[x])

    centers = []

    while center_map[x][y] > 185:

        if x != 0 and y != 0:
            centers.append((x,y))


            color = [0,0,255]

            for i in range(x -7, x + 7):
                for j in range(y-7, y+7):
                    if i >= 0 and i < xbound and j >= 0 and j < ybound:
                        color_image[i][j] = color

        blackOutBox(center_map, x, y, 125)
        (x,y) = np.unravel_index(center_map.argmax(), center_map.shape)

    cv2.imwrite('Images/center_points.jpg',color_image)
    return centers





def blackOutBox(center_map, x, y, rng):
    xbound = len(center_map)
    ybound = len(center_map[x])

    for i in range(x - rng, x + rng):
        for j in range(y - rng, y + rng):
            if i >= 0 and i < xbound and j >= 0 and j < ybound:
                center_map[i][j] = 0

