#ifndef KAHAWAI_H
#define KAHAWAI_H
#include "../idlib/precompiled.h"

#include <windows.h>
#include <string.h>
#include <tchar.h>

//Master renders hi quality, slave renders low quality
enum KAHAWAI_MODE { Master=0, Slave=1, Client=2, Undefined=3};
#define MAPPING_SIZE (1024*768*3)+18

#define KAHAWAI_MAP_FILE "kahawai.dat"

//Kahawai Video modes: 640x480, 800x600, 1024x768, 1280x960,1600x1200
const int KAHAWAI_WIDTH[5] = {640,800,1024,1280,1600};
const int KAHAWAI_HEIGHT[5] = {480,600,768,960,1200} ; 

extern KAHAWAI_MODE kahawaiProcessId;
extern HANDLE kahawaiMutex;
extern HANDLE kahawaiMap;
extern HANDLE kahawaiSlaveBarrier;
extern HANDLE kahawaiMasterBarrier;
extern int kahawaiLoVideoMode;
extern int kahawaiHiVideoMode;
extern int kahawaiIFrameSkip;

bool InitKahawai();

#endif