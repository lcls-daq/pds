tgtnames := lgctest
libnames := logbookclient

libsrcs_logbookclient := WSLogBook.cc
libincs_logbookclient := python3/include/python3.6m

tgtsrcs_lgctest := lgctest.cc
tgtincs_lgctest := python3/include/python3.6m
tgtlibs_lgctest := pds/logbookclient 
tgtlibs_lgctest += python3/python3.6m
tgtlibs_lgctest += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/configdata pdsdata/xtcdata pdsdata/psddl_pdsdata
tgtslib_lgctest := dl pthread rt
