libnames := nscam

ifneq ($(findstring x86_64-rhel7,$(tgt_arch)),)
CPPFLAGS += -std=c++11
endif

libsrcs_nscam := $(wildcard *.cc)
libincs_nscam := pdsdata/include ndarray/include boost/include zest/include
