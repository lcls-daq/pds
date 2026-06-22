#ifndef Pds_NsCam_Types_hh
#define Pds_NsCam_Types_hh

#include <iostream>

#define AS_ENUM(T,E) E,
#define AS_STRING_CASE(T,E) case T::E: return #E;

#define CREATE_ENUM(ENUMS, ETYPE) \
  enum class ETYPE : unsigned { ENUMS(AS_ENUM) }; \
  inline char const* toString(const ETYPE& etype) { \
    switch (etype) { \
      ENUMS(AS_STRING_CASE) \
      default: \
        return nullptr; \
    } \
  } \
  inline std::ostream& operator<<(std::ostream& os, const ETYPE& etype) { \
    return os << toString(etype); \
  }

#define SENSORS(FUNC) \
  FUNC(SensorType,DAEDALUS) \
  FUNC(SensorType,ICARUS) \
  FUNC(SensorType,ICARUS2)

#define BOARDS(FUNC) \
  FUNC(BoardType,LLNL_V1) \
  FUNC(BoardType,LLNL_V4)

#define COMMS(FUNC) \
  FUNC(CommType,RS422) \
  FUNC(CommType,GIGE)

#define TEMPS(FUNC) \
  FUNC(TempType,C) \
  FUNC(TempType,F) \
  FUNC(TempType,K)

#define PRESSURES(FUNC) \
  FUNC(PressureType,torr) \
  FUNC(PressureType,psi) \
  FUNC(PressureType,bar) \
  FUNC(PressureType,atm) \
  FUNC(PressureType,inHg)

#define SIDES(FUNC) \
  FUNC(SideType,AB) \
  FUNC(SideType,A) \
  FUNC(SideType,B)

#define TRIGGERS(FUNC) \
  FUNC(TriggerType,HARDWARE) \
  FUNC(TriggerType,SOFTWARE) \
  FUNC(TriggerType,DUAL)

#define OSCILLATORS(FUNC) \
  FUNC(OscillatorType,RELAXATION) \
  FUNC(OscillatorType,RING) \
  FUNC(OscillatorType,RINGNOOSC) \
  FUNC(OscillatorType,EXTERNAL)

namespace Pds {
  namespace NsCam {
    CREATE_ENUM(SENSORS, SensorType)
    CREATE_ENUM(BOARDS, BoardType)
    CREATE_ENUM(COMMS, CommType)
    CREATE_ENUM(TEMPS, TempType)
    CREATE_ENUM(PRESSURES, PressureType)
    CREATE_ENUM(SIDES, SideType)
    CREATE_ENUM(TRIGGERS, TriggerType)
    CREATE_ENUM(OSCILLATORS, OscillatorType)
  }
}

#undef AS_ENUM
#undef AS_STRING_CASE
#undef CREATE_ENUM
#undef SENSORS
#undef BOARDS
#undef COMMS
#undef TEMPS
#undef PRESSURES
#undef SIDES
#undef TRIGGERS
#undef OSCILLATORS

#endif
