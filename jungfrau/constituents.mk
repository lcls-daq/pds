libnames := jungfrauseg

ifneq ($(findstring x86_64-rhel9,$(tgt_arch)),)
libnames += jungfrau
endif

ifneq ($(findstring x86_64-rhel7,$(tgt_arch)),)
libnames += jungfrau
CPPFLAGS += -std=c++11
endif

libsrcs_jungfrauseg := Segment.cc
libincs_jungfrauseg := pdsdata/include ndarray/include boost/include

libsrcs_jungfrau := $(filter-out $(libsrcs_jungfrauseg),$(wildcard *.cc))
libincs_jungfrau := pdsdata/include ndarray/include boost/include slsdet/include
