#include "kahawaiBase.h"
#ifdef KAHAWAI

#include "OpenGLCapturer.h"


//OpenGL Imports
#if defined( _WIN32 )

#include <windows.h>
#include <gl/gl.h>
#include <stdint.h>

#elif defined( MACOS_X )

// magic flag to keep tiger gl.h from loading glext.h
#define GL_GLEXT_LEGACY
#include <OpenGL/gl.h>

#elif defined( __linux__ )

// using our local glext.h
// http://oss.sgi.com/projects/ogl-sample/ABI/
#define GL_GLEXT_LEGACY
#define GLX_GLXEXT_LEGACY
#include <GL/gl.h>
#include <GL/glx.h>

#else

#include <gl.h>

#endif


OpenGLCapturer::OpenGLCapturer(int width, int height)
	:_height(height),
	_width(width)
{
	_buffer = new uint8_t[width*height*SOURCE_BITS_PER_PIXEL];
}


OpenGLCapturer::~OpenGLCapturer(void)
{
	delete[] _buffer;
}

uint8_t* OpenGLCapturer::CaptureScreen()
{
	glReadBuffer( GL_FRONT );
	glReadPixels( 0, 0, _width, _height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, _buffer ); 
	return _buffer;
}
#endif