#ifndef KAHAWAI_UTILS_H
#define KAHAWAI_UTILS_H

typedef unsigned char byte;


//MACRO to obtain the size of a bitmap in YUV420p given its resolution
#define YUV420pBitsPerPixel(W, H) (((W*H)*3)/2)

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