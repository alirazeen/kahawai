#ifndef KAHAWAI_UTILS_H
#define KAHAWAI_UTILS_H

typedef unsigned char byte;

#include "Networking.h"

//MACRO to obtain the size of a bitmap in YUV420p given its resolution
#define YUV420pBitsPerPixel(W, H) (((W*H)*3)/2)

//MACRO TO SAVE CAPTURES ONLY IN DEBUG MODE
#ifdef _DEBUG
#define LogVideoFrame(condition,subfolder,fileName,data,frame_size) if(condition)KahawaiSaveVideoFrame(subfolder,fileName,data,frame_size);
#define LogYUVFrame(condition,subfolder,serialId,data,width,height) if(condition)KahawaiSaveYUVFrame(subfolder,serialId,data,width,height)
#else
#define LogVideoFrame(subfolder,fileName,data,frame_size)
#define LogYUVFrame(subfolder,serialId,data,width,height)
#endif

//Kahawai Log
#define KAHAWAI_LOG_FILE "kahawai.log"
enum KahawaiLogLevel { KahawaiDebug=0, KahawaiError=1 };
//Public general purpose methods
void CaptureFrameBuffer(int width, int height, char* filename);
void KahawaiLog(char* content, KahawaiLogLevel errorLevel);
void KahawaiWriteFile(const char* filename, char* content, int length, int suffix = 0);
void KahawaiSaveVideoFrame(const char* subfolder, char* fileName, char* data, int frame_size);
void KahawaiSaveYUVFrame(const char* subfolder, int serialId, char* data, int width, int height);
void VerticalFlip(int width, int height, byte* pixelData, int bitsPerPixel);
void CaptureFrameBuffer(int width, int height, char* filename);

#endif