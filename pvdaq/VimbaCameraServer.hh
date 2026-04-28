#ifndef Pds_PvDaq_VimbaCameraServer_hh
#define Pds_PvDaq_VimbaCameraServer_hh

#include "pds/pvdaq/AreaDetectorServer.hh"
#include "pds/config/VimbaConfigType.hh"

namespace Pds {
  namespace PvDaq {
    class AlviumCamServer : public AreaDetectorServer {
    public:
      AlviumCamServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      virtual ~AlviumCamServer();
    protected:
      virtual bool configure();
      virtual void show_configuration();
      virtual Xtc* alloc_config_type(Pds::InDatagram* dg);
      virtual char* alloc_data_type(Xtc& xtc);
    private:
      char* _reverse_x;
      char* _reverse_y;
      char* _contrast;
      char* _correction;
      char* _correction_type;
      char* _correction_set;
      char* _pixel_mode;
      double _contrast_dark_limit;
      double _contrast_bright_limit;
      double _contrast_shape;
      double   _black_level;
      double   _gamma;
      char*  _family;
      char*  _manufacturer_id;
      char*  _dev_version;
      char*  _firmware_id;
      AlviumConfigType::VmbBool _reverse_x_ddl;
      AlviumConfigType::VmbBool _reverse_y_ddl;
      AlviumConfigType::VmbBool _contrast_ddl;
      AlviumConfigType::VmbBool _correction_ddl;
      AlviumConfigType::RoiMode _roi_mode_ddl;
      AlviumConfigType::ImgCorrectionType _correction_type_ddl;
      AlviumConfigType::ImgCorrectionSet  _correction_set_ddl;
      AlviumConfigType::PixelMode _pixel_mode_ddl;
      AlviumConfigType::TriggerMode _trigger_mode_ddl;
      AlviumConfigType _cfg;
    };
  }
}

#endif
