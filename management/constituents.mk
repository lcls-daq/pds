libnames := management

libsrcs_management := $(wildcard *.cc)

#DEFINES += -DBUILD_LARGE_STREAM_BUFFER

#ifneq ($(findstring -opt,$(tgt_arch)),)
#DEFINES += -DBUILD_ZCP
#endif
