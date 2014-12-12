__author__ = 'Clinic'

from stitch import stitch, tuple_diff
import numpy as np
import matplotlib.pyplot as plt
import cv2
import matplotlib.cm as cm
from scipy.spatial import KDTree
import re
import os

print "floating_image executing"

def imread_gray(filename):
    im = cv2.imread(filename)
    im = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
    return im

def mass_combine(flim_list, outline_images=False):
    x_min = 0
    x_max = 0
    y_min = 0
    y_max = 0
    for flim in flim_list:
        x_min = min((x_min, flim.top_left[0]))
        x_max = max((x_max, flim.bot_right[0]))
        y_min = min((y_min, flim.top_left[1]))
        y_max = max((y_max, flim.bot_right[1]))


    if x_min > 0:
        for flim in flim_list:
            print "old pos is " + str(flim.top_left)
            new_pos = (flim.top_left[0] - x_min, flim.top_left[1])
            print "new pos is " + str(new_pos)
            flim.update_pos(new_pos)

    if y_min < 0:
        for flim in flim_list:
            print "old pos is " + str(flim.top_left)
            new_pos = (flim.top_left[0], flim.top_left[1] + abs(y_min))
            print "new pos is " + str(new_pos)
            flim.update_pos(new_pos)

    x_min = 1000000
    x_max = 0
    y_min = 1000000
    y_max = 0
    for flim in flim_list:
        x_min = min((x_min, flim.top_left[0]))
        x_max = max((x_max, flim.bot_right[0]))
        y_min = min((y_min, flim.top_left[1]))
        y_max = max((y_max, flim.bot_right[1]))
    print "minima are ", (x_min, y_min), ", and ", (x_max, y_max)

    flim_list.sort(key=lambda flim: flim.top_left)

    # offset will be negative if it's not working correctly
    offset = (x_min, y_min)
    size = (x_max - x_min, y_max - y_min)
    new_image = np.zeros((size[1], size[0]))
    i = 0
    for flim in flim_list:
        i += 1
        origin = tuple_diff(flim.top_left, offset)
        dest = new_image[origin[1]:origin[1]+flim.size[1], origin[0]:origin[0]+flim.size[0]]
        print "sticking image ", i, " on the thing at position ", flim.top_left
        if outline_images:
            cv2.rectangle(flim.image, (0,0), (flim.image.shape[1], flim.image.shape[0]), color=255, thickness=5)
        new_image[origin[1]:origin[1]+flim.size[1], origin[0]:origin[0]+flim.size[0]] = flim.image
    print "done sticking images together"
    return new_image

class floating_image:
    def __init__(self, image, position):
        self.image = image
        self.position = position
        self.size = (image.shape[1], image.shape[0])
        self.update_position_extras()

    def update_position_extras(self):
        self.top_left = self.position
        self.bot_right = (self.top_left[0] + self.size[0], self.top_left[1] + self.size[1])

    def update_pos(self, new_pos, max_change=(1000000, 1000000)):
        print "new_pos is " + str(new_pos)
        if abs(new_pos[0] - self.position[0]) > max_change[0]:
            print "refusing to move: too much x change"
            return False
        elif abs(new_pos[1] - self.position[1]) > max_change[1]:
            print "refusing to move: too much y change"
            return False
        else:
            self.position = new_pos
            self.update_position_extras()
            return True

    def align_other_to_me(self, other):
        if other.position[0] > self.position[0]:
            offset_est = (other.position[0] - self.position[0], other.position[1] - self.position[1])
        else:
            offset_est = (self.position[0] - other.position[0], self.position[1] - other.position[1])
        print "aligning images at " + str(self.position) + " and " + str(other.position)
        offset = stitch(self.image, other.image, offset_est)
        print "resulting offset is " + str(offset)

        other.update_pos((self.position[0] + offset[0], self.position[1] + offset[1]), max_change=(100000,100000))
        print "resulting position is " + str(other.position)

    def contains(self, point):
        if (self.top_left[0] <= point[0] <= self.bot_right[0]) and (self.top_left[1] <= point[1] <= self.bot_right[1]):
            return True
        return False

    def intersects(self, other):
        otl = other.top_left
        obr = other.bot_right
        obl = (otl[0], obr[1])
        otr = (obr[0], otl[1])

        for corner in [otl, obr, obl, otr]:
            if self.contains(corner):
                return True
        return False

    def __repr__(self):
        return "(image at " + str(self.top_left) + ")"

def grab_from_folder(folder_name):
    file_list = os.listdir(folder_name)
    print file_list
    image_file = re.compile('Focused_[0-9]+_[0-9]+.jpg')
    images_with_positions = []
    for filename in file_list:
        if not image_file.match(filename):
            continue
        fullname = folder_name + filename
        filename = filename.replace(".jpg", "")
        filename = filename.replace("Focused_", "")
        x_y = tuple(filename.split("_"))
        x_y = (int(x_y[0]), -int(x_y[1]))
        image = cv2.imread(fullname)
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        print x_y
        flim = floating_image(image, x_y)

        images_with_positions.append(flim)
    return images_with_positions

def mass_align(flim_array, center_location):
    # setup
    area_tree = KDTree([flim.top_left for flim in flim_array])
    aligned = set()
    frontier = []
    distances, indices = area_tree.query([center_location], k=1, eps=0.1, p=2, distance_upper_bound=1000)
    print "indices", indices
    frontier.append(flim_array[indices[0]])
    aligned.add(flim_array[indices[0]])

    while not len(frontier) == 0:
        curr = frontier.pop()
        dum, adjacent_indices = area_tree.query([curr.top_left], k=4, eps=0.1, p=2, distance_upper_bound=1000)
        valid_adjacent = [x for x in adjacent_indices[0] if x < len(flim_array)]

        adjacents = [flim_array[i] for i in valid_adjacent if not flim_array[i] in aligned]

        for flim in adjacents:
            curr.align_other_to_me(flim)
            frontier.append(flim)
            aligned.add(flim)
        area_tree = KDTree([flim.top_left for flim in flim_array])

dirname="Kathleen/Code/Images/JT Images/"
flim_array = grab_from_folder(dirname)

flim_array[10].align_other_to_me(flim_array[11])

mass_align(flim_array, (15000, -4000))
print "calling mass combine"
total_im = mass_combine(flim_array, outline_images=True)
print "mass combine ends"
shape = total_im.shape
smaller = cv2.resize(total_im, (shape[0]/3, shape[1]/3))

plt.imshow(smaller, cmap=cm.gray)
plt.show()