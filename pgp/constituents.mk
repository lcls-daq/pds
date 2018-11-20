CPPFLAGS += -D_LINUX

#tgtnames    := xstatus xreset xcntrst xdebug xloop

#tgtsrcs_xstatus := xstatus.cpp
#tgtincs_xstatus := pgpcard

#tgtsrcs_xreset := xreset.cpp
#tgtincs_xreset := pgpcard

#tgtsrcs_xcntrst := xcntrst.cpp
#tgtincs_xcntrst := pgpcard

#tgtsrcs_xdebug := xdebug.cpp
#tgtincs_xdebug := pgpcard

#tgtsrcs_xloop := xloop.cpp
#tgtincs_xloop := pgpcard



#tgtlibs_xstatus :=
#tgtslib_xstatus :=

#tgtnames := cxistat

#tgtsrcs_cxistat := cxistat.cc


#ignore_src := SrpV3.cc

libnames := pgp pgpv3

libsrcs_pgpv3 := SrpV3.cc Reg.cc AxiVersion.cc
libincs_pgpv3 := pgpcard aesdriver/include boost/include
liblibs_pgpv3 := boost/boost_thread

libsrcs_pgp := $(filter-out $(libsrcs_pgpv3),$(wildcard *.cc))
#libsinc_pgp := 
libincs_pgp := pgpcard aesdriver/include boost/include

CPPFLAGS += -fno-strict-aliasing

