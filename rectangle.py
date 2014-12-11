__author__ = 'Clinic'

class rectangle:
    def __init__(self, x, y, width, height):
        self.xmin=int(x)
        self.ymin=int(y)
        self.xmax=int(x+width)
        self.ymax=int(y+height)

    @staticmethod
    def new(offset, size):
        return rectangle(offset[0], offset[1], size[0], size[1])

    def contains(self, point):
        if self.xmin <= point[0] <= self.xmax and self.ymin <= point[1] <= self.ymax:
            return True
        return False

    def overlap_size(self, point):
        return (self.xmax - point[0], self.ymax - point[1])

    def size(self):
        return (self.xmax - self.xmin, self.ymax - self.ymin)

    def origin(self):
        return (self.xmin, self.ymin)

    def move_up_left(self, offset):
        self.xmin -= offset[0]
        self.xmax -= offset[0]
        self.ymin -= offset[1]
        self.ymax -= offset[1]

    def __str__(self):
        return "((" + str(self.xmin) + ", " + str(self.ymin) + "), (" + str(self.xmax) + ", " + str(self.ymax) + "))"