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

#include "shader_utils.hpp"

#include <iostream>
#include <fstream>

#include <assimp/texture.h>
#include <SDL_image.h>
#include <GL/glew.h>

using namespace std;

namespace ShaderUtils {

  namespace {

    GLenum GetShaderType(const string& ext)
    {
      GLenum shaderType = 0;

      if (ext == "vert") { shaderType = GL_VERTEX_SHADER; }
      else if (ext == "frag") { shaderType = GL_FRAGMENT_SHADER; }
      else if (ext == "geom") { shaderType = GL_GEOMETRY_SHADER; }
      else if (ext == "tesc") { shaderType = GL_TESS_CONTROL_SHADER; }
      else if (ext == "tese") { shaderType = GL_TESS_EVALUATION_SHADER; }
      else if (ext == "comp") { shaderType = GL_COMPUTE_SHADER; }
      else { cout << "Wrong shader extension: " << ext << endl; }
      return shaderType;
    }

    void printProgramLog(GLuint program)
    {
      // Make sure name is shader
      if (!glIsProgram(program))
      {
        return;
      }
      // Program log length
      int infoLogLength = 0;
      int maxLength = infoLogLength;

      // Get info string length
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

      //Allocate string
      char* infoLog = new char[maxLength];

      // Get info log
      glGetProgramInfoLog(program, maxLength, &infoLogLength, infoLog);
      if (infoLogLength > 0)
      {
        // Print Log
        cout << infoLog << endl;
      }

      // Deallocate string
      delete[] infoLog;
    }

    void printShaderLog(GLuint shader)
    {
      // Make sure name is shader
      if (glIsShader(shader))
      {
        return;
      }

      // Shader log length
      int infoLogLength = 0;
      int maxLength = infoLogLength;

      // Get info string length
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

      // Allocate string
      char* infoLog = new char[maxLength];

      // Get info log
      glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);
      if (infoLogLength > 0)
      {
        // Print Log
        cout << infoLog << endl;
      }

      // Deallocate string
      delete[] infoLog;
    }
  }


  uint32_t LoadShader(const string& shaderPath, const std::string& shaderDefines)
  {
    // Get shader type
    string ext = shaderPath.substr(shaderPath.rfind(".") + 1);
    GLenum shaderType = GetShaderType(ext);
    if (shaderType == 0)
    {
      return 0;
    }

    // Open file
    GLuint shaderID = 0;
    std::string shaderString;
    std::ifstream sourceFile(shaderPath);
    if (!sourceFile)
    {
      cout << "Unable to open file " << shaderPath << endl;
      return 0;
    }

    // Get shader source
    shaderString.assign((std::istreambuf_iterator< char >(sourceFile)), std::istreambuf_iterator< char >());

    // Insert the shaderDefines after the #version
    auto pos = shaderString.find( "#version");
    if (pos != string::npos)
    {
      pos = shaderString.find('\n', pos + 8) + 1;
      if (pos != string::npos)
      {
        shaderString.insert(pos, shaderDefines);
      }
    }
    
    // Create shader ID
    shaderID = glCreateShader(shaderType);

    // Set shader source
    const GLchar* shaderSource = shaderString.c_str();
    glShaderSource(shaderID, 1, (const GLchar**)&shaderSource, NULL);

    // Compile shader source
    glCompileShader(shaderID);

    // Check shader for errors
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &shaderCompiled);
    if (shaderCompiled != GL_TRUE)
    {
      cout << "Unable to compile shader " << shaderPath << endl;
      printShaderLog(shaderID);
      glDeleteShader(shaderID);
      return 0;
    }

    return shaderID;
  }

  uint32_t LoadProgram(const std::vector<std::string>& shaderPaths, const std::string& definesPath)
  {
    // Open file
    std::string shaderDefines;
    std::ifstream sourceFile(definesPath);
    if (sourceFile)
    {
      // Get the shader defines
      shaderDefines.assign((std::istreambuf_iterator< char >(sourceFile)), std::istreambuf_iterator< char >());
    }

    // Generate program
    GLuint programId = glCreateProgram();

    // Load all shaders
    vector<uint32_t> shaderIds;
    for (const string& p : shaderPaths)
    {
      // Load shader
      uint32_t shaderId = LoadShader(p, shaderDefines);
      shaderIds.push_back(shaderId);

      // Check for errors
      if (shaderId == 0)
      {
        for (auto id : shaderIds) { glDeleteShader(id); }
        glDeleteProgram(programId);
        return 0;
      }

      // Attach shader to program
      glAttachShader(programId, shaderId);
    }

    // Link program
    glLinkProgram(programId);

    // Check for errors
    GLint programSuccess = GL_TRUE;
    glGetProgramiv(programId, GL_LINK_STATUS, &programSuccess);
    if (programSuccess != GL_TRUE)
    {
      cout << "Error linking program " << programId << endl;
      printProgramLog(programId);
      for (auto id : shaderIds) { glDeleteShader(id); }
      glDeleteProgram(programId);
      return false;
    }

    // Clean up excess shader references
    for (auto id : shaderIds) { glDeleteShader(id); }

    return programId;
  }

  uint32_t LoadTexture(SDL_Surface* surface)
  {
    if (surface == NULL) {
      return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

    int modeTbl[2][2] = { {GL_RGB , GL_BGR}, { GL_RGBA, GL_BGRA } };
    const SDL_PixelFormat* f = surface->format;
    int modeChannels = (f->BytesPerPixel == 3 ? 0 : 1);
    int modeOrdering = ((f->Rshift < f->Gshift) && (f->Gshift < f->Bshift) ? 0 : 1);

    int format = modeTbl[modeChannels][modeOrdering];
    int internalFormat = modeTbl[modeChannels][0];

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateTextureMipmap(texture);

    SDL_FreeSurface(surface);

    return texture;
  }

  GLuint LoadTexture(const std::string& filePath, uint32_t& bpp)
  {
    SDL_Surface* surface = IMG_Load(filePath.c_str());

    if (surface == NULL) 
    {
      cout << "Couldn't load texture " << filePath << endl;
      cout << SDL_GetError() << endl;
      return 0;
    }
    bpp = surface->format->BytesPerPixel;
    return LoadTexture(surface);
  }

  uint32_t LoadCubeMapTexture(
    const std::string& cubeMapRight,
    const std::string& cubeMapLeft,
    const std::string& cubeMapUp,
    const std::string& cubeMapDown,
    const std::string& cubeMapBack,
    const std::string& cubeMapFront)
  {
    vector<std::string> cubeMapFacesFileNames = {
      cubeMapRight, cubeMapLeft, cubeMapUp, cubeMapDown, cubeMapBack, cubeMapFront
    };

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    for (uint32_t i = 0;i < 6; ++i)
    {
      SDL_Surface* surface = IMG_Load(cubeMapFacesFileNames[i].c_str());

      if (surface == NULL) 
      {
        glDeleteTextures(1, &texture);
        cout << "Couldn't load texture " << cubeMapFacesFileNames[i] << endl;
        cout << SDL_GetError() << endl;
        return 0;
      }

      int modeTbl[2][2] = { { GL_RGB , GL_BGR },{ GL_RGBA, GL_BGRA } };
      const SDL_PixelFormat* f = surface->format;
      int modeChannels = (f->BytesPerPixel == 3 ? 0 : 1);
      int modeOrdering = ((f->Rshift < f->Gshift) && (f->Gshift < f->Bshift) ? 0 : 1);

      int format = modeTbl[modeChannels][modeOrdering];
      int internalFormat = modeTbl[modeChannels][0];

      glTexImage2D(
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
        0, internalFormat, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);

      SDL_FreeSurface(surface);
    }

    return texture;
  }

  uint32_t LoadTexture(const aiTexture* tex) 
  {
    SDL_Surface* surface = NULL;

    if (tex->mHeight == 0) 
    {
      surface = IMG_Load_RW(SDL_RWFromConstMem((void *)tex->pcData, tex->mWidth), 0);
    }

    return LoadTexture(surface);
  }

  uint32_t LoadTexture(const void* texData, uint32_t texWidth) 
  {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texWidth, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
    glGenerateTextureMipmap(texture);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
  }
}
