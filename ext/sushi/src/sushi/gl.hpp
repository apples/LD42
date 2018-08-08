//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_GL_HPP
#define SUSHI_GL_HPP

#ifdef __EMSCRIPTEN__
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>
#include "gles_shim.hpp"
#else
#include <glad/glad.h>
#endif

#include <glm/glm.hpp>

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#endif //SUSHI_GL_HPP
