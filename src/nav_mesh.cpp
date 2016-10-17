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

#include "nav_mesh.hpp"

#include <unordered_set>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/intersect.hpp>

#include <Recast.h>
#include <DetourCommon.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <DebugDraw.h>
#include <RecastDebugDraw.h>
#include <RecastDump.h>
#include <DetourDebugDraw.h>

#include <GL/glew.h>

using namespace shooter;

namespace {
  
  inline bool inRange(const float* v1, const float* v2, const float r, const float h)
  {
    const float dx = v2[0] - v1[0];
    const float dy = v2[1] - v1[1];
    const float dz = v2[2] - v1[2];
    return (dx*dx + dz*dz) < r*r && fabsf(dy) < h;
  }

  inline void VelocityVerlet(float dt, float* pos, float* vel)
  {
    static const float acc[3] = { 0.f, -10.f, 0.f };

    // pos += dt * (vel + .5f * dt * acc);
    // vel += dt * acc;
    rcVmad(pos, pos, vel, dt);
    rcVmad(pos, pos, acc, .5f * dt * dt);
    rcVmad(vel, vel, acc, dt);
  }

  // Copied from RecastLayers.cpp to avoid dependencies
  inline bool overlapRange(const float amin, const float amax, const float bmin, const float bmax)
  {
    return (amin > bmax || amax < bmin) ? false : true;
  }

  // Copied from RecastDebugDraw.cpp to avoid dependencies
  void getContourCenter(const rcContour* cont, const float* orig, float cs, float ch, float* center)
  {
    center[0] = 0;
    center[1] = 0;
    center[2] = 0;
    if (!cont->nverts)
      return;
    for (int i = 0; i < cont->nverts; ++i)
    {
      const int* v = &cont->verts[i * 4];
      center[0] += (float)v[0];
      center[1] += (float)v[1];
      center[2] += (float)v[2];
    }
    const float s = 1.0f / cont->nverts;
    center[0] *= s * cs;
    center[1] *= s * ch;
    center[2] *= s * cs;
    center[0] += orig[0];
    center[1] += orig[1] + 4 * ch;
    center[2] += orig[2];
  }

  // Copied from RecastDebugDraw.cpp to avoid dependencies
  const rcContour* findContourFromSet(const rcContourSet& cset, unsigned short reg)
  {
    for (int i = 0; i < cset.nconts; ++i)
    {
      if (cset.conts[i].reg == reg)
        return &cset.conts[i];
    }
    return 0;
  }

  /// OpenGL debug draw implementation.
  class DebugDrawGL : public duDebugDraw
  {
  public:
    virtual void depthMask(bool state) { glDepthMask(state ? GL_TRUE : GL_FALSE); }

    virtual void texture(bool state)
    {
      if (state)
      {
        glEnable(GL_TEXTURE_2D);
      }
      else
      {
        glDisable(GL_TEXTURE_2D);
      }
    }

    virtual void begin(duDebugDrawPrimitives prim, float size = 1.0f)
    {
      switch (prim)
      {
      case DU_DRAW_POINTS:
        glPointSize(size);
        glBegin(GL_POINTS);
        break;
      case DU_DRAW_LINES:
        glLineWidth(size);
        glBegin(GL_LINES);
        break;
      case DU_DRAW_LINE_STRIP:
        glLineWidth(size);
        glBegin(GL_LINE_STRIP);
        break;
      case DU_DRAW_TRIS:
        glBegin(GL_TRIANGLES);
        break;
      case DU_DRAW_QUADS:
        glBegin(GL_QUADS);
        break;
      };
    }

    virtual void vertex(const float* pos, unsigned int color)
    {
      glColor4ubv((GLubyte*)&color);
      glVertex3fv(pos);
    }

    virtual void vertex(const float x, const float y, const float z, unsigned int color)
    {
      glColor4ubv((GLubyte*)&color);
      glVertex3f(x, y, z);
    }

    virtual void vertex(const float* pos, unsigned int color, const float* uv)
    {
      glColor4ubv((GLubyte*)&color);
      glTexCoord2fv(uv);
      glVertex3fv(pos);
    }

    virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
    {
      glColor4ubv((GLubyte*)&color);
      glTexCoord2f(u, v);
      glVertex3f(x, y, z);
    }

    virtual void end()
    {
      glEnd();
      glLineWidth(1.0f);
      glPointSize(1.0f);
    }
  };

}


// based on Sample_SoloMesh::handleBuild()
NavMesh::NavMesh(
  float agentHeight, 
  float agentRadius,
  float agentMaxClimb, 
  float agentWalkableSlopeAngle,
  float cellSize, 
  float cellHeight,
  float maxEdgeLen, 
  float maxEdgeError,
  float regionMinSize, 
  float regionMergeSize,
  float detailSampleDist, 
  float detailSampleMaxError,
  const float* verts, 
  const float* normals,
  const int* tris, 
  int ntris,
  const float* minBound, 
  const float* maxBound,
  float maxJumpGroundRange,
  float maxJumpDistance,
  float initialJumpForwardSpeed,
  float initialJumpUpSpeed,
  float idealJumpPointsDist,
  float maxIntersectionPosHeight
)
  : m_keepInterResults(true)
  , m_totalBuildTimeMs(0)
  , m_triareas(nullptr)
  , m_hf(nullptr)
  , m_chf(nullptr)
  , m_cset(nullptr)
  , m_pmesh(nullptr)
  , m_cfg(nullptr)
  , m_dmesh(nullptr)
  , m_navMesh(nullptr)
  , m_navQuery(nullptr)
  , m_filter(nullptr)
{
  //
  // Step 1. Initialize build config.
  //

  int nverts = ntris * 3;

  m_cfg = new rcConfig();
  m_cfg->cs = cellSize;
  m_cfg->ch = cellHeight;
  m_cfg->walkableSlopeAngle = agentWalkableSlopeAngle;
  m_cfg->walkableHeight = (int)ceil(agentHeight / cellHeight);
  m_cfg->walkableClimb = (int)floor(agentMaxClimb / cellHeight);
  m_cfg->walkableRadius = (int)ceil(agentRadius / cellSize);
  m_cfg->maxEdgeLen = (int)(maxEdgeLen / cellSize);
  m_cfg->maxSimplificationError = maxEdgeError;
  m_cfg->minRegionArea = (int)rcSqr(regionMinSize);    // Note: area = size*size
  m_cfg->mergeRegionArea = (int)rcSqr(regionMergeSize);  // Note: area = size*size
  m_cfg->maxVertsPerPoly = DT_VERTS_PER_POLYGON;
  m_cfg->detailSampleDist = detailSampleDist < 0.9f ? 0 : cellSize * detailSampleDist;
  m_cfg->detailSampleMaxError = cellHeight * detailSampleMaxError;


  // Set the area where the navigation will be build.
  // Here the bounds of the input mesh are used, but the
  // area could be specified by an user defined box, etc.
  rcVcopy(m_cfg->bmin, minBound);
  rcVcopy(m_cfg->bmax, maxBound);
  rcCalcGridSize(m_cfg->bmin, m_cfg->bmax, m_cfg->cs, &m_cfg->width, &m_cfg->height);

  rcContext *m_ctx = new rcContext;

  // Reset build times gathering.
  m_ctx->resetTimers();

  // Start the build process.  
  m_ctx->startTimer(RC_TIMER_TOTAL);

  m_ctx->log(RC_LOG_PROGRESS, "Building navigation:");
  m_ctx->log(RC_LOG_PROGRESS, " - %d x %d cells", m_cfg->width, m_cfg->height);
  m_ctx->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", nverts / 1000.0f, ntris / 1000.0f);

  //
  // Step 2. Rasterize input polygon soup.
  //

  // Allocate voxel heightfield where we rasterize our input data to.
  m_hf = rcAllocHeightfield();
  if (!m_hf)
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
    return;
  }
  if (!rcCreateHeightfield(m_ctx, *m_hf, m_cfg->width, m_cfg->height, m_cfg->bmin, m_cfg->bmax, m_cfg->cs, m_cfg->ch))
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
    return;
  }

  // Allocate array that can hold triangle area types.
  // If you have multiple meshes you need to process, allocate
  // and array which can hold the max number of triangles you need to process.
  m_triareas = new unsigned char[ntris];
  if (!m_triareas)
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'm_triareas' (%d).", ntris);
    return;
  }

  // Find triangles which are walkable based on their slope and rasterize them.
  // If your input data is multiple meshes, you can transform them here, calculate
  // the are type for each of the meshes and rasterize them.
  memset(m_triareas, 0, ntris*sizeof(unsigned char));
  rcMarkWalkableTriangles(m_ctx, m_cfg->walkableSlopeAngle, verts, nverts, tris, ntris, m_triareas);
  rcRasterizeTriangles(m_ctx, verts, nverts, tris, m_triareas, ntris, *m_hf, m_cfg->walkableClimb);

  if (!m_keepInterResults)
  {
    delete[] m_triareas;
    m_triareas = 0;
  }

  //
  // Step 3. Filter walkables surfaces.
  //

  // Once all geoemtry is rasterized, we do initial pass of filtering to
  // remove unwanted overhangs caused by the conservative rasterization
  // as well as filter spans where the character cannot possibly stand.
  rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg->walkableClimb, *m_hf);
  rcFilterLedgeSpans(m_ctx, m_cfg->walkableHeight, m_cfg->walkableClimb, *m_hf);
  rcFilterWalkableLowHeightSpans(m_ctx, m_cfg->walkableHeight, *m_hf);


  //
  // Step 4. Partition walkable surface to simple regions.
  //

  // Compact the heightfield so that it is faster to handle from now on.
  // This will result more cache coherent data as well as the neighbours
  // between walkable cells will be calculated.
  m_chf = rcAllocCompactHeightfield();
  if (!m_chf)
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
    return;
  }
  if (!rcBuildCompactHeightfield(m_ctx, m_cfg->walkableHeight, m_cfg->walkableClimb, *m_hf, *m_chf))
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
    return;
  }

  if (!m_keepInterResults)
  {
    rcFreeHeightField(m_hf);
    m_hf = 0;
  }

  // Erode the walkable area by agent radius.
  if (!rcErodeWalkableArea(m_ctx, m_cfg->walkableRadius, *m_chf))
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
    return;
  }

  /*
  // (Optional) Mark areas.
  const ConvexVolume* vols = m_geom->getConvexVolumes();
  for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
    rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);
  */

  // Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
  // There are 3 martitioning methods, each with some pros and cons:
  // 1) Watershed partitioning
  //   - the classic Recast partitioning
  //   - creates the nicest tessellation
  //   - usually slowest
  //   - partitions the heightfield into nice regions without holes or overlaps
  //   - the are some corner cases where this method creates produces holes and overlaps
  //      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
  //      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
  //   * generally the best choice if you precompute the nacmesh, use this if you have large open areas
  // 2) Monotone partioning
  //   - fastest
  //   - partitions the heightfield into regions without holes and overlaps (guaranteed)
  //   - creates long thin polygons, which sometimes causes paths with detours
  //   * use this if you want fast navmesh generation
  // 3) Layer partitoining
  //   - quite fast
  //   - partitions the heighfield into non-overlapping regions
  //   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
  //   - produces better triangles than monotone partitioning
  //   - does not have the corner cases of watershed partitioning
  //   - can be slow and create a bit ugly tessellation (still better than monotone)
  //     if you have large open areas with small obstacles (not a problem if you use tiles)
  //   * good choice to use for tiled navmesh with medium and small sized tiles

  // Watershed partitioning

  // Prepare for region partitioning, by calculating distance field along the walkable surface.
  if (!rcBuildDistanceField(m_ctx, *m_chf))
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
    return;
  }

  // Partition the walkable surface into simple regions without holes.
  if (!rcBuildRegions(m_ctx, *m_chf, 0, m_cfg->minRegionArea, m_cfg->mergeRegionArea))
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build watershed regions.");
    return;
  }


  //
  // Step 5. Trace and simplify region contours.
  //

  // Create contours.
  m_cset = rcAllocContourSet();
  if (!m_cset)
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
    return;
  }
  if (!rcBuildContours(m_ctx, *m_chf, m_cfg->maxSimplificationError, m_cfg->maxEdgeLen, *m_cset))
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
    return;
  }

  //
  // Step 6. Build polygons mesh from contours.
  //

  // Build polygon navmesh from the contours.
  m_pmesh = rcAllocPolyMesh();
  if (!m_pmesh)
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
    return;
  }
  if (!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg->maxVertsPerPoly, *m_pmesh))
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
    return;
  }

  //
  // Step 7. Create detail mesh which allows to access approximate height on each polygon.
  //

  m_dmesh = rcAllocPolyMeshDetail();
  if (!m_dmesh)
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
    return;
  }

  if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf, m_cfg->detailSampleDist, m_cfg->detailSampleMaxError, *m_dmesh))
  {
    m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
    return;
  }

  // Build the Jump Down OffMesh connections
  BuildJumpConnections(
    agentHeight, 
    agentRadius, 
    maxJumpGroundRange, 
    maxJumpDistance, 
    initialJumpForwardSpeed, 
    initialJumpUpSpeed, 
    idealJumpPointsDist);

  if (!m_keepInterResults)
  {
    rcFreeCompactHeightfield(m_chf);
    m_chf = 0;
    rcFreeContourSet(m_cset);
    m_cset = 0;
  }

  // At this point the navigation mesh data is ready, you can access it from m_pmesh.
  // See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

  //
  // (Optional) Step 8. Create Detour data from Recast poly mesh.
  //

  // Update poly flags from areas.
  for (int i = 0; i < m_pmesh->npolys; ++i)
  {
    if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
      m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;

    if (m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND)
    {
      m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
    }
    else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER)
    {
      m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
    }
  }

  dtNavMeshCreateParams params;
  memset(&params, 0, sizeof(params));
  params.verts = m_pmesh->verts;
  params.vertCount = m_pmesh->nverts;
  params.polys = m_pmesh->polys;
  params.polyAreas = m_pmesh->areas;
  params.polyFlags = m_pmesh->flags;
  params.polyCount = m_pmesh->npolys;
  params.nvp = m_pmesh->nvp;
  params.detailMeshes = m_dmesh->meshes;
  params.detailVerts = m_dmesh->verts;
  params.detailVertsCount = m_dmesh->nverts;
  params.detailTris = m_dmesh->tris;
  params.detailTriCount = m_dmesh->ntris;
    
  params.offMeshConVerts = &m_OffMeshConVerts[0];
  params.offMeshConRad = &m_OffMeshConRad[0];
  params.offMeshConDir = &m_OffMeshConDir[0];
  params.offMeshConAreas = &m_OffMeshConAreas[0];
  params.offMeshConFlags = &m_OffMeshConFlags[0];
  params.offMeshConUserID = &m_OffMeshConUserID[0];
  params.offMeshConCount = m_OffMeshConVerts.size() / 6;
    
  params.walkableHeight = agentHeight;
  params.walkableRadius = agentRadius;
  params.walkableClimb = agentMaxClimb;
  rcVcopy(params.bmin, m_pmesh->bmin);
  rcVcopy(params.bmax, m_pmesh->bmax);
  params.cs = m_cfg->cs;
  params.ch = m_cfg->ch;
  params.buildBvTree = true;

  unsigned char* navData = 0;
  int navDataSize = 0;
  if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
  {
    m_ctx->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
    return;
  }

  m_navMesh = dtAllocNavMesh();
  if (!m_navMesh)
  {
    dtFree(navData);
    m_ctx->log(RC_LOG_ERROR, "Could not create Detour navmesh");
    return;
  }

  dtStatus status;

  status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
  if (dtStatusFailed(status))
  {
    dtFree(navData);
    m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh");
    return;
  }

  m_navQuery = dtAllocNavMeshQuery();
  if (!m_navQuery)
  {
    dtFree(m_navQuery);
    m_ctx->log(RC_LOG_ERROR, "Could not create Detour navmesh query");
    return;
  }

  status = m_navQuery->init(m_navMesh, 2048);
  if (dtStatusFailed(status))
  {
    m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh query");
    return;
  }

  m_filter = new dtQueryFilter;
  if (m_filter)
  {
    m_filter->setAreaCost(SAMPLE_POLYAREA_GROUND, 1.0f);
    m_filter->setAreaCost(SAMPLE_POLYAREA_WATER, 10.0f);
    m_filter->setAreaCost(SAMPLE_POLYAREA_JUMP, 1.5f);
    m_filter->setIncludeFlags(SAMPLE_POLYFLAGS_ALL ^ SAMPLE_POLYFLAGS_DISABLED);
    m_filter->setExcludeFlags(0);
  }

  CalcIntersectionPositions(maxIntersectionPosHeight);

  m_ctx->stopTimer(RC_TIMER_TOTAL);

  // Show performance stats.
  duLogBuildTimes(*m_ctx, m_ctx->getAccumulatedTime(RC_TIMER_TOTAL));
  m_ctx->log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", m_pmesh->nverts, m_pmesh->npolys);

  m_totalBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL) / 1000.0f;
}

shooter::NavMesh::~NavMesh()
{
  delete m_filter;
  dtFreeNavMeshQuery(m_navQuery);
  dtFreeNavMesh(m_navMesh);
  rcFreePolyMeshDetail(m_dmesh);
  rcFreePolyMesh(m_pmesh);
  rcFreeContourSet(m_cset);
  rcFreeCompactHeightfield(m_chf);
  rcFreeHeightField(m_hf);
  delete[] m_triareas;
  delete m_cfg;
}

void NavMesh::DebugRender() const
{
  DebugDrawGL dd;

  // Draw bounds
  const float* bmin = m_cfg->bmin;
  const float* bmax = m_cfg->bmax;
  duDebugDrawBoxWire(&dd, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], duRGBA(255, 255, 255, 128), 1.0f);
  dd.begin(DU_DRAW_POINTS, 5.0f);
  dd.vertex(bmin[0], bmin[1], bmin[2], duRGBA(255, 255, 255, 128));
  dd.end();

  // Draw mesh
  //duDebugDrawHeightfieldSolid(&dd, *m_hf);
  //duDebugDrawHeightfieldWalkable(&dd, *m_hf);
  //duDebugDrawCompactHeightfieldSolid(&dd, *m_chf);
  //duDebugDrawCompactHeightfieldDistance(&dd, *m_chf);
  //duDebugDrawRawContours(&dd, *m_cset, 0.5f);
  //duDebugDrawContours(&dd, *m_cset);
  //duDebugDrawRegionConnections(&dd, *m_cset);
  //duDebugDrawPolyMesh(&dd, *m_pmesh);
  duDebugDrawPolyMeshDetail(&dd, *m_dmesh);
  //duDebugDrawNavMesh(&dd, *m_navMesh, 0);
  //duDebugDrawNavMeshNodes(&dd, *m_navQuery);

  for (const auto& verts : m_DebugOffMeshConVerts)
  {
    dd.begin(DU_DRAW_LINE_STRIP, 1.f);
    for (unsigned i = 0; i < verts.size(); i += 3)
    {
      dd.vertex(verts[i], verts[i+1], verts[i+2], duRGBA(255, 0, 0, 255));
    }
    dd.end();

    dd.begin(DU_DRAW_POINTS, 2.f);
    for (unsigned i = 0; i < verts.size(); i += 3)
    {
      dd.vertex(verts[i], verts[i + 1], verts[i + 2], duRGBA(0, 255, 0, 255));
    }
    dd.end();
  }

  dd.begin(DU_DRAW_POINTS, 5.f);
  for (unsigned i = 0; i < m_IntersectionPositions.size(); i += 3)
  {
    dd.vertex(m_IntersectionPositions[i], m_IntersectionPositions[i + 1], m_IntersectionPositions[i + 2], duRGBA(0, 255, 0, 255));
  }
  dd.end();
}

bool NavMesh::FindPath(
  const float* startPos,
  const float* endPos,
  float* outPathStartPos,
  float* outPathEndPos,
  dtPolyRef* outPathPolys,
  int& outNrPathPolys) const
{
  if (!m_navMesh)
    return false;

  dtPolyRef startRef = cInvalidPolyRef;
  dtPolyRef endRef = cInvalidPolyRef;
  float polyPickExt[3] = { 2.f, 4.f, 2.f };

  m_navQuery->findNearestPoly(startPos, polyPickExt, m_filter, &startRef, outPathStartPos);
  m_navQuery->findNearestPoly(endPos, polyPickExt, m_filter, &endRef, outPathEndPos);

  if (startRef && endRef)
  {
    return m_navQuery->findPath(startRef, endRef, outPathStartPos, outPathEndPos, m_filter, outPathPolys, &outNrPathPolys, MAX_POLYS) == DT_SUCCESS;
  }

  return false;
}

void NavMesh::GetSteerPosOnPath(
  const float* startPos,
  const float* endPos,
  const dtPolyRef* visited,
  int visitedSize,
  dtPolyRef* inoutPath,
  int& inoutPathSize,
  float minTargetDist,
  float* outSteerPos,
  bool& outOffMeshConn,
  bool& outEndOfPath) const
{
  if (!m_navMesh || !inoutPathSize)
    return;

  if (visited)
  {
    inoutPathSize = FixupCorridor(inoutPath, inoutPathSize, MAX_POLYS, visited, visitedSize);
    inoutPathSize = FixupShortcuts(inoutPath, inoutPathSize, m_navQuery);
  }

  // Find steer target.
  static const int MAX_STEER_POINTS = 3;
  float steerPath[MAX_STEER_POINTS * 3];
  unsigned char steerPathFlags[MAX_STEER_POINTS];
  dtPolyRef steerPathPolys[MAX_STEER_POINTS];
  int nsteerPath = 0;

  m_navQuery->findStraightPath(startPos, endPos, inoutPath, inoutPathSize,
    steerPath, steerPathFlags, steerPathPolys, &nsteerPath, MAX_STEER_POINTS);

  if (!nsteerPath) { return; }

  // Find vertex far enough to steer to.
  int ns = 0;
  while (ns < nsteerPath)
  {
    // Stop at Off-Mesh link or when end of path or when point is further than slop away.
    if ((steerPathFlags[ns] & (DT_STRAIGHTPATH_OFFMESH_CONNECTION | DT_STRAIGHTPATH_END)) ||
      !inRange(&steerPath[ns * 3], startPos, minTargetDist, 1.0f))
      break;
    ns++;
  }
  // Failed to find good point to steer to.
  if (ns >= nsteerPath) { return; }

  dtVcopy(outSteerPos, &steerPath[ns * 3]);
  outSteerPos[1] = startPos[1];

  unsigned char steerFlags = steerPathFlags[ns];
  dtPolyRef steerPoly = steerPathPolys[ns];

  if (inRange(startPos, outSteerPos, minTargetDist, 1.0f))
  {
    if (steerFlags & DT_STRAIGHTPATH_END)
    {
      // Reached end of path.
      inoutPathSize--;
      dtVcopy(outSteerPos, endPos);
      outEndOfPath = true;
    }
    else if (steerFlags & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
    {
      assert(steerPoly == inoutPath[1]);

      float offMeshStartPos[3] = { 0 }, offMeshEndPos[3] = { 0 };
      dtStatus status = m_navMesh->getOffMeshConnectionPolyEndPoints(inoutPath[0], inoutPath[1], offMeshStartPos, offMeshEndPos);
      assert(status == DT_SUCCESS);

      // Advance the path over the off-mesh connection.
      inoutPathSize -= 2;
      for (int i = 0; i < inoutPathSize; ++i)
        inoutPath[i] = inoutPath[i + 2];

      dtVcopy(outSteerPos, offMeshEndPos);
      outOffMeshConn = true;
    }
  }
}

bool NavMesh::GetFloorInfo(
  const float* pt, 
  const float hrange, 
  float& outY, 
  float& outDistY,
  bool* outWalkable, 
  float* outBorderDistance) const
{
  const int ix = (int)floorf((pt[0] - m_chf->bmin[0]) / m_chf->cs);
  const int iz = (int)floorf((pt[2] - m_chf->bmin[2]) / m_chf->cs);

  if (ix < 0 || iz < 0 || ix >= m_chf->width || iz >= m_chf->height)
    return false;

  bool found = false;
  int foundIx = -1;
  float foundY = FLT_MAX;
  float foundDistY = FLT_MAX;

  const rcCompactCell& c = m_chf->cells[ix + iz * m_chf->width];
  for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
  {
    const rcCompactSpan& s = m_chf->spans[i];
    const float y = m_chf->bmin[1] + s.y * m_chf->ch;
    const float dist = abs(pt[1] - y);
    if (dist < hrange && dist < foundDistY)
    {
      found = true;
      foundIx = i;
      foundY = y;
      foundDistY = dist;
    }
  }

  if (found)
  {
    outY = foundY;
    outDistY = foundDistY;

    bool walkable = (m_chf->areas[foundIx] != RC_NULL_AREA);

    if (outWalkable)
    {
      *outWalkable = walkable;
    }

    if (walkable && outBorderDistance)
    {
      *outBorderDistance = 1.f * m_chf->dist[foundIx] / m_chf->maxDistance;
    }
  }
  
  return found;
}

bool NavMesh::CheckCollision(
  const float* pos1,
  const float* pos2,
  const float height,
  const float range) const
{
  const int w = m_hf->width;
  const int h = m_hf->height;
  const float cs = m_hf->cs;
  const float ch = m_hf->ch;
  const float* orig = m_hf->bmin;

  float ptMin[3] = { pos1[0], pos1[1], pos1[2] };
  float ptMax[3] = { pos2[0], pos2[1], pos2[2] };

  for (unsigned i = 0; i < 3; i++)
    if (ptMin[i] > ptMax[i])
      rcSwap(ptMin[i], ptMax[i]);

  const float ymin = ptMin[1];
  const float ymax = ptMax[1] + height;
  const int ix0 = rcClamp((int)floorf((ptMin[0] - range - orig[0]) / cs), 0, w - 1);
  const int ix1 = rcClamp((int)floorf((ptMax[0] + range - orig[0]) / cs), 0, w - 1);
  const int iz0 = rcClamp((int)floorf((ptMin[2] - range - orig[2]) / cs), 0, h - 1);
  const int iz1 = rcClamp((int)floorf((ptMax[2] + range - orig[2]) / cs), 0, h - 1);

  for (int z = iz0; z <= iz1; ++z)
  {
    for (int x = ix0; x <= ix1; ++x)
    {
      const rcSpan* s = m_hf->spans[x + z*w];
      if (!s) continue;

      while (s)
      {
        const float symin = orig[1] + s->smin*ch;
        const float symax = orig[1] + s->smax*ch;
        if (overlapRange(ymin, ymax, symin, symax))
          return true;
        s = s->next;
      }
    }
  }

  return false;
}

int NavMesh::FixupCorridor(dtPolyRef* path, const int npath, const int maxPath,
  const dtPolyRef* visited, const int nvisited)
{
  int furthestPath = -1;
  int furthestVisited = -1;

  // Find furthest common polygon.
  for (int i = npath - 1; i >= 0; --i)
  {
    bool found = false;
    for (int j = nvisited - 1; j >= 0; --j)
    {
      if (path[i] == visited[j])
      {
        furthestPath = i;
        furthestVisited = j;
        found = true;
      }
    }
    if (found)
      break;
  }

  // If no intersection found just return current path. 
  if (furthestPath == -1 || furthestVisited == -1)
    return npath;

  // Concatenate paths.  

  // Adjust beginning of the buffer to include the visited.
  const int req = nvisited - furthestVisited;
  const int orig = rcMin(furthestPath + 1, npath);
  int size = rcMax(0, npath - orig);
  if (req + size > maxPath)
    size = maxPath - req;
  if (size)
    memmove(path + req, path + orig, size * sizeof(dtPolyRef));

  // Store visited
  for (int i = 0; i < req; ++i)
    path[i] = visited[(nvisited - 1) - i];

  return req + size;
}

// This function checks if the path has a small U-turn, that is,
// a polygon further in the path is adjacent to the first polygon
// in the path. If that happens, a shortcut is taken.
// This can happen if the target (T) location is at tile boundary,
// and we're (S) approaching it parallel to the tile edge.
// The choice at the vertex can be arbitrary, 
//  +---+---+
//  |:::|:::|
//  +-S-+-T-+
//  |:::|   | <-- the step can end up in here, resulting U-turn path.
//  +---+---+
int NavMesh::FixupShortcuts(dtPolyRef* path, int npath, const dtNavMeshQuery* navQuery)
{
  if (npath < 3)
    return npath;

  // Get connected polygons
  static const int maxNeis = 16;
  dtPolyRef neis[maxNeis];
  int nneis = 0;

  const dtMeshTile* tile = 0;
  const dtPoly* poly = 0;
  if (dtStatusFailed(navQuery->getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
    return npath;

  for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
  {
    const dtLink* link = &tile->links[k];
    if (link->ref != 0)
    {
      if (nneis < maxNeis)
        neis[nneis++] = link->ref;
    }
  }

  // If any of the neighbour polygons is within the next few polygons
  // in the path, short cut to that polygon directly.
  static const int maxLookAhead = 6;
  int cut = 0;
  for (int i = dtMin(maxLookAhead, npath) - 1; i > 1 && cut == 0; i--) {
    for (int j = 0; j < nneis; j++)
    {
      if (path[i] == neis[j]) {
        cut = i;
        break;
      }
    }
  }
  if (cut > 1)
  {
    int offset = cut - 1;
    npath -= offset;
    for (int i = 1; i < npath; i++)
      path[i] = path[i + offset];
  }

  return npath;
}

void NavMesh::BuildJumpConnections(
  float agentHeight,
  float agentRadius,
  float maxGroundRange,
  float maxJumpDownDistance,
  float initialForwardSpeed,
  float initialUpSpeed,
  float idealJumpPointsDist)
{
  static const float up[3] = { 0.f, 1.f, 0.f };

  const rcPolyMesh& mesh = *m_pmesh;
  const int nvp = mesh.nvp;
  const float cs = mesh.cs;
  const float ch = mesh.ch;
  const float* orig = mesh.bmin;

  // for all navmesh border segments
  for (int i = 0; i < mesh.npolys; ++i)
  {
    const unsigned short* p = &mesh.polys[i*nvp * 2];
    for (int j = 0; j < nvp; ++j)
    {
      if (p[j] == RC_MESH_NULL_IDX) break;
      if ((p[nvp + j] & 0x8000) == 0) continue;
      const int nj = (j + 1 >= nvp || p[j + 1] == RC_MESH_NULL_IDX) ? 0 : j + 1;
      const int vi[2] = { p[j], p[nj] };

      const unsigned short* v1 = &mesh.verts[vi[0] * 3];
      float pt1[3] = {
        orig[0] + v1[0] * cs,
        orig[1] + v1[1] * ch + ch,
        orig[2] + v1[2] * cs
      };

      const unsigned short* v2 = &mesh.verts[vi[1] * 3];
      float pt2[3] = {
        orig[0] + v2[0] * cs,
        orig[1] + v2[1] * ch + ch,
        orig[2] + v2[2] * cs
      };

      float segDir[3] = { 0 };
      rcVsub(segDir, pt2, pt1);

      float segLen = dtVlen(segDir);

      // rotate segDir 90 degrees and normalize
      float normal[3] = { -segDir[2], 0.f, segDir[0] };
      rcVnormalize(normal);

      float vel[3] = { 0 }; // velocity vector
      rcVmad(vel, vel, normal, initialForwardSpeed);
      rcVmad(vel, vel, up, initialUpSpeed);

      int nrJumpPoints = (int)round(segLen / idealJumpPointsDist);
      if (!nrJumpPoints) continue;

      // try to make off-mesh connections at equally distant points long the segment
      float jumpPointDist = segLen / nrJumpPoints;
      for (float t = jumpPointDist; t < segLen; t += jumpPointDist)
      {
        float ptJump[3] = { 0 };
        rcVmad(ptJump, pt1, segDir, t / segLen);

        float ptLand[3] = { 0 };
        if (CheckOffMeshLink(agentHeight, agentRadius, ptJump, vel, maxJumpDownDistance, ptLand))
        {
          m_OffMeshConUserID.push_back(1000 + m_OffMeshConVerts.size() / 6);

          m_OffMeshConVerts.push_back(ptJump[0]);
          m_OffMeshConVerts.push_back(ptJump[1]);
          m_OffMeshConVerts.push_back(ptJump[2]);

          m_OffMeshConVerts.push_back(ptLand[0]);
          m_OffMeshConVerts.push_back(ptLand[1]);
          m_OffMeshConVerts.push_back(ptLand[2]);
        }
      }
    }
  }

  unsigned nbOffMeshConVerts = m_OffMeshConVerts.size() / 6;
  m_OffMeshConRad.assign(nbOffMeshConVerts, 0.8f);
  m_OffMeshConFlags.assign(nbOffMeshConVerts, SAMPLE_POLYFLAGS_JUMP);
  m_OffMeshConAreas.assign(nbOffMeshConVerts, SAMPLE_POLYAREA_JUMP);
  m_OffMeshConDir.assign(nbOffMeshConVerts, 0);
}

bool NavMesh::CheckOffMeshLink(
  float agentHeight,
  float agentRadius, 
  const float* origPos, 
  const float* origVel, 
  float maxHeight, 
  float* outLinkPt)
{
  static const float cSimulationStep = 0.016f;
  float cellDiagSq = 2 * m_hf->cs * m_hf->cs + m_hf->ch * m_hf->ch;
  float startY = origPos[1];
  float pos[3], vel[3], lastPos[3];
  rcVcopy(pos, origPos);
  rcVcopy(vel, origVel);
  int cnt = 0;

  m_DebugOffMeshConVerts.push_back(std::vector<float>());

  bool walkable = false;
  float floorDist = FLT_MAX, floorY = FLT_MAX;

  do
  {
    rcVcopy(lastPos, pos);

    m_DebugOffMeshConVerts.back().push_back(pos[0]);
    m_DebugOffMeshConVerts.back().push_back(pos[1]);
    m_DebugOffMeshConVerts.back().push_back(pos[2]);

    // Optimization: It's quite expensive to call checkCollision for all the sampled points 
    // along the jump down curve, so we do the physics simulation with a fixed 16 ms step
    // and only check the collision if we traveled a distance more then the heightfield cell's diagonal
    do
    {
      VelocityVerlet(cSimulationStep, pos, vel);
    } while (rcVdistSqr(lastPos, pos) < cellDiagSq);

    if (CheckCollision(lastPos, pos, agentHeight, agentRadius))
    {
      bool hasFloorPos = GetFloorInfo(pos, 1.0f, floorY, floorDist, &walkable);
      if (!hasFloorPos || (hasFloorPos && pos[1] < floorY))
      {
        break;
      }
    }

    cnt++;
  } while (cnt < 100);

  float prevFloorDist = FLT_MAX, prevFloorY = FLT_MAX;
  if (GetFloorInfo(lastPos, 1.0f, prevFloorY, prevFloorDist, &walkable)
    && walkable
    && (startY - prevFloorY > 1.f)
    && (startY - prevFloorY < maxHeight))
  {
    float t = prevFloorDist / (prevFloorDist + floorDist);
    dtVlerp(pos, lastPos, pos, t);

    rcVcopy(outLinkPt, pos);

    m_DebugOffMeshConVerts.back().push_back(pos[0]);
    m_DebugOffMeshConVerts.back().push_back(pos[1]);
    m_DebugOffMeshConVerts.back().push_back(pos[2]);

    return true;
  }

  m_DebugOffMeshConVerts.pop_back();

  return false;
}

void NavMesh::CalcIntersectionPositions(float maxIntersectionPosHeight)
{
  const float* orig = m_cset->bmin;
  const float cs = m_cset->cs;
  const float ch = m_cset->ch;

  for (int i = 0; i < m_cset->nconts; ++i)
  {
    const rcContour* cont = &m_cset->conts[i];

    std::unordered_set<unsigned short> regs;
    for (int j = 0; j < cont->nverts; ++j)
    {
      const int* v = &cont->verts[j * 4];
      unsigned short reg = (unsigned short)v[3];
      if ((reg == 0) || (reg == cont->reg)) continue;

      regs.insert(reg);
    }

    if (regs.size() == 1 || regs.size() > 2)
    {
      float pos[3];
      getContourCenter(cont, orig, cs, ch, pos);

      // Make sure we don't end up on the roof tops
      if (pos[1] < maxIntersectionPosHeight)
      {
        m_IntersectionPositions.push_back(pos[0]);
        m_IntersectionPositions.push_back(pos[1]);
        m_IntersectionPositions.push_back(pos[2]);
      }
    }
  }
}