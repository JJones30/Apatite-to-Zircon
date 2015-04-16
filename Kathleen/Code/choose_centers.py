import numpy as np
import cv2
import csv

def chooseCenters(center_map, color_image,gray_image):
    """
    Choose centers from a map of likelihoods created from stacking several different
    algorithms. Not used when connected components is used to find crystal bodies.
    :param center_map: 0-255 valued image where higher values are more likely to be centers
    :param color_image: color image to insert chosen dots onto
    :return: array of center points, also writes and image with those center points on it
    """

    minVal = 175 # minimum value of the peak chosen to be a crystal
    crystalSize = 125# approximate size for a crystal
    textureArea = 50
    minSumArea = 1900000 # minimum value of the sum of all points in the crystal-sized area
    cutOff = 300000.0 # maximum value for ranking system to be considered apatite

    # Use a sample from a known crystal to compare texture of new crystals
    texture_image = cv2.imread('Images/Orig/3396_664.jpg',0)
    texturePoint = (700,1175)
    #Display point in image that is used to compare textures
    sample_text = np.copy(texture_image)
    for x in range(texturePoint[0] - 5, texturePoint[0] + 5):
        for y in range(texturePoint[1] - 5, texturePoint[1] + 5):
            sample_text[x][y] = 255
    print "made chosen texture image"
    cv2.imwrite('Images/choose_texture_point.jpg',sample_text)

    # Get statistics from sample point
    meanTexture = np.mean(texture_image)
    meanImage = np.mean(gray_image)
    texture_sum = sumCrystalArea(texture_image, texturePoint[0], texturePoint[1], textureArea)
    texture_avg = (texture_sum/((textureArea*2)**2))/meanTexture
    texture_diff = variationDetector(texture_image, texturePoint[0], texturePoint[1], textureArea)

    # Start with the peak value in the center map for first potential crystal
    (x,y) = np.unravel_index(center_map.argmax(), center_map.shape) # choose max value

    xbound = len(center_map)
    ybound = len(center_map[x])

    centers = []

    # Continue finding crystals until values reach the cutoff
    while center_map[x][y] > minVal:

        # Calculate statistics about new point and compare against sample point
        sumArea = sumCrystalArea(center_map,x,y,crystalSize)
        if sumArea < minSumArea:
            center_map[x][y] = minVal
        else:
            if x != 0 and y != 0:
                score = textureMatcher(gray_image, (x,y), textureArea, meanImage, [texture_avg, texture_diff])
                ranged = (min(score, cutOff)/cutOff) * 255

                red = [0,0,255]
                green = [0,255,0]

                # create image displaying center points and add point
                # to list of centers if not filtered out
                for i in range(x -7, x + 7):
                    for j in range(y-7, y+7):
                        if i >= 0 and i < xbound and j >= 0 and j < ybound:
                            if ranged > 200:
                                color_image[i][j] = red
                            else:
                                color_image[i][j] = green
                                centers.append((x,y))

            # set all values around chosen point to 0 to prevent multiple "centers" in small area
            blackOutBox(center_map, x, y, crystalSize)

            (x,y) = np.unravel_index(center_map.argmax(), center_map.shape) #choose new max

    print "center image done"
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

def textureMatcher(img, testPoint,crystalSize, avgImage, compareValues):
    """Calculate a score based on brightness and variance of crystal"""
    texture_sum = sumCrystalArea(img, testPoint[0], testPoint[1], crystalSize)
    varSum = variationDetector(img, testPoint[0], testPoint[1], crystalSize)
    texture_avg = texture_sum/((crystalSize*2)**2)/avgImage

    # Multiply scores by weighting factors so they affect the total score in the right proportions
    score = 3*abs(compareValues[0] - texture_avg) + .000001*abs(compareValues[1] - varSum)

    return score

def variationDetector(img, x, y, rng):
    """
    Calculate the vairance of brightness in a crystal
    :param img: grayscale image
    :param x: x coordinate of center point
    :param y: y coordinate of center point
    :param rng: distance in both x and y directions from the center point
    that is used for the crystals
    :return:
    """
    totVal = sumCrystalArea(img, x, y, rng)
    avgVal = totVal/(2*rng)**2

    xbound = len(img)
    ybound = len(img[x])
    total = 0

    # sum distnace from avg for each point
    for i in range(x - rng, x + rng):
        for j in range(y - rng, y + rng):
            if i >= 0 and i < xbound and j >= 0 and j < ybound:
                total += abs(avgVal-img[i][j])

    return total


def rankCenters(color_image,gray_image, centers, bodies):
    """
    Score each center of a crystal based on brightness and variance compared to a known crystal
    then use the score to filter out non-apatite bodies
    :param color_image: Color image of slide
    :param gray_image: Grayscale image of slide
    :param centers: List of centers of potential apatite crystals
    :param bodies: List of points in each body (Currently not used, but could be used in future for
    reasoning about texture)
    :return: list of centers filtered to include only crystals that are likely to be apatite
    """

    textureArea = 50 # size of area used around center to examine texture
    cutOff = 1 # Maximum raw score allowed before the ranged score defaults to maximum value
    maxScore = 230 # Maximum score to be included (score out of 255, higher is less likely to be apatite)


    # Currently using a sample image. In future may want to get values from several sample images and then just
    # use calculated values instead of repeating the analysis of the same image for each run
    texture_image = cv2.imread('Images/Orig/3396_664.jpg',0) # Image used to for a sample point to determine texture
    texturePoint = (700,1175) # Center of known apatite crystal in sample image to compare against


    # Calculate average value of both the slide and the sample image to normalize results for slide's
    # brightness
    meanTexture = np.mean(texture_image)
    meanImage = np.mean(gray_image)

    # Calculate brightness and variation for sample point to compare against
    texture_sum = sumCrystalArea(texture_image, texturePoint[0], texturePoint[1], textureArea)
    texture_avg = (texture_sum/((textureArea*2)**2))/meanTexture
    texture_diff = variationDetector(texture_image, texturePoint[0], texturePoint[1], textureArea)


    red = [0,0,255]
    green = [0,255,0]
    xbound = len(color_image)
    ybound = len(color_image[0])
    filteredCenters = []

    # Score each potential center and filter out the ones with scores that are too high to be considered apatite
    for (x,y) in centers:
        x = int(x)
        y = int(y)
        # Calculate score based on brightness and variation in comparison to sample point values
        score = textureMatcher(gray_image, (x,y), textureArea, meanImage, [texture_avg, texture_diff])
        ranged = (min(score, cutOff)/cutOff) * 255 # Represent score as a value between 0-255 (lower is more likely
                                                   # too be apatite). This can allow the score to be easily visualized as a color

        # Visualize the results of the algorithm by making chosen points green and filtered ones red
        for i in range(x -7, x + 7):
            for j in range(y-7, y+7):
                if i >= 0 and i < xbound and j >= 0 and j < ybound:
                    if ranged > maxScore:
                        color_image[i][j] = red
                    else:
                        color_image[i][j] = green

        # Keep center if the score is low enough
        if ranged <= maxScore:
            filteredCenters.append((x,y))

    print "center filtering done"
    cv2.imwrite('Images/center_points_filtered.jpg',color_image)

    return filteredCenters


def writeToFile(centers):
    """
    write list of center points to the csv file 'centers.csv'
    :param centers: list of center points of crystals
    :return: None
    """
    with open('centers.csv', 'wb') as csvfile:
        writer = csv.writer(csvfile, quoting=csv.QUOTE_MINIMAL)
        writer.writerow(centers)