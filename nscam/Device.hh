#ifndef Pds_NsCam_Device_hh
#define Pds_NsCam_Device_hh

#include "pds/nscam/Types.hh"

#include <memory>
#include <string>
#include <map>

namespace Pds {
  namespace NsCam {
    class Comm;

    struct SubRegister {
      std::string regname;
      uint32_t start_bit;
      uint32_t width;
      bool writable;
      uint32_t minV;
      uint32_t maxV;
      SubRegister(std::string regname,
                  uint32_t start_bit,
                  uint32_t width,
                  bool writable);
      //SubRegister(SubRegister& other);

      uint32_t maxValue() const;
      double resolution() const;
    };

    class Device {
    public:
      Device(std::shared_ptr<Comm> comm,
             const std::map<std::string, uint16_t>& regnames,
             const std::map<std::string, SubRegister>& subregnames,
             const std::map<std::string, uint32_t>& defaults);
      Device(std::shared_ptr<Comm> comm,
             const std::map<std::string, uint16_t>& regnames,
             const std::map<std::string, SubRegister>& subregnames,
             const std::map<std::string, uint32_t>& defaults,
             const std::map<std::string, std::string>& aliases,
             const std::map<std::string, std::string>& monitors);
      virtual ~Device() = default;

      virtual uint32_t getRegister(const std::string& regname);
      virtual uint32_t getRegister(uint16_t address);
      virtual void setRegister(const std::string& regname, uint32_t value);
      virtual void setRegister(uint16_t address, uint32_t value);
      virtual uint32_t getSubRegister(const std::string& subregname);
      virtual uint32_t getSubRegister(const SubRegister& subreg);
      virtual void setSubRegister(const std::string& subregname, uint32_t value);
      virtual void setSubRegister(const SubRegister& subreg, uint32_t value);

      virtual CommType commType() const;
      virtual std::string commName() const;

    protected:
      virtual void addRegister(const std::string& regname, uint16_t address);
      virtual void addSubRegister(const std::string& regname, const SubRegister& subreg);
      virtual void addDefault(const std::string& regname, uint32_t value);

    private:
      uint16_t lookupRegister(const std::string& regname) const;
      SubRegister& lookupSubRegister(const std::string& subregname);

      std::map<std::string, uint16_t> regnames_;
      std::map<std::string, SubRegister> subregnames_;
      std::map<std::string, uint32_t> defaults_;
      std::map<std::string, std::string> aliases_;
      std::map<std::string, std::string> monitors_;
      std::shared_ptr<Comm> comm_;
    };
  }
}

#endif
