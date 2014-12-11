__author__ = 'Clinic'

import cv2
import matplotlib.pyplot as plt


def stitch(im1, im2, offset, max_error=(50,50), disp_heatmap=False, ruin_everything=False):
    """im1 and im2 are images to be combined. offset is in format (x,y), distance down and right
       from im1 to im2, measured from the top left corner, estimated. We assume the overlap is large-ish."""

    # subimage is a piece of im2 to be found in im1
    subimage, subimage_origin = subimage_for_search(im1, im2, offset, max_error)

    subimage_search_heatmap = cv2.matchTemplate(im1, subimage, cv2.TM_CCORR_NORMED)

    mml = cv2.minMaxLoc(subimage_search_heatmap)
    max_location = mml[3]
    im2_origin = tuple_diff(max_location, subimage_origin)
    sub_size = (subimage.shape[1], subimage.shape[0])

    if ruin_everything:
        cv2.rectangle(im2, color=(255, 255, 255), thickness=10, pt1=subimage_origin, pt2=tuple_sum(subimage_origin, sub_size))
    if disp_heatmap:
        plt.subplot(221)
        plt.title("stitch heatmap")
        plt.imshow(subimage_search_heatmap)
        plt.subplot(222)
        plt.title("im1")
        plt.imshow(im1)
        plt.subplot(223)
        plt.title("im2")
        plt.imshow(im2)
        plt.subplot(224)
        plt.imshow(subimage)
        plt.title("subimage")
        plt.show()

    return im2_origin


def tuple_diff(t1, t2):
    assert len(t1) == len(t2)
    return tuple([t1[i] - t2[i] for i in range(len(t1))])


def tuple_sum(t1, t2):
    assert len(t1) == len(t2)
    return tuple([t1[i] + t2[i] for i in range(len(t1))])


def tuple_ave(tuples):
    first = tuples[0]
    for other_tuple in tuples[1:]:
        first = tuple_sum(first, other_tuple)

    return tuple([x/len(tuples) for x in first])


def subimage_for_search(im1, im2, offset, max_error=(50, 50)):
    # assumes im2 is to the right of im2, but makes no assumptions about above/belowness
    min_x = 0
    max_x = im1.shape[1] - offset[0]
    min_y = max([0, offset[1]])
    max_y = min([im1.shape[0], offset[1] + im2.shape[0]])
    overlap_size = (max_x - min_x, max_y - min_y)

    subimage = im2[max_error[1] + min_y:max_y - max_error[1], max_error[0] + min_x:max_x - max_error[0]]
    subimage_origin = (max_error[0] + min_x, max_error[1] + min_y)
    #subimage origin is relative to im2
    return subimage, subimage_origin


def expand(im, new_width, new_height, origin, color):
    assert(im.shape[1] + origin[0] <= new_width)
    assert(im.shape[0] + origin[1] <= new_height)
    border_right = new_width - origin[0] - im.shape[1]
    border_left = origin[0]
    border_up = origin[1]
    border_down = new_height - origin[1] - im.shape[0]
    im = cv2.copyMakeBorder(im, border_up, border_down, border_left, border_right, cv2.BORDER_REPLICATE)
    return im, (border_left, border_up)


def recombine(im1, im2, offset, noisy=False, color = (0,0,0)):
    if noisy:
        print "recombine called with args " + str((im1.shape, im2.shape, offset))
    # offset is in width-height
    # but image.shape returns [height, width, channels]
    # we will assume images are the same size
    #
    # okay now is the time to get this code actually right
    # we will ensure that im1 is on the left, but not that it is on top, as follows:
    if offset[0] < 0:
        print "shuffling images for lr consistency"
        im3 = im1
        im1 = im2
        im2 = im3
        offset = (offset[0]*-1, offset[1]*-1)

    # now im1 is on the left. We're not assuming the images are the same size any more.
    im1_origin = [0, 0]
    if offset[1] < 0:
        print "new dim calc method 1"
        im1_origin[1] = -1*offset[1]
        new_height = max(im1.shape[0] + abs(offset[1]), im2.shape[0])
    else:
        print "new dim calc method 2"
        new_height = max(im1.shape[0], im2.shape[0] + offset[1])
    new_width = max(im1.shape[1], im2.shape[1] + offset[0])
    # now we can use im1_origin as the origin for recombine

    if noisy:
        print "input im1 size is " + str(im1.shape)
        print "input im2 size is " + str(im2.shape)
        print "input offset is " + str(offset)
        print "new height is " + str(new_height)
        print "new width is " + str(new_width)

    expanded, im1_origin = expand(im1, new_width, new_height, im1_origin, color)
    adjusted_offset = tuple_sum(offset, im1_origin)

    expanded[adjusted_offset[1]:adjusted_offset[1]+im2.shape[0], adjusted_offset[0]:adjusted_offset[0]+im2.shape[1]] \
        = im2
    return expanded, im1_origin


def multi_stitch(rows, offset_lists, vert_offsets, noisy=False):
    row_offsets = []
    row_origin_positions = []
    row_images = []
    for i in range(len(rows)):
        row_offsets.append(stitch_row(rows[i], offset_lists[i]))

    for i in range(len(rows)):
        offset = stitch_row(rows[i], offset_lists[i], noisy=noisy)
        row_image, im1_origin = combine_row(rows[i], offset)
        row_images.append(row_image)
        row_origin_positions.append(im1_origin)


    vert_offsets_true = stack_rows(row_images, vert_offsets)
    print "true vert offsets: " + str(vert_offsets_true)
    print "expected vert offsets: " + str(vert_offsets)
    big_im, origin_pos = combine_row(row_images, vert_offsets_true)
    plt.imshow(big_im)
    plt.show()

def stitch_row(row, offsets, noisy=False):
    if offsets[0] == (0,0):
        offsets = offsets[1:]
    assert len(row) == len(offsets)+1
    found_offsets = []
    last_offset = (0,0)
    for i in range(len(offsets)):
        if noisy:
            print "last offset is " + str(last_offset)
        new_offset = tuple_sum(stitch(row[i], row[i+1], offsets[i], disp_heatmap=noisy), last_offset)
        found_offsets.append(new_offset)
        last_offset = new_offset

    return found_offsets

def stack_rows(rows, offsets):
    found_offsets = []
    for i in range(len(offsets)):
        new_offset = stitch(rows[i], rows[i+1], offsets[i])
        found_offsets.append(new_offset)

    return found_offsets

def combine_row(row, offsets):
    if offsets[0] == (0,0):
        offsets = offsets[1:]
    base = row[0]
    origin_pos = (0,0)
    for i in range(len(offsets)):
        base, origin_adjust = recombine(base, row[i+1], tuple_sum(offsets[i], origin_pos))
        origin_pos = tuple_sum(origin_adjust, origin_pos)

    return base, origin_pos

if False:
    im11 = cv2.imread("images/3x3/1.jpg")
    im21 = cv2.imread("images/3x3/457_0.jpg")
    im31 = cv2.imread("images/3x3/914_0.jpg")
    im12 = cv2.imread("images/3x3/0_480.jpg")
    im22 = cv2.imread("images/3x3/457_480.jpg")
    im32 = cv2.imread("images/3x3/914_480.jpg")
    im13 = cv2.imread("images/3x3/0_960.jpg")
    im23 = cv2.imread("images/3x3/457_960.jpg")
    im33 = cv2.imread("images/3x3/914_960.jpg")

    row_1_images = [im11, im21, im31]
    row_1_offsets = [(457, 0), (914, 0)]

    row_2_images = [im12, im22, im32]
    row_2_offsets = [(457, 480), (914, 480)]

    row_3_images = [im13, im23, im33]
    row_3_offsets = [(457, 960), (914, 960)]

    vert_offsets = [(0, 480), (0, 1000)]
    rows = [row_1_images, row_2_images, row_3_images]
    row_offsets = [row_1_offsets, row_2_offsets, row_3_offsets]

    multi_stitch(rows, row_offsets, vert_offsets, noisy=True)

