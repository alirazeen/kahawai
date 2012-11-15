#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "X264Encoder.h"

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
int X264Encoder::Encode(void* pictureIn, void** pictureOut, transform apply, byte* base)
{
	x264_picture_t		pic_out;
	x264_nal_t*			nals;
	int					i_nals;

	x264_picture_t* inputPicture = (x264_picture_t*) pictureIn;

	//Delta Encoding//
	if(apply)
	{
		//Apply the delta transform if invoked from delta encoding (hi - lo)
		for (int i=0 ; i< _y420pFrameSize ; i++) 
		{
			inputPicture->img.plane[0][i] = apply(inputPicture->img.plane[0][i],base[i]);
		}
	}

	//Encode the frame
	int frame_size = x264_encoder_encode(_encoder, &nals, &i_nals, inputPicture, &pic_out);

	*pictureOut = nals[0].p_payload;

	return frame_size;
}
#endif
