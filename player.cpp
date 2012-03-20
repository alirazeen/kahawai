#include <stdint.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_thread.h>

extern "C" {
#include "x264.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

extern AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
extern AVDictionary *format_opts, *codec_opts;


AVDictionary *filter_codec_opts(AVDictionary *opts, enum CodecID codec_id,
	int encoder) {
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
			if (av_opt_find(avcodec_opts[0], t->key, NULL, flags, 0)
				|| (codec && codec->priv_class
				&& av_opt_find(&codec->priv_class, t->key, NULL, flags,
				0)))
				av_dict_set(&ret, t->key, t->value, 0);
			else if (t->key[0] == prefix
				&& av_opt_find(avcodec_opts[0], t->key + 1, NULL, flags, 0))
				av_dict_set(&ret, t->key + 1, t->value, 0);
		}
		return ret;
}

AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
	AVDictionary *codec_opts) {
		int i;
		AVDictionary **opts;

		if (!s->nb_streams)
			return NULL;
		opts = (AVDictionary**)av_mallocz(s->nb_streams * sizeof(*opts));
		if (!opts) {
			av_log(NULL, AV_LOG_ERROR,
				"Could not alloc memory for stream options.\n");
			return NULL;
		}
		for (i = 0; i < s->nb_streams; i++)
			opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codec->codec_id,
			0);
		return opts;
}



