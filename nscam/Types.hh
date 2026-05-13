#ifndef Pds_NsCam_Types_hh
#define Pds_NsCam_Types_hh

#define AS_ENUM(T,E) E,
#define AS_STRING_CASE(T,E) case T::E: return #E;

#define CREATE_ENUM(ENUMS, ETYPE) \
  enum class ETYPE : unsigned { ENUMS(AS_ENUM) }; \
  inline char const* toString(const ETYPE etype) { \
    switch (etype) { \
      ENUMS(AS_STRING_CASE) \
      default: \
        return nullptr; \
    } \
  }

#define BOARDS(F) \
  F(BoardType,LLNL_V1) \
  F(BoardType,LLNL_V4)

#define COMMS(F) \
  F(CommType,RS422) \
  F(CommType,GIGE)

namespace Pds {
  namespace NsCam {
    CREATE_ENUM(BOARDS, BoardType)
    CREATE_ENUM(COMMS, CommType)
  }
}

#undef AS_ENUM
#undef AS_STRING_CASE
#undef CREATE_ENUM
#undef BOARDS
#undef COMMS

#endif
