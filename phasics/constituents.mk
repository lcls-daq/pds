libnames := phasics

libsrcs_phasics := \
		 PhasicsConfigurator.cc \
		 PhasicsServer.cc \
		 PhasicsManager.cc
libincs_phasics := libdc1394/include
liblibs_phasics += libdc1394/dc1394
liblibs_phasics += libdc1394/raw1394
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
