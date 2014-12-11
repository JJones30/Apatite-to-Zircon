__author__ = 'Clinic'

import numpy as np
import math
import util

class heuristic:
    def evaluateAll(self, image, points):
        scoreDict = {}
        for point in points:
            scoreDict[point] = self.evaluate(image, point)

    def evaluate(self, image, point):
        return 0

class raycastSdHeuristic(heuristic):
    def evaluate(self, image, point):
        return util.rotatingWhiteDistance(image, point)

