libnames := jungfrau jungfrauseg

libsrcs_jungfrauseg := Segment.cc
libincs_jungfrauseg := pdsdata/include ndarray/include boost/include

libsrcs_jungfrau := $(filter-out $(libsrcs_jungfrauseg),$(wildcard *.cc))
libincs_jungfrau := pdsdata/include ndarray/include boost/include slsdet/include
