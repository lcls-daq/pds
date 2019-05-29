tgtnames := lgctest
libnames := logbookclient

libsrcs_logbookclient := WSLogBook.cc
libincs_logbookclient := python3/include/python3.6m

tgtsrcs_lgctest := lgctest.cc
tgtincs_lgctest := python3/include/python3.6m
tgtlibs_lgctest := pds/logbookclient 
tgtlibs_lgctest += python3/python3.6m
tgtslib_lgctest := dl pthread rt
