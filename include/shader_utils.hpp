//
// Copyright (c) 2016 Iulian Marinescu Ghetau giulian2003@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely.  If you use this software in a product, an acknowledgment in the 
// product documentation would be appreciated but is not required.
//

#ifndef SHADERS_UTILS_HPP
#define SHADERS_UTILS_HPP

#include <cstdint>
#include <string>
#include <vector>

struct aiTexture;
struct SDL_Surface;

namespace ShaderUtils {

  /// Load a shader from a file
  /// @return OpenGL shader object or 0 if error 
  uint32_t LoadShader(const std::string& shaderPath, const std::string& definesPath);
  
  /// Load a Program containing vertex, fragment, geometry, etc. shaders as files
  /// @return OpenGL program object or 0 if error 
  uint32_t LoadProgram(const std::vector<std::string>& shaderPaths, const std::string& definesPath);

  /// Load Texture from a file
  /// @return OpenGL texture object or 0 if error 
  uint32_t LoadTexture(const std::string& filePath, uint32_t& outBpp);
  
  /// Load embeded texture
  /// @return OpenGL texture object or 0 if error 
  uint32_t LoadTexture(const aiTexture* filePath);

  /// Load texture from data buffer
  /// @return OpenGL texture object or 0 if error 
  uint32_t LoadTexture(const void* texData, uint32_t texWidth);

  /// Load texture from SDL_Surface
  /// @return OpenGL texture object or 0 if error 
  uint32_t LoadTexture(SDL_Surface* surface);
  
  /// Load cube map texture
  /// @return OpenGL texture object or 0 if error 
  uint32_t LoadCubeMapTexture(
    const std::string& cubeMapRight,
    const std::string& cubeMapLeft,
    const std::string& cubeMapUp,
    const std::string& cubeMapDown,
    const std::string& cubeMapBack,
    const std::string& cubeMapFront);
}

#endif //SHADERS_UTILS_HPP