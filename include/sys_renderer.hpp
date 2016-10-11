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

#ifndef SYS_RENDERER_HPP
#define SYS_RENDERER_HPP

#include <vector>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace shooter {

  class Resources;
  class NavMesh;
  struct Scene;
  struct Model;
  struct CompTransform;
  struct CompAnimation;
  struct CompRenderable;
  struct CompState;
  struct CompDamagebleSkeleton;
  struct CompBullet;

  /// Render system. Renders the scene and all debug information.
  class SysRenderer 
  {
  public:

      /// Initialize different OpenGL buffers needed for rendering
      SysRenderer();

      /// Clean up
      ~SysRenderer();

      /// Render the scene. Called from the game loop.
      void Render(const Resources& resources, const Scene& scene) const;

  private:
  
    /// Render entities
    static void RenderEntities(
      const glm::mat4& viewMat, 
      const glm::mat4& projMat,
      uint32_t glMVPUniBuf, 
      uint32_t glBonesUniBuf,
      const Resources& resources,
      const CompTransform* transforms,
      const CompRenderable* renderables,
      const CompAnimation* animations,
      const CompState* states,
      uint32_t nrModels);

    /// Render bullets
    static void RenderBullets(
      uint32_t glProgram,
      uint32_t glVao,
      const glm::vec3& cameraPos,
      const CompBullet* bullets,
      uint32_t nrBullets);
  
    /// Render skybox
    static void RenderSkyBox(
      const glm::mat4& viewMat,
      const glm::mat4& projMat,
      uint32_t glMVPUniBuf,
      uint32_t glVao,
      uint32_t glProgram,
      uint32_t glTexture);

    /// Render model debug info
    static void DebugRenderModels(
      const glm::mat4& viewMat, 
      const glm::mat4& projMat,
      const Resources& resources,
      const CompTransform* transforms,
      const CompRenderable* renderables,
      const CompAnimation* animations,
      const CompDamagebleSkeleton* damagebles,
      uint32_t nrModels);

    /// Render NavMesh debug info
    static void DebugRenderNavMesh(
      const glm::mat4& view, 
      const glm::mat4& proj, 
      const NavMesh& navMesh
    );

    /// Render Skeleton debug info
    static void DebugRenderSkeleton(
      const glm::mat4& modelView, 
      const glm::mat4& proj, 
      const Model& model, 
      const glm::mat4* bones, 
      uint32_t nrBones
    );

    /// Render Damageble Skeleton debug info
    static void DebugRenderDamagebleSkeleton(
      const glm::mat4& view, 
      const glm::mat4& proj, 
      const Model& model, 
      const CompTransform& trans, 
      const CompAnimation& anim, 
      const CompDamagebleSkeleton& damSkeleton);

    uint32_t mMVPUniBuf; ///< Model-View-Projection matrices OpenGL uniform buffer
    uint32_t mLightUniBuf; ///< Light OpenGL uniform buffer
    uint32_t mBonesUniBuf; ///< Bones OpenGL uniform buffer
    uint32_t mBulletVao; ///< Bullet's OpenGL vertex array object
    uint32_t mSkyBoxVao; ///< Skybox's OpenGL vertex array object

    std::vector<uint32_t> mUniBufs; ///< All the OpenGL uniform buffers (used for easy deallocation)
  };

}

#endif // SYS_RENDERER_HPP
