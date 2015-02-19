import numpy as np
import cv2

def chooseCenters(center_map, color_image,gray_image):
    """
    :param center_map: 0-255 valued image where higher values are more likesly to be centers
    :param color_image: color image to insert chosen dots onto
    :return: array of center points, also writes and image with those center points on it
    """

    minVal = 175 # minimum value of the peak chosen to be a crystal
    #minVal = 150
    crystalSize = 125 # approximate size for a crystal
    #minSumArea = 2250000
    minSumArea = 1900000 # minimum value of the sum of all points in the crystal-szied area

    texture_image = cv2.imread('Images/3396_664.jpg',0)
    texturePoint = (700,1175)


    #Display point in image
    sample_text = np.copy(texture_image)
    for x in range(texturePoint[0] - 5, texturePoint[0] + 5):
        for y in range(texturePoint[1] - 5, texturePoint[1] + 5):
            sample_text[x][y] = 255
    print "made chosen texture image"
    cv2.imwrite('Images/choose_texture_point.jpg',sample_text)

    texture_sum = sumCrystalArea(texture_image, texturePoint[0], texturePoint[1], crystalSize)

    (x,y) = np.unravel_index(center_map.argmax(), center_map.shape) # choose max value

    xbound = len(center_map)
    ybound = len(center_map[x])

    centers = []

    while center_map[x][y] > minVal:

        sumArea = sumCrystalArea(center_map,x,y,crystalSize)
        if sumArea < minSumArea:
            center_map[x][y] = minVal
        else:
            if x != 0 and y != 0:

                score = textureMatcher(gray_image, (x,y), crystalSize, texture_sum)
                ranged = (min(score, 2000000.0)/2000000.0) * 255
                #print ranged

                centers.append((x,y))

                #c = (center_map[x][y] - 175)*5
                #color = [25*(c+1),75*(c%3),255-25*(c+1)]
                #color = [0,(center_map[x][y] - minVal)*5,255 - (center_map[x][y] - minVal)*5] # color value based on peak value
                red = [0,0,255]
                green = [0,255,0]
                color = [0,255-ranged,ranged]

                for i in range(x -7, x + 7):
                    for j in range(y-7, y+7):
                        if i >= 0 and i < xbound and j >= 0 and j < ybound:
                            if ranged > 200:
                                color_image[i][j] = red
                            else:
                                color_image[i][j] = green


            blackOutBox(center_map, x, y, crystalSize) # set all values around chosen point to 0 to prevent multiple "centers" in small area
            (x,y) = np.unravel_index(center_map.argmax(), center_map.shape) #choose new max

    cv2.imwrite('Images/center_points.jpg',color_image)
    return centers





def blackOutBox(center_map, x, y, rng):
    """
    :param center_map: 0-255 value image where high values are more likely to be centers
    :param x: x value of chosen center
    :param y: y value of chosen center
    :param rng: approximate radius of a crystal
    :return: None, adjusts center map so that crystal area around center is zero
    """
    xbound = len(center_map)
    ybound = len(center_map[x])

    for i in range(x - rng, x + rng):
        for j in range(y - rng, y + rng):
            if i >= 0 and i < xbound and j >= 0 and j < ybound:
                center_map[i][j] = 0

def sumCrystalArea(center_map, x, y, rng):
    """
    :param center_map: 0-255 value image where high values are more likely to be centers
    :param x: x value of chosen center
    :param y: y value of chosen center
    :param rng: approximate radius of a crystal
    :return: sum of all values in the crystal-sized area around the chosen center
    """
    xbound = len(center_map)
    ybound = len(center_map[x])
    total = 0

    for i in range(x - rng, x + rng):
        for j in range(y - rng, y + rng):
            if i >= 0 and i < xbound and j >= 0 and j < ybound:
                total += center_map[i][j]
    return total

def textureMatcher(img, testPoint,crystalSize, pointSum):





    textureSum = sumCrystalArea(img, testPoint[0], testPoint[1], crystalSize)

    return abs(pointSum - textureSum)