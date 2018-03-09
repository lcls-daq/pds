/*
 * Epix10kaStatusRegisters.cc
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

#include <stdio.h>
#include <string.h>
#include "pds/epix10ka/Epix10kaStatusRegisters.hh"
#include "pds/epix10ka/Epix10kaConfigurator.hh"
#include "pds/epix10ka/Epix10kaDestination.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
//   <status>
//      <register> <name>version</name> <address>0</address> <lane>0</lane> <vc>2</vc> <size>1</size> </register>
//      <register> <name>regStatus</name> <address>2</address> <lane>0</lane> <vc>2</vc> <size>1</size>
//         <field> <bits>0</bits> <label>LocLinkReady</label> </field>
//         <field> <bits>1</bits> <label>RemLinkReady</label> </field>
//         <field> <bits>2</bits> <label>PibLinkReady</label> </field>
//         <field> <bits>7:4</bits> <label>CntCellError</label> </field>
//         <field> <bits>15:12</bits> <label>CntLinkError</label> </field>
//         <field> <bits>19:16</bits> <label>CntLinkDown</label> </field>
//         <field> <bits>27:24</bits> <label>CntOverFlow</label> </field>
//      </register>
//      <register> <name>txCount</name> <address>3</address> <lane>0</lane> <vc>2</vc> <size>1</size> </register>
//      <register> <name>AdcChipIdReg</name> <address>49153</address> <lane>0</lane> <vc>1</vc> <size>1</size>
//         <field> <bits>7:0</bits><label>AdcChipId</label></field>
//      </register>
//      <register> <name>AdcChipGradeReg</name> <address>49154</address> <lane>0</lane> <vc>1</vc> <size>1</size>
//         <field> <bits>6:4</bits><label>AdcChipGrade</label></field>
//      </register>
//   </status>


using namespace Pds::Epix10ka;

class Epix10kaDestination;
class StatusRegister;
//class StatusLane;

enum Addresses {versionAddr=0, laneStatusAddr=2, txCountAddr=3, chipIdAddr=49153, chipGradeAddr=49154};

void Epix10kaStatusRegisters::print() {
  printf("Epix10kaStatusRegisters: version(0x%x)\n", version);
  lane.print();
  printf("\ttxCounter(%u) AdcChipReg(0x%x), AdcChipGradeReg(%u)\n",
      txCounter, AdcChipIdReg&0xff, (AdcChipGradeReg>>4)&7);
}


void StatusLane::print() {
  printf("\tLink Status: locLink(%u) remLink(%u) pibLink(%u)\n",
      locLinkReady, remLinkReady, PibLinkReady);
  printf("\t\tcellErrorCount(%u) linkErrorCount(%u) linkDownCount(%u) bufferOverflowCount(%u)\n",
      cellErrorCount, linkErrorCount, linkDownCount, bufferOverflowCount);
}

int Epix10kaStatusRegisters::read() {
  int ret = 0;
  Epix10kaDestination d;
  d.dest(Epix10kaDestination::VC2);
  ret |= pgp->readRegister(&d,versionAddr,0x10,(uint32_t*)&version);
  ret |= pgp->readRegister(&d,laneStatusAddr,0x11,(uint32_t*)&lane);
  ret |= pgp->readRegister(&d,txCountAddr,0x12,(uint32_t*)&txCounter);
  d.dest(Epix10kaDestination::VC1);
  ret |= pgp->readRegister(&d,chipIdAddr,0x23,(uint32_t*)&AdcChipIdReg);
  ret |= pgp->readRegister(&d,chipGradeAddr,0x24,(uint32_t*)&AdcChipGradeReg);
  if (ret != 0) {
    printf("\tEpix10ka Status Registers encountered error while reading!\n");
  }
  return ret;
}

unsigned Epix10kaStatusRegisters::readVersion(uint32_t* retp) {
  Epix10kaDestination d;
  d.dest(Epix10kaDestination::VC2);
  unsigned ret = pgp->readRegister(&d,versionAddr,0x111,retp);
  return(ret);
}

