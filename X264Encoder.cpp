#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "X264Encoder.h"
#include "Measurement.h"

#define ZERO_LATENCY "zerolatency"

X264Encoder::X264Encoder(int height, int width, int fps, int crf, int preset, int gop)
	:_height(height),
	_width(width),
	_fps(fps),
	_gop(gop),
	_crf(crf),
	_preset(preset),
	_y420pFrameSize((width*height*3)/2) //YUV420p 4 bytes each 6 pixels
{
	x264_param_t param;

	//define compression parameters
	x264_param_default_preset(&param, x264_preset_names[_preset], ZERO_LATENCY );
	//For threading
	param.i_threads = 9;
	param.b_sliced_threads = 1;

	param.i_width = width;
	param.i_height = height;
	param.b_intra_refresh = 0;
	//Rate control:
	param.rc.i_rc_method = X264_RC_CRF;
	param.rc.f_rf_constant = (float)_crf;
	//For streaming:
	param.b_repeat_headers = 1;
	param.b_annexb = 1;


	if(_gop<0)
	{
		param.i_fps_num = fps;
		param.i_fps_den = 1;
		// Intra refresh:
		param.i_keyint_max = fps;
	}
	else
	{
		param.i_fps_num = fps;
		param.i_fps_den = 1;
		// Intra refresh:
		param.i_keyint_max = _gop; //maximum number of frames in between each iFrame
		param.i_scenecut_threshold = 0; //no scene_cut
	}

	////	It may be useful to look at this parameters as well //
	//		param.b_deterministic = 0; //This may be good enough in delta, dont know if it will work with i/P frame
	//		param.rc.i_lookahead = 0;
	//		param.i_sync_lookahead = 0;
	//

	x264_param_apply_profile(&param, "baseline");
	_encoder = x264_encoder_open(&param);

}


X264Encoder::~X264Encoder(void)
{
}

/**
 * Encodes a picture frame using the x264 encoder
 * Can optionally apply a transformation operator (i.e. delta or patch)
 * @param pictureIn a x264_picture_t structure with an input picture
 * @param pictureOut a x264_picture_t structure with an output picture
 * @param apply (Optional) a pointer to a binary operator function
 * @param base (Optional) the second operand to the operator function
 * @return a x264_nal_t representing the Network Allocation Unit for the compressed frame
 */
int X264Encoder::Encode(void* pictureIn, void** pictureOut, kahawaiTransform apply, byte* base)
{
	x264_picture_t		pic_out;
	x264_nal_t*			nals;
	int					i_nals;

	x264_picture_t* inputPicture = (x264_picture_t*) pictureIn;

	//Delta Encoding//
	if(apply)
	{

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase("ENCODE_BEFORE_ENCODE_PATCHING", _numEncodedFrames);
#endif
		//Apply the delta transform if invoked from delta encoding (hi - lo)
		for (int i=0 ; i< _y420pFrameSize ; i++) 
		{
			inputPicture->img.plane[0][i] = apply(inputPicture->img.plane[0][i],base[i]);
		}
	}

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase("ENCODE_BEFORE_ACTUAL_ENCODING", _numEncodedFrames);
#endif

	//Encode the frame
	int frame_size = x264_encoder_encode(_encoder, &nals, &i_nals, inputPicture, &pic_out);

	*pictureOut = nals[0].p_payload;

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase("ENCODE_ALL_DONE", _numEncodedFrames);
#endif

	_numEncodedFrames++;
	return frame_size;
}



int X264Encoder::GetBlackFrame(SwsContext* convertCtx, void** pictureOut)
{
	uint8_t* blankSourceFrame = new uint8_t[_width*_height*SOURCE_BITS_PER_PIXEL];
	memset(blankSourceFrame, 0, _width*_height*SOURCE_BITS_PER_PIXEL);

	x264_picture_t *blankFrame = new x264_picture_t;
	x264_picture_alloc(blankFrame, X264_CSP_I420, _width, _height);

	x264_nal_t*			nals;
	int					i_nals;
	x264_picture_t		pic_out;
	void *encodedFrame = NULL;

	int srcstride = _width * SOURCE_BITS_PER_PIXEL; //RGB Stride
	uint8_t *src[3]= {blankSourceFrame, NULL, NULL};
	sws_scale(convertCtx, src, &srcstride, 0, _height, blankFrame->img.plane, blankFrame->img.i_stride);

	int frameSize = x264_encoder_encode(_encoder, &nals, &i_nals, blankFrame, &pic_out);
	*pictureOut = nals[0].p_payload;

	x264_picture_clean(blankFrame);
	delete[] blankSourceFrame;

	return frameSize;
}

#endif
