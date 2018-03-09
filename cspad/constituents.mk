libnames := cspad

libsrcs_cspad := \
                 CspadLinkCounters.cc \
                 CspadLinkRegisters.cc \
                 CspadDestination.cc \
                 CspadQuadRegisters.cc \
                 CspadConcentratorRegisters.cc \
                 CspadConfigurator.cc \
                 Processor.cc \
                 CspadServer.cc \
                 CspadManager.cc \
                 CspadOccurrence.cc
#                 CompressionProcessor.cc \

#libsinc_cspad :=
libincs_cspad := pgpcard aesdriver/include
libincs_cspad += pdsdata/include ndarray/include boost/include 
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
