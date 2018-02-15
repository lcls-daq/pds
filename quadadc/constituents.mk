#libnames := quadadc quadadcbase
libnames := quadadc

libsrcs_quadadc := Manager.cc Server.cc
libincs_quadadc := hsd/include evgr pdsdata/include ndarray/include boost/include
#libsrcs_quadadcbase := $(filter-out $(libsrcs_quadadc),$(wildcard *.cc))
#libincs_quadadcbase := evgr
