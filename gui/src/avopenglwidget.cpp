// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <avopenglwidget.h>
#include <avopenglframeuploader.h>
#include <streamsession.h>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLDebugLogger>
#include <QThread>
#include <QTimer>

#define MOUSE_TIMEOUT_MS 1000

//#define DEBUG_OPENGL

// video file decode - fred - clean up later!
///home/pi/dev/rpi-ffmpeg/out/armv7-buster-static-rel/install/include/arm-linux-gnueabihf/libavutil/pixfmt.h
#include "libavutil/pixfmt.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>

#include "libavutil/hwcontext_drm.h"
//fred
//#include <EGL/egl.h>
//#include <EGL/eglext.h>
//#include <GLES2/gl2ext.h>
#include "glhelp.h"

static EGLint texgen_attrs[] = {
   EGL_DMA_BUF_PLANE0_FD_EXT,
   EGL_DMA_BUF_PLANE0_OFFSET_EXT,
   EGL_DMA_BUF_PLANE0_PITCH_EXT,
   EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
   EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
   EGL_DMA_BUF_PLANE1_FD_EXT,
   EGL_DMA_BUF_PLANE1_OFFSET_EXT,
   EGL_DMA_BUF_PLANE1_PITCH_EXT,
   EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
   EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT,
   EGL_DMA_BUF_PLANE2_FD_EXT,
   EGL_DMA_BUF_PLANE2_OFFSET_EXT,
   EGL_DMA_BUF_PLANE2_PITCH_EXT,
   EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
   EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT,
};

static const char *shader_vert_glsl = R"glsl(
#version 300 es

in vec2 pos_attr;

out vec2 uv_var;

void main()
{
	uv_var = pos_attr;
	gl_Position = vec4(pos_attr * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);
}
)glsl";

static const GLchar* fragment_shader_source =
	"#version 300 es\n"
	"#extension GL_OES_EGL_image_external : require\n"
	"precision mediump float;\n"
	"uniform samplerExternalOES plane1;\n"
	"in vec2 uv_var;\n"
	"out vec4 out_color;\n"
	"void main() {	\n"
	"	out_color = texture2D( plane1, uv_var );\n"
	"}\n";


static const char *yuv420p_shader_frag_glsl_NEW = R"glsl(
#version 300 es
#extension GL_OES_EGL_image_external : require
precision mediump float;

uniform samplerExternalOES plane1; // Y
uniform samplerExternalOES plane2; // U
uniform samplerExternalOES plane3; // V

in vec2 uv_var;
out vec4 out_color;

void main()
{
	vec3 yuv = vec3(
		(texture2D(plane1, uv_var).r - (16.0 / 255.0)) / ((235.0 - 16.0) / 255.0),
		(texture2D(plane2, uv_var).r - (16.0 / 255.0)) / ((240.0 - 16.0) / 255.0) - 0.5,
		(texture2D(plane3, uv_var).r - (16.0 / 255.0)) / ((240.0 - 16.0) / 255.0) - 0.5);
	vec3 rgb = mat3(
		1.0,		1.0,		1.0,
		0.0,		-0.21482,	2.12798,
		1.28033,	-0.38059,	0.0) * yuv;
	out_color = vec4(rgb, 1.0);
)glsl";

static const char *yuv420p_shader_frag_glsl = R"glsl(
#version 300 es
#extension GL_OES_EGL_image_external : require
precision mediump float;

uniform samplerExternalOES plane1; // Y
uniform samplerExternalOES plane2; // U
uniform samplerExternalOES plane3; // V

in vec2 uv_var;
out vec4 out_color;

void main()
{
	vec3 yuv = vec3(
		(texture2D(plane1, uv_var).r - (16.0 / 255.0)) / ((235.0 - 16.0) / 255.0),
		(texture2D(plane2, uv_var).r - (16.0 / 255.0)) / ((240.0 - 16.0) / 255.0) - 0.5,
		(texture2D(plane3, uv_var).r - (16.0 / 255.0)) / ((240.0 - 16.0) / 255.0) - 0.5);
	//~ vec3 rgb = mat3(
		//~ 1.0,		1.0,		1.0,
		//~ 0.0,		-0.21482,	2.12798,
		//~ 1.28033,	-0.38059,	0.0) * yuv;
	//out_color = vec4(rgb, 1.0);
	out_color = vec4(yuv, 1.0);
}
)glsl";

static const char *nv12_shader_frag_glsl = R"glsl(
#version 300 es
#extension GL_OES_EGL_image_external : require
precision mediump float;

//uniform sampler2D plane1; // Y
uniform samplerExternalOES plane1;
uniform samplerExternalOES plane2; // interlaced UV

in vec2 uv_var;

out vec4 out_color;

void main()
{
	vec3 yuv = vec3(
		texture2D(plane1, uv_var));
		//(texture2D(plane1, uv_var).r - (16.0 / 255.0)) / ((235.0 - 16.0) / 255.0),
		//(texture2D(plane2, uv_var).r - (16.0 / 255.0)) / ((240.0 - 16.0) / 255.0) - 0.5,
		//(texture2D(plane2, uv_var).g - (16.0 / 255.0)) / ((240.0 - 16.0) / 255.0) - 0.5
	);
	vec3 rgb = mat3(
		1.0,		1.0,		1.0,
		0.0,		-0.21482,	2.12798,
		1.28033,	-0.38059,	0.0) * yuv;
	out_color = vec4(rgb, 1.0);
}
)glsl";

//fred added
ConversionConfig conversion_configs[] = {
	{
		AV_PIX_FMT_DRM_PRIME,
		shader_vert_glsl,
		fragment_shader_source,
		1,
		{
			{ 1, 1, 1, GL_R8, GL_RED }// TODO
		}
	},	
	{
		AV_PIX_FMT_YUV420P,
		shader_vert_glsl,
		yuv420p_shader_frag_glsl,
		3,
		{
			{ 1, 1, 1, GL_R8, GL_RED },
			{ 2, 2, 1, GL_R8, GL_RED },
			{ 2, 2, 1, GL_R8, GL_RED }
		}
	},
	{
		AV_PIX_FMT_NV12,
		shader_vert_glsl,
		nv12_shader_frag_glsl,
		2,
		{
			{ 1, 1, 1, GL_R8, GL_RED },
			{ 2, 2, 2, GL_RG8, GL_RG }
		}
	}
};

static const float vert_pos[] = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	1.0f, 1.0f
};

QSurfaceFormat AVOpenGLWidget::CreateSurfaceFormat()
{
	QSurfaceFormat format;  // We want:  GL_VERSION  : OpenGL ES 3.1 Mesa 21.3.0-devel (git-c679dbe09c)
	format.setDepthBufferSize(0);
	format.setStencilBufferSize(0);
	format.setVersion(3, 1);//was: 3,2
	format.setRenderableType(QSurfaceFormat::OpenGLES);//added
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setSwapBehavior(QSurfaceFormat::SingleBuffer);//added,  QSurfaceFormat::DefaultSwapBehavior
#ifdef DEBUG_OPENGL
	format.setOption(QSurfaceFormat::DebugContext, true);
#endif
	return format;
}

AVOpenGLWidget::AVOpenGLWidget(StreamSession *session, QWidget *parent)
	: QOpenGLWidget(parent),
	session(session)
{
	enum AVPixelFormat pixel_format = chiaki_ffmpeg_decoder_get_pixel_format(session->GetFfmpegDecoder());
	pixel_format = AV_PIX_FMT_DRM_PRIME;//new
	conversion_config = nullptr;
	for(auto &cc : conversion_configs)
	{
		if(pixel_format == cc.pixel_format)
		{
			conversion_config = &cc;
			printf("AVOpenGLWidget - Found Matching ConvConf\n");
			break;
		}
	}

	if(!conversion_config)
		throw Exception("No matching video conversion config can be found");

	setFormat(CreateSurfaceFormat());

	frame_uploader_context = nullptr;
	frame_uploader = nullptr;
	frame_uploader_thread = nullptr;
	frame_fg = 0;

	setMouseTracking(true);
	mouse_timer = new QTimer(this);
	connect(mouse_timer, &QTimer::timeout, this, &AVOpenGLWidget::HideMouse);
	ResetMouseTimeout();
}

AVOpenGLWidget::~AVOpenGLWidget()
{
	if(frame_uploader_thread)
	{
		frame_uploader_thread->quit();
		frame_uploader_thread->wait();
		delete frame_uploader_thread;
	}
	delete frame_uploader;
	delete frame_uploader_context;
	delete frame_uploader_surface;
}

void AVOpenGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	QOpenGLWidget::mouseMoveEvent(event);
	ResetMouseTimeout();
}

void AVOpenGLWidget::ResetMouseTimeout()
{
	unsetCursor();
	mouse_timer->start(MOUSE_TIMEOUT_MS);
}

void AVOpenGLWidget::HideMouse()
{
	setCursor(Qt::BlankCursor);
}

void AVOpenGLWidget::SwapFrames()
{
	QMutexLocker lock(&frames_mutex);
	frame_fg = 1 - frame_fg;
	QMetaObject::invokeMethod(this, "update");
}


// Refresh/swap Texture Here!!??
// *frame == return of chiaki_ffmpeg_decoder_pull_frame
//
bool AVOpenGLFrame::Update(AVFrame *frame, ChiakiLog *log)
{	
	//printf("\nBEGIN AVOpenGLFrame::Update\n");
	//auto f = QOpenGLContext::currentContext()->extraFunctions();
	//printf("PIX Format in Update():  %d\n", frame->format); // 181 is the one!
	//printf("DRM_PRIME as int in Update(): %d\n", AV_PIX_FMT_DRM_PRIME); //prints 182
	
	if(frame->format != AV_PIX_FMT_DRM_PRIME)
	{	
		printf("Inside Check.  Ie not a AV_PIX_FMT_DRM_PRIME Frame\n");
		return false;
	}
	
	//needed for aspect ratio fitting
	width = frame->width;
	height = frame->height;

	
	
	/// My one
	const AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor*)frame->data[0];
	int fd = -1;
	fd = desc->objects[0].fd;
	//printf("FD: %d\n", fd);
	GLuint texture = 1;

	EGLint attribs[50];
	EGLint *a = attribs;
	const EGLint *b = texgen_attrs;
	
	*a++ = EGL_WIDTH;
	*a++ = av_frame_cropped_width(frame);
	*a++ = EGL_HEIGHT;
	*a++ = av_frame_cropped_height(frame);
	*a++ = EGL_LINUX_DRM_FOURCC_EXT;
	*a++ = desc->layers[0].format;

	int i, j;
	for (i = 0; i < desc->nb_layers; ++i) {
            for (j = 0; j < desc->layers[i].nb_planes; ++j) {
                const AVDRMPlaneDescriptor * const p = desc->layers[i].planes + j;
                const AVDRMObjectDescriptor * const obj = desc->objects + p->object_index;
                *a++ = *b++;
                *a++ = obj->fd;
                *a++ = *b++;
                *a++ = p->offset;
                *a++ = *b++;
                *a++ = p->pitch;
                if (obj->format_modifier == 0) {
                   b += 2;
                }
                else {
                   *a++ = *b++;
                   *a++ = (EGLint)(obj->format_modifier & 0xFFFFFFFF);
                   *a++ = *b++;
                   *a++ = (EGLint)(obj->format_modifier >> 32);
                }
            }
     }
	
	 *a = EGL_NONE;

	EGLDisplay egl_display = eglGetCurrentDisplay();

	const EGLImage image = eglCreateImageKHR( egl_display,
                                              EGL_NO_CONTEXT,
                                              EGL_LINUX_DMA_BUF_EXT,
                                              NULL, attribs);
	if (!image) {
		printf("Failed to create EGLImage\n");
		return -1;
	}

	/// his
	glGenTextures(1, &texture);
	glEnable(GL_TEXTURE_EXTERNAL_OES);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);

	new_texture = texture;

	eglDestroyImageKHR(&egl_display, image);
	
	//printf("END AVOpenGLFrame::Update\n\n");
	return true;
}

void AVOpenGLWidget::initializeGL()
{	
	printf("\nAVOpenGLWidget::initializeGL\n");
	
	auto f = QOpenGLContext::currentContext()->extraFunctions();

	const char *gl_version = (const char *)f->glGetString(GL_VERSION);
	CHIAKI_LOGI(session->GetChiakiLog(), "\n\n\nOpenGL initialized with version \"%s\"", gl_version ? gl_version : "(null)");

#ifdef DEBUG_OPENGL
	auto logger = new QOpenGLDebugLogger(this);
	logger->initialize();
	connect(logger, &QOpenGLDebugLogger::messageLogged, this, [](const QOpenGLDebugMessage &msg) {
		qDebug() << msg;
	});
	logger->startLogging();
#endif

	auto CheckShaderCompiled = [&](GLuint shader) {
		GLint compiled = 0;
		f->glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if(compiled == GL_TRUE)
			return;
		GLint info_log_size = 0;
		f->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_size);
		QVector<GLchar> info_log(info_log_size);
		f->glGetShaderInfoLog(shader, info_log_size, &info_log_size, info_log.data());
		f->glDeleteShader(shader);
		CHIAKI_LOGE(session->GetChiakiLog(), "Failed to Compile Shader:\n%s", info_log.data());
	};

	GLuint shader_vert = f->glCreateShader(GL_VERTEX_SHADER);
	f->glShaderSource(shader_vert, 1, &conversion_config->shader_vert_glsl, nullptr);
	f->glCompileShader(shader_vert);
	CheckShaderCompiled(shader_vert);

	GLuint shader_frag = f->glCreateShader(GL_FRAGMENT_SHADER);
	f->glShaderSource(shader_frag, 1, &conversion_config->shader_frag_glsl, nullptr);
	f->glCompileShader(shader_frag);
	CheckShaderCompiled(shader_frag);

	program = f->glCreateProgram();
	f->glAttachShader(program, shader_vert);
	f->glAttachShader(program, shader_frag);
	f->glBindAttribLocation(program, 0, "pos_attr");
	f->glLinkProgram(program);

	GLint linked = 0;
	f->glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if(linked != GL_TRUE)
	{
		GLint info_log_size = 0;
		f->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_size);
		QVector<GLchar> info_log(info_log_size);
		f->glGetProgramInfoLog(program, info_log_size, &info_log_size, info_log.data());
		f->glDeleteProgram(program);
		CHIAKI_LOGE(session->GetChiakiLog(), "Failed to Link Shader Program:\n%s", info_log.data());
		return;
	}

	//for(int i=0; i<2; i++) // two swap frames. Don't want.
	for(int i=0; i<1; i++)
	{
		frames[i].conversion_config = conversion_config;
		f->glGenTextures(conversion_config->planes, frames[i].tex);
		f->glGenBuffers(conversion_config->planes, frames[i].pbo);
		uint8_t uv_default[] = {0x7f, 0x7f};
		for(int j=0; j<conversion_config->planes; j++)
		{	
			f->glEnable(GL_TEXTURE_EXTERNAL_OES);
			//f->glBindTexture(GL_TEXTURE_EXTERNAL_OES, frames[i].tex[j]);
			f->glBindTexture(GL_TEXTURE_EXTERNAL_OES, frames[i].new_texture);
			//f->glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//f->glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			//f->glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			//f->glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			f->glTexImage2D(GL_TEXTURE_EXTERNAL_OES, 0, conversion_config->plane_configs[j].internal_format, 1, 1, 0, conversion_config->plane_configs[j].format, GL_UNSIGNED_BYTE, j > 0 ? uv_default : nullptr);

            // ORIG
			//~ f->glBindTexture(GL_TEXTURE_2D, frames[i].tex[j]);
			//~ f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//~ f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			//~ f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			//~ f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			//~ f->glTexImage2D(GL_TEXTURE_2D, 0, conversion_config->plane_configs[j].internal_format, 1, 1, 0, conversion_config->plane_configs[j].format, GL_UNSIGNED_BYTE, j > 0 ? uv_default : nullptr);
		}
		frames[i].width = 0;
		frames[i].height = 0;
	}

	f->glUseProgram(program);

	// bind only as many planes as we need
	const char *plane_names[] = {"plane1", "plane2", "plane3"};
	for(int i=0; i<sizeof(plane_names)/sizeof(char *); i++)
	{
		f->glUniform1i(f->glGetUniformLocation(program, plane_names[i]), i);
	}

	f->glGenVertexArrays(1, &vao);
	f->glBindVertexArray(vao);

	f->glGenBuffers(1, &vbo);
	f->glBindBuffer(GL_ARRAY_BUFFER, vbo);
	f->glBufferData(GL_ARRAY_BUFFER, sizeof(vert_pos), vert_pos, GL_STATIC_DRAW);

	f->glBindBuffer(GL_ARRAY_BUFFER, vbo);
	f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	f->glEnableVertexAttribArray(0);

	f->glCullFace(GL_BACK);
	f->glEnable(GL_CULL_FACE);
	f->glClearColor(0.0, 0.0, 0.0, 1.0);

	frame_uploader_context = new QOpenGLContext(nullptr);
	frame_uploader_context->setFormat(context()->format());
	frame_uploader_context->setShareContext(context());
	if(!frame_uploader_context->create())
	{
		CHIAKI_LOGE(session->GetChiakiLog(), "Failed to create upload OpenGL context");
		return;
	}

	frame_uploader_surface = new QOffscreenSurface();
	frame_uploader_surface->setFormat(context()->format());
	frame_uploader_surface->create();
	frame_uploader = new AVOpenGLFrameUploader(session, this, frame_uploader_context, frame_uploader_surface);
	frame_fg = 0;

	frame_uploader_thread = new QThread(this);
	frame_uploader_thread->setObjectName("Frame Uploader");
	frame_uploader_context->moveToThread(frame_uploader_thread);
	frame_uploader->moveToThread(frame_uploader_thread);
	frame_uploader_thread->start();
	
	printf("END AVOpenGLWidget::initializeGL\n");
}

void AVOpenGLWidget::paintGL()
{	
	//printf("\nPaint GL\n");
	auto f = QOpenGLContext::currentContext()->extraFunctions();

	f->glClear(GL_COLOR_BUFFER_BIT);

	int widget_width = (int)(width() * devicePixelRatioF());
	int widget_height = (int)(height() * devicePixelRatioF());

	QMutexLocker lock(&frames_mutex);
	AVOpenGLFrame *frame = &frames[frame_fg];

	GLsizei vp_width, vp_height;
	if(!frame->width || !frame->height)
	{
		vp_width = widget_width;
		vp_height = widget_height;
	}
	else
	{
		float aspect = (float)frame->width / (float)frame->height;
		if(aspect < (float)widget_width / (float)widget_height)
		{
			vp_height = widget_height;
			vp_width = (GLsizei)(vp_height * aspect);
		}
		else
		{
			vp_width = widget_width;
			vp_height = (GLsizei)(vp_width / aspect);
		}
	}

	f->glViewport((widget_width - vp_width) / 2, (widget_height - vp_height) / 2, vp_width, vp_height);

	f->glBindTexture(GL_TEXTURE_EXTERNAL_OES, frame->new_texture);

	//for(int i=0; i<3; i++)
	//~ for(int i=0; i<3; i++)
	//~ {
		//~ f->glActiveTexture(GL_TEXTURE0 + i);
		//~ //f->glBindTexture(GL_TEXTURE_EXTERNAL_OES, frame->tex[i]);
		//~ f->glBindTexture(GL_TEXTURE_EXTERNAL_OES, frame->new_texture);
	//~ }

	f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glDeleteTextures(1, &frame->new_texture);

	//f->glFinish();
	//printf("END GlPaint\n\n");
}
