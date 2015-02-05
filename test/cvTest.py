__author__ = 'Clinic'

# -----------------------
# -- File: cvTest.py
# -- Author: Kaya Woodall
# -- Purpose: Holds all functions which support the testing framework
# -- Instructions: Place any python image processing algorithm in algorithms.py
# --               Then, update the name of the algorithm called in algorithms.currAlgorithm()
# -----------------------


# Import Statements
import cv2
import os
import datetime
import algorithms
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from matplotlib.widgets import Slider, Button


# A namespace to hold information about the test images
class TestImages: pass
global testIms

# An additional namespace to hold information about the sliders
class AllSliders: pass
global sliders

# Color details
BCOLOR = 'white' # Background for a button
HCOLOR = 'lightgrey' # Hover over a button
SCOLOR = 'white' # Background for a slider


# -----------------------
# --HELPER FUNCTIONS:----
# --LOADING TEST IMAGES--
# -----------------------

# Returns a boolean indicating if the given file is an image
def isImageFile(f):
    if f[-3:] in ['png', 'jpg']:
        return True
    if f[-4:] == 'jpeg':
        return True
    else:
        return False


# Reads a color image with a given filename
def readColorImg(fname):
    return cv2.imread(fname, cv2.CV_LOAD_IMAGE_COLOR)


# Gets the names of all image files in the specified directory
def getImagesInDir(path='.'):
    imgFileNames = [f for f in os.listdir(path) if isImageFile(f)]
    print "Images in directory are: ", imgFileNames
    return imgFileNames


# -----------------------
# --HELPER FUNCTIONS:----
# --SAVING OUTPUT--------
# -----------------------

# Returns true if the name is unique in the given directory
def isUniqueName(fname, path='.'):
    existingFnames =[f for f in os.listdir(path)]
    isUnique = True if fname not in existingFnames else False
    return isUnique


# Constructs the txt file when saving an output image
def outputText(saveFolder, outputRoot):
    global sliders

    # Construct the file name
    filePath =  saveFolder+outputRoot+'.txt'
    f = open(filePath, 'w')

    # Write date and time information
    currDate = datetime.datetime.now().strftime("%m-%d-%y")
    currTime = datetime.datetime.now().strftime("%H:%M:%S")
    dateString = "Date: " + str(currDate) + "\n"
    timeString = "Time: " + str(currTime) + "\n"
    f.write(dateString)
    f.write(timeString)

    # Write slider information
    for i in range(0, sliders.numSliders):
        currSlider = sliders.array[i]
        sliderString1 = "Slider "+str(currSlider[0])+" has range from "+str(currSlider[1])+" to "+str(currSlider[2])+"\n"
        sliderString2 = str(currSlider[0])+" value shown is "+str(currSlider[3])+"\n"
        f.write(sliderString1)
        f.write(sliderString2)

    # Close the file
    print "Writing metadata to "+outputRoot+'.txt'
    f.close()


# Saves the image contained in the given axis to folder 'Outputs/'
def saveWithinAxis(fig, ax):
    global testIms
    saveFolder = 'Outputs/'
    # Construct the filename
    fname = testIms.fnames[testIms.currIndex]
    [root, ext] = fname.split('.')
    outputRoot = root+'-out'
    outputFname = outputRoot+'.'+ext

    # Check if that filename already exists
    isUnique = isUniqueName(outputFname, saveFolder)
    versionNum = 1
    while isUnique == False:
        print "Filename", outputFname, "already exists! Modifying..."
        outputRoot = root+'-out'+'-v'+str(versionNum)
        outputFname = outputRoot+'.'+ext
        versionNum += 1
        isUnique = isUniqueName(outputFname, saveFolder)

    # Save the figure bounded by the axis
    extent = ax.get_window_extent().transformed(fig.dpi_scale_trans.inverted())
    print "Saving image as", outputFname
    savePath = saveFolder + outputFname
    fig.savefig(savePath, bbox_inches=extent)

    # Create the relevant metadata txt file
    outputText(saveFolder, outputRoot)


# -----------------------
# --HELPER FUNCTIONS:----
# --DISPLAY--------------
# -----------------------

# Applies an image processing algorithm to a given image
# Place algorithms in algorithms.py
def processImg(im, x, y):
    return algorithms.currAlgorithm(im, x, y)


# Displays a single image given location parameters
def displayImage(fig, im, mode, index, coords):
    global testIms
    COLOR, GREY = 0, 1

    print "Displaying image"
    testIms.axesDict[index] = fig.add_subplot(coords[0], coords[1], coords[2])
    print "testIms.axesDict is now: ", testIms.axesDict
    if mode == COLOR:
        plt.imshow(im)
    if mode == GREY:
        plt.imshow(im, cmap=cm.Greys_r)
    plt.axis('off')

# Displays the original and processed images side by side in the given figure
def showTwoImages(fname, fig, isFirst, a=120, b=120):
    global testIms
    COLOR, GREY = 0, 1

    # Display first image
    im1 = readColorImg(fname)
    displayImage(fig, im1, COLOR, 0, [1, 2, 1])
    # Display second image
    im2 = processImg(im1, a, b)
    displayImage(fig, im2, GREY, 1, [1, 2, 2])

    plt.show() if isFirst else plt.draw()


# Displays the original and 5 processed image at various slider values
def showSixImages(fname, fig, activeSlider, a=120, b=120):
    global testIms
    COLOR, GREY = 0, 1

    plt.clf()

    # Display original image
    im1 = readColorImg(fname)
    displayImage(fig, im1, COLOR, 1, [2, 3, 1])

    # Determine additional slider values
    if activeSlider == "s1":
        currSlider = sliders.array[0]

    if activeSlider == "s2":
        currSlider = sliders.array[1]

    sliderMin = currSlider[1]
    sliderMax = currSlider[2]
    #sliderVal = currSlider[3]
    sliderRange = sliderMax - sliderMin
    sliderStep = sliderRange / 6

    for i in range(1,6):
        imgNum = i+1
        newSliderVal = sliderMin + i*sliderStep
        if activeSlider == "s1":
            imNew = processImg(im1, newSliderVal, b)
        if activeSlider == "s2":
            imNew = processImg(im1, a, newSliderVal)
        displayImage(fig, imNew, GREY, 1, [2, 3, imgNum])



    # -----------------------
    # ----SELECT BUTTONS-----
    # -----------------------
    Lbound = 0.155
    Ubound = 0.53
    width = 0.17
    height = 0.036
    horiz = 0.27
    vert = 0.43
    p11 = [Lbound, Ubound, width, height]
    p12 = [Lbound+horiz, Ubound, width, height]
    p13 = [Lbound+2*horiz, Ubound, width, height]
    p21 = [Lbound, Ubound-vert, width, height]
    p22 = [Lbound+horiz, Ubound-vert, width, height]
    p23 = [Lbound+2*horiz, Ubound-vert, width, height]

    locationArray = [p11, p12, p13, p21, p22, p23]
    for p in locationArray:
        selectAx = plt.axes(p)
        bSelect = Button(selectAx, 'Select Image', color=BCOLOR, hovercolor=HCOLOR)
        # ...When button pressed...
        def selectImg(event):
            print "3"
        bSelect.on_clicked(selectImg)
    plt.draw()


# -----------------------
# --MAIN FUNCTION--------
# -----------------------

def main():
    # -----------------------
    # ---GLOBAL NAMESPACES---
    # -----------------------
    global testingDirectory
    testingDirectory = '.'

    global testIms
    testIms = TestImages()
    testIms.fnames = []
    testIms.len = 0
    testIms.currIndex = 0
    testIms.axesDict = {}

    global sliders
    sliders = AllSliders()
    sliders.numSliders = 0
    sliders.array = []


    # -----------------------
    # --------SET UP---------
    # -----------------------
    # Create new figure
    fig = plt.figure()

    # Get all image files in current folder, add to testIms namespace
    imArray = getImagesInDir(testingDirectory)
    if len(imArray) == 0:
        print "No Images to test in directory"+testingDirectory+ "Quitting now..."
        exit()
    testIms.fnames = imArray
    testIms.len = len(imArray)


    # -----------------------
    # --------SLIDERS--------
    # -----------------------
    s1Name = 'Low Threshold'
    s1Min = 0
    s1Max = 255
    s1Initial = 40
    sax1 = plt.axes([0.2, 0.15, 0.57, 0.03], axisbg=SCOLOR)
    s1 = Slider(sax1, s1Name, s1Min, s1Max, s1Initial)
    sliders.array.append([s1Name, s1Min, s1Max, s1Initial])
    sliders.numSliders += 1

    s2Name = 'High Threshold'
    s2Min = 0
    s2Max = 255
    s2Initial = 120
    sax2 = plt.axes([0.2, 0.1, 0.57, 0.03], axisbg=SCOLOR)
    s2 = Slider(sax2, s2Name, s2Min, s2Max, s2Initial)
    sliders.array.append([s2Name, s2Min, s2Max, s2Initial])
    sliders.numSliders += 1

    print sliders.array
    # ...When sliders changed...
    def updateSliders(val):
        lowThresh = s1.val
        sliders.array[0][3] = s1.val
        highThresh = s2.val
        sliders.array[1][3] = s2.val
        showTwoImages(testIms.fnames[testIms.currIndex], fig, False, lowThresh, highThresh)
    s1.on_changed(updateSliders)
    s2.on_changed(updateSliders)


    # -----------------------
    # ------NEXT BUTTON------
    # -----------------------
    nextAx = plt.axes([0.305, 0.035, 0.17, 0.036])
    bNext = Button(nextAx, 'Next Image', color=BCOLOR, hovercolor=HCOLOR)
    # ...When button pressed...
    def nextImg(event):
        if testIms.currIndex == (testIms.len-1):
            print "Reached end of test images, repeating..."
            testIms.currIndex = 0
        else:
            testIms.currIndex += 1
        showTwoImages(testIms.fnames[testIms.currIndex], fig, False, s1.val, s2.val)
    bNext.on_clicked(nextImg)


    # -----------------------
    # ------SAVE BUTTON------
    # -----------------------
    saveAx = plt.axes([0.55, 0.035, 0.17, 0.036])
    bSave = Button(saveAx, 'Save Output', color=BCOLOR, hovercolor=HCOLOR)
    # ...When button pressed...
    def saveOut(event):
        outputAx = testIms.axesDict[1]
        saveWithinAxis(fig, outputAx)
    bSave.on_clicked(saveOut)


    # -----------------------
    # ---PREVIEW S1 BUTTON---
    # -----------------------
    prevS1Ax = plt.axes([0.86, 0.15, 0.11, 0.036])
    bPrevS1 = Button(prevS1Ax, 'Preview', color=BCOLOR, hovercolor=HCOLOR)
    # ...When button pressed...
    def previewS1(event):
        a = sliders.array[0][3]
        b = sliders.array[1][3]
        showSixImages(testIms.fnames[testIms.currIndex], fig, "s1", a, b)
    bPrevS1.on_clicked(previewS1)


    # -----------------------
    # ---PREVIEW S2 BUTTON---
    # -----------------------
    prevS2Ax = plt.axes([0.86, 0.1, 0.11, 0.036])
    bPrevS2 = Button(prevS2Ax, 'Preview', color=BCOLOR, hovercolor=HCOLOR)
    # ...When button pressed...
    def previewS2(event):
        a = sliders.array[0][3]
        b = sliders.array[1][3]
        showSixImages(testIms.fnames[testIms.currIndex], fig, "s2", a, b)
    bPrevS2.on_clicked(previewS2)


    # Display images
    showTwoImages(testIms.fnames[testIms.currIndex], fig, True, s1.val, s2.val)





if __name__ == "__main__":
    main()