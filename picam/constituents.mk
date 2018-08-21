libnames := pdspicam picamutils

libsrcs_picamutils := PiUtils.cc
libincs_picamutils := picam/include

libsrcs_pdspicam := PicamManager.cc PiUtils.cc PimaxManager.cc PimaxServer.cc PimaxConfigWrapper.cc
libincs_pdspicam := pdsdata/include ndarray/include boost/include picam/include

tgtnames := piConfigure
#tgtnames := piConfigure piGui

libPicam := picam/picam picam/GenApi_gcc40_v2_4 picam/GCBase_gcc40_v2_4 picam/MathParser_gcc40_v2_4 picam/log4cpp_gcc40_v2_4 picam/Log_gcc40_v2_4
libPicam += picam/piac picam/pidi picam/picc picam/pida picam/PvBase picam/PvDevice picam/PvBuffer picam/PvPersistence picam/ftd2xx
libPicam += picam/PvStream picam/PvGenICam picam/PvSerial picam/PtUtilsLib
libPicam += picam/PtConvertersLib picam/EbTransportLayerLib picam/EbUtilsLib

incPicam := picam/include

tgtsrcs_piConfigure := piConfigure.cc
tgtincs_piConfigure := $(incPicam)
tgtlibs_piConfigure := $(libPicam)

tgtsrcs_piGui := piGui.cc
tgtlibs_piGui := $(libPicam)
tgtsinc_piGui := /usr/include/gtk-2.0 /usr/lib64/gtk-2.0/include /usr/include/atk-1.0 /usr/include/cairo /usr/include/pango-1.0 /usr/include/glib-2.0 /usr/lib64/glib-2.0/include /usr/include/pixman-1 /usr/include/freetype2 /usr/include/libpng12
tgtsinc_piGui += $(incPicam)
tgtslib_piGui := gtk-x11-2.0 gdk-x11-2.0 atk-1.0 gio-2.0 pangoft2-1.0 gdk_pixbuf-2.0 pangocairo-1.0 cairo pango-1.0 freetype fontconfig gobject-2.0 gmodule-2.0 gthread-2.0 rt glib-2.0
tgtlnkflag_piGui := piGui_cameras_dialog.o piGui_main_window.o piGui_repetitive_gate_dialog.o
tgtlnkflag_piGui += piGui_exposure_dialog.o piGui_parameters_dialog.o

