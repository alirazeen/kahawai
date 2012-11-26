#include<iostream>

using namespace std;

#ifdef KAHAWAI
#include "kahawaiBase.h"
#include "OpenGLCapturer.h"

#include<fstream>
#include<string>

#ifdef WIN32
#define LOG_RETURN "\r\n"
#else
#define LOG_RETURN "\n"
#endif


/************************************************************************/
/* General Kahawai Utilities                                            */ 
/************************************************************************/

void KahawaiLog(char* content, KahawaiLogLevel errorLevel, ...)
{
	va_list formatArguments;
	int minLevel;
#ifdef _DEBUG
	minLevel = 0;
#else
	minLevel = 1;
#endif

	if(errorLevel >= minLevel)
	{
		char formatted[1000];
		va_start(formatArguments,errorLevel);
		vsprintf_s(formatted,content,formatArguments);
		KahawaiWriteFile(KAHAWAI_LOG_FILE,formatted,strlen(formatted));
		KahawaiWriteFile(KAHAWAI_LOG_FILE,LOG_RETURN,strlen(LOG_RETURN));

	}
}


void KahawaiSaveYUVFrame(const char* subfolder, int serialId, char* data, int width, int height)
{
	char savePath[250];
	sprintf_s(savePath,"%s\\%s\\frame%04d.yuv",g_resultsPath,subfolder, serialId);
	KahawaiWriteFile(savePath,data,YUV420pBitsPerPixel(width,height));

}

void KahawaiSaveVideoFrame(const char* subfolder, char* fileName, char* data, int frame_size)
{
	char savePath[250];
	sprintf_s(savePath,"%s\\%s\\%s",g_resultsPath,subfolder, fileName);
	KahawaiWriteFile(savePath,data,frame_size);

}

void KahawaiWriteFile(const char* filename, char* content, int length, int suffix)
{
	char name[100];
	sprintf_s(name,100, filename,suffix);
	ofstream movieOut (name, ios::out | ios::binary | ios::app);
	movieOut.write(content, length);
	movieOut.close();
}




//////////////////////////////////////////////////////////////////////////
// Flips vertically a bitmap
//////////////////////////////////////////////////////////////////////////
void VerticalFlip(int width, int height, byte* pixelData, int bitsPerPixel)
{
	byte* temp = new byte[width*bitsPerPixel];
	height--; //remember height array ends at height-1


	for (int y = 0; y < (height+1)/2; y++) 
	{
		memcpy(temp,&pixelData[y*width*bitsPerPixel],width*bitsPerPixel);
		memcpy(&pixelData[y*width*bitsPerPixel],&pixelData[(height-y)*width*bitsPerPixel],width*bitsPerPixel);
		memcpy(&pixelData[(height-y)*width*bitsPerPixel],temp,width*bitsPerPixel);
	}
	delete[] temp;
}

void CaptureFrameBuffer(int width, int height, char* filename) 
{
	static int suffix = 0;
	x264_picture_t		pic_in;
	byte* buffer = new byte[width*height*3];
	struct SwsContext*	convertCtx;
	int srcstride = width * 3;

	//TODO: Replace with the appropriate Capturer
	OpenGLCapturer capturer(1024,768);
	buffer = (byte*)capturer.CaptureScreen();
	//glReadBuffer( GL_FRONT );
	//glReadPixels( 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer ); 

	VerticalFlip(width,height, buffer,3);

	x264_picture_alloc(&pic_in, X264_CSP_I420, width, height);

	convertCtx = sws_getContext(width, height, PIX_FMT_RGB24, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	uint8_t *src[3]= {buffer, NULL, NULL}; 

	sws_scale(convertCtx, src, &srcstride, 0, height, pic_in.img.plane, pic_in.img.i_stride);

	char name[100];
	sprintf_s(name,100, filename,suffix);
	ofstream movieOut (name, ios::out | ios::binary | ios::app);
	movieOut.write((char*)pic_in.img.plane[0], (width*height*3)/2);
	movieOut.close();
	x264_picture_clean(&pic_in);
	delete[] buffer;
	suffix++;

}


//Public Accessors



#endif