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

#include "sys_renderer.hpp"
#include "resources.hpp"
#include "shader_defines.h"
#include "scene.hpp"
#include "camera_utils.hpp"
#include "sys_animation.hpp"

#include <iostream>
#include <sstream>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <SDL2/SDL.h>

using namespace std;
using namespace glm;

namespace shooter {

  SysRenderer::SysRenderer()
    : mMVPUniBuf(0)
    , mLightUniBuf(0)
    , mBonesUniBuf(0)
    , mBulletVao(0)
    , mSkyBoxVao(0)
  {
    glGenBuffers(1, &mMVPUniBuf);
    glBindBuffer(GL_UNIFORM_BUFFER, mMVPUniBuf);
    glBufferData(GL_UNIFORM_BUFFER, cMatricesUniBufferSize, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    mUniBufs.push_back(mMVPUniBuf);

    Light light = { vec4(.6f, .3f, 0.f, 1.f), vec4(1.f, .5f, 0.f, 1.f), vec4(0.f, 1.f, 0.f, 1.f) };
    glGenBuffers(1, &mLightUniBuf);
    glBindBuffer(GL_UNIFORM_BUFFER, mLightUniBuf);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Light), (void *)(&light), GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    mUniBufs.push_back(mLightUniBuf);

    vector<mat4> skeletonBones(MAX_BONES, mat4());
    glGenBuffers(1, &mBonesUniBuf);
    glBindBuffer(GL_UNIFORM_BUFFER, mBonesUniBuf);
    glBufferData(GL_UNIFORM_BUFFER, MAX_BONES * sizeof(mat4), &skeletonBones[0], GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    mUniBufs.push_back(mBonesUniBuf);

    // Generate Bullet Vertex Array Object
    float quadVertices[] = {
      -1.f, -.05f, 0.f, 0.f, 1.f,
      -1.f,   1.f, 0.f, 0.f, 0.f,
       1.f,   1.f, 0.f, 1.f, 0.f,
       1.f, -.05f, 0.f, 1.f, 1.f,
    };

    uint32_t quadIndices[] = {
      0, 1, 2, 2, 3, 0
    };

    uint32_t uniBuf = 0;

    glGenVertexArrays(1, &mBulletVao);
    glBindVertexArray(mBulletVao);

    glGenBuffers(1, &uniBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uniBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
    mUniBufs.push_back(uniBuf);

    glGenBuffers(1, &uniBuf);
    glBindBuffer(GL_ARRAY_BUFFER, uniBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    mUniBufs.push_back(uniBuf);

    glEnableVertexAttribArray(VERT_POSITION_LOC);
    glVertexAttribPointer(VERT_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);

    glEnableVertexAttribArray(VERT_DIFFUSE_TEX_COORD_LOC);
    glVertexAttribPointer(VERT_DIFFUSE_TEX_COORD_LOC, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

    float skyBoxVertices[] = {
       -1.f, -1.f, 1.f,
        1.f, -1.f, 1.f,
        1.f,  1.f, 1.f,
       -1.f,  1.f, 1.f, 
    };

    uint32_t skyBoxIndices[] = {
      0, 1, 2, 2, 3, 0
    };

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &mSkyBoxVao);
    glBindVertexArray(mSkyBoxVao);

    glGenBuffers(1, &uniBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uniBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyBoxIndices), skyBoxIndices, GL_STATIC_DRAW);
    mUniBufs.push_back(uniBuf);

    glGenBuffers(1, &uniBuf);
    glBindBuffer(GL_ARRAY_BUFFER, uniBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyBoxVertices), skyBoxVertices, GL_STATIC_DRAW);
    mUniBufs.push_back(uniBuf);

    glEnableVertexAttribArray(VERT_POSITION_LOC);
    glVertexAttribPointer(VERT_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  SysRenderer::~SysRenderer()
  {
    glDeleteVertexArrays(1, &mBulletVao);
    glDeleteVertexArrays(1, &mSkyBoxVao);
    glDeleteBuffers(mUniBufs.size(), mUniBufs.data());
  }

  void SysRenderer::DebugRenderSkeleton(const mat4& modelView, const mat4& proj, const Model& model, const mat4* bones, uint32_t nrBones)
  {
    unsigned colRed = ((unsigned int)255) | ((unsigned int)0 << 8) | ((unsigned int)0 << 16) | ((unsigned int)255 << 24);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(value_ptr(modelView));

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(value_ptr(proj));

    glColor4ubv((GLubyte*)&colRed);
    
    glLineWidth(1.f);
    glBegin(GL_LINES);
    
    for (unsigned nodeIndex = 1; nodeIndex < nrBones; ++nodeIndex)
    {
      int16_t parentIndex = model.nodesParents[nodeIndex];
      vec4 v1 = bones[nodeIndex] * model.invBonesOffsets[nodeIndex][3];
      vec4 v2 = bones[parentIndex] * model.invBonesOffsets[parentIndex][3];

      glVertex3f(v1.x, v1.y, v1.z);
      glVertex3f(v2.x, v2.y, v2.z);
    }

    glEnd();
  }

  void SysRenderer::DebugRenderNavMesh(const mat4& view, const mat4& proj, const NavMesh& navMesh)
  {
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(value_ptr(view));

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(value_ptr(proj));

    navMesh.DebugRender();
  }

  void SysRenderer::DebugRenderDamagebleSkeleton(
    const mat4& view, 
    const mat4& proj, 
    const Model& model,
    const CompTransform& trans,
    const CompAnimation& anim, 
    const CompDamagebleSkeleton& damSkeleton)
  {
    unsigned colGreen = ((unsigned int)0) | ((unsigned int)255 << 8) | ((unsigned int)0 << 16) | ((unsigned int)255 << 24);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(value_ptr(proj));

    glMatrixMode(GL_MODELVIEW);

    mat4 modelMat = CalcTransMat(trans);

    GLUquadric* gObj = gluNewQuadric();
    gluQuadricDrawStyle(gObj, GLU_LINE);
    glColor4ubv((GLubyte*)&colGreen);

    for (const CompDamagebleBone& damBone : damSkeleton.skeleton)
    {
      vec3 cylA(modelMat * anim.globalTrans[damBone.boneIx1] * model.invBonesOffsets[damBone.boneIx1][3]);
      vec3 cylB(modelMat * anim.globalTrans[damBone.boneIx2] * model.invBonesOffsets[damBone.boneIx2][3]);
      
      const glm::vec3 worldUp(0.f, 1.f, 1.f);
      float cylH = distance(cylA, cylB);
      vec3 front = (cylB - cylA) / cylH; // normalize
      vec3 right = normalize(cross(front, worldUp));
      mat4 cylTrans = glm::mat4(glm::mat3(right, cross(right, front), front)); // rotation
      cylTrans[3] = vec4(cylA, 1.f); // translation

      glLoadMatrixf(value_ptr(view * cylTrans)); // update the GL_MODELVIEW matrix

      gluCylinder(gObj, damBone.radius, damBone.radius, cylH, 10, 10);
    }

    gluDeleteQuadric(gObj);
  }

  void SysRenderer::RenderBullets(
    uint32_t glProgram, 
    uint32_t glVao,
    const vec3& cameraPos,
    const CompBullet* bullets,
    uint32_t bulletNr
  )
  {
    float time = SDL_GetTicks() * 0.001f;

    glUseProgram(glProgram);
    glBindVertexArray(glVao);
    glUniform3fv(CAMERA_POS_LOC, 1, value_ptr(cameraPos));
    glUniform1i(BILLBOARD_IN_WORLD_SPACE, GL_FALSE);
    glUniform1f(BILLBOARD_WIDTH, .03f);

    for (uint32_t i = 0; i < bulletNr; i++)
    {
      const CompBullet& bullet = bullets[i];
      vec3 up = bullet.endPos - bullet.startPos;
      float len = length(up);
      up /= len; // Normalize

      glUniform1f(GLOBAL_TIME_LOC, time);
      glUniform3fv(BILLBOARD_ORIGIN_LOC, 1, value_ptr(bullet.startPos));
      glUniform3fv(BILLBOARD_ROTATION_AXIS, 1, value_ptr(up));
      glUniform1f(BILLBOARD_HEIGHT, len);

      // Draw
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
  }

  void SysRenderer::RenderEntities(
    const mat4& viewMat,
    const mat4& projMat,
    uint32_t glMVPUniBuf,
    uint32_t glBonesUniBuf,
    const Resources& resources,
    const CompTransform* transforms,
    const CompRenderable* renderables, 
    const CompAnimation* animations, 
    const CompState* states,
    uint32_t nrModels)
  {
    glBindBufferRange(GL_UNIFORM_BUFFER, MATRICES_BINDING, glMVPUniBuf, 0, cMatricesUniBufferSize);
    glBufferSubData(GL_UNIFORM_BUFFER, cProjMatrixOffset, cMatrixSize, value_ptr(projMat));
    glBufferSubData(GL_UNIFORM_BUFFER, cViewMatrixOffset, cMatrixSize, value_ptr(viewMat));

    glUseProgram(resources.GetProgram("simple"));

    glActiveTexture(GL_TEXTURE0 + DIFFUSE_TEX_UNIT);

    for (uint32_t i = 0; i < nrModels; i++)
    {
      const Model& model = resources.GetModel(renderables[i].modelName);
      if (model.meshes.empty())
      {
        continue;
      }

      const uint32_t& state = states[i].state;
      if (state & EStateDead)
      {
        continue;
      }

      mat4 modelMat = CalcTransMat(transforms[i]);
      mat4 modelViewMat = viewMat * modelMat;

      FrustumPlanes planes = CalcFrustumPlanes(projMat * modelViewMat);
      vec3 halfSize = (model.maxBound - model.minBound) * 0.5f;
      if (!IsBoxInFrustum(model.minBound + halfSize, halfSize, planes))
      {
        continue;
      }

      mat4 normalMatrix = inverseTranspose(modelViewMat);

      // upload the model, view and projection matrices to the GPU
      glBindBufferRange(GL_UNIFORM_BUFFER, MATRICES_BINDING, glMVPUniBuf, 0, cMatricesUniBufferSize);
      glBufferSubData(GL_UNIFORM_BUFFER, cModelMatrixOffset, cMatrixSize, value_ptr(modelMat));
      glBufferSubData(GL_UNIFORM_BUFFER, cNormalMatrixOffset, cMatrixSize, value_ptr(normalMatrix));

      // upload the bones global transformations
      auto &globalTrans = animations[i].globalTrans;
      glBindBufferRange(GL_UNIFORM_BUFFER, BONES_BINDING, glBonesUniBuf, 0, globalTrans.size() * sizeof(mat4));
      glBufferSubData(GL_UNIFORM_BUFFER, 0, globalTrans.size() * sizeof(mat4), globalTrans.data());

      for (const Mesh& mesh : model.meshes)
      {
        // bind material uniform
        glBindBufferRange(GL_UNIFORM_BUFFER, MATERIAL_BINDING, model.materialsCol[mesh.materialIndex], 0, sizeof(MaterialColors));

        const MaterialTextures& matTex = model.materialsTex[mesh.materialIndex];

        if (!matTex.empty())
          glBindTexture(GL_TEXTURE_2D, matTex.front().second);
        else
          glBindTexture(GL_TEXTURE_2D, 0);

        // bind VAO
        glBindVertexArray(mesh.vao);

        // draw
        glDrawElements(GL_TRIANGLES, mesh.numFaces * 3, GL_UNSIGNED_INT, 0);
      }
    }

    const mat4 mat4Identity(1.f);
    glBindBufferRange(GL_UNIFORM_BUFFER, MATRICES_BINDING, glMVPUniBuf, 0, cMatricesUniBufferSize);
    glBufferSubData(GL_UNIFORM_BUFFER, cModelMatrixOffset, cMatrixSize, value_ptr(mat4Identity));
    glBufferSubData(GL_UNIFORM_BUFFER, cNormalMatrixOffset, cMatrixSize, value_ptr(mat4Identity));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glUseProgram(0);
  }

  void SysRenderer::DebugRenderModels(
    const mat4& viewMat, 
    const mat4& projMat,
    const Resources& resources,
    const CompTransform* transforms,
    const CompRenderable* renderables,
    const CompAnimation* animations,
    const CompDamagebleSkeleton* damSkeletons,
    uint32_t nrModels)
  {
    for (uint32_t i = 0; i < nrModels; i++)
    {
      const Model& model = resources.GetModel(renderables[i].modelName);
      if (model.meshes.empty())
      {
        continue;
      }

      mat4 modelMat = CalcTransMat(transforms[i]);
      mat4 modelViewMat = viewMat * modelMat;

      FrustumPlanes planes = CalcFrustumPlanes(projMat * modelViewMat);
      vec3 halfSize = (model.maxBound - model.minBound) * 0.5f;
      if (!IsBoxInFrustum(model.minBound + halfSize, halfSize, planes))
      {
        continue;
      }

      auto &globalTrans = animations[i].globalTrans;

      DebugRenderSkeleton(modelViewMat, projMat, model, globalTrans.data(), globalTrans.size());

      DebugRenderDamagebleSkeleton(
        viewMat, projMat,
        model,
        transforms[i],
        animations[i],
        damSkeletons[i]);
    }
  }

  void SysRenderer::RenderSkyBox(
    const mat4& viewMat,
    const mat4& projMat,
    uint32_t glMVPUniBuf,
    uint32_t glVao,
    uint32_t glProgram,
    uint32_t glTexture)
  {
    vec4 unproj = inverse(projMat) * vec4(1.f, 1.f, 1.f, 1.f);
    vec3 unproj2 = vec3(vec3(unproj) / unproj.w);

    // Remove the translation because we are always in the center of the skybox
    mat4 viewMatNoTrans(viewMat);
    viewMatNoTrans[3] = vec4(0.f, 0.f, 0.f, 1.f);

    glBindBufferRange(GL_UNIFORM_BUFFER, MATRICES_BINDING, glMVPUniBuf, 0, cMatricesUniBufferSize);
    glBufferSubData(GL_UNIFORM_BUFFER, cProjMatrixOffset, cMatrixSize, value_ptr(projMat));
    glBufferSubData(GL_UNIFORM_BUFFER, cViewMatrixOffset, cMatrixSize, value_ptr(viewMatNoTrans));

    glUseProgram(glProgram);

    glActiveTexture(GL_TEXTURE0 + DIFFUSE_TEX_UNIT);
    glBindTexture(GL_TEXTURE_CUBE_MAP, glTexture);

    glBindVertexArray(glVao);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }

  void SysRenderer::Render(const Resources& resources, const Scene& scene) const
  {
    const mat4 mat4Identity(1.f);
    mat4 projMat = CalcProjMat(scene.camera.frustum);
    mat4 viewMat = CalcViewMat(scene.camera.trans);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    
    glFrontFace(GL_CW);
    RenderEntities(viewMat, projMat, mMVPUniBuf, mBonesUniBuf, resources,
      scene.transforms.data(), scene.renderables.data(), scene.animations.data(), 
      scene.states.data(), scene.transforms.size());

    glFrontFace(GL_CCW);
    resources.GetMap().Render(resources, viewMat, projMat, scene.camera.trans.position, mMVPUniBuf);

    RenderBullets(
      resources.GetProgram("flame"),
      mBulletVao,
      scene.camera.trans.position,
      scene.bullets.data(),
      scene.nrValidBullets);

    RenderSkyBox(viewMat, projMat, mMVPUniBuf, mSkyBoxVao,
      resources.GetProgram("skyBox"), resources.GetSkyBoxTexture());

    if (scene.debugging)
    {
      glUseProgram(0);

      DebugRenderModels(
        viewMat, projMat, 
        resources,
        scene.transforms.data(), 
        scene.renderables.data(), 
        scene.animations.data(), 
        scene.damagebles.data(), 
        scene.transforms.size());

      DebugRenderNavMesh(viewMat, projMat, resources.GetNavMesh());
    }
  }
}
