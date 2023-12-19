//#include "libavutil/frame.h"

//this was needed due to av_frame_cropped_height error
static inline int av_frame_cropped_width(const AVFrame * const frame)
{
    return frame->width - (frame->crop_left + frame->crop_right);
}
static inline int av_frame_cropped_height(const AVFrame * const frame)
{
    return frame->height - (frame->crop_top + frame->crop_bottom);
}
//patch done
