__author__ = 'Clinic'

# -----------------------
# -- File: algorithms.py
# -- Author: Kaya Woodall
# -- Purpose: Holds any image processing algorithms to be tested in the
# --          framework described in cvTest.py
# -- Instructions: Place (or import) any python image processing algorithm here
# --               Then, update the name of the algorithm called in currAlgorithm()
# -----------------------


# Import statements
# Import external modules as needed
import cv2

# Call the algorithm you wish to test
# !!! Update the call signature of the specific algorithm here
def currAlgorithm(rgb, a, b):
    return thresholding(rgb, a, b)
    # return yourAlgorithmName(rgb, a, b)


# Double-thresholds an image
def thresholding(rgb, threshLow, threshHigh):
    greyIm = cv2.cvtColor(rgb, cv2.COLOR_RGB2GRAY)
    greyIm = cv2.threshold(greyIm, threshHigh, threshHigh, cv2.THRESH_TRUNC)[1]
    greyIm = cv2.threshold(greyIm, threshLow, 0, cv2.THRESH_TOZERO)[1]
    return greyIm


# Define or import additional algorithms as needed
# def yourAlgorithmName(rgb, a, b):
    # Your code (or calls to external modules) here