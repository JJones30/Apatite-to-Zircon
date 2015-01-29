__author__ = 'Clinic'
import cv2
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from sklearn.decomposition import PCA

def get_texture_statistics(image, position, tile_size):
    """assumes grayscale input image"""
    try:
        tile = image[position[1]:position[1]+tile_size[1], position[0]:position[0]+tile_size[1]]
        # mean intensity
        mean_intensity, stdev_intensity = cv2.meanStdDev(tile)
        # simple edginess
        edge_tile_hor = cv2.Scharr(tile, -1, 1, 0)
        edge_tile_vert = cv2.Scharr(tile, -1, 0, 1)
        edge_tile = cv2.addWeighted(edge_tile_hor, 0.5, edge_tile_vert, 0.5, 0.0)
        mean_edge, stdev_edge = cv2.meanStdDev(edge_tile)

        return [mean_intensity[0][0], stdev_intensity[0][0], mean_edge[0][0], stdev_edge[0][0], position[0], position[1]]
    except:
        return

image = cv2.imread("C:\Users\Clinic\PycharmProjects\Apatite-to-Zircon\\test_images\\5x5_1\Focused_13308_5059.jpg")
image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

scores = []
for x in range(160):
    print x
    for y in range(120):
        score = get_texture_statistics(image, (x*10, y*10), (20,20))
        if len(score)==6:
            scores.append(score)

if False:
    plt.subplot(221)
    plt.title("mean intensity vs stdev intensity")
    plt.scatter([x[0] for x in scores], [x[1] for x in scores])
    plt.subplot(222)
    plt.title("mean intensity vs mean edge")
    plt.scatter([x[0] for x in scores], [x[2] for x in scores])
    plt.subplot(223)
    plt.title("mean edge vs stdev edge")
    plt.scatter([x[2] for x in scores], [x[3] for x in scores])
    plt.subplot(224)
    plt.title("stdev intensity vs stdev edge")
    plt.scatter([x[1] for x in scores], [x[3] for x in scores])
    plt.show()

print "1"
pca = PCA(n_components=2)
print "2"
print "scores"
print scores[0][0]
decomp = pca.fit_transform(scores)
print "3"
print decomp
plt.scatter([x[0] for x in decomp], [x[1] for x in decomp])
plt.show()