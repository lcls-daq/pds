/*
 * pnCCDConfigurator.cc
 *
 *  Created on: May 30, 2013
 *      Author: jackp
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pds/pgp/Configurator.hh"
#include "pds/pnccd/pnCCDConfigurator.hh"
#include "pds/config/pnCCDConfigType.hh"

using namespace Pds::pnCCD;

class pnCCDDestination;

enum {pnCCDMagicWord=0x1c000300,pnCCDFrameSize=131075}; // ((1<<19)/4) + 4 - 1

pnCCDConfigurator::pnCCDConfigurator(int f, unsigned d) :
                           Pds::Pgp::Configurator(f, d),
                           _rhisto(0) {
  printf("pnCCDConfigurator constructor\n");
  //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
  //    _rhisto = (unsigned*) calloc(1000, 4);
  //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
}

pnCCDConfigurator::~pnCCDConfigurator() {}

void pnCCDConfigurator::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigFileName, name);
  printf("pnCCDConfigurator::runTimeConfigName(%s)\n", name);
}

void pnCCDConfigurator::print() {}

void pnCCDConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

bool pnCCDConfigurator::_flush(unsigned index=0) {
//  enum {numberOfTries=5};
//  unsigned version = 0;
//  unsigned failCount = 0;
  bool ret = false;
//  printf("\n\t--flush-%u-", index);
//  while ((failCount++<numberOfTries) && (Failure == _statRegs.readVersion(&version))) {
//    printf("%s(%u)-", _d.name(), failCount);
//  }
//  if (failCount<numberOfTries) {
//    printf("%s version(0x%x)\n\t", _d.name(), version);
//  }
//  else {
    ret = true;
//    printf("\n\t");
//  }
  return ret;
}

unsigned pnCCDConfigurator::configure( pnCCDConfigType* c, unsigned mask) {
  _config = c;
  timespec      start, end;
  bool printFlag = !(mask & 0x2000);
  mask = ~mask;
  unsigned ret = 0;
  clock_gettime(CLOCK_REALTIME, &start);
  allocateVC(0xf, 0xf);
  writeScratch((unsigned)((c->payloadSizePerLink())/4) - 1);
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("\tret(%u) - done, %s \n", ret, ret ? "FAILED" : "SUCCEEDED");
    printf(" it took %lld.%lld milliseconds with mask 0x%x\n", diff/1000000LL, diff%1000000LL, mask&0x1f);
  }
  dumpFrontEnd();
  return ret;
}

void pnCCDConfigurator::dumpFrontEnd() {
  dumpPgpCard();
}
