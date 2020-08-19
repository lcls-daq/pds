libnames := zyla

libsrcs_zyla := $(wildcard *.cc)
libincs_zyla := pdsdata/include ndarray/include boost/include
libincs_zyla += epics/include epics/include/os/Linux
