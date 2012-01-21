# List of packages (low level first)
ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
#  packages := service camera
  packages := service collection xtc
  packages += config mon vmon
  packages += utility management client offlineclient
  packages += camera
  packages += firewire
  packages += phasics
else
  packages := service collection xtc 
  packages += config mon vmon
  packages += utility management client offlineclient
  packages += ipimb encoder camera acqiris evgr epicsArch rceProxy \
              princeton epicstools pgp cspad xamps fexamp gsc16ai timepix
  packages += cspad2x2
endif
