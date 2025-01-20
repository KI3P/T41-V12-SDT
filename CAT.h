#ifndef CAT_H
#define CAT_H

extern int my_ptt;
extern bool catTX;

extern int CATOptions();
extern char *processCATCommand(char *buffer);
extern void CATSerialEvent();
extern int ChangeBand(long f, bool updateRelays);

#endif // V12_CAT

