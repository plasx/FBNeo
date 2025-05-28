#ifndef DRIVER_LIST_H
#define DRIVER_LIST_H

// This file contains the driver list for the Metal build
// Including CPS2 drivers for proper emulation support

// CPS2 Drivers
extern struct BurnDriver BurnDrvCpsMvsc;
extern struct BurnDriver BurnDrvCpsMvscu;
extern struct BurnDriver BurnDrvCpsMvscj;
extern struct BurnDriver BurnDrvCpsMvscjr1;
extern struct BurnDriver BurnDrvCpsMvscr1;

// Marvel vs. Capcom and its variants
extern struct BurnDriver BurnDrvCpsMshvsf;
extern struct BurnDriver BurnDrvCpsMshvsfj;
extern struct BurnDriver BurnDrvCpsMshvsfu;
extern struct BurnDriver BurnDrvCpsMshvsfu1;

// Street Fighter games
extern struct BurnDriver BurnDrvCpsSfa3;
extern struct BurnDriver BurnDrvCpsSfa3b;
extern struct BurnDriver BurnDrvCpsSfa3u;
extern struct BurnDriver BurnDrvCpsSfz3j;
extern struct BurnDriver BurnDrvCpsSfz3a;

// External declaration - actual definition is in driverlist.cpp
extern struct BurnDriver* pDriver[];

#endif // DRIVER_LIST_H 