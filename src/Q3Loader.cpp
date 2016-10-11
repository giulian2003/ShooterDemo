/**
* Simple Q3 Map loader.
*
* For comments or bug reports, contact : nicolas.baudrey@wanadoo.fr
*
* 01/01/2016: Modified by Iulian Ghetau to read data from a stream, convert to OpenGL coordinate system
* and post process the bezier patches.
*
* For comments or bug reports, contact : giulian2003@gmail.com
*
*/

#include <Q3Loader.h>
#include <cstdio>

template <class taType>
void swizzle3(taType* const t) {
  taType temp;
  temp = t[1];
  t[1] = t[2];
  t[2] = -temp;
}

template <class taType>
void scale3(taType* const t, float scale)
{
  t[0] = static_cast<taType>(t[0] * scale);
  t[1] = static_cast<taType>(t[1] * scale);
  t[2] = static_cast<taType>(t[2] * scale);
}

template <class taType>
void swap(taType& a, taType&b)
{
  taType t = a;
  a = b;
  b = t;
}

void fix_int_bound(float (&t)[3])
{
  t[0] = static_cast<float>(reinterpret_cast<int&>(t[0]));
  t[1] = static_cast<float>(reinterpret_cast<int&>(t[1]));
  t[2] = static_cast<float>(reinterpret_cast<int&>(t[2]));
}

template <class taType>
void sub3(const taType* x, const taType* y, taType* out)
{
  out[0] = x[0] - y[0];
  out[1] = x[1] - y[1];
  out[2] = x[2] - y[2];
}

template <class taType>
void cross3(const taType* x, const taType* y, taType* out)
{
  out[0] = x[1] * y[2] - x[2] * y[1];
  out[1] = x[2] * y[0] - x[0] * y[2];
  out[2] = x[0] * y[1] - x[1] * y[0];
}

template <class taType>
bool parallel3(const taType* v1, const taType* v2)
{
  const taType epsilon = 1.e-5f;
  
  taType m = 0.f;
  if (abs(v2[0]) >= epsilon) { m = v1[0] / v2[0]; }
  else if (abs(v2[1]) >= epsilon) { m = v1[1] / v2[1]; }
  else if (abs(v2[2]) >= epsilon) { m = v1[2] / v2[2]; }
  else { return false; }
    
  return (abs(v2[0] * m - v1[0]) < epsilon) 
    && (abs(v2[1] * m - v1[1]) < epsilon) 
    && (abs(v2[2] * m - v1[2]) < epsilon);
}

template <class taType>
taType mix(const taType& A, const taType& B, float t)
{
  return (taType)(A * (1.0f - t) + B * t);
}

template <>
TVertex mix(const TVertex& x, const TVertex& y, float t)
{
  TVertex v = {0};

  v.mPosition[0] = mix(x.mPosition[0], y.mPosition[0], t);
  v.mPosition[1] = mix(x.mPosition[1], y.mPosition[1], t);
  v.mPosition[2] = mix(x.mPosition[2], y.mPosition[2], t);
  v.mTexCoord[0][0] = mix(x.mTexCoord[0][0], y.mTexCoord[0][0], t);
  v.mTexCoord[0][1] = mix(x.mTexCoord[0][1], y.mTexCoord[0][1], t);
  v.mTexCoord[1][0] = mix(x.mTexCoord[1][0], y.mTexCoord[1][0], t);
  v.mTexCoord[1][1] = mix(x.mTexCoord[1][1], y.mTexCoord[1][1], t);
  v.mNormal[0] = mix(x.mNormal[0], y.mNormal[0], t);
  v.mNormal[1] = mix(x.mNormal[1], y.mNormal[1], t);
  v.mNormal[2] = mix(x.mNormal[2], y.mNormal[2], t);
  v.mColor[0] = mix(x.mColor[0], y.mColor[0], t);
  v.mColor[1] = mix(x.mColor[1], y.mColor[1], t);
  v.mColor[2] = mix(x.mColor[2], y.mColor[2], t);
  v.mColor[3] = mix(x.mColor[3], y.mColor[3], t);

  return v;
}

template <class taType>
taType quadraticBezier(const taType& A, const taType& B, const taType& C, float t)
{
  taType D = mix(A, B, t);
  taType E = mix(B, C, t);

  return mix(D, E, t);
}

template <class taType>
taType quadraticBezierSurface(const taType* patch, unsigned stride, unsigned ix, float stepX, unsigned iy, float stepY, taType* cache, uint64_t & cacheFill)
{
  cache += ix * 3;
  const uint64_t cacheFillMask = (1ULL << ix);

  if ((cacheFill & cacheFillMask) == 0)
  {
    // cache these values instead of calculating them all the time
    cacheFill &= cacheFillMask;
    for (int i = 0; i < 3; i++, patch += stride)
    {
      cache[i] = quadraticBezier(patch[0], patch[1], patch[2], ix * stepX);
    }
  }

  return quadraticBezier(cache[0], cache[1], cache[2], iy * stepY);
}


/**
 * Check if the header of the map is valid.
 *
 * @param pMap  The map to test.
 *
 * @return True if the map is valid, false otherwise.
 */
bool isValid(const TMapQ3& pMap)
{
  // Check if the header is equal to ID Software Magic Number.
  if
    (strncmp(pMap.mHeader.mMagicNumber, cMagicNumber.c_str(), 4) != 0)
  {
    return false;
  }

  // Check if the version number is equal to the Q3 map.
  if
    (pMap.mHeader.mVersion != cVersion)
  {
    return false;
  }

  return true;
}

/**
 * Read the header of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
bool readHeader(std::istream& bspData, TMapQ3& pMap)
{
  bspData.read((char*)&pMap.mHeader, sizeof(THeader));

  return isValid(pMap);
}

/**
 * Read the texture lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readTexture(std::istream& bspData, TMapQ3& pMap)
{
  int  lNbTextures = pMap.mHeader.mLumpes[cTextureLump].mLength / sizeof(TTexture);

  bspData.seekg(pMap.mHeader.mLumpes[cTextureLump].mOffset, std::ios_base::beg);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cTextureLump].mOffset);

  for  (int lTextureCounter = 0; lTextureCounter < lNbTextures; ++lTextureCounter)
  {
    TTexture lTexture;
    
    bspData.read((char*)&lTexture, sizeof(TTexture));

    pMap.mTextures.push_back(lTexture);
  }
}

/**
 * Read the entity lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readEntity(std::istream& bspData, TMapQ3& pMap)
{
  // Set the entity size.
  pMap.mEntity.mSize = pMap.mHeader.mLumpes[cEntityLump].mLength;
  
  // Allocate the entity buffer.
  pMap.mEntity.mBuffer.assign(pMap.mEntity.mSize, 0);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cEntityLump].mOffset, std::ios_base::beg);
  
  // Read the buffer.
  bspData.read(&pMap.mEntity.mBuffer[0], pMap.mEntity.mSize);
};

/**
 * Read the plane lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readPlane(std::istream& bspData, TMapQ3& pMap, float scale = 1.f, bool coordSysOpenGL = false)
{
  int  lNbPlanes = pMap.mHeader.mLumpes[cPlaneLump].mLength / sizeof(TPlane);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cPlaneLump].mOffset, std::ios_base::beg);

  for  (int lPlaneCounter = 0; lPlaneCounter < lNbPlanes; ++lPlaneCounter)
  {
    TPlane lPlane;
    
    bspData.read((char*)&lPlane, sizeof(TPlane));

    lPlane.mDistance *= scale;
    
    if (coordSysOpenGL)
    {
      swizzle3(lPlane.mNormal);
    }
    
    pMap.mPlanes.push_back(lPlane);
  }
}

/**
 * Read the node lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readNode(std::istream& bspData, TMapQ3& pMap, float scale = 1.f, bool coordSysOpenGL = false)
{
  int  lNbNodes = pMap.mHeader.mLumpes[cNodeLump].mLength / sizeof(TNode);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cNodeLump].mOffset, std::ios_base::beg);

  for  (int lNodeCounter = 0; lNodeCounter < lNbNodes; ++lNodeCounter)
  {
    TNode lNode;
    
    bspData.read((char*)&lNode, sizeof(TNode));

    fix_int_bound(lNode.mMaxs);
    fix_int_bound(lNode.mMins);
    scale3(lNode.mMaxs, scale);
    scale3(lNode.mMins, scale);
    
    if (coordSysOpenGL)
    {
      swizzle3(lNode.mMaxs);
      swizzle3(lNode.mMins);
      swap(lNode.mMaxs[2], lNode.mMins[2]);
    }

    pMap.mNodes.push_back(lNode);
  }  
}

/**
 * Read the leaf lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readLeaf(std::istream& bspData, TMapQ3& pMap, float scale = 1.f, bool coordSysOpenGL = false)
{
  int  lNbLeaves = pMap.mHeader.mLumpes[cLeafLump].mLength / sizeof(TLeaf);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cLeafLump].mOffset, std::ios_base::beg);

  for  (int lLeafCounter = 0; lLeafCounter < lNbLeaves; ++lLeafCounter)
  {
    TLeaf lLeaf;
    
    bspData.read((char*)&lLeaf, sizeof(TLeaf));

    fix_int_bound(lLeaf.mMaxs);
    fix_int_bound(lLeaf.mMins);
    scale3(lLeaf.mMaxs, scale);
    scale3(lLeaf.mMins, scale);

    if (coordSysOpenGL)
    {
      swizzle3(lLeaf.mMaxs);
      swizzle3(lLeaf.mMins);
      swap(lLeaf.mMaxs[2], lLeaf.mMins[2]);
    }

    pMap.mLeaves.push_back(lLeaf);
  }  
}

/**
 * Read the leafface lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readLeafFace(std::istream& bspData, TMapQ3& pMap)
{
  int  lNbLeafFaces = pMap.mHeader.mLumpes[cLeafFaceLump].mLength / sizeof(TLeafFace);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cLeafFaceLump].mOffset, std::ios_base::beg);

  for  (int lLeafFaceCounter = 0; lLeafFaceCounter < lNbLeafFaces; ++lLeafFaceCounter)
  {
    TLeafFace lLeafFace;
    
    bspData.read((char*)&lLeafFace, sizeof(TLeafFace));

    pMap.mLeafFaces.push_back(lLeafFace);
  }      
}

/**
 * Read the leafbrush lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readLeafBrush(std::istream& bspData, TMapQ3& pMap)
{
  int  lNbLeafBrushes = pMap.mHeader.mLumpes[cLeafBrushLump].mLength / sizeof(TLeafBrush);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cLeafBrushLump].mOffset, std::ios_base::beg);

  for  (int lLeafBrusheCounter = 0; lLeafBrusheCounter < lNbLeafBrushes; ++lLeafBrusheCounter)
  {
    TLeafBrush lLeafBrush;
    
    bspData.read((char*)&lLeafBrush, sizeof(TLeafBrush));

    pMap.mLeafBrushes.push_back(lLeafBrush);
  }  
}

/**
 * Read the model lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readModel(std::istream& bspData, TMapQ3& pMap, float scale = 1.f, bool coordSysOpenGL = false)
{
  int  lNbModels = pMap.mHeader.mLumpes[cModelLump].mLength / sizeof(TModel);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cModelLump].mOffset, std::ios_base::beg);

  for  (int lModelCounter = 0; lModelCounter < lNbModels; ++lModelCounter)
  {
    TModel lModel;
    
    bspData.read((char*)&lModel, sizeof(TModel));

    scale3(lModel.mMaxs, scale);
    scale3(lModel.mMins, scale);

    if (coordSysOpenGL)
    {
      swizzle3(lModel.mMaxs);
      swizzle3(lModel.mMins);
    }

    pMap.mModels.push_back(lModel);
  }
}

/**
 * Read the brush lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readBrush(std::istream& bspData, TMapQ3& pMap)
{
  int  lNbBrushes = pMap.mHeader.mLumpes[cBrushLump].mLength / sizeof(TBrush);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cBrushLump].mOffset, std::ios_base::beg);

  for  (int lBrusheCounter = 0; lBrusheCounter < lNbBrushes; ++lBrusheCounter)
  {
    TBrush lBrush;
    
    bspData.read((char*)&lBrush, sizeof(TBrush));

    pMap.mBrushes.push_back(lBrush);
  }
}

/**
 * Read the brush side lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readBrushSide(std::istream& bspData, TMapQ3& pMap)
{
  int  lNbBrushSides = pMap.mHeader.mLumpes[cBrushSideLump].mLength / sizeof(TBrushSide);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cBrushSideLump].mOffset, std::ios_base::beg);

  for (int lBrushSideCounter = 0; lBrushSideCounter < lNbBrushSides; ++lBrushSideCounter)
  {
    TBrushSide lBrushSide;
    
    bspData.read((char*)&lBrushSide, sizeof(TBrushSide));

    pMap.mBrushSides.push_back(lBrushSide);
  }
}

/**
 * Read the vertex lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readVertex(std::istream& bspData, TMapQ3& pMap, float scale = 1.f, bool coordSysOpenGL = false)
{
  int  lNbVertices = pMap.mHeader.mLumpes[cVertexLump].mLength / sizeof(TVertex);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cVertexLump].mOffset, std::ios_base::beg);

  for  (int lVerticeCounter = 0; lVerticeCounter < lNbVertices; ++lVerticeCounter)
  {
    TVertex lVertex;
    
    bspData.read((char*)&lVertex, sizeof(TVertex));

    scale3(lVertex.mPosition, scale);

    if (coordSysOpenGL)
    {
      swizzle3(lVertex.mPosition);
      swizzle3(lVertex.mNormal);
    }

    pMap.mVertices.push_back(lVertex);
  }
}

/**
 * Read the meshvert lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readMeshVert(std::istream& bspData, TMapQ3& pMap)
{
  int  lNbMeshVertices = pMap.mHeader.mLumpes[cMeshVertLump].mLength / sizeof(TMeshVert);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cMeshVertLump].mOffset, std::ios_base::beg);

  for  (int lVerticeCounter = 0; lVerticeCounter < lNbMeshVertices; ++lVerticeCounter)
  {
    TMeshVert lMeshVertex;
    
    bspData.read((char*)&lMeshVertex, sizeof(TMeshVert));

    pMap.mMeshVertices.push_back(lMeshVertex);
  }
}

/**
 * Read the effect lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readEffect(std::istream& bspData, TMapQ3& pMap)
{
  int  lNbEffects = pMap.mHeader.mLumpes[cEffectLump].mLength / sizeof(TEffect);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cEffectLump].mOffset, std::ios_base::beg);

  for  (int lEffectCounter = 0; lEffectCounter < lNbEffects; ++lEffectCounter)
  {
    TEffect lEffect;
    
    bspData.read((char*)&lEffect, sizeof(TEffect));

    pMap.mEffects.push_back(lEffect);
  }  
}

/**
 * Read the face lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readFace(std::istream& bspData, TMapQ3& pMap, float scale = 1.f, bool coordSysOpenGL = false)
{
  int  lNbFaces = pMap.mHeader.mLumpes[cFaceLump].mLength / sizeof(TFace);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cFaceLump].mOffset, std::ios_base::beg);

  for  (int lFaceCounter = 0; lFaceCounter < lNbFaces; ++lFaceCounter)
  {
    TFace lFace;
    
    bspData.read((char*)&lFace, sizeof(TFace));

    if (lFace.mLightmapIndex < 0) { lFace.mLightmapIndex = -1; }
    if (lFace.mTextureIndex < 0) { lFace.mTextureIndex = -1; }

    scale3(lFace.mLightmapOrigin, scale);
    scale3(lFace.mLightmapVecs[0], scale);
    scale3(lFace.mLightmapVecs[1], scale);

    if (coordSysOpenGL)
    {
      swizzle3(lFace.mNormal);
    }

    pMap.mFaces.push_back(lFace);
  }
}

/**
 * Read the lightmap lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readLightMap(std::istream& bspData, TMapQ3& pMap)
{
  int  lNbLightMaps = pMap.mHeader.mLumpes[cLightMapLump].mLength / sizeof(TLightMap);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cLightMapLump].mOffset, std::ios_base::beg);

  for  (int lLightMapCounter = 0; lLightMapCounter < lNbLightMaps; ++lLightMapCounter)
  {
    TLightMap lLightMap;
    
    bspData.read((char*)&lLightMap, sizeof(TLightMap));

    pMap.mLightMaps.push_back(lLightMap);
  }
}

/**
 * Read the lightvol lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readLightVol(std::istream& bspData, TMapQ3& pMap)
{
  int  lNbLightVols = pMap.mHeader.mLumpes[cLightVolLump].mLength / sizeof(TLightVol);

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cLightVolLump].mOffset, std::ios_base::beg);

  for  (int lLightVolCounter = 0; lLightVolCounter < lNbLightVols; ++lLightVolCounter)
  {
    TLightVol lLightVol;
    
    bspData.read((char*)&lLightVol, sizeof(TLightVol));

    pMap.mLightVols.push_back(lLightVol);
  }
}

/**
 * Read the visdata lump of the Q3 map.
 *
 * @param pFile  The stream on the Q3 file data.
 * @param pMap  The map structure to fill.
 */
void readVisData(std::istream& bspData, TMapQ3& pMap)
{
  pMap.mVisData.mNbClusters = 0;
  pMap.mVisData.mBytesPerCluster = 0;

  if (pMap.mHeader.mLumpes[cVisDataLump].mLength <= 0)
  {
    return;
  }

  // Go to the start of the chunk.
  bspData.seekg(pMap.mHeader.mLumpes[cVisDataLump].mOffset, std::ios_base::beg);

  bspData.read((char*)&pMap.mVisData.mNbClusters, sizeof(int));
  bspData.read((char*)&pMap.mVisData.mBytesPerCluster, sizeof(int));

  // Allocate the buffer.
  int lBufferSize = pMap.mVisData.mNbClusters * pMap.mVisData.mBytesPerCluster;
  pMap.mVisData.mBuffer.assign(lBufferSize, 0);

  bspData.read((char*)&pMap.mVisData.mBuffer[0], lBufferSize);
}

/**
 * Dump all the Q3 map in a text file.
 * Must be used only for debug purpose.
 *
 * @param pFile  The file to dump the informations.
 * @param pMap  The Q3 map to dump in string.
 */
void debugInformations(const TMapQ3& pMap, FILE* pFile)
{
  // Check if the given stream is valid.
  if
    (! pFile)
  {
    printf("debugInformations :: Invalid stream handle.\n");
    return ;
  }

  // Check if the map is valid.
  if
    (! isValid(pMap))
  {
    printf("debugInformations :: Invalid Q3 map header.\n");
    return;
  }

  fprintf(pFile, "********* Header *********\n");
  fprintf(pFile, "Magic Number : %s\n", pMap.mHeader.mMagicNumber);
  fprintf(pFile, "Version : %d\n", pMap.mHeader.mVersion);
  for
    (unsigned int lLumpCounter = 0; lLumpCounter < 17; ++lLumpCounter)
  {
    fprintf(pFile, "Lump %d\n", lLumpCounter);
    fprintf(pFile, "\tOffset : %d\n", pMap.mHeader.mLumpes[lLumpCounter].mOffset);
    fprintf(pFile, "\tLength : %d\n", pMap.mHeader.mLumpes[lLumpCounter].mLength);
  }
  fprintf(pFile, "\n");
  
  fprintf(pFile, "********* Entity Lump *********\n");
  fprintf(pFile, "Size : %d\n", pMap.mEntity.mSize);
  if
    (pMap.mEntity.mSize != 0)
  {
    fprintf(pFile, "Buffer : %s\n", pMap.mEntity.mBuffer.c_str());
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* Texture Lump *********\n");
  for
    (unsigned int lTextureCounter = 0; lTextureCounter < pMap.mTextures.size(); ++lTextureCounter)
  {
    fprintf(pFile, "Texture %d\n", lTextureCounter);
    fprintf(pFile, "\tName : %s\n", pMap.mTextures[lTextureCounter].mName);
    fprintf(pFile, "\tFlags : %d\n", pMap.mTextures[lTextureCounter].mFlags);
    fprintf(pFile, "\tContents : %d\n", pMap.mTextures[lTextureCounter].mContents);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* Plane Lump *********\n");
  for
    (unsigned int lPlaneCounter = 0; lPlaneCounter < pMap.mPlanes.size(); ++lPlaneCounter)
  {
    fprintf(pFile, "Plane %d\n", lPlaneCounter);
    fprintf(pFile, "\tNormal : %f %f %f\n", pMap.mPlanes[lPlaneCounter].mNormal[0], pMap.mPlanes[lPlaneCounter].mNormal[1], pMap.mPlanes[lPlaneCounter].mNormal[2]);
    fprintf(pFile, "\tDistance : %f\n", pMap.mPlanes[lPlaneCounter].mDistance);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* Node Lump *********\n");
  for
    (unsigned int lNodeCounter = 0; lNodeCounter < pMap.mNodes.size(); ++lNodeCounter)
  {
    fprintf(pFile, "Node %d\n", lNodeCounter);
    fprintf(pFile, "\tPlane index : %d\n", pMap.mNodes[lNodeCounter].mPlane);
    fprintf(pFile, "\tChildren index : %d %d\n", pMap.mNodes[lNodeCounter].mChildren[0], pMap.mNodes[lNodeCounter].mChildren[1]);
    fprintf(pFile, "\tMin Bounding Box : %f %f %f\n", pMap.mNodes[lNodeCounter].mMins[0], pMap.mNodes[lNodeCounter].mMins[1], pMap.mNodes[lNodeCounter].mMins[2]);
    fprintf(pFile, "\tMax Bounding Box : %f %f %f\n", pMap.mNodes[lNodeCounter].mMaxs[0], pMap.mNodes[lNodeCounter].mMaxs[1], pMap.mNodes[lNodeCounter].mMaxs[2]);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* Leaf Lump *********\n");
  for
    (unsigned int lLeafCounter = 0; lLeafCounter < pMap.mLeaves.size(); ++lLeafCounter)
  {
    fprintf(pFile, "Leaf %d\n", lLeafCounter);
    fprintf(pFile, "\tCluster %d\n", pMap.mLeaves[lLeafCounter].mCluster);
    fprintf(pFile, "\tMin Bounding Box : %f %f %f\n", pMap.mLeaves[lLeafCounter].mMins[0], pMap.mLeaves[lLeafCounter].mMins[1], pMap.mLeaves[lLeafCounter].mMins[2]);
    fprintf(pFile, "\tMax Bounding Box : %f %f %f\n", pMap.mLeaves[lLeafCounter].mMaxs[0], pMap.mLeaves[lLeafCounter].mMaxs[1], pMap.mLeaves[lLeafCounter].mMaxs[2]);
    fprintf(pFile, "\tLeafFace %d\n", pMap.mLeaves[lLeafCounter].mLeafFace);
    fprintf(pFile, "\tNb LeafFace %d\n", pMap.mLeaves[lLeafCounter].mNbLeafFaces);
    fprintf(pFile, "\tLeafBrush %d\n", pMap.mLeaves[lLeafCounter].mLeafBrush);
    fprintf(pFile, "\tNb LeafBrushes %d\n", pMap.mLeaves[lLeafCounter].mNbLeafBrushes);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* LeafFace Lump *********\n");
  for
    (unsigned int lLeafFaceCounter = 0; lLeafFaceCounter < pMap.mLeafFaces.size(); ++lLeafFaceCounter)
  {
    fprintf(pFile, "LeafFace %d\n", lLeafFaceCounter);
    fprintf(pFile, "\tFaceIndex %d\n", pMap.mLeafFaces[lLeafFaceCounter].mFaceIndex);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* LeafBrush Lump *********\n");
  for
    (unsigned int lLeafBrushCounter = 0; lLeafBrushCounter < pMap.mLeafBrushes.size(); ++lLeafBrushCounter)
  {
    fprintf(pFile, "LeafBrush %d\n", lLeafBrushCounter);
    fprintf(pFile, "\tBrushIndex %d\n", pMap.mLeafBrushes[lLeafBrushCounter].mBrushIndex);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* Model Lump *********\n");
  for
    (unsigned int lModelCounter = 0; lModelCounter < pMap.mModels.size(); ++lModelCounter)
  {
    fprintf(pFile, "Model %d\n", lModelCounter);
    fprintf(pFile, "\tMin Bounding Box : %f %f %f\n", pMap.mModels[lModelCounter].mMins[0], pMap.mModels[lModelCounter].mMins[1], pMap.mModels[lModelCounter].mMins[2]);
    fprintf(pFile, "\tMax Bounding Box : %f %f %f\n", pMap.mModels[lModelCounter].mMaxs[0], pMap.mModels[lModelCounter].mMaxs[1], pMap.mModels[lModelCounter].mMaxs[2]);
    fprintf(pFile, "\tFace %d\n", pMap.mModels[lModelCounter].mFace);
    fprintf(pFile, "\tNbFaces %d\n", pMap.mModels[lModelCounter].mNbFaces);
    fprintf(pFile, "\tBrush %d\n", pMap.mModels[lModelCounter].mBrush);
    fprintf(pFile, "\tNbBrushes %d\n", pMap.mModels[lModelCounter].mNBrushes);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* Brush Lump *********\n");
  for
    (unsigned int lBrushCounter = 0; lBrushCounter < pMap.mBrushes.size(); ++lBrushCounter)
  {
    fprintf(pFile, "Brush %d\n", lBrushCounter);
    fprintf(pFile, "\tBrushSide %d\n", pMap.mBrushes[lBrushCounter].mBrushSide);
    fprintf(pFile, "\tNbBrushSides %d\n", pMap.mBrushes[lBrushCounter].mNbBrushSides);
    fprintf(pFile, "\tTextureIndex %d\n", pMap.mBrushes[lBrushCounter].mTextureIndex);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* BrushSide Lump *********\n");
  for
    (unsigned int lBrushSideCounter = 0; lBrushSideCounter < pMap.mBrushSides.size(); ++lBrushSideCounter)
  {
    fprintf(pFile, "BrushSide %d\n", lBrushSideCounter);
    fprintf(pFile, "\tPlaneIndex %d\n", pMap.mBrushSides[lBrushSideCounter].mPlaneIndex);
    fprintf(pFile, "\tTextureIndex %d\n", pMap.mBrushSides[lBrushSideCounter].mTextureIndex);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* Vertex Lump *********\n");
  for
    (unsigned int lVertexCounter = 0; lVertexCounter < pMap.mVertices.size(); ++lVertexCounter)
  {
    fprintf(pFile, "Vertex %d\n", lVertexCounter);
    fprintf(pFile, "\tPosition : %f %f %f\n", pMap.mVertices[lVertexCounter].mPosition[0], pMap.mVertices[lVertexCounter].mPosition[1], pMap.mVertices[lVertexCounter].mPosition[2]);
    fprintf(pFile, "\tTexCoord0 : %f %f\n", pMap.mVertices[lVertexCounter].mTexCoord[0][0], pMap.mVertices[lVertexCounter].mTexCoord[0][1]);
    fprintf(pFile, "\tTexCoord0 : %f %f\n", pMap.mVertices[lVertexCounter].mTexCoord[1][0], pMap.mVertices[lVertexCounter].mTexCoord[1][1]);
    fprintf(pFile, "\tNormal : %f %f %f\n", pMap.mVertices[lVertexCounter].mNormal[0], pMap.mVertices[lVertexCounter].mNormal[1], pMap.mVertices[lVertexCounter].mNormal[2]);
    fprintf(pFile, "\tColor : %d %d %d %d\n", pMap.mVertices[lVertexCounter].mColor[0], pMap.mVertices[lVertexCounter].mColor[1], pMap.mVertices[lVertexCounter].mColor[2], pMap.mVertices[lVertexCounter].mColor[3]);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* MeshVert Lump *********\n");
  for
    (unsigned int lMeshVertCounter = 0; lMeshVertCounter < pMap.mMeshVertices.size(); ++lMeshVertCounter)
  {
    fprintf(pFile, "MeshVert %d\n", lMeshVertCounter);
    fprintf(pFile, "\tVertex Index : %d\n", pMap.mMeshVertices[lMeshVertCounter]);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* Effect Lump *********\n");
  for
    (unsigned int lEffectCounter = 0; lEffectCounter < pMap.mEffects.size(); ++lEffectCounter)
  {
    fprintf(pFile, "Effect %d\n", lEffectCounter);
    fprintf(pFile, "\tName : %s\n", pMap.mEffects[lEffectCounter].mName);
    fprintf(pFile, "\tBrush : %d\n", pMap.mEffects[lEffectCounter].mBrush);
    fprintf(pFile, "\tUnknown : %d\n", pMap.mEffects[lEffectCounter].mUnknown);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* Face Lump *********\n");
  for
    (unsigned int lFaceCounter = 0; lFaceCounter < pMap.mFaces.size(); ++lFaceCounter)
  {
    fprintf(pFile, "Face %d\n", lFaceCounter);
    fprintf(pFile, "\tTextureIndex : %d\n", pMap.mFaces[lFaceCounter].mTextureIndex);
    fprintf(pFile, "\tEffectIndex : %d\n", pMap.mFaces[lFaceCounter].mEffectIndex);
    fprintf(pFile, "\tType : %d\n", pMap.mFaces[lFaceCounter].mType);
    fprintf(pFile, "\tVertex : %d\n", pMap.mFaces[lFaceCounter].mVertex);
    fprintf(pFile, "\tNbVertices : %d\n", pMap.mFaces[lFaceCounter].mNbVertices);
    fprintf(pFile, "\tMeshVertex : %d\n", pMap.mFaces[lFaceCounter].mMeshVertex);
    fprintf(pFile, "\tNbMeshVertices : %d\n", pMap.mFaces[lFaceCounter].mNbMeshVertices);
    fprintf(pFile, "\tLightMapIndex : %d\n", pMap.mFaces[lFaceCounter].mLightmapIndex);
    fprintf(pFile, "\tLightMapCorner : %d %d\n", pMap.mFaces[lFaceCounter].mLightmapCorner[0], pMap.mFaces[lFaceCounter].mLightmapCorner[1]);
    fprintf(pFile, "\tLightmapSize : %d %d\n", pMap.mFaces[lFaceCounter].mLightmapSize[0], pMap.mFaces[lFaceCounter].mLightmapSize[1]);
    fprintf(pFile, "\tLightmapOrigin : %f %f %f\n", pMap.mFaces[lFaceCounter].mLightmapOrigin[0], pMap.mFaces[lFaceCounter].mLightmapOrigin[1], pMap.mFaces[lFaceCounter].mLightmapOrigin[2]);
    fprintf(pFile, "\tLightmapVecs S : %f %f %f\n", pMap.mFaces[lFaceCounter].mLightmapVecs[0][0], pMap.mFaces[lFaceCounter].mLightmapVecs[0][1], pMap.mFaces[lFaceCounter].mLightmapVecs[0][2]);
    fprintf(pFile, "\tLightmapVecs T : %f %f %f\n", pMap.mFaces[lFaceCounter].mLightmapVecs[1][0], pMap.mFaces[lFaceCounter].mLightmapVecs[1][1], pMap.mFaces[lFaceCounter].mLightmapVecs[1][2]);
    fprintf(pFile, "\tNormal : %f %f %f\n", pMap.mFaces[lFaceCounter].mNormal[0], pMap.mFaces[lFaceCounter].mNormal[1], pMap.mFaces[lFaceCounter].mNormal[2]);
    fprintf(pFile, "\tPatchSize : %d %d\n", pMap.mFaces[lFaceCounter].mPatchSize[0], pMap.mFaces[lFaceCounter].mPatchSize[1]);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* LightMap Lump *********\n");
  fprintf(pFile, "NbLightMaps %d\n", pMap.mLightMaps.size());
  fprintf(pFile, "\n");

  fprintf(pFile, "********* LightVol Lump *********\n");
  for
    (unsigned int lLightVolCounter = 0; lLightVolCounter < pMap.mLightVols.size(); ++lLightVolCounter)
  {
    fprintf(pFile, "LightVol %d\n", lLightVolCounter);
    fprintf(pFile, "\tAmbient : %d %d %d\n", pMap.mLightVols[lLightVolCounter].mAmbient[0], pMap.mLightVols[lLightVolCounter].mAmbient[1], pMap.mLightVols[lLightVolCounter].mAmbient[2]);
    fprintf(pFile, "\tDirectional : %d %d %d\n", pMap.mLightVols[lLightVolCounter].mDirectional[0], pMap.mLightVols[lLightVolCounter].mDirectional[1], pMap.mLightVols[lLightVolCounter].mDirectional[2]);
    fprintf(pFile, "\tDir : %d %d\n", pMap.mLightVols[lLightVolCounter].mDir[0], pMap.mLightVols[lLightVolCounter].mDir[1]);
  }
  fprintf(pFile, "\n");

  fprintf(pFile, "********* VisData Lump *********\n");
  fprintf(pFile, "\tNbCluster %d\n", pMap.mVisData.mNbClusters);
  fprintf(pFile, "\tBytePerCluster %d\n", pMap.mVisData.mBytesPerCluster);
  fprintf(pFile, "\n");
}

void flipWindingOrder(TMapQ3& pMap)
{
  for (unsigned i = 0; i < pMap.mMeshVertices.size(); i += 3)
  {
    swap(pMap.mMeshVertices[i], pMap.mMeshVertices[i + 2]);
  }
}

unsigned getNbBezierPatches(TMapQ3& pMap)
{
  unsigned nbPatches = 0;
  for (unsigned i = 0; i < pMap.mFaces.size(); i++)
  {
    TFace& face = pMap.mFaces[i];
    if (face.mType == 2)
    {
      nbPatches += (face.mPatchSize[0] - 1) * (face.mPatchSize[1] - 1) / 4;
    }
  }

  return nbPatches;
}

// add 9 indices for the patch
void addPatchIndices(int baseIndex, int stride, std::vector<TMeshVert>& outIndices)
{
  // for each patch, calculate indexes, relative to face.mVertex
  for (int j = 0; j < 3; j++)
  {
    for (int i = 0; i < 3; i++)
    {
      outIndices.push_back(baseIndex + i + stride * j);
    }
  }
}

// For a 3x3 control points patch (A1 .. A9), check if the 
// group of points (A1, A2, A3), (A4, A5, A6) and (A7, A8, A9)
// form parallel lines.
bool colinearPatchX(const TVertex* patch, unsigned stride)
{
  float v1[3], v2[3];
  const TVertex* p = patch;
  for (int i = 0; i < 3; i++, p += stride)
  {
    sub3(p[1].mPosition, p[0].mPosition, v1);
    sub3(p[2].mPosition, p[1].mPosition, v2);
    if (!parallel3(v1, v2))
    {
      return false;
    }
  }

  p = patch;
  for (int i = 0; i < 2; i++, p += stride)
  {
    sub3(p[1].mPosition, p[0].mPosition, v1);
    sub3(p[stride + 1].mPosition, p[stride].mPosition, v2);
    if (!parallel3(v1, v2))
    {
      return false;
    }
  }

  return true;
}

// For a 3x3 control points patch (A1 .. A9), check if the 
// group of points (A1, A4, A7), (A2, A5, A8) and (A3, A6, A9)
// form parallel lines.
bool colinearPatchY(const TVertex* patch, unsigned stride)
{
  float v1[3], v2[3];
  const TVertex* p = patch;
  for (int i = 0; i < 3; i++, ++p)
  {
    sub3(p[stride].mPosition, p[0].mPosition, v1);
    sub3(p[2*stride].mPosition, p[stride].mPosition, v2);
    if (!parallel3(v1, v2))
    {
      return false;
    }
  }

  p = patch;
  for (int i = 0; i < 2; i++, ++p)
  {
    sub3(p[stride].mPosition, p[0].mPosition, v1);
    sub3(p[stride + 1].mPosition, p[1].mPosition, v2);
    if (!parallel3(v1, v2))
    {
      return false;
    }
  }

  return true;
}

void addPatchTriangles(const TVertex* patch, unsigned stride, int nbVertexPerSide, int baseVertexIndex, std::vector<TVertex>& outVertices, std::vector<TMeshVert>& outIndices)
{
  baseVertexIndex = outVertices.size() - baseVertexIndex;

  unsigned nbVerticesX = nbVertexPerSide;
  if (colinearPatchX(patch, stride))
  {
    nbVerticesX = 2;
  }

  unsigned nbVerticesY = nbVertexPerSide;
  if (colinearPatchY(patch, stride))
  {
    nbVerticesY = 2;
  }

  // add vertices
  uint64_t cacheFill = 0u;
  std::vector<TVertex> cache(nbVerticesX * 3);
  float sx = 1.f / (nbVerticesX - 1);
  float sy = 1.f / (nbVerticesY - 1);
  for (unsigned j = 0; j < nbVerticesY; j++)
  {
    for (unsigned i = 0; i < nbVerticesX; i++)
    {
      TVertex newVert = quadraticBezierSurface(patch, stride, i, sx, j, sy, &cache[0], cacheFill);
      outVertices.push_back(newVert);
    }
  }

  // add indices
  for (unsigned j = 0; j < nbVerticesY - 1; j++)
  {
    for (unsigned i = 0; i < nbVerticesX - 1; i++)
    { // for each cell in the patch, triangulate
      int ix0 = baseVertexIndex + i + j * nbVertexPerSide;
      int ix1 = ix0 + 1;
      int ix2 = ix0 + nbVertexPerSide;
      int ix3 = ix2 + 1;

      // first triangle
      outIndices.push_back(ix0);
      outIndices.push_back(ix3);
      outIndices.push_back(ix1);

      // second triangle
      outIndices.push_back(ix0);
      outIndices.push_back(ix2);
      outIndices.push_back(ix3);
    }
  }
}

void triangulateBezierPatches(TMapQ3& pMap, unsigned int nbVerticesPerPatchSide = 9)
{
  // reserve memory
  unsigned nbPatches = getNbBezierPatches(pMap);
  pMap.mVertices.reserve(pMap.mVertices.size() + nbPatches * nbVerticesPerPatchSide * nbVerticesPerPatchSide);
  pMap.mMeshVertices.reserve(pMap.mMeshVertices.size() + nbPatches * (nbVerticesPerPatchSide - 1) * (nbVerticesPerPatchSide - 1) / 4 * 6);

  for (unsigned i = 0; i < pMap.mFaces.size(); i++)
  {
    TFace& face = pMap.mFaces[i];
    if (face.mType != 2) continue;

    int baseVertex = pMap.mVertices.size();
    int baseMeshVertex = pMap.mMeshVertices.size();

    const int stride = face.mPatchSize[0];
    const int iSize = (face.mPatchSize[0] - 1) / 2;
    const int jSize = (face.mPatchSize[1] - 1) / 2;

    for (int i = 0; i < iSize; i++)
    {
      for (int j = 0; j < jSize; j++)
      { 
        // for each patch of 3x3 control points
        int basePatchIndex = 2 * (i + j * stride); // relative to f.mVertex
        addPatchTriangles(
          &pMap.mVertices[face.mVertex + basePatchIndex], stride, 
          nbVerticesPerPatchSide, baseVertex, pMap.mVertices, pMap.mMeshVertices);
      }
    }

    face.mVertex = baseVertex;
    face.mMeshVertex = baseMeshVertex;
    face.mNbVertices = pMap.mVertices.size() - baseVertex;
    face.mNbMeshVertices = pMap.mMeshVertices.size() - baseMeshVertex;
  }

  pMap.mVertices.resize(pMap.mVertices.size());
  pMap.mMeshVertices.resize(pMap.mMeshVertices.size());
}

void indexBezierPatches(TMapQ3& pMap)
{
  // reserve memory
  unsigned nbPatches = getNbBezierPatches(pMap);
  pMap.mMeshVertices.reserve(pMap.mMeshVertices.size() + nbPatches * 9);

  for (unsigned i = 0; i < pMap.mFaces.size(); i++)
  {
    TFace& face = pMap.mFaces[i];
    if (face.mType != 2) continue;

    int baseMeshVertex = pMap.mMeshVertices.size();

    const int stride = face.mPatchSize[0];
    int iSize = (face.mPatchSize[0] - 1) / 2;
    int jSize = (face.mPatchSize[1] - 1) / 2;

    for (int i = 0; i < iSize; i++)
    {
      for (int j = 0; j < jSize; j++)
      { // for each patch of 3x3 control points
        int basePatchIndex = 2 * (i + j * stride); // relative to f.mVertex
        addPatchIndices(basePatchIndex, stride, pMap.mMeshVertices);
      }
    }

    face.mMeshVertex = baseMeshVertex;
    face.mNbMeshVertices = pMap.mMeshVertices.size() - baseMeshVertex;
  }
}

/**
 * Read the map.
 *
 * @param pFilename  The filename of the map to read.
 *
 * @return true if the loading successed, false otherwise.
 */
bool readMap(std::istream& bspData, TMapQ3& pMap, float scale, unsigned postProcessSteps)
{

  // Read the header.
  if (!readHeader(bspData, pMap))
  {
    printf("readMap :: Invalid Q3 map header.\n");
    return false;
  }

  bool coordSysOpenGL = ((postProcessSteps & PostProcess_CoordSysOpenGL) != 0);

  // Read the entity lump.
  readEntity(bspData, pMap);

  // Read the texture lump.
  readTexture(bspData, pMap);

  // Read the plane lump.
  readPlane(bspData, pMap, scale, coordSysOpenGL);

  // Read the node lump.
  readNode(bspData, pMap, scale, coordSysOpenGL);

  // Read the leaf lump.
  readLeaf(bspData, pMap, scale, coordSysOpenGL);

  // Read the leaf face lump.
  readLeafFace(bspData, pMap);

  // Read the leaf brush lump.
  readLeafBrush(bspData, pMap);

  // Read the model lump.
  readModel(bspData, pMap, scale, coordSysOpenGL);

  // Read the brush lump.
  readBrush(bspData, pMap);

  // Read the brushside lump.
  readBrushSide(bspData, pMap);

  // Read the vertex lump.
  readVertex(bspData, pMap, scale, coordSysOpenGL);

  // Read the meshvert lump.
  readMeshVert(bspData, pMap);

  // Read the effect lump.
  readEffect(bspData, pMap);

  // Read the face lump.
  readFace(bspData, pMap, scale, coordSysOpenGL);

  // Read the lightmap lump.
  readLightMap(bspData, pMap);

  // Read the lightvol lump.
  readLightVol(bspData, pMap);

  // read the visdata lump.
  readVisData(bspData, pMap);

  if (postProcessSteps & PostProcess_TriangulateBezierPatches)
  {
    triangulateBezierPatches(pMap);
  }
  else if (postProcessSteps & PostProcess_IndexBezierPatches)
  {
    indexBezierPatches(pMap);
  }

  if (postProcessSteps & PostProcess_FlipWindingOrder)
  {
    flipWindingOrder(pMap);
  }

  return true;
};


