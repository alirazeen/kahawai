#include<iostream>

using namespace std;

#ifdef KAHAWAI
#include "Kahawai.h"

#include<fstream>
#include<string>




/************************************************************************/
/* General Kahawai Utilities                                            */ 
/************************************************************************/

void KahawaiWriteFile(const char* filename, char* content, int length, int suffix)
{
	char name[100];
	sprintf_s(name,100, filename,suffix);
	ofstream movieOut (name, ios::out | ios::binary | ios::app);
	movieOut.write(content, length);
	movieOut.close();
}


//////////////////////////////////////////////////////////////////////////
// Basic Delta encoding transformation
//////////////////////////////////////////////////////////////////////////
byte Delta(byte hi, byte lo)
{
	int resultPixel = ((hi - lo) / 2) + 127;

	if (resultPixel > 255)
		resultPixel = 255;

	if (resultPixel < 0)
		resultPixel = 0;

	return (byte)resultPixel;
}

//////////////////////////////////////////////////////////////////////////
// Basic Patch decoding transformation
//////////////////////////////////////////////////////////////////////////
byte Patch(byte delta, byte lo)
{
	int resultPixel = (2 * (delta - 127)) + lo;

	if (resultPixel > 255)
		resultPixel = 255;

	if (resultPixel < 0)
		resultPixel = 0;

	return (byte)resultPixel;
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

bool CreateKahawaiThread(LPTHREAD_START_ROUTINE function, Kahawai* instance)
{
	DWORD ThreadID;
	HANDLE thread = CreateThread(NULL,0,function,(void*) instance, 0, &ThreadID);

	if(thread==NULL)
		return false;

	return true;
}

VOID CaptureFrameBuffer(int width, int height, char* filename) 
{
	static int suffix = 0;
	x264_picture_t		pic_in;
	byte* buffer = new byte[width*height*3];
	struct SwsContext*	convertCtx;
	int srcstride = width * 3;

	glReadBuffer( GL_FRONT );
	glReadPixels( 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer ); 

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
	delete buffer;
	suffix++;

}

AVDictionary* Kahawai::FilterCodecOptions(AVDictionary *opts, enum CodecID codec_id,
	int encoder) {
		AVCodecContext*		_avcodec_opts[AVMEDIA_TYPE_NB];
		AVDictionary *ret = NULL;
		AVDictionaryEntry *t = NULL;
		AVCodec *codec =
			encoder ?
			avcodec_find_encoder(codec_id) :
		avcodec_find_decoder(codec_id);
		int flags =
			encoder ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
		char prefix = 0;

		if (!codec)
			return NULL;

		switch (codec->type) {
		case AVMEDIA_TYPE_VIDEO:
			prefix = 'v';
			flags |= AV_OPT_FLAG_VIDEO_PARAM;
			break;
		case AVMEDIA_TYPE_AUDIO:
			prefix = 'a';
			flags |= AV_OPT_FLAG_AUDIO_PARAM;
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			prefix = 's';
			flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
			break;
		default:
			break;
		}

		while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) {
			if (av_opt_find(_avcodec_opts[0], t->key, NULL, flags, 0)
				|| (codec && codec->priv_class
				&& av_opt_find(&codec->priv_class, t->key, NULL, flags,
				0)))
				av_dict_set(&ret, t->key, t->value, 0);
			else if (t->key[0] == prefix
				&& av_opt_find(_avcodec_opts[0], t->key + 1, NULL, flags, 0))
				av_dict_set(&ret, t->key + 1, t->value, 0);
		}
		return ret;
}

AVDictionary** Kahawai::SetupFindStreamInfoOptions(AVFormatContext *s,
	AVDictionary *codec_opts) {
		AVDictionary **opts;

		if (!s->nb_streams)
			return NULL;
		opts = (AVDictionary**)av_mallocz(s->nb_streams * sizeof(*opts));
		if (!opts) {
			av_log(NULL, AV_LOG_ERROR,
				"Could not alloc memory for stream options.\n");
			return NULL;
		}
		for (unsigned int i = 0; i < s->nb_streams; i++)
			opts[i] = FilterCodecOptions(codec_opts, s->streams[i]->codec->codec_id,
			0);
		return opts;
}

//Public Accessors

bool Kahawai::IsMaster()
{
	return _role == Master;
}

bool Kahawai::IsSlave()
{
	return _role == Slave;
}

bool Kahawai::IsClient()
{
	return _role == Client;
}

bool Kahawai::IsServer()
{
	return _role != Client;
}

KAHAWAI_MODE Kahawai::GetRole()
{
	return _role;
}

bool Kahawai::IsDelta()
{
	return profile == DeltaEncoding;
}

bool Kahawai::IsIFrame()
{
	return profile == IPFrame;
}

ENCODING_PROFILE Kahawai::GetMode()
{
	return profile;
}

bool Kahawai::IsOffloading()
{
	return _offloading;
}

#endif