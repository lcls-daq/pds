libnames := epix10ka

libsrcs_epix10ka := \
                 Epix10kaDestination.cc \
		 Epix10kaConfigurator.cc \
		 Epix10kaServer.cc \
		 Epix10kaManager.cc
#libsinc_epix10ka :=
libincs_epix10ka := pgpcard aesdriver/include pgp pdsdata/include ndarray/include boost/include
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fopenmp
LXFlAGS += -fopenmp
LXFlAGS += -z now
LXFLAGS += -fopenmp
LXFLAGS += -z now
DEFINES += -fopenmp
