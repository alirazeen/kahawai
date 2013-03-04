#ifndef _KAHAWAI_BASE_H
#define _KAHAWAI_BASE_H
#ifdef KAHAWAI

#ifdef _DEBUG
#ifndef VERIFY
#define VERIFY(f)          assert(f)
#endif
#else
#ifndef VERIFY
#define VERIFY(f)          ((void)(f))
#endif
#endif 

#define DEBUG_PATH "E:\\Benchmarks"
#define MAX_ATTEMPTS 100 //Maximum reconnection attempts


#include "kahawaiDependencies.h"


class Kahawai;


//Defines, enums and constants

const char kahawaiMaster[8] = "leader\n";

//debugging and storing of frames
extern char				g_resultsPath[200]; //Path to save the captured frames


//Kahawai special helper routines
bool CreateKahawaiThread(LPTHREAD_START_ROUTINE function, void* instance);

//Typedef for delta/patch form functions
typedef byte (*kahawaiTransform)(byte apply,byte patch);


#define KAHAWAI_MAP_FILE "kahawai.dat"
#define FIRST_VIDEO_STREAM 0
#define IFRAME_MUTEX "IFRAME"
#define KAHAWAI_DEFAULT_PORT 9001
#define KAHAWAI_LOCALHOST "127.0.0.1"
#define SOURCE_BITS_PER_PIXEL 4 //We are using alpha
#define TARGET_FPS 60


enum ENCODING_MODE { DeltaEncoding, IPFrame, H264};
enum CAPTURE_MODE { OpenGL, DirectX};
#else
extern bool offloading;
#endif
#endif