#pragma once

#include <iostream>
#include <GLFW/glfw3.h>

#define _DEBUG_ON 1

#if(_DEBUG_ON == 1)

#define CHECK_ERROR_GL_W_MSG(msg) {\
  GLenum _glErr = glGetError(); \
  if (_glErr != GL_NO_ERROR) { \
    std::cerr << "GL error " << _glErr << " " << __FILE__ << ":" << __LINE__ << std::endl << msg; \
    std::exit(-2); \
  }}

#define CHECK_ERROR_GL() CHECK_ERROR_GL_W_MSG("")

#define DEBUG_STDOUT(msg) std::cout << msg;
#define DEBUG_STDERR(msg) std::cerr << msg;

#else

#define CHECK_ERROR_GL_W_MSG(msg)
#define CHECK_ERROR_GL()

#define DEBUG_OUT(msg)
#define DEBUG_ERR(msg)

#endif
