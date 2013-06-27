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
#define DEFAULT_BIT_DEPTH 3

#define PORT_OFFSET_IFRAME_MUXER 5
#define PORT_OFFSET_INPUT_HANDLER 10

#define MEASUREMENT_FRAME_WARM_UP 100
#define MAX_FRAMES_TO_RUN 60000000

#define FRAME_GAP 0
#define MAX_INPUT_QUEUE_LENGTH_CLIENT 60 //XXX: Why 60? Because some arbitrary number is needed and the game runs at 60 FPS so we'll keep it at 60.
#define MAX_INPUT_QUEUE_LENGTH_SERVER 60

#define RECONNECTION_WAIT 5000

enum ENCODING_MODE { DeltaEncoding, IPFrame, H264};
enum CAPTURE_MODE { OpenGL, DirectX};
#else
extern bool offloading;
#endif
#endif