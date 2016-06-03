// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_pci9056.h $
// $Rev$
// $Date$

#ifndef __GSC_PCI9056_H__
#define __GSC_PCI9056_H__

#include "gsc_common.h"



// #defines	*******************************************************************

#define	GSC_PCI_9056_ENCODE(s,o)	GSC_REG_ENCODE(GSC_REG_PCI,(s),(o))
#define	GSC_PLX_9056_ENCODE(s,o)	GSC_REG_ENCODE(GSC_REG_PLX,(s),(o))

// PLX PCI9056 PCI Configuration Registers
#define GSC_PCI_9056_VIDR			GSC_PCI_9056_ENCODE(2, 0x00)
#define GSC_PCI_9056_DIDR			GSC_PCI_9056_ENCODE(2, 0x02)
#define GSC_PCI_9056_CR				GSC_PCI_9056_ENCODE(2, 0x04)
#define GSC_PCI_9056_SR				GSC_PCI_9056_ENCODE(2, 0x06)
#define GSC_PCI_9056_REV			GSC_PCI_9056_ENCODE(1, 0x08)
#define GSC_PCI_9056_CCR			GSC_PCI_9056_ENCODE(3, 0x09)
#define GSC_PCI_9056_CLSR			GSC_PCI_9056_ENCODE(1, 0x0C)
#define GSC_PCI_9056_LTR			GSC_PCI_9056_ENCODE(1, 0x0D)
#define GSC_PCI_9056_HTR			GSC_PCI_9056_ENCODE(1, 0x0E)
#define GSC_PCI_9056_BISTR			GSC_PCI_9056_ENCODE(1, 0x0F)
#define GSC_PCI_9056_BAR0			GSC_PCI_9056_ENCODE(4, 0x10)
#define GSC_PCI_9056_BAR1			GSC_PCI_9056_ENCODE(4, 0x14)
#define GSC_PCI_9056_BAR2			GSC_PCI_9056_ENCODE(4, 0x18)
#define GSC_PCI_9056_BAR3			GSC_PCI_9056_ENCODE(4, 0x1C)
#define GSC_PCI_9056_BAR4			GSC_PCI_9056_ENCODE(4, 0x20)
#define GSC_PCI_9056_BAR5			GSC_PCI_9056_ENCODE(4, 0x24)
#define GSC_PCI_9056_CIS			GSC_PCI_9056_ENCODE(4, 0x28)
#define GSC_PCI_9056_SVID			GSC_PCI_9056_ENCODE(2, 0x2C)
#define GSC_PCI_9056_SID			GSC_PCI_9056_ENCODE(2, 0x2E)
#define GSC_PCI_9056_ERBAR			GSC_PCI_9056_ENCODE(4, 0x30)
#define GSC_PCI_9056_CAP_PTR		GSC_PCI_9056_ENCODE(1, 0x34)
#define GSC_PCI_9056_ILR			GSC_PCI_9056_ENCODE(1, 0x3C)
#define GSC_PCI_9056_IPR			GSC_PCI_9056_ENCODE(1, 0x3D)
#define GSC_PCI_9056_MGR			GSC_PCI_9056_ENCODE(1, 0x3E)
#define GSC_PCI_9056_MLR			GSC_PCI_9056_ENCODE(1, 0x3F)
#define GSC_PCI_9056_PMCAPID		GSC_PCI_9056_ENCODE(1, 0x40)
#define GSC_PCI_9056_PMNEXT			GSC_PCI_9056_ENCODE(1, 0x41)
#define GSC_PCI_9056_PMC			GSC_PCI_9056_ENCODE(2, 0x42)
#define GSC_PCI_9056_PMCSR			GSC_PCI_9056_ENCODE(2, 0x44)
#define GSC_PCI_9056_PMCSR_BSE		GSC_PCI_9056_ENCODE(1, 0x46)
#define GSC_PCI_9056_PMDATA			GSC_PCI_9056_ENCODE(1, 0x47)
#define GSC_PCI_9056_HS_CNTL		GSC_PCI_9056_ENCODE(1, 0x48)
#define GSC_PCI_9056_HS_NEXT		GSC_PCI_9056_ENCODE(1, 0x49)
#define GSC_PCI_9056_HS_CSR			GSC_PCI_9056_ENCODE(1, 0x4A)
#define GSC_PCI_9056_PVPDID			GSC_PCI_9056_ENCODE(1, 0x4C)
#define GSC_PCI_9056_PVPD_NEXT		GSC_PCI_9056_ENCODE(1, 0x4D)
#define GSC_PCI_9056_PVPDAD			GSC_PCI_9056_ENCODE(2, 0x4E)
#define GSC_PCI_9056_PVPDATA		GSC_PCI_9056_ENCODE(4, 0x50)

// PLX PCI9056 feature set registers: Local Configuration Registers
#define GSC_PLX_9056_LAS0RR			GSC_PLX_9056_ENCODE(4, 0x00)
#define GSC_PLX_9056_LAS0BA			GSC_PLX_9056_ENCODE(4, 0x04)
#define GSC_PLX_9056_MARBR			GSC_PLX_9056_ENCODE(4, 0x08)
#define GSC_PLX_9056_BIGEND			GSC_PLX_9056_ENCODE(1, 0x0C)
#define GSC_PLX_9056_LMISC1			GSC_PLX_9056_ENCODE(1, 0x0D)
#define GSC_PLX_9056_PROT_AREA		GSC_PLX_9056_ENCODE(1, 0x0E)
#define GSC_PLX_9056_LMISC2			GSC_PLX_9056_ENCODE(1, 0x0F)
#define GSC_PLX_9056_EROMRR			GSC_PLX_9056_ENCODE(4, 0x10)
#define GSC_PLX_9056_EROMBA			GSC_PLX_9056_ENCODE(4, 0x14)
#define GSC_PLX_9056_LBRD0			GSC_PLX_9056_ENCODE(4, 0x18)
#define GSC_PLX_9056_DMRR			GSC_PLX_9056_ENCODE(4, 0x1C)
#define GSC_PLX_9056_DMLBAM			GSC_PLX_9056_ENCODE(4, 0x20)
#define GSC_PLX_9056_DMLBAI			GSC_PLX_9056_ENCODE(4, 0x24)
#define GSC_PLX_9056_DMPBAM			GSC_PLX_9056_ENCODE(4, 0x28)
#define GSC_PLX_9056_DMCFGA			GSC_PLX_9056_ENCODE(4, 0x2C)
#define GSC_PLX_9056_LAS1RR			GSC_PLX_9056_ENCODE(4, 0xF0)
#define GSC_PLX_9056_LAS1BA			GSC_PLX_9056_ENCODE(4, 0xF4)
#define GSC_PLX_9056_LBRD1			GSC_PLX_9056_ENCODE(4, 0xF8)
#define GSC_PLX_9056_DMDAC			GSC_PLX_9056_ENCODE(4, 0xFC)
#define GSC_PLX_9056_ARB			GSC_PLX_9056_ENCODE(1, 0x100)
#define GSC_PLX_9056_PABTADR		GSC_PLX_9056_ENCODE(1, 0x104)

// PLX PCI9056 feature set registers: Runtime Registers
#define GSC_PLX_9056_MBOX0			GSC_PLX_9056_ENCODE(4, 0x40)
#define GSC_PLX_9056_MBOX1			GSC_PLX_9056_ENCODE(4, 0x44)
#define GSC_PLX_9056_MBOX2			GSC_PLX_9056_ENCODE(4, 0x48)
#define GSC_PLX_9056_MBOX3			GSC_PLX_9056_ENCODE(4, 0x4C)
#define GSC_PLX_9056_MBOX4			GSC_PLX_9056_ENCODE(4, 0x50)
#define GSC_PLX_9056_MBOX5			GSC_PLX_9056_ENCODE(4, 0x54)
#define GSC_PLX_9056_MBOX6			GSC_PLX_9056_ENCODE(4, 0x58)
#define GSC_PLX_9056_MBOX7			GSC_PLX_9056_ENCODE(4, 0x5C)
#define GSC_PLX_9056_P2LDBELL		GSC_PLX_9056_ENCODE(4, 0x60)
#define GSC_PLX_9056_L2PDBELL		GSC_PLX_9056_ENCODE(4, 0x64)
#define GSC_PLX_9056_INTCSR			GSC_PLX_9056_ENCODE(4, 0x68)
#define GSC_PLX_9056_CNTRL			GSC_PLX_9056_ENCODE(4, 0x6C)
#define GSC_PLX_9056_VIDR			GSC_PLX_9056_ENCODE(2, 0x70)	// PCI offset 0x00
#define GSC_PLX_9056_DIDR			GSC_PLX_9056_ENCODE(2, 0x72)	// PCI offset 0x02
#define GSC_PLX_9056_REV			GSC_PLX_9056_ENCODE(1, 0x74)	// PCI offset 0x08

// PLX PCI9056 feature set registers: DMA Registers
#define GSC_PLX_9056_DMAMODE0		GSC_PLX_9056_ENCODE(4, 0x80)
#define GSC_PLX_9056_DMAPADR0		GSC_PLX_9056_ENCODE(4, 0x84)
#define GSC_PLX_9056_DMALADR0		GSC_PLX_9056_ENCODE(4, 0x88)
#define GSC_PLX_9056_DMASIZ0		GSC_PLX_9056_ENCODE(4, 0x8C)
#define GSC_PLX_9056_DMADPR0		GSC_PLX_9056_ENCODE(4, 0x90)
#define GSC_PLX_9056_DMAMODE1		GSC_PLX_9056_ENCODE(4, 0x94)
#define GSC_PLX_9056_DMAPADR1		GSC_PLX_9056_ENCODE(4, 0x98)
#define GSC_PLX_9056_DMALADR1		GSC_PLX_9056_ENCODE(4, 0x9C)
#define GSC_PLX_9056_DMASIZ1		GSC_PLX_9056_ENCODE(4, 0xA0)
#define GSC_PLX_9056_DMADPR1		GSC_PLX_9056_ENCODE(4, 0xA4)
#define GSC_PLX_9056_DMACSR0		GSC_PLX_9056_ENCODE(1, 0xA8)
#define GSC_PLX_9056_DMACSR1		GSC_PLX_9056_ENCODE(1, 0xA9)
#define GSC_PLX_9056_DMAARB			GSC_PLX_9056_ENCODE(4, 0xAC)
#define GSC_PLX_9056_DMATHR			GSC_PLX_9056_ENCODE(4, 0xB0)
#define GSC_PLX_9056_DMADAC0		GSC_PLX_9056_ENCODE(4, 0xB4)
#define GSC_PLX_9056_DMADAC1		GSC_PLX_9056_ENCODE(4, 0xB8)

// PLX PCI9056 feature set registers: Messaging Queue Registers
#define GSC_PLX_9056_OPQIS			GSC_PLX_9056_ENCODE(4, 0x30)
#define GSC_PLX_9056_OPQIM			GSC_PLX_9056_ENCODE(4, 0x34)
#define GSC_PLX_9056_IQP			GSC_PLX_9056_ENCODE(4, 0x40)
#define GSC_PLX_9056_OQP			GSC_PLX_9056_ENCODE(4, 0x44)
#define GSC_PLX_9056_MQCR			GSC_PLX_9056_ENCODE(4, 0xC0)
#define GSC_PLX_9056_QBAR			GSC_PLX_9056_ENCODE(4, 0xC4)
#define GSC_PLX_9056_IFHPR			GSC_PLX_9056_ENCODE(4, 0xC8)
#define GSC_PLX_9056_IFTPR			GSC_PLX_9056_ENCODE(4, 0xCC)
#define GSC_PLX_9056_IPHPR			GSC_PLX_9056_ENCODE(4, 0xD0)
#define GSC_PLX_9056_IPTPR			GSC_PLX_9056_ENCODE(4, 0xD4)
#define GSC_PLX_9056_OFHPR			GSC_PLX_9056_ENCODE(4, 0xD8)
#define GSC_PLX_9056_OFTPR			GSC_PLX_9056_ENCODE(4, 0xDC)
#define GSC_PLX_9056_OPHPR			GSC_PLX_9056_ENCODE(4, 0xE0)
#define GSC_PLX_9056_OPTPR			GSC_PLX_9056_ENCODE(4, 0xE4)
#define GSC_PLX_9056_QSR			GSC_PLX_9056_ENCODE(4, 0xE8)



#endif
