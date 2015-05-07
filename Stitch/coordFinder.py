__author__ = 'Clinic'
import sys
import cv2
import os


def search(map_dir, target, noisy):
    # currently assumes that the four scales are 1, 4, 16, 32
    big_map_file = map_dir + "\composites\composite_1.jpg"
    big_filesize = os.path.getsize(big_map_file)
    if big_filesize < 5*1024*1024:
        print "using full image - smallest file size"
        comp_num = 1
        xdown = 1
        ydown = 1
    elif big_filesize < 20*1024*1024:
        print "using quarter image - second smallest file size"
        comp_num = 4
        xdown = 0.25
        ydown = 0.25
    elif big_filesize < 80*1024*1024:
        print "using sixteenth image - second largest file size"
        comp_num = 16
        xdown = 1.0/16
        ydown = 1.0/16
    else:
        print "using thirty secondth image - largest file size"
        comp_num = 32
        xdown = 1.0/32
        ydown = 1.0/32
    print "big filezise:", big_filesize
    map_im = cv2.imread(map_dir + "\composites\composite_" + str(comp_num) + ".jpg")
    target_im = cv2.imread(target)
    print "done reading"

    xup = 1/xdown
    yup = 1/ydown
    #map_small = cv2.resize(map_im, (0, 0), fx=xdown, fy=ydown, interpolation=cv2.INTER_NEAREST)
    target_small = cv2.resize(target_im, (0, 0), fx=xdown, fy=ydown, interpolation=cv2.INTER_NEAREST)
    print "done resizing"

    hmap = cv2.matchTemplate(map_im, target_small, cv2.TM_CCOEFF_NORMED)
    print "done heat mapping "
    min_max_loc = cv2.minMaxLoc(hmap)
    print "mml:", min_max_loc
    max_loc = min_max_loc[3]
    # FIXME fill in fetching shape of image from offsets file and make it possible to negate the distanec from y=0 for justin
    if noisy and False:
        hmap = cv2.resize(hmap, dsize=(0, 0), fx=xup, fy=yup)
        plt.imshow(hmap)
        plt.show()
    return (max_loc[0]/xdown, max_loc[1]/ydown)

if __name__ == "__main__":
    #map, target, write_dest = sys.argv[1], sys.argv[2], sys.argv[3]
    noisy = False
    if len(sys.argv) == 1:
        map_dir = "C:\Users\Clinic\PycharmProjects\Apatite-to-Zircon\\test_images\\BearForceTwo"
        target = "C:\Users\Clinic\PycharmProjects\Apatite-to-Zircon\\test_images\BearForceTwo\Focused_3022_15946.jpg"
        write_dest = "C:\Users\Clinic\PycharmProjects\Apatite-to-Zircon\out.txt"
        import matplotlib.pyplot as plt
        noisy = True
    print "calling search"
    max_loc = search(map_dir, target, noisy)
    print max_loc