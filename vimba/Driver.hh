#ifndef Pds_Vimba_Driver_hh
#define Pds_Vimba_Driver_hh

#include "vimba/include/VimbaC.h"

#include <stdexcept>
#include <string>

namespace Pds {
  namespace Vimba {
    class VimbaException : public std::runtime_error {
      public:
        VimbaException(VmbError_t code, const std::string& msg);
        virtual ~VimbaException() throw();
        VmbError_t code() const;
      protected:
        static std::string buildMsg(VmbError_t code, const std::string& msg);
      private:
        VmbError_t  _code;
    };

    class Camera {
      public:
        enum TriggerMode { FreeRun, External, Software, UnsupportedTrigger };
        enum PixelFormat { Mono8, Mono10, Mono10p, Mono12, Mono12p, Mono14, Mono16, UnsupportedFormat };
        enum CorrectionType { DefectPixelCorrection, FixedPatternNoiseCorrection, UnsupportedCorrection };
        enum CorrectionSet { Preset, User, UnsupportedSet };

        Camera(VmbCameraInfo_t* info);
        virtual ~Camera();

        void close();
        bool isOpen() const;
        bool isAcquiring() const;
        bool imageCorrectionEnabled() const;
        void listFeatures() const;
        VmbBool_t reverseX() const;
        VmbBool_t reverseY() const;
        VmbBool_t contrastEnable() const;
        VmbBool_t acquisitionFrameRateEnable() const;
        TriggerMode triggerModeEnum() const;
        PixelFormat pixelFormatEnum() const;
        VmbInt64_t payloadSize() const;
        VmbInt64_t deviceLinkThroughputLimit() const;
        VmbInt64_t deviceGenCPMajorVersion() const;
        VmbInt64_t deviceGenCPMinorVersion() const;
        VmbInt64_t deviceTLMajorVersion() const;
        VmbInt64_t deviceTLMinorVersion() const;
        VmbInt64_t deviceSFNCMajorVersion() const;
        VmbInt64_t deviceSFNCMinorVersion() const;
        VmbInt64_t deviceSFNCPatchVersion() const;
        VmbInt64_t deviceLinkSpeed() const;
        VmbInt64_t deviceIndicatorLuminance() const;
        VmbInt64_t height() const;
        VmbInt64_t heightStep() const;
        VmbInt64_t heightMax() const;
        VmbInt64_t width() const;
        VmbInt64_t widthStep() const;
        VmbInt64_t widthMax() const;
        VmbInt64_t offsetX() const;
        VmbInt64_t offsetXStep() const;
        VmbInt64_t offsetY() const;
        VmbInt64_t offsetYStep() const;
        VmbInt64_t sensorHeight() const;
        VmbInt64_t sensorWidth() const;
        VmbInt64_t contrastShape() const;
        VmbInt64_t contrastBrightLimit() const;
        VmbInt64_t contrastDarkLimit() const;
        VmbInt64_t timestamp() const;
        VmbInt64_t maxDriverBuffersCount() const;
        VmbInt64_t streamAnnounceBufferMinimum() const;
        VmbInt64_t streamAnnouncedBufferCount() const;
        VmbInt64_t correctionDataSize() const;
        VmbInt64_t correctionEntryType() const;
        double acquisitionFrameRate() const;
        double deviceTemperature() const;
        double deviceLinkCommandTimeout() const;
        double blackLevel() const;
        double gain() const;
        double gamma() const;
        double exposureTime() const;
        std::string deviceManufacturer() const;
        std::string deviceVendorName() const;
        std::string deviceFamilyName() const;
        std::string deviceModelName() const;
        std::string deviceFirmwareID() const;
        std::string deviceFirmwareVersion() const;
        std::string deviceVersion() const;
        std::string deviceSerialNumber() const;
        std::string deviceUserID() const;
        const char* triggerMode() const;
        const char* exposureMode() const;
        const char* deviceTemperatureSelector() const;
        const char* deviceScanType() const;
        const char* deviceLinkThroughputLimitMode() const;
        const char* deviceIndicatorMode() const;
        const char* shutterMode() const;
        const char* pixelFormat() const;
        const char* pixelSize() const;
        const char* contrastConfigurationMode() const;
        const char* blackLevelSelector() const;
        const char* gainSelector() const;
        const char* acquisitionFrameRateMode() const;
        const char* streamBufferHandlingMode() const;
        const char* correctionMode() const;
        const char* correctionSelector() const;
        const char* correctionSet() const;
        const char* correctionSetDefault() const;

        bool unsetImageRoi();
        bool setImageRoi(VmbInt64_t offset_x, VmbInt64_t offset_y, VmbInt64_t width, VmbInt64_t height);
        bool setImageFlip(VmbBool_t flipX, VmbBool_t flipY);
        bool setImageCorrections(VmbBool_t enabled, CorrectionType corr_type, CorrectionSet corr_set);
        bool setDeviceLinkThroughputLimit(VmbInt64_t limit);
        bool setAcquisitionMode(VmbUint32_t nframes);
        bool setTriggerMode(TriggerMode mode, double exposure_us, bool invert=false);
        bool setAnalogControl(double black_level, double gain, double gamma);
        bool setContrastEnhancement(VmbBool_t enabled, VmbInt64_t shape, VmbInt64_t dark_limit, VmbInt64_t bright_limit);
        bool setPixelFormat(PixelFormat format);
        bool registerFrame(const VmbFrame_t* frame);
        bool registerFrames(const VmbFrame_t* frame, VmbUint32_t nframes);
        bool unregisterFrame(const VmbFrame_t* frame);
        bool unregisterFrames(const VmbFrame_t* frame, VmbUint32_t nframes);
        bool unregisterAllFrames();
        bool captureStart();
        bool captureEnd();
        bool acquisitionStart();
        bool acquisitionStop();
        bool queueFrame(const VmbFrame_t* frame, VmbFrameCallback cb = NULL);
        bool queueFrames(const VmbFrame_t* frame, VmbUint32_t nframes, VmbFrameCallback cb = NULL);
        bool waitFrame(const VmbFrame_t* frame, VmbUint32_t timeout, bool* had_timeout = NULL);
        bool flushFrames();
        bool reset();
        bool sendSoftwareTrigger();
        bool resetTimestamp();
        bool latchTimestamp();
      protected:
        const char* getEnum(const char* name) const;
        VmbBool_t getBool(const char* name) const;
        VmbInt64_t getInt(const char* name) const;
        VmbInt64_t getIntStep(const char* name) const;
        double getDouble(const char* name) const;
        std::string getString(const char* name) const;
        VmbBool_t isWritable(const char* name) const;

        bool setEnum(const char* name, const char* value);
        bool setBool(const char* name, VmbBool_t value);
        bool setInt(const char* name, VmbInt64_t value);
        bool setDouble(const char* name, double value);

        bool runCommand(const char* name);

        VmbError_t getBoolFeature(const char* name, VmbBool_t* value) const;
        VmbError_t setBoolFeature(const char* name, VmbBool_t value);
        VmbError_t getEnumFeature(const char* name, const char** value) const;
        VmbError_t setEnumFeature(const char* name, const char* value);
        VmbError_t getIntFeature(const char* name, VmbInt64_t* value) const;
        VmbError_t setIntFeature(const char* name, VmbInt64_t value);
        VmbError_t getFloatFeature(const char* name, double* value) const;
        VmbError_t setFloatFeature(const char* name, double value);
        VmbError_t getStringFeature(const char* name, std::string& value) const;
        VmbError_t setStringFeature(const char* name, std::string value);
        VmbError_t getStringFeature(const char* name, char* value, VmbUint32_t size) const;
        VmbError_t setStringFeature(const char* name, const char* value);
      private:
        void listAccess(const char* name) const;
        void listEnumRange(const char* name) const;
        bool checkImageRoi(VmbInt64_t offset_x, VmbInt64_t offset_y, VmbInt64_t width, VmbInt64_t height) const;
        bool setCorrectionsEnabled(VmbBool_t enabled);
        bool setCorrectionsType(CorrectionType corr_type);
        bool setCorrectionsSet(CorrectionSet corr_set);
      public:
        static std::string getGeniCamPath();
        static bool getVersionInfo(VmbVersionInfo_t* info);
        static bool getCameraInfo (VmbCameraInfo_t* info, VmbUint32_t index, const char* serial_id);
      private:
        VmbHandle_t       _cam;
        VmbCameraInfo_t   _info; 
        VmbUint32_t       _num_features;
        VmbFeatureInfo_t* _features;
        bool              _capture;
    };
  }
}

#endif
