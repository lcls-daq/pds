# List of packages (low level first)
packages := service collection xtc
packages += config confignfs configsql mon vmon
packages += utility management client
packages += ipimb camera evgr epicstools pgp imp
packages += pnccd epicsArch
packages += oceanoptics fli andor usdusb
packages += cspad cspad2x2
packages += ioc
packages += rayonix
packages += epixSampler
packages += epix
packages += epix10k
packages += epix10ka
packages += epix10ka2m
packages += epix100a
packages += genericpgp
packages += udpcam
packages += lecroy
packages += pvdaq
packages += monreq
packages += gsc16ai
packages += jungfrau

ifneq ($(findstring x86_64,$(tgt_arch)),)
  packages += firewire uxi archon logbookclient offlineclient
##  No DDL
#  packages += phasics
else
  packages += encoder acqiris \
              princeton
endif
##  No DDL
#  packages += xamps
#  packages += fexamp

ifneq ($(findstring x86_64-rhel7,$(tgt_arch)),)
  packages += zyla picam archon quadadc vimba
endif

ifneq ($(findstring x86_64-rhel9,$(tgt_arch)),)
  packages += zyla picam archon quadadc vimba
endif
