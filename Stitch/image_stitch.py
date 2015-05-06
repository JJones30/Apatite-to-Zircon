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
import sys
sys.path.insert(0, 'C:/Users/Ray Donelick/Documents/Ravi/Apatite-to-Zircon')

from Stitch.util import tuple_diff
import random

print "floating_image executing"
inf = 2**100000


def im_read_gray(filename):
    im = cv2.imread(filename)
    im = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
    return im


def find_corners(flim_list):
    x_min = inf
    y_min = inf
    x_max = -inf
    y_max = -inf
    for flim in flim_list:
        x_min = min((x_min, flim.top_left[0]))
        y_min = min((y_min, flim.top_left[1]))
        x_max = max((x_max, flim.bot_right[0]))
        y_max = max((y_max, flim.bot_right[1]))
    return (x_min, y_min), (x_max, y_max)


def mass_combine(flim_list, outline_images=False, feather=True, run_num=0, backdrop=()):
    print(str(run_num) + " "),
    move_to_0_0(flim_list)
    top_left, bot_right = find_corners(flim_list)

    # offset will be negative if it's not working correctly
    offset = (0, 0)
    size = bot_right
    if run_num == 0:
        new_image = np.zeros((size[1], size[0]), dtype=np.uint8)
    elif run_num == 1:
        new_image = backdrop

    # shuffling the list of floating images before and after feathering cuts down on stitching artifacts
    random.shuffle(flim_list)

    for flim in flim_list:
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
                print "mass combine called with invalid run_num parameter"
                return
            new_image[origin[1]:origin[1]+flim.size[1], origin[0]:origin[0]+flim.size[0]] = new
        else:
            new_image[origin[1]:origin[1]+flim.size[1], origin[0]:origin[0]+flim.size[0]] = flim.image
    if run_num == 1:
        return new_image
    else:
        return mass_combine(flim_list, outline_images=outline_images, feather=feather, run_num=1, backdrop=new_image)


def measure_agreement(flim_array):
    if True:
        print "\nmeasuring agreement. this step might be very slow for large images. disable above this print statement."
    else:
        # the results
        return "Unknown"
    start = time.time()
    overlap_pixels = 0
    agreement_sum = 0
    for flim_ind_1 in range(len(flim_array)):
        for flim_ind_2 in range(flim_ind_1+1, len(flim_array)):
            flim_1 = flim_array[flim_ind_1]
            flim_2 = flim_array[flim_ind_2]
            assert isinstance(flim_1, FloatingImage)
            assert isinstance(flim_2, FloatingImage)
            if flim_1.intersects(flim_2):
                tl, br = flim_1.intersection(flim_2)
                width = br[0] - tl[0]
                height = br[1] - tl[1]
                overlap_pixels += width*height

                agreement = get_overlap_hists(flim_1, flim_2, (0,0))
                agreement_sum += agreement[0][0]
    end = time.time()
    print "agreement runtime:", str(end-start) + "s"
    return agreement_sum/overlap_pixels

def get_overlap_hists(self, other, max_err, debug=False):
        intersection = self.intersection(other)
        inter_size = (intersection[1][0] - intersection[0][0], intersection[1][1] - intersection[0][1])
        self_subimg = self.get_subimage(intersection[0], intersection[1])
        other_subimg = other.get_subimage(intersection[0], intersection[1])
        self_inner = self_subimg[max_err[1]:inter_size[1]-max_err[1], max_err[0]:inter_size[0]-max_err[0]]
        other_inner = other_subimg[max_err[1]:inter_size[1]-max_err[1], max_err[0]:inter_size[0]-max_err[0]]
        strategy = cv2.TM_CCOEFF_NORMED
        self_other_hist = cv2.matchTemplate(self_subimg, other_inner, strategy)
        other_self_hist = cv2.matchTemplate(other_subimg, self_inner, strategy)

        if debug:
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

        return self_other_hist, other_self_hist


class FloatingImage:
    """Floating images absolutely must all be the same size. Tuning currently strongly expects 1600x1200."""
    # the sticking points for these requirements is mostly the map, for determining intersections, and also
    # rectangle intersection
    def __init__(self, image, position):
        self.image = image
        self.position = position
        self.initial_position = position
        self.path = [(position, "initial")]
        self.size = (image.shape[1], image.shape[0])
        self.top_left = self.position
        self.bot_right = (self.top_left[0] + self.size[0], self.top_left[1] + self.size[1])
        self.top_right = (self.top_left[0] + self.size[0], self.top_left[1])
        self.bot_left = (self.top_left[0], self.top_left[1] + self.size[1])
        self.name = "unnamed"
        self.rel_pos_ests = {}

    def update_position_extras(self, tracked=True, navigator_name="unknown"):
        """called every time any internal function changes this object's position."""
        self.top_left = self.position
        self.bot_right = (self.top_left[0] + self.size[0], self.top_left[1] + self.size[1])
        self.top_right = (self.top_left[0] + self.size[0], self.top_left[1])
        self.bot_left = (self.top_left[0], self.top_left[1] + self.size[1])
        if tracked:
            # path is a list of all positions the floating image has ever had, for debugging or analysis purposes
            self.path.append((self.top_left, navigator_name))

    def update_pos(self, new_pos, max_change=(inf, inf), navigator_name="update_pos"):
        """sets position of this object directly to new_pos, unless max_change is exceeded in x or y"""
        if abs(new_pos[0] - self.position[0]) > max_change[0]:
            print "refusing to move: too much x change"
            return False
        elif abs(new_pos[1] - self.position[1]) > max_change[1]:
            print "refusing to move: too much y change"
            return False
        else:
            self.position = new_pos
            self.update_position_extras(navigator_name=navigator_name)
            return True

    def move(self, move_vector, navigator_name="move"):
        """changes position by the given vector. does not have a
        maximum because caller clearly knows the move distance already."""
        self.update_pos((self.position[0] + move_vector[0], self.position[1] + move_vector[1]),
                        navigator_name=navigator_name)

    def set_pos_relative_to_me(self, other, rel_pos):
        other.update_pos((self.top_left[0] + rel_pos[0], self.top_left[1] + rel_pos[1]),
                         navigator_name=self.name + " set relative pos")

    def record_initial_relative_position(self, other):
        """call this on all pairs of images that might be pairwise-aligned later, using an estimate of their relative
        position from before any alignments are performed"""
        offset = tuple_diff(other.top_left, self.top_left)
        self.rel_pos_ests[other.name] = offset



    def align_other_to_me(self, other, max_err=(100, 100), min_overlap=(200, 200), debug=False, blur_hist=True):
        """given two images, check if they overlap. If they do, minimize their image misalignment. Returns true if an
        alignment is performed, or false if the images don't overlap or no alignment can be agreed upon. Requires
        that the two images already be predicted to overlap by the microscope guess and that they are no more than
        max_err from their correct aligned position."""

        if other.name in self.rel_pos_ests.keys():
            self.set_pos_relative_to_me(other, self.rel_pos_ests[other.name])
        else:
            print "other not in dictionary of original relative positions - not aligning"
            return False

        # crop from each image the region predicted to overlap with the other
        intersection = self.intersection(other)
        inter_size = (intersection[1][0] - intersection[0][0], intersection[1][1] - intersection[0][1])
        if abs(inter_size[0]) < min_overlap[0] or abs(inter_size[1]) < min_overlap[1]:
            print "Overlap too small - not aligning"
            return False
        self_other_hist, other_self_hist = get_overlap_hists(self, other, max_err, debug=debug)

        if blur_hist:
            # blur_hist blurs the histogram before searching for a maximum
            # this protects against spurious single-pixel maxima and stitches slightly more conservatively overall
            self_other_hist = cv2.blur(self_other_hist, (3,3))
            other_self_hist = cv2.blur(other_self_hist, (3,3))
        self_other_max = cv2.minMaxLoc(self_other_hist)[3]
        other_self_max = cv2.minMaxLoc(other_self_hist)[3]
        self_offset_opinion = tuple_diff(self_other_max, max_err)
        other_offset_opinion = tuple_diff(max_err, other_self_max)
        disagreement = tuple_diff(self_offset_opinion, other_offset_opinion)
        # stitching is usually accurate to within 1 pixel or so. We allow 5 because disagreements in the 3-5
        # range are usually the result of blurry images where humans can't be pixel precise either
        if disagreement[0] > 5 or disagreement[1] > 5:
            print "disagreement is", disagreement, "consider fixing things"
            print "images:", self.name, other.name, "positions:", self.top_left, other.top_left
            print "not performint recommended adjust; using microscope guess"

            return True

        if self_offset_opinion[0] == max_err[0] or self_offset_opinion[1] == max_err[1]:
            print "histogram maximum is on border of histogram. Probable error."
            print "images:", self.name, other.name
            print "not performing recommended adjust; using microscope guess"

            return False

        ave = (int((self_offset_opinion[0] + other_offset_opinion[0])/2),
               int((self_offset_opinion[1] + other_offset_opinion[1])/2))
        other.move(ave)
        return True

    def contains(self, point):
        """a rectangle contains its own top left corner, but none of the others"""
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
        return "(image " + self.name + " at " + str(self.top_left) + ")"


def grab_from_folder(folder_name):
    file_list = os.listdir(folder_name)
    print "files detected:"
    print len(file_list)
    image_file = re.compile('Focused_[-0-9]+_[-0-9]+.jpg')
    images_with_positions = []
    i = 0
    print "files read:"
    for filename in file_list:
        if i % 25 == 0:
            print(str(i) + " "),
        i += 1
        if not image_file.match(filename):
            # if you're working in a complicated directory for some reason, print statement
            # here is nifty for debugging
            continue
        fullname = folder_name + "\\" + filename
        filename = filename.replace(".jpg", "")
        filename = filename.replace("Focused_", "")
        x_y = tuple(filename.split("_"))
        x_y = (int(x_y[0]), -int(x_y[1]))
        image = cv2.imread(fullname)
        if len(image.shape) == 3:
            image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        flim = FloatingImage(image, x_y)

        images_with_positions.append(flim)
    return images_with_positions


def mass_align(flim_array, center_location, noisy=False):
    """this is the important step in image stitching. the images are aligned to minimize disagreement."""
    if noisy:
        # this is among the less useful debug tools, but it can be nice for understanding the algorithm, if you need to
        graph_frame_locations(flim_array, "before mass align")
    # setup
    area_tree = KDTree([flim.top_left for flim in flim_array])
    aligned = set()
    frontier = []
    distances, indices = area_tree.query([center_location], k=1, eps=0.1, p=2, distance_upper_bound=1000)
    frontier.append(flim_array[indices[0]])
    aligned.add(flim_array[indices[0]])

    # this while loop is in the spirit of breadth-first search, and also prim;s
    # first, the kdtree is used to identify the image closest to the center. This image is placed in the "aligned" set
    # and not moved in the future. All of its neighbors are added to the frontier, aligned to it,
    # and placed in "aligned." The image is then removed from the frontier. Repeat until the frontier is empty.
    while not len(frontier) == 0:
        curr = frontier.pop()
        dists, adjacent_indices = area_tree.query([curr.top_left], k=5, eps=0.1, p=1, distance_upper_bound=1200)
        valid_adjacent = [x for x in adjacent_indices[0] if x < len(flim_array)][1:]
        adjacents = [flim_array[i] for i in valid_adjacent if not flim_array[i] in aligned]

        for flim in adjacents:
            align = curr.align_other_to_me(flim, debug=False)
            if align:
                frontier.append(flim)
                aligned.add(flim)
            else:
                print "failed to align", flim.name, "to", curr.name
        area_tree = KDTree([flim.top_left for flim in flim_array])

    move_to_0_0(flim_array)
    offsets = {}
    for flim in flim_array:
        offsets[flim.initial_position] = flim.top_left
    return offsets

def move_to_0_0(flim_array):
    top_left, bot_right = find_corners(flim_array)
    x_min, y_min = top_left
    if top_left == (0,0):
        return
    for flim in flim_array:
        flim.move((-x_min, -y_min))


def graph_frame_locations(flim_array, title=""):
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
    # in the minimal allowed print set
    print "after alignment, center is", center
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


def csv_of_dictionary(dictionary, key_col_name, val_col_name):
    def line(key, value):
        return str(key).ljust(18, ' ') + ": " + str(value) + "\n"
    return key_col_name.ljust(18, ' ') + ": " + val_col_name + "\n" + ''.join([line(key, dictionary[key]) for key in dictionary.keys()])


def write_storage_dir(dirname, full_image, offsets, agreements):
    # in the minimal allowed print set
    print "writing to", dirname
    if os.path.exists(dirname):
        sizes = [1, 4, 16, 32] # changing these values could bother coordFinder
        def comp_name(size):
            return "composite_" + str(size) + ".jpg"
        composites_dirname = dirname + "\\composites"
        if not os.path.exists(composites_dirname):
            os.mkdir(composites_dirname)

        for i in sizes:
            scaled_im = cv2.resize(full_image, dsize=(0, 0), fx=1.0/i, fy=1.0/i, interpolation=cv2.INTER_NEAREST)
            cv2.imwrite(dirname + "\\composites\\" + comp_name(i), scaled_im)

        offsets_file = open(dirname + "\\offsets file.txt", mode="w+")
        offsets_file.write(csv_of_dictionary(offsets, "initial", "final"))
        offsets_file.write("\nMETADATA\n")
        offsets_file.write("image_dim=" + str(full_image.shape) + "\n")
        try:
            offsets_file.write("agreement increase ratio: " + str(agreements[1][0]/agreements[0][0]))
            print "agreement increase ratio:", str(agreements[1]/agreements[0])
        except:
            offsets_file.write("agreement increase ratio: unmeasured")
    else:
        print "error: file", dirname, "does not exist"



def main(argv):
    read_dir_name = ""
    for index in range(4):
        read_dir_name += argv[index]
        read_dir_name += " "
    read_dir_name = read_dir_name[:-1]
    # should produce something like:
    # read_dir_name = "C:\Users\Clinic\PycharmProjects\Apatite-to-Zircon\\test_images\\10x10_1"
    # where 10x10_1 is the directory name of a
    flim_array = grab_from_folder(read_dir_name)
    move_to_0_0(flim_array)
    #pre_agreement = measure_agreement(flim_array)

    for i in range(len(flim_array)):
        flim = flim_array[i]
        flim.name = "init_pos: " + str(flim.top_left)
    if len(argv) == 5:
        xmin, xmax, ymin, ymax = argv[2], argv[3], argv[4], argv[5]
        flim_array = trim_flim_array(xmin, xmax, ymin, ymax, flim_array)
        move_to_0_0(flim_array)

    for flim_1 in flim_array:
        for flim_2 in flim_array:
            flim_1.record_initial_relative_position(flim_2)

    if True:
        start = time.time()

        offsets = mass_align(flim_array, center_location=get_center(flim_array))
        print "calling mass combine: two steps, 0 and 1"
        total_im = mass_combine(flim_array, outline_images=False, feather=True)
        end = time.time()
        print "\nmass-combine ran in runtime:", end-start

        #post_agreement = measure_agreement(flim_array)
        write_storage_dir(read_dir_name, total_im, offsets, (pre_agreement, post_agreement))

        if False:
            def get_points(path_step, flim_array):
                points= [flim.path[path_step] for flim in flim_array if len(flim.path) == 8]
                print "points kept:", len(points)
                x, y = [x[0] for x in points], [x[1] for x in points]
                return x, y

            for path_step in range(len(flim_array[0].path)):
                print "path step:", path_step
                x, y = get_points(path_step, flim_array)
                plt.scatter(x, y)
            plt.show()

            for flim in flim_array:
                print flim.path

        if False:
            # plots the paths of each image through the process
            # change desirables to change which moves are displayed
            # under the current arrangement, [2,3,4] should show the
            # two parts of the pairwise_align step
            desirables = [3,4]
            #point_names = ["initial", "nonnegative", "to original offset", "align with neighbor"]
            fig, ax = plt.subplots()
            for flim in flim_array:
                x, y = [x[0] for x in flim.path], [x[1] for x in flim.path]
                x = [x[i] for i in desirables]
                y = [y[i] for i in desirables]
                ax.plot(x, y)

                for i in range(len(x)):
                    ax.annotate(str(i), (x[i], y[i]))
            plt.show()

if __name__ == "__main__":
    # takes: (read dir string, write filename string, xmin, xmax, ymin, ymax)
    main(sys.argv[1:])
