
#include <chiaki/ffmpegdecoder.h>

#include <libavcodec/avcodec.h>




struct buffer_data
{
         uint8_t *ptr; /* corresponding position pointer in the file */
         size_t size; ///< size left in the buffer /* file current pointer to the end */
};
 
// Key points, custom buffer data should be defined here outside
struct buffer_data bd = {0};

static AVBufferRef *hw_device_ctx = NULL;
static enum AVPixelFormat hw_pix_fmt;

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt) {
            return *p;
	}
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

static enum AVCodecID chiaki_codec_av_codec_id(ChiakiCodec codec)
{
	switch(codec)
	{
		case CHIAKI_CODEC_H265:
		case CHIAKI_CODEC_H265_HDR:
			return AV_CODEC_ID_H265;
		default:
			return AV_CODEC_ID_H264;
	}
}


CHIAKI_EXPORT ChiakiErrorCode chiaki_ffmpeg_decoder_init(ChiakiFfmpegDecoder *decoder, ChiakiLog *log,
		ChiakiCodec codec, const char *hw_decoder_name,
		ChiakiFfmpegFrameAvailable frame_available_cb, void *frame_available_cb_user)
{
	decoder->log = log;
	decoder->frame_available_cb = frame_available_cb;
	decoder->frame_available_cb_user = frame_available_cb_user;
	
	//av_log_set_level(AV_LOG_DEBUG);//fred

	ChiakiErrorCode err = chiaki_mutex_init(&decoder->mutex, false);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	decoder->hw_device_ctx = NULL;
	decoder->hw_pix_fmt = AV_PIX_FMT_NONE;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)
	avcodec_register_all();
#endif

	hw_decoder_name = "drm";//new
	enum AVHWDeviceType type;

	type = av_hwdevice_find_type_by_name(hw_decoder_name);
	if(type == AV_HWDEVICE_TYPE_NONE)
	{
		CHIAKI_LOGE(log, "Hardware decoder \"%s\" not found", hw_decoder_name);
		goto error_codec_context;
	}
	//else
	//	printf("Using hardware decoder \"%s\"", type);


	decoder->av_codec = avcodec_find_decoder_by_name("h264_v4l2m2m");//new
	if(!decoder->av_codec)
	{
		CHIAKI_LOGE(log, "%s Codec not available", chiaki_codec_name(codec));
		goto error_mutex;
	}

	decoder->hw_pix_fmt = AV_PIX_FMT_DRM_PRIME;//new, but doesn't really matter
	hw_pix_fmt = AV_PIX_FMT_DRM_PRIME;

	decoder->codec_context = avcodec_alloc_context3(decoder->av_codec);
	if(!decoder->codec_context)
	{
		CHIAKI_LOGE(log, "Failed to alloc codec context");
		goto error_mutex;
	}
	
	decoder->codec_context->get_format  = get_hw_format; // Negotiating pixel format
	//[h264_v4l2m2m @ 0x1165d10] Format drm_prime chosen by get_format().
	//[h264_v4l2m2m @ 0x1165d10] avctx requested=181 (drm_prime); get_format requested=181 (drm_prime)
	
	//seems needed for:  avctx requested=181 (drm_prime);
	decoder->codec_context->pix_fmt = AV_PIX_FMT_DRM_PRIME;   /* request a DRM frame */
    decoder->codec_context->coded_height = 720;
	decoder->codec_context->coded_width = 1280;
	
	if (avcodec_open2(decoder->codec_context, decoder->av_codec, NULL) < 0) {
		printf("Could not open codec\n");
		exit(1);
	}

		// glHevc does not need a hw_device_ctx
		if(av_hwdevice_ctx_create(&decoder->hw_device_ctx, type, NULL, NULL, 0) < 0)
		{
			CHIAKI_LOGE(log, "Failed to create hwdevice context");
			goto error_codec_context;
		}
		decoder->codec_context->hw_device_ctx = av_buffer_ref(decoder->hw_device_ctx);
	
	
	
	
	

		//decoder->codec_context->profile = FF_PROFILE_H264_BASELINE;
		//decoder->codec_context->get_format  = get_hw_format; // Negotiating pixel format
		//decoder->codec_context->get_format  = AV_PIX_FMT_DRM_PRIME;

	
	//decoder->codec_context->pix_fmt = AV_PIX_FMT_DRM_PRIME; // yes!! Works when Init is run later only.
	printf("Actual Fmt: %d\n", decoder->codec_context->pix_fmt); // 181 !! :D
	
	// sort of works?
	if (hw_decoder_init(decoder->codec_context, type) < 0) {
		printf("Failed hw_decoder_init\n");
		return -1;
	}
	
	
	if(decoder->codec_context->pix_fmt != AV_PIX_FMT_DRM_PRIME)
		printf("Couldn't negoitiate AV_PIX_FMT_DRM_PRIME\n");
	
	
	printf("~~~ END chiaki_ffmpeg_decoder_init\n\n");
	return CHIAKI_ERR_SUCCESS;
error_codec_context:
	if(decoder->hw_device_ctx)
		av_buffer_unref(&decoder->hw_device_ctx);
	avcodec_free_context(&decoder->codec_context);
error_mutex:
	chiaki_mutex_fini(&decoder->mutex);
	return CHIAKI_ERR_UNKNOWN;
}

CHIAKI_EXPORT void chiaki_ffmpeg_decoder_fini(ChiakiFfmpegDecoder *decoder)
{
	avcodec_close(decoder->codec_context);
	avcodec_free_context(&decoder->codec_context);
	if(decoder->hw_device_ctx)
		av_buffer_unref(&decoder->hw_device_ctx);
}

// Collecting Packets to make full frame
// Orig
CHIAKI_EXPORT bool chiaki_ffmpeg_decoder_video_sample_cb_ORIG(uint8_t *buf, size_t buf_size, void *user)
{
	ChiakiFfmpegDecoder *decoder = user;

	chiaki_mutex_lock(&decoder->mutex);
	AVPacket packet;
	av_init_packet(&packet);
	packet.data = buf;
	packet.size = buf_size;
	int r;

send_packet:
	r = avcodec_send_packet(decoder->codec_context, &packet);
	if(r != 0)
	{
		if(r == AVERROR(EAGAIN))
		{
			CHIAKI_LOGE(decoder->log, "AVCodec internal buffer is full removing frames before pushing");
			AVFrame *frame = av_frame_alloc();
			if(!frame)
			{
				CHIAKI_LOGE(decoder->log, "Failed to alloc AVFrame");
				goto hell;
			}
			r = avcodec_receive_frame(decoder->codec_context, frame);
			av_frame_free(&frame);
			if(r != 0)
			{
				CHIAKI_LOGE(decoder->log, "Failed to pull frame");
				goto hell;
			}
			goto send_packet;
		}
		else
		{
			char errbuf[128];
			av_make_error_string(errbuf, sizeof(errbuf), r);
			CHIAKI_LOGE(decoder->log, "Failed to push frame: %s", errbuf);
			goto hell;
		}
	}
	chiaki_mutex_unlock(&decoder->mutex);

	decoder->frame_available_cb(decoder, decoder->frame_available_cb_user);
	return true;
hell:
	chiaki_mutex_unlock(&decoder->mutex);
	return false;
}

// Collecting Packets to make full frame?
//
CHIAKI_EXPORT bool chiaki_ffmpeg_decoder_video_sample_cb(uint8_t *buf, size_t buf_size, void *user)
{
	ChiakiFfmpegDecoder *decoder = user;
	
	//printf("\nCB!! BEGIN decoder_video_sample_cb\n");

	chiaki_mutex_lock(&decoder->mutex);
	AVPacket packet;
	av_init_packet(&packet);
	packet.data = buf;
	packet.size = buf_size;
	AVFrame *frame = NULL;//new
	int r;

	// from Kodi  -  https://github.com/xbmc/xbmc/blob/abeca92abbb63d14acda27525983cf839dab4c3c/xbmc/guilib/FFmpegImage.cpp#L154
	// Actually line 389 better.
	
	// 0 on success, otherwise negative error code: AVERROR(EAGAIN): input is not accepted in the current state - user must read output with avcodec_receive_frame()
	// (once all output is read, the packet should be resent, and the call will not fail with EAGAIN).

	if(&packet)
	{
		r = avcodec_send_packet(decoder->codec_context, &packet);
		// printf("Send Packet (sample_cb) return;  %d\n", r);
		// In particular, we don't expect AVERROR(EAGAIN), because we read all
		// decoded frames with avcodec_receive_frame() until done.
		if (r < 0) {
		  printf("Error return in send_packet\n");
	    }
	}
	 
	chiaki_mutex_unlock(&decoder->mutex);
	decoder->frame_available_cb(decoder, decoder->frame_available_cb_user);
	// the rest now happens in chiaki_ffmpeg_decoder_pull_frame below.
	//printf("END decoder_video_sample_cb\n\n");
	return true;
hell:
	chiaki_mutex_unlock(&decoder->mutex);
	return false;
}

// Don't want to use this. Want to Keep in HW.
static AVFrame *pull_from_hw(ChiakiFfmpegDecoder *decoder, AVFrame *hw_frame)
{
	printf("pull_from_hw\n");
	AVFrame *sw_frame = av_frame_alloc();
	if(av_hwframe_transfer_data(sw_frame, hw_frame, 0) < 0)
	{
		CHIAKI_LOGE(decoder->log, "Failed to transfer frame from hardware");
		av_frame_unref(sw_frame);
		sw_frame = NULL;
	}
	av_frame_unref(hw_frame);
	return sw_frame;
}


// Does the Decode!?  Whats deifferent from  chiaki_ffmpeg_decoder_video_sample_cb
// from Callback? Tries to get a full new frame. If none keep 'last_frame'
// Need to fix this
//

// Original code
CHIAKI_EXPORT AVFrame *chiaki_ffmpeg_decoder_pull_frame_ORIG(ChiakiFfmpegDecoder *decoder)
{
	chiaki_mutex_lock(&decoder->mutex);
	// always try to pull as much as possible and return only the very last frame
	AVFrame *frame_last = NULL;
	AVFrame *frame = NULL;
	while(true)
	{
		AVFrame *next_frame;
		if(frame_last)
		{
			av_frame_unref(frame_last);
			next_frame = frame_last;
		}
		else
		{
			next_frame = av_frame_alloc();
			if(!next_frame)
				break;
		}
		frame_last = frame;
		frame = next_frame;
		int r = avcodec_receive_frame(decoder->codec_context, frame);
		if(!r)
			//frame = decoder->hw_device_ctx ? pull_from_hw(decoder, frame) : frame;
			frame = frame;
		else
		{
			if(r != AVERROR(EAGAIN))
				CHIAKI_LOGE(decoder->log, "Decoding with FFMPEG failed");
			av_frame_free(&frame);
			frame = frame_last;
			break;
		}
	}
	chiaki_mutex_unlock(&decoder->mutex);

	return frame;
}

// This decodes a full frame. My version.
// 
CHIAKI_EXPORT AVFrame *chiaki_ffmpeg_decoder_pull_frame(ChiakiFfmpegDecoder *decoder)
{
	//printf("\nBEGIN chiaki_ffmpeg_decoder_pull_frame\n");
	chiaki_mutex_lock(&decoder->mutex);

	AVFrame *frame = NULL;
	int ret=-1;

	
        if (!(frame = av_frame_alloc())) {
            fprintf(stderr, "Can not alloc frame\n");
            ret = AVERROR(ENOMEM);
            goto the_end;
        }
		
		ret = avcodec_receive_frame(decoder->codec_context, frame); //ret 0 ==success
		//printf("PIX Format after receive_frame:  %d\n", frame->format);//181, -1, 181, -1, etc
		if (ret == 0) {
			//printf("Frame Successfully Decoded\n");
		}
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			printf("AVERROR(EAGAIN)\n");
			av_frame_free(&frame);
			//return 0; /// goes sends/gets another packet.
		} else if (ret < 0) {
			fprintf(stderr, "Error while decoding\n");
		}

the_end:
	chiaki_mutex_unlock(&decoder->mutex);
	//printf("END chiaki_ffmpeg_decoder_pull_frame\n");
	return frame;
}

CHIAKI_EXPORT enum AVPixelFormat chiaki_ffmpeg_decoder_get_pixel_format(ChiakiFfmpegDecoder *decoder)
{
	// TODO: this is probably very wrong, especially for hdr
	return decoder->hw_device_ctx
		? AV_PIX_FMT_NV12
		: AV_PIX_FMT_YUV420P;
}

