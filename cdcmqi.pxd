cdef extern from "libsrc/ImageSEGConverter.cpp":
  pass

cdef extern from "include/dcmqi/ImageSEGConverter.h" namespace "dcmqi":
  cdef cppclass ImageSEGConverter:
    bool itkimage2dcmsegWrapper()
