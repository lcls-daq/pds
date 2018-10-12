libnames := epix10ka2m

libsrcs_epix10ka2m := \
		 Configurator.cc \
		 Destination.cc \
		 Server.cc \
		 ServerSim.cc \
		 Manager.cc

libincs_epix10ka2m := pgpcard aesdriver/include pgp pdsdata/include ndarray/include boost/include
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fopenmp
LXFlAGS += -fopenmp
LXFlAGS += -z now
LXFLAGS += -fopenmp
LXFLAGS += -z now
DEFINES += -fopenmp
