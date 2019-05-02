# distutils: language = c++

from cdcmqi import ImageSEGConverter

def main():
  segConverter_ptr = new ImageSEGConverter()

  cdef ImageSEGConverter ImageSEGConverter_stack
