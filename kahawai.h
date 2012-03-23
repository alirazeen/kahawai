#ifndef KAHAWAI_H
#define KAHAWAI_H

#include <windows.h>
#include <string.h>
#include <tchar.h>

//Master renders hi quality, slave renders low quality
enum KAHAWAI_MODE { Master=0, Slave=1, Client=2, Undefined=3};
#define MAPPING_SIZE (1024*768*3)+18

#define KAHAWAI_MAP_FILE "kahawai.dat"

extern KAHAWAI_MODE kahawaiProcessId;
extern HANDLE kahawaiMutex;
extern HANDLE kahawaiMap;
extern HANDLE kahawaiSlaveBarrier;
extern HANDLE kahawaiMasterBarrier;
extern int kahawaiLoVideoHeight;
extern int kahawaiHiVideoHeight;
extern int kahawaiIFrameSkip;
extern int kahawaiServerPort;
extern char kahawaiServerIP[75];

bool InitKahawai();

#endif