__author__ = 'Clinic'

import re
import os
import time
import sys

import numpy as np
import matplotlib.pyplot as plt
import cv2
import matplotlib.cm as cm
from scipy.spatial import KDTree

from Stitch.util import tuple_diff


print "floating_image executing"
inf = 2**100000


def im_read_gray(filename):
    im = cv2.imread(filename)
    im = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
    return im


def find_min(flim_list):
    x_min = inf
    y_min = inf
    for flim in flim_list:
        x_min = min((x_min, flim.top_left[0]))
        y_min = min((y_min, flim.top_left[1]))
    return x_min, y_min


def find_max(flim_list):
    x_max = -inf
    y_max = -inf
    for flim in flim_list:
        x_max = max((x_max, flim.bot_right[0]))
        y_max = max((y_max, flim.bot_right[1]))
    return x_max, y_max


def mass_combine(flim_list, outline_images=False, feather=True, run_num=0, backdrop=()):
    print "mass combine with depth", run_num
    move_to_0_0(flim_list)

    x_max, y_max = find_max(flim_list)

    flim_list.sort(key=lambda x: x.top_left)

    # offset will be negative if it's not working correctly
    offset = (0, 0)
    size = (x_max, y_max)
    if run_num == 0:
        print "generating blank backdrop"
        new_image = np.zeros((size[1], size[0]), dtype=np.uint8)
    elif run_num == 1:
        print "using fancy backdrop"
        new_image = backdrop
        print "shape of backdrop is", backdrop.shape
    i = 0
    for flim in flim_list:
        i += 1
        origin = tuple_diff(flim.top_left, offset)
        if outline_images:
            cv2.rectangle(flim.image, (0, 0), (flim.image.shape[1], flim.image.shape[0]), color=255, thickness=5)
        if feather:
            original = new_image[origin[1]:origin[1]+flim.size[1], origin[0]:origin[0]+flim.size[0]]
            if run_num == 0:
                new = cv2.max(flim.image, original)
            elif run_num == 1:
                new = cv2.addWeighted(flim.image, 0.5, original, 0.5, 0)
            else:
                return "something is horribly broken"
            new_image[origin[1]:origin[1]+flim.size[1], origin[0]:origin[0]+flim.size[0]] = new
        else:
            new_image[origin[1]:origin[1]+flim.size[1], origin[0]:origin[0]+flim.size[0]] = flim.image
    if run_num == 1:
        return new_image
    else:
        return mass_combine(flim_list, outline_images=outline_images, feather=feather, run_num=1, backdrop=new_image)


class FloatingImage:
    # it is currently fundamental to floating images that they are all the same dimensions
    def __init__(self, image, position):
        self.image = image
        self.position = position
        self.size = (image.shape[1], image.shape[0])
        self.top_left = self.position
        self.bot_right = (self.top_left[0] + self.size[0], self.top_left[1] + self.size[1])
        self.top_right = (self.top_left[0] + self.size[0], self.top_left[1])
        self.bot_left = (self.top_left[0], self.top_left[1] + self.size[1])
        self.name = "unnamed"
        self.rel_pos_ests = {}

    def update_position_extras(self):
        self.top_left = self.position
        self.bot_right = (self.top_left[0] + self.size[0], self.top_left[1] + self.size[1])
        self.top_right = (self.top_left[0] + self.size[0], self.top_left[1])
        self.bot_left = (self.top_left[0], self.top_left[1] + self.size[1])

    def update_pos(self, new_pos, max_change=(inf, inf)):
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

    def move(self, move_vector):
        #print self.name, "is moving", move_vector
        self.update_pos((self.position[0] + move_vector[0], self.position[1] + move_vector[1]))

    def set_pos_relative_to_me(self, other, rel_pos):
        other.update_pos((self.top_left[0] + rel_pos[0], self.top_left[1] + rel_pos[1]))

    def record_initial_relative_position(self, other):
        offset = tuple_diff(other.top_left, self.top_left)
        self.rel_pos_ests[other.name] = offset

    #FIXME: set noisy to False
    def align_other_to_me(self, other, max_err=(100, 100), min_overlap=(200, 200), noisy=False, blur_hist=True, moves=[]):
        #print "aligning", other.name, "to", self.name
        if other.name in self.rel_pos_ests.keys():
            self.set_pos_relative_to_me(other, self.rel_pos_ests[other.name])
            #print "flims at", self.top_left, "and", other.top_left

        intersection = self.intersection(other)
        inter_size = (intersection[1][0] - intersection[0][0], intersection[1][1] - intersection[0][1])
        if abs(inter_size[0]) < min_overlap[0] or abs(inter_size[1]) < min_overlap[1]:
            print "Overlap too small - not aligning"
            return False
        self_subimg = self.get_subimage(intersection[0], intersection[1])
        other_subimg = other.get_subimage(intersection[0], intersection[1])
        self_inner = self_subimg[max_err[1]:inter_size[1]-max_err[1], max_err[0]:inter_size[0]-max_err[0]]
        other_inner = other_subimg[max_err[1]:inter_size[1]-max_err[1], max_err[0]:inter_size[0]-max_err[0]]
        strategy = cv2.TM_CCOEFF_NORMED
        self_other_hist = cv2.matchTemplate(self_subimg, other_inner, strategy)
        other_self_hist = cv2.matchTemplate(other_subimg, self_inner, strategy)

        def make_noise():
            plt.figure()
            plt.subplot(321)
            plt.title("overlapping region from im1")
            plt.imshow(self_subimg, cmap=cm.gray)
            plt.subplot(322)
            plt.title("overlapping region from im2")
            plt.imshow(other_subimg, cmap=cm.gray)
            plt.subplot(323)
            plt.title("subimage with border removed")
            plt.imshow(self_inner, cmap=cm.gray)
            plt.subplot(324)
            plt.title("subimage with border removed")
            plt.imshow(other_inner, cmap=cm.gray)
            plt.subplot(325)
            plt.title("subimage position in im2")
            plt.imshow(self_other_hist)
            plt.subplot(326)
            plt.title("subimage position in im1")
            plt.imshow(other_self_hist)
            plt.show()
        if blur_hist:
            self_other_hist = cv2.blur(self_other_hist, (3,3))
            other_self_hist = cv2.blur(other_self_hist, (3,3))
        self_other_max = cv2.minMaxLoc(self_other_hist)[3]
        other_self_max = cv2.minMaxLoc(other_self_hist)[3]
        self_offset_opinion = tuple_diff(self_other_max, max_err)
        other_offset_opinion = tuple_diff(max_err, other_self_max)
        disagreement = tuple_diff(self_offset_opinion, other_offset_opinion)
        if disagreement[0] > 5 or disagreement[1] > 5:
            print "disagreement is", disagreement, "consider fixing things"
            print "images:", self.name, other.name, "positions:", self.top_left, other.top_left
            print "not performint recommended adjust; using microscope guess"
            if noisy:
                make_noise()
            moves.append("fail_disagree")
            return True

        if self_offset_opinion[0] == max_err[0] or self_offset_opinion[1] == max_err[1]:
            print "slider is all the way to the wall, something is probably broken"
            print "images:", self.name, other.name
            print "not performint recommended adjust; using microscope guess"
            if noisy:
                make_noise()
            moves.append("fail_wall")
            return True


        ave = (int((self_offset_opinion[0] + other_offset_opinion[0])/2),
               int((self_offset_opinion[1] + other_offset_opinion[1])/2))
        other.move(ave)
        moves.append(ave)
        return True

    def contains(self, point):
        """contains (0,0) but not (1600, 1200)"""
        if (self.top_left[0] <= point[0] <= self.bot_right[0]) and (self.top_left[1] <= point[1] <= self.bot_right[1]):
            return True
        return False

    def intersects(self, other):
        otl = other.top_left
        obr = other.bot_right
        obl = other.bot_left
        otr = other.top_right

        for corner in [otl, obr, obl, otr]:
            if self.contains(corner):
                return True
        return False

    def intersection(self, other):
        """returns a rectangle in the format ((tl_x, tl_y), (br_x, br_y))"""
        #
        intersection = None
        if self.contains(other.top_left):
            intersection = (other.top_left, self.bot_right)
        elif self.contains(other.bot_right):
            intersection = (self.top_left, other.bot_right)
        elif self.contains(other.top_right):
            intersection = ((self.top_left[0], other.top_right[1]), (other.top_right[0], self.bot_left[1]))
        elif self.contains(other.bot_left):
            intersection = ((other.top_left[0], self.top_left[1]), (self.top_right[0], other.bot_left[1]))
        else:
            print "no intersection found"
        assert self.contains(intersection[0])
        assert self.contains(intersection[1])
        assert other.contains(intersection[0])
        assert other.contains(intersection[1])
        return intersection

    def get_subimage(self, top_left, bot_right):
        if (not self.contains(top_left)) or (not self.contains(bot_right)):
            print "failed to get subimage: desired rect not totally contained in image"
            print "failing image located at", self.top_left
            print "points requested are", top_left, bot_right
            return None
        local_top_left = (top_left[0] - self.top_left[0], top_left[1] - self.top_left[1])
        local_bot_right = (bot_right[0] - self.top_left[0], bot_right[1] - self.top_left[1])
        return self.image[local_top_left[1]:local_bot_right[1], local_top_left[0]:local_bot_right[0]]

    def __repr__(self):
        return "(image" + self.name + " at " + str(self.top_left) + ")"


def grab_from_folder(folder_name):
    file_list = os.listdir(folder_name)
    print len(file_list), "files detected"
    image_file = re.compile('Focused_[-0-9]+_[-0-9]+.jpg')
    images_with_positions = []
    i = 0
    for filename in file_list:
        if i % 100 == 0:
            print i, "images read"
        i += 1
        if not image_file.match(filename):
            print "file", filename, "has invalid name; ignoring"
            continue
        fullname = folder_name + filename
        filename = filename.replace(".jpg", "")
        filename = filename.replace("Focused_", "")
        x_y = tuple(filename.split("_"))
        x_y = (int(x_y[0]), -int(x_y[1]))
        image = cv2.imread(fullname)
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        flim = FloatingImage(image, x_y)

        images_with_positions.append(flim)
    return images_with_positions


def mass_align(flim_array, center_location, noisy=False, moves=[]):
    if noisy:
        prettily_display(flim_array, "before mass align")
    # setup
    area_tree = KDTree([flim.top_left for flim in flim_array])
    aligned = set()
    frontier = []
    distances, indices = area_tree.query([center_location], k=1, eps=0.1, p=2, distance_upper_bound=1000)
    print "index of middle:", indices[0]
    frontier.append(flim_array[indices[0]])
    aligned.add(flim_array[indices[0]])

    while not len(frontier) == 0:
        curr = frontier.pop()
        #print ""
        #print "finding neighbors of", curr.top_left, "which is", curr.name
        dists, adjacent_indices = area_tree.query([curr.top_left], k=5, eps=0.1, p=1, distance_upper_bound=1200)
        #print "dists", dists
        #print "adjacent indices:", adjacent_indices
        valid_adjacent = [x for x in adjacent_indices[0] if x < len(flim_array)][1:]
        #print "valid adjacent:", valid_adjacent

        #print "aligned:", aligned
        adjacents = [flim_array[i] for i in valid_adjacent if not flim_array[i] in aligned]
        #print "unaligned adjacents:", adjacents

        for flim in adjacents:
            align = curr.align_other_to_me(flim, noisy=False, moves=moves)
            if align:
                frontier.append(flim)
                aligned.add(flim)
            else:
                print "failed to align", flim.name, "to", curr.name

        area_tree = KDTree([flim.top_left for flim in flim_array])

        #print ""


def move_to_0_0(flim_array):
    x_min, y_min = find_min(flim_array)
    for flim in flim_array:
        flim.move((-x_min, -y_min))


def prettily_display(flim_array, title=""):
    total_im = mass_combine(flim_array, outline_images=False)
    shape = total_im.shape
    smaller = cv2.resize(total_im, (shape[1]/3, shape[0]/3))
    plt.imshow(smaller, cmap=cm.gray)
    plt.title(title)
    plt.show()


def get_center(flim_array):
    """assumes flim_array is already zeroed"""
    x_max = max([flim.bot_right[0] for flim in flim_array])
    y_max = max([flim.bot_right[1] for flim in flim_array])
    nearly_center = (x_max/2, y_max/2)
    center = tuple_diff(nearly_center, (800, 600))
    print "center is", center
    return center


def visualize(flim_array):
    width = max([flim.bot_right[0] for flim in flim_array]) + 50
    height = max([flim.bot_right[1] for flim in flim_array]) + 50
    display_image = np.zeros((height, width, 3), np.uint8)
    i = 0
    for flim in flim_array:
        i += 1

        cv2.circle(display_image, flim.top_left, 100, [25 * (i + 1), 75*(i % 3), 255-25*(i+1)], thickness=-1)
        cv2.rectangle(display_image, flim.top_left, flim.bot_right, [25*(i+1), 75*(i % 3), 255-25*(i+1)], thickness=20)
    return display_image


def trim_flim_array(xmin, xmax, ymin, ymax, flim_array):
    new_array = []
    for flim in flim_array:
        if xmin < flim.top_left[0] < xmax:
            if ymin < flim.top_left[1] < ymax:
                new_array.append(flim)
    return new_array


def main(argv):
    read_dir_name = argv[0]
    write_file_name = argv[1]

    flim_array = grab_from_folder(read_dir_name)
    move_to_0_0(flim_array)
    if len(argv) > 2:
        xmin, xmax, ymin, ymax = argv[2], argv[3], argv[4], argv[5]
        flim_array = trim_flim_array(xmin, xmax, ymin, ymax, flim_array)
        move_to_0_0(flim_array)
    for i in range(len(flim_array)):
        flim_array[i].name = "index " + str(i)

    for flim_1 in flim_array:
        for flim_2 in flim_array:
            flim_1.record_initial_relative_position(flim_2)

    if True:
        moves = []
        start = time.time()
        move_to_0_0(flim_array)
        tl = find_min(flim_array)
        br = find_max(flim_array)

        #plt.show()
        print "tl:", tl, "br:", br

        mass_align(flim_array, center_location=get_center(flim_array), moves=moves)
        print "\n\ncalling mass combine\n\n"
        total_im = mass_combine(flim_array, outline_images=False, feather=True)
        end = time.time()
        print "mass combine ends"
        print "runtime:", end-start
        shape = total_im.shape
        print "initial shape:", shape
        if shape[1] > shape[0]:
            pass
            #h_ratio = 2000.0/shape[1]
            #print "new shape:", (int(shape[0]*h_ratio), 2000)
            #smaller = cv2.resize(total_im, (2000, int(shape[0]*h_ratio)))
        else:
            pass
            #w_ratio = 2000/shape[0]
            #smaller = cv2.resize(total_im, (shape[1]*w_ratio, 2000))

        #plt.imshow(smaller, cmap=cm.gray )
        #plt.show()
        cv2.imwrite(write_file_name, total_im)
        failed_moves = [x for x in moves if isinstance(x, type(""))]
        success_moves = [x for x in moves if (x not in failed_moves)]
        print "num failed moves:", len(failed_moves)
        print "num success moves:", len(success_moves)
        #plt.hist2d([x[0] for x in moves], [x[1] for x in moves], bins=50)
        #plt.show()

if __name__ == "__main__":
    # takes: (read dir string, write filename string, xmin, xmax, ymin, ymax)
    main(sys.argv[1:])
