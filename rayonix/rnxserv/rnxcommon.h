/* rnxcommon.h */
#ifndef _RNX_COMMON_H
#define _RNX_COMMON_H

int frameCountAdvance();
int frameCountReset();

#define RNX_CONTROL_PORT      30042

#define RNX_TRIGGER_PORT      30051
#define RNX_STATUS_PORT       30052

#define RNX_STATUS_MAXTEMPS   64
#define RNX_STATUS_MAGIC      0x51

#define RNX_WORK_FRAMEREADY   10
#define RNX_WORK_STATUSCMD    11

#define RNX_WORK_OPENDEV      20
#define RNX_WORK_CLOSEDEV     21
#define RNX_WORK_STARTACQ     22
#define RNX_WORK_ENDACQ       23
#define RNX_WORK_SWTRIGGER    24
#define RNX_WORK_DARK         25

#define RNX_BINNING_MIN       2
#define RNX_BINNING_MAX       10

typedef struct rnxStatus {
  int magic;
  int ntemps;
  int pad1;
  int pad2;
  int pad3;
  int temperature[RNX_STATUS_MAXTEMPS]; /* hundredths of deg C */
} rnxStatus_t;

#endif /* _RNX_COMMON_H */
