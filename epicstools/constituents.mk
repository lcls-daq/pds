libnames := epicstools eventcodetools

# List source files for each library
libsrcs_epicstools := EpicsCA.cc
libincs_epicstools := epics/include epics/include/os/Linux

libsrcs_eventcodetools := EventcodeQuery.cc
libincs_eventcodetools := epics/include epics/include/os/Linux pds/configdata

tgtnames := epicsmonitor
tgtsrcs_epicsmonitor := epicsmonitor.cc
tgtlibs_epicsmonitor := pds/epicstools
tgtlibs_epicsmonitor += epics/ca epics/Com
tgtincs_epicsmonitor := epics/include epics/include/os/Linux
