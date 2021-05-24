#include "PhysPatch.h"
#include "GrassManager.h"


PhysPatch::PhysPatch(GrassPatch* a_pGrassPatch)
{
   m_pBasePatch = a_pGrassPatch;
   m_pD3DDevice = a_pGrassPatch->GetD3DDevicePtr();
   m_pD3DDeviceCtx = a_pGrassPatch->GetD3DDeviceCtxPtr();
   m_dwVertexStride[0] = sizeof(VertexPhysData);
   m_dwVertexStride[1] = sizeof(VertexAnimData);
   m_dwVertexOffset = 0;

   this->numBlades = a_pGrassPatch->VerticesCount();
   this->bladePhysData = new BladePhysData[numBlades];
   GenerateBuffer();
}


void PhysPatch::GenerateBuffer(void)
{
   D3D11_BUFFER_DESC bufferDesc =
   {
      numBlades * sizeof(VertexPhysData),
      D3D11_USAGE_DYNAMIC,
      D3D11_BIND_VERTEX_BUFFER,
      D3D11_CPU_ACCESS_WRITE,
      0
   };

   m_pD3DDevice->CreateBuffer(&bufferDesc, NULL, &m_pPhysVertexBuffer);
   bufferDesc.ByteWidth = numBlades * sizeof(VertexAnimData);
   m_pD3DDevice->CreateBuffer(&bufferDesc, NULL, &m_pAnimVertexBuffer);
}


void PhysPatch::IASetPhysVertexBuffer0(void)
{
   m_pD3DDeviceCtx->IASetVertexBuffers(0, 1, &m_pPhysVertexBuffer, m_dwVertexStride, &m_dwVertexOffset);
}


void PhysPatch::IASetAnimVertexBuffer0(void)
{
   m_pD3DDeviceCtx->IASetVertexBuffers(0, 1, &m_pAnimVertexBuffer, m_dwVertexStride + 1, &m_dwVertexOffset);
}


DWORD PhysPatch::AnimVerticesCount(void)
{
   return m_dwVerticesCount[1];
}

DWORD PhysPatch::PhysVerticesCount(void)
{
   return m_dwVerticesCount[0];
}


PhysPatch::~PhysPatch(void)
{
   delete[] bladePhysData;
   SAFE_RELEASE(m_pAnimVertexBuffer);
   SAFE_RELEASE(m_pPhysVertexBuffer);
}


void PhysPatch::Reinit(void)
{
   for (UINT i = 0; i < numBlades; i++)
   {
      BladePhysData* bp = &bladePhysData[i];

      bp->startPosition = PosToWorld(XMLoadFloat3(&m_pBasePatch->m_pVertices[i].vPos));

      bp->startDirection = XMLoadFloat3(&m_pBasePatch->m_pVertices[i].vRotAxe) * 0.01745f;
      bp->startDirectionY = XMLoadFloat3(&m_pBasePatch->m_pVertices[i].vYRotAxe) * 0.01745f;

      for (UINT j = 0; j < NUM_SEGMENTS; j++)
      {
         bp->w[j] = create(0, 0, 0);
         bp->g[j] = create(0, 0, 0);
         bp->R[j] = MakeIdentityMatrix();;
         *(bp->T + j) = XMMatrixMultiply(MakeRotationMatrix(bp->startDirectionY), MakeRotationMatrix(bp->startDirection));

         bp->NeedPhysics = 0;
         bp->brokenFlag = 0;
         bp->brokenTime = 0.0f;
      }

      bp->A[0] = bp->T[0];
      bp->position[0] = bp->startPosition;
      bp->grabbedMeshIndex = -1;
   }
}

void PhysPatch::TransferFromOtherLod(const PhysPatch& a_PhysPatch, bool a_bLod0ToLod1)
{
   /* just 2 cases: transfer data from lod0 to lod1 and from lod1 to lod0*/
   UINT i, j;
   DWORD dwBladeX;
   DWORD dwBladeZ;
   DWORD dwBaseVertIndex;
   DWORD dwStartVertIndex = 0;
   /* 1st case: lod0 => lod1 */
   if (a_bLod0ToLod1)
   {
      /* Each 16 vertices in a_PhysPatch representing 16 grass blades in 4x4 block */
      for (dwBaseVertIndex = 0; dwBaseVertIndex < a_PhysPatch.numBlades; dwBaseVertIndex += 16)
      {
         dwBladeZ = 0;
         dwBladeX = 0;
         for (i = 0; i < 2; ++i)
            for (j = 0; j < 2; ++j)
            {
               if (a_PhysPatch.bladePhysData[dwBaseVertIndex + i * 4 + j].fTransparency >
                  a_PhysPatch.bladePhysData[dwBaseVertIndex + dwBladeZ * 4 + dwBladeX].fTransparency)
               {
                  dwBladeZ = i;
                  dwBladeX = j;
               }
            }
         bladePhysData[dwStartVertIndex++] = a_PhysPatch.bladePhysData[dwBaseVertIndex + dwBladeZ * 4 + dwBladeX];

         dwBladeZ = 2;
         dwBladeX = 0;
         for (i = 2; i < 4; ++i)
            for (j = 0; j < 2; ++j)
            {
               if (a_PhysPatch.bladePhysData[dwBaseVertIndex + i * 4 + j].fTransparency >
                  a_PhysPatch.bladePhysData[dwBaseVertIndex + dwBladeZ * 4 + dwBladeX].fTransparency)
               {
                  dwBladeZ = i;
                  dwBladeX = j;
               }
            }
         bladePhysData[dwStartVertIndex++] = a_PhysPatch.bladePhysData[dwBaseVertIndex + dwBladeZ * 4 + dwBladeX];

         dwBladeZ = 0;
         dwBladeX = 2;
         for (i = 0; i < 2; ++i)
            for (j = 2; j < 4; ++j)
            {
               if (a_PhysPatch.bladePhysData[dwBaseVertIndex + i * 4 + j].fTransparency >
                  a_PhysPatch.bladePhysData[dwBaseVertIndex + dwBladeZ * 4 + dwBladeX].fTransparency)
               {
                  dwBladeZ = i;
                  dwBladeX = j;
               }
            }
         bladePhysData[dwStartVertIndex++] = a_PhysPatch.bladePhysData[dwBaseVertIndex + dwBladeZ * 4 + dwBladeX];

         dwBladeZ = 2;
         dwBladeX = 2;
         for (i = 2; i < 4; ++i)
            for (j = 2; j < 4; ++j)
            {
               if (a_PhysPatch.bladePhysData[dwBaseVertIndex + i * 4 + j].fTransparency >
                  a_PhysPatch.bladePhysData[dwBaseVertIndex + dwBladeZ * 4 + dwBladeX].fTransparency)
               {
                  dwBladeZ = i;
                  dwBladeX = j;
               }
            }
         bladePhysData[dwStartVertIndex++] = a_PhysPatch.bladePhysData[dwBaseVertIndex + dwBladeZ * 4 + dwBladeX];
      }
   }
   else
   {
      /* fuck, that was just 1st case... so, now from lod1 => lod0 */
      for (dwBaseVertIndex = 0; dwBaseVertIndex < a_PhysPatch.numBlades; dwBaseVertIndex++)
      {
         for (i = 0; i < 4; ++i)
         {
            memcpy(bladePhysData + dwStartVertIndex, a_PhysPatch.bladePhysData + dwBaseVertIndex, sizeof(BladePhysData));
            BladePhysData* bp = &bladePhysData[dwStartVertIndex];

            bp->position[0] = bp->startPosition = PosToWorld(XMLoadFloat3(&m_pBasePatch->m_pVertices[dwBaseVertIndex * 4 + i].vPos));
            bp->startDirection = XMLoadFloat3(&m_pBasePatch->m_pVertices[dwBaseVertIndex * 4 + i].vRotAxe) * (float)PI / 180.0f;
            bp->startDirectionY = XMLoadFloat3(&m_pBasePatch->m_pVertices[dwBaseVertIndex * 4 + i].vYRotAxe) * (float)PI / 180.0f;
            bp->fTransparency = m_pBasePatch->m_pVertices[dwBaseVertIndex * 4 + i].fTransparency;
            dwStartVertIndex++;
         }
      }
   }
}


static float maxvphi = 0.0;
void PhysPatch::UpdateBuffer(void)
{
   /* copy data */
   DWORD i, k;
   BladePhysData* bp;
   VertexPhysData* vpd;
   VertexAnimData* vad;
   /* updating buffer */
   D3D11_MAPPED_SUBRESOURCE pPhysVertices;
   D3D11_MAPPED_SUBRESOURCE pAnimVertices;
   m_dwVerticesCount[0] = m_dwVerticesCount[1] = 0;

   m_pD3DDeviceCtx->Map(m_pPhysVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &pPhysVertices);
   m_pD3DDeviceCtx->Map(m_pAnimVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &pAnimVertices);

   vpd = (VertexPhysData*)pPhysVertices.pData;
   vad = (VertexAnimData*)pAnimVertices.pData;
   for (i = 0; i < numBlades; i++)
   {
      bp = bladePhysData + i;

      if (bp->NeedPhysics)
      {
         for (k = 1; k < NUM_SEGMENTS; k++)
         {
            XMStoreFloat4x4(&vpd->R[k - 1], bp->T[k]);
         }

         XMStoreFloat3(&vpd->Position, bp->startPosition);
         vpd->fTransparency = m_pBasePatch->m_pVertices[i].fTransparency;
         vpd->vColor = m_pBasePatch->m_pVertices[i].vColor;
         m_dwVerticesCount[0]++;
         vpd++;
      }
      else
      {
         XMStoreFloat3(&vad->vPos, bp->startPosition);
         vad->vRotAxe = m_pBasePatch->m_pVertices[i].vRotAxe;
         vad->vYRotAxe = m_pBasePatch->m_pVertices[i].vYRotAxe;
         vad->fTransparency = m_pBasePatch->m_pVertices[i].fTransparency;
         vad->vColor = m_pBasePatch->m_pVertices[i].vColor;
         m_dwVerticesCount[1]++;
         vad++;
      }
   }

   m_pD3DDeviceCtx->Unmap(m_pPhysVertexBuffer, 0);
   m_pD3DDeviceCtx->Unmap(m_pAnimVertexBuffer, 0);
}


void PhysPatch::SetTransform (const XMFLOAT4X4* a_pMtx)
{
   m_pTransform = a_pMtx;
}


float3 PhysPatch::PosToWorld (const float3& v)
{
   XM_TO_M(*m_pTransform, tr);
   return XMVector3TransformCoord(v, tr);
}


//controlled
float PhysPatch::hardness;
float PhysPatch::mass;
float PhysPatch::windStrength;
float PhysPatch::fTerrRadius;
float PhysPatch::fWindTexTile;
const WindData* PhysPatch::pWindData = NULL;
const AirData* PhysPatch::pAirData = NULL;
float PhysPatch::fHeightScale = 0;
const TerrainHeightData* PhysPatch::pHeightData = NULL;

//non controlled
float PhysPatch::gravity = 10.0f;
float PhysPatch::dampfing = 0.98f;
float PhysPatch::maxAngle = (float)PI / 2.0;
float PhysPatch::maxBrokenTime = 10.0f;
bool  PhysPatch::transmitSpringMomentDownwards = false;
float PhysPatch::eTime = 0.0f;


void PhysPatch::RotateSegment(PhysPatch::BladePhysData* bp, int j, const float3& psi, const GrassPropsUnified& props) {};


void RotateSegment(PhysPatch::BladePhysData* bp, int j, const float3& psi, const GrassPropsUnified& props)
{
   bp->R[j] = XMMatrixMultiply(bp->R[j], MakeRotationMatrix(psi));
   bp->T[j] = XMMatrixMultiply(bp->T[j - 1], bp->R[j]);

   float3x3 transposed_t;
   float3 pos_delta, axis = create(0.0, bp->segmentHeight, 0.0);
   transposed_t = XMMatrixTranspose(bp->T[j]);
   pos_delta = XMVector3TransformCoord(axis, transposed_t);
   bp->position[j] = bp->position[j - 1] + pos_delta;
}


float3 GetDw(float3x3& T, float3x3& R, float3& fw, float3& sum, float3& w, float invJ, float segmentHeight, float Hardness, int j)
{
   float3 localSum, g, m_f;
   float3 halfAxis = create(0.0f, segmentHeight * 0.5f, 0.0f);

   localSum = XMVector3TransformCoord(sum, T);
   m_f = XMVector3Cross(halfAxis, localSum);

   g = Hardness * MakeRotationVector(R);
   return (m_f - g) * invJ;
}


void CalcTR(float3x3& Tres, float3x3& Rres, float3x3& T, float3x3& T_1, float3x3& R, float3& psi)
{
   Rres = XMMatrixMultiply(R, MakeRotationMatrix(psi));
   Tres = XMMatrixMultiply(T_1, Rres);
}


void BrokenAnim(float gTime, PhysPatch::BladePhysData* bp, const GrassPropsUnified& props)
{
   float3 proj, dir, axis;
   float3 vY = create(0.0f, 1.0f, 0.0f);

   // Rotate in plane between blade and its projection on the ground
   dir = bp->position[1] - bp->position[0];

   axis = XMVector3Cross(dir, vY);
   axis = XMVector3Normalize(axis);

   // Calculate angle of rotation
   float phase = gety(bp->startDirectionY);

   axis *= sin(20.f * phase + gTime * 0.1f) * 0.005f;
   float3x3 mR = MakeRotationMatrix(axis);
   bp->T[1] = XMMatrixMultiply(mR, bp->T[1]);
}


void PhysPatch::Broken(float3& vNormal, float3& dir, PhysPatch::BladePhysData* bp, float fDist, const GrassPropsUnified& props)
{
   float3 vx, dir1;
   XMMATRIX mRot, mRotX, mRotY, mRotXY, mRotZ;
   float a[3] = { 0.95f, 0.9f, 0.8f };

   float3 cur_pos = create(0.0f, 0.0f, 0.0f);

   int uS = rand() % 2;
   if (uS == 0) uS = -1;
   if (bp->brokenFlag == 2) uS = 1;
   if (bp->brokenFlag == 3) uS = -1;

   float3 pos_delta;
   XMMATRIX transposed_t;
   float2 vTexCoord;
   float3 axis = create(0.0, bp->segmentHeight, 0.0);

   if (bp->brokenFlag > 1)
   {
      float YRot = 640.f;
      int uR = 320;//uR = 150;

      bp->R[1] = XMMatrixIdentity();
      bp->T[1] = XMMatrixIdentity();

      dir1 = XMVector3Cross(vNormal, dir);
      dir = XMVector3Cross(vNormal, dir1);
      dir = XMVector3Normalize(dir);

      float XRot = fDist * 1.2f;
      bp->T[1] = MakeRotationMatrix(dir * XRot * uS);
      transposed_t = XMMatrixTranspose(bp->T[0]);
      bp->R[1] = XMMatrixMultiply(transposed_t, bp->T[1]);

      transposed_t = XMMatrixTranspose(bp->T[1]);
      pos_delta = XMVector3TransformCoord(axis, transposed_t);
      bp->position[1] = bp->position[0] + pos_delta;
      bp->w[1] = create(0, 0, 0);

      // Update positions
      for (int j = 2; j < NUM_SEGMENTS; j++)
      {
         bp->T[j] = XMMatrixMultiply(bp->T[j - 1], bp->R[j]);

         transposed_t = XMMatrixTranspose(bp->T[j]);
         pos_delta = XMVector3TransformCoord(axis, transposed_t);
         bp->position[j] = bp->position[j - 1] + pos_delta;
      }
      return;
   }

   for (int j = 1; j < NUM_SEGMENTS; j++)
   {
      int uR = rand() % 128;//64;
      float YRot = 640.f;
      if (bp->brokenFlag > 1) uR = 320;//uR = 150;
      mRotY = XMMatrixRotationY((float)(uS * uR) * (float)M_PI / YRot);
      dir1 = XMVector3TransformCoord(dir, mRotY);
      vx = XMVector3Cross(dir1, vNormal);

      dir1 = XMVector3Cross(vNormal, vx);
      mRot = XMMatrixLookAtLH((cur_pos), (cur_pos + dir1), vNormal);

      float XRot = 0.8f;
      if (j > 1)XRot = 0.99f;
      if (bp->brokenFlag > 1) XRot = fDist * a[j - 1];
      mRotX = XMMatrixRotationX(-(float)M_PI * 0.5f * XRot);
      bp->T[j] = XMMatrixMultiply(mRot, mRotX);

      transposed_t = XMMatrixTranspose(bp->T[j]);
      pos_delta = XMVector3TransformCoord(axis, transposed_t);
      bp->position[j] = bp->position[j - 1] + pos_delta;

      vTexCoord = create(getx(bp->position[j]) / fTerrRadius * 0.5f + 0.5f, getz(bp->position[j]) / fTerrRadius * 0.5f + 0.5f);
      float height = pHeightData->GetHeight(getx(vTexCoord), gety(vTexCoord)) * fHeightScale;

      float angle = 0.0f;
      if (gety(bp->position[j]) > height + 0.2f)
         angle = -asinf(clamp((gety(bp->position[j]) - height - 0.2f) / bp->segmentHeight, 0, 1));
      else if (gety(bp->position[j]) < height)
         angle = asinf(clamp((height + 0.5f - gety(bp->position[j])) / bp->segmentHeight, 0, 1));

      if (angle != 0.0f)
      {
         mRotX = XMMatrixRotationX(angle);
         bp->T[j] = XMMatrixMultiply(bp->T[j], mRotX);

         // Update position
         transposed_t = XMMatrixTranspose(bp->T[j]);
         pos_delta = XMVector3TransformCoord(axis, transposed_t);
         bp->position[j] = bp->position[j - 1] + pos_delta;
      }
   }
}


void PhysPatch::Animatin(PhysPatch::BladePhysData* bp, float2& vTexCoord, Mesh* a_pMeshes[], const GrassPropsUnified& props)
{
   float3 g = create(0.0f, -9.8f, 0.0f);
   float3 halfAxis = create(0.0f, bp->segmentHeight * 0.5f, 0.0f);
   float3 axis = create(0.0, bp->segmentHeight, 0.0);
   float3 sum = create(0.0f, 0.0f, 0.0f), localSum;

   float3 w, w_;
   for (int j = 1; j < NUM_SEGMENTS; j++)
   {
      bp->w[j] = create(0, 0, 0);
      bp->T[j] = bp->T[j - 1];
      for (int k = 0; k < 4; k++)
      {
         sum = g * getcoord(props.vMassSegment, j - 1);
         localSum = XMVector3TransformCoord(sum, bp->T[j]);
         float3 G;
         G = XMVector3Cross(halfAxis, localSum);
         bp->R[j] = MakeRotationMatrix(G / getcoord(props.vHardnessSegment, j - 1));
         bp->T[j] = XMMatrixMultiply(bp->T[j - 1], bp->R[j]);
      }

      w_ = pWindData->GetWindValueA(vTexCoord, fWindTexTile, 40, j - 1);
         //pAirData->GetAirValue(vTexCoord);//create(0, 0, 0, 0);// pWindData->GetWindValueA(vTexCoord, fWindTexTile, windStrength, j - 1);
      sum = g * getcoord(props.vMassSegment, j - 1);
      localSum = XMVector3TransformCoord(sum, bp->T[j]);
      float3 G;
      G = XMVector3Cross(halfAxis, localSum);
      w = XMVector3TransformCoord(w_, bp->T[j]);
      G += w;
      bp->R[j] = MakeRotationMatrix(G / getcoord(props.vHardnessSegment, j - 1));
      bp->T[j] = XMMatrixMultiply(bp->T[j - 1], bp->R[j]);

      float3x3 transposed_t;
      transposed_t = XMMatrixTranspose(bp->T[j]);
      float3 pos_delta;
      pos_delta = XMVector3TransformCoord(axis, transposed_t);
      bp->position[j] = bp->position[j - 1] + pos_delta;

      if (a_pMeshes[0]->CheckCollision(bp->position[j - 1], bp->position[j], NULL))
      {
         float3 psi;
         if (a_pMeshes[0]->Collide(&psi, bp->position[j - 1], bp->position[j], bp, j))
         {
            psi = XMVector3TransformCoord(psi, bp->T[j]);
            RotateSegment(bp, j, psi, props);
            for (int i = j + 1; i < NUM_SEGMENTS; i++)
               bp->T[i] = XMMatrixMultiply(bp->T[i - 1], bp->R[i]);

            bp->A[j] = bp->T[j];
         }
         bp->NeedPhysics = 2;
         bp->physicTime = 0;
      }
   }
}

static float3 GetVel(float3x3& T, float3& w, float segmentHeight)
{
   float3 halfAxis = create(0.0f, segmentHeight * 0.5f, 0.0f), r;
   float3x3 transposed_t;
   transposed_t = XMMatrixTranspose(T);
   r = XMVector3Cross(w, halfAxis);
   r = XMVector3TransformCoord(r, transposed_t);
   return r;
}

void Phisics(PhysPatch::BladePhysData* bp, float3& w, float dTime, Mesh* a_pMeshes[], const GrassPropsUnified& props)
{
   float3 g = create(0.0f, -9.8f, 0.0f);
   float3 halfAxis = create(0.0f, bp->segmentHeight * 0.5f, 0.0f);
   float3 axis = create(0.0, bp->segmentHeight, 0.0);
   float3 sum = create(0.0f, 0.0f, 0.0f);
   float3 psi;
   float d = powf(0.98f, dTime * 0.01f);
   if (d > 0.9998f) d = 0.9998f;
   int start_segment = 1;
   if (bp->brokenFlag > 1)
      start_segment = 2;

   float3 wind, vZ, v, r;
   vZ = create(0, 0, 0);
   for (int j = start_segment; j < NUM_SEGMENTS; j++)
   {
      float oneThirdMMulLSqr = 0.33f * getcoord(props.vMassSegment, j - 1) * bp->segmentHeight * bp->segmentHeight;
      float invJ = 0.75f * 1.0f / oneThirdMMulLSqr;
      r = GetVel(bp->T[j], bp->w[j], bp->segmentHeight);
      v = vZ + r;
      wind = 0.02f * (w - v);

      float h = getcoord(props.vHardnessSegment, j - 1);
      // sum = g * props.vMassSegment[j-1] * bp->T[j][5] + w;
      sum = g * getcoord(props.vMassSegment, j - 1) + wind;

      float3 Dw = GetDw(bp->T[j], bp->R[j], bp->w[j], sum, wind, invJ, bp->segmentHeight, h, j);
      float3 w_ = bp->w[j] + dTime * Dw;
      psi = dTime * w_;

      float3x3 Tres, Rres;
      CalcTR(Tres, Rres, bp->T[j], bp->T[j - 1], bp->R[j], psi);

      r = GetVel(Tres, w_, bp->segmentHeight);
      v = vZ + r;
      wind = 0.02f * (w - v);
      //      sum = g * props.vMassSegment[j-1] * Tres[5] + w;
      sum = g * getcoord(props.vMassSegment, j - 1) + wind;
      float3 Dw_ = GetDw(Tres, Rres, w_, sum, wind, invJ, bp->segmentHeight, h, j);
      bp->w[j] += 0.5f * dTime * (Dw + Dw_);
      bp->w[j] *= d;
      psi = dTime * bp->w[j];
      RotateSegment(bp, j, psi, props);

      r = GetVel(bp->T[j], bp->w[j], bp->segmentHeight);
      vZ = vZ + 2.f * r;

      if (a_pMeshes[0]->Collide(&psi, bp->position[j - 1], bp->position[j], bp, j))
      {
         psi = XMVector3TransformCoord(psi, bp->T[j]);
         RotateSegment(bp, j, psi, props);
         bp->w[j] = create(0, 0, 0);

         for (int i = j + 1; i < NUM_SEGMENTS; i++)
         {
            bp->T[i] = XMMatrixMultiply(bp->T[i - 1], bp->R[i]);
         }
         for (int i = 1; i < NUM_SEGMENTS; i++)
            bp->w[i] = create(0, 0, 0);

      }
   }
}



static float gTime = 0.f;
void PhysPatch::UpdatePhysics(const float3& viewPos, float physLodDst, bool collision, float3 spherePosition, float sphereRadius, float dTime, const std::vector<GrassPropsUnified>& grassProps, const IndexMapData& indexMapData, Mesh* a_pMeshes[], UINT a_uNumMeshes)
{
   float3 w = create(0.0f, 0.0f, 0.0f);
   float a[3] = { (float)M_PI / 2.5f, (float)M_PI / 10.0f, (float)M_PI / 10.0f };
   float3 g = create(0.0f, -9.8f, 0.0f);
   float AnimOrPhys = physLodDst;
   //      TerrainHeightData *pHD = m_pTerrain->HeightDataPtr();

   gTime += dTime;
   if (dTime >= 0.1f)
      dTime = 0.1f;

   if (dTime <= 0.001f)
      dTime = 0.001f;

   float3 sum, localSum;
   float3 psi;
   float3 vDir = spherePosition - viewPos;
   vDir = vDir * create(1.0f, 0.0f, 1.0f, 1.0f);
   AnimOrPhys = XMVectorGetX(XMVector3Length(vDir));
   vDir = XMVector3Normalize(vDir);

   for (DWORD i = 0; i < numBlades; i++)
   {
      BladePhysData* bp = &bladePhysData[i];
      float3 vDirP = bp->startPosition - viewPos;

      //texcoord
      float2 vTexCoord = create(getx(bp->startPosition) / fTerrRadius * 0.5f + 0.5f, getz(bp->startPosition) / fTerrRadius * 0.5f + 0.5f);

      //index map
      UINT uX = UINT(getx(vTexCoord) * (indexMapData.uWidth - 1));
      UINT uY = UINT(gety(vTexCoord) * (indexMapData.uHeight - 1));
      UCHAR subTypeIndex = 0;
      if (indexMapData.pData)
         subTypeIndex = indexMapData.pData[uX + uY * indexMapData.uWidth];
      const GrassPropsUnified& props = grassProps[subTypeIndex];

      // height map
      float height = pHeightData->GetHeight(getx(vTexCoord), gety(vTexCoord)) * fHeightScale;
      sety(bp->startPosition, height);
      bp->position[0] = bp->startPosition;

      /*********************
      Calculations
      *********************/

      bool near_car = false;
      bp->segmentHeight = gety(props.vSizes);
      bp->segmentWidth = getx(props.vSizes);

      float r = sphereRadius + 0.12f;
      float br = sphereRadius + NUM_SEGMENTS * bp->segmentHeight;


      if (XMVectorGetX(XMVector3Length(bp->position[0] - spherePosition)) < 0.7f * br)
         near_car = true;

      if (bp->brokenFlag == -1)
      {
         BrokenAnim(gTime, bp, props);
         bp->NeedPhysics = 1;
         continue;
      }
      if (false) /*if (near_car)*/ //crash bellow if true, no time to realize why
      {
         float3 vNormal = create(0, 0, 0);
         float3 vDist = create(0, 0, 0); 
         int OldbrokenFlag = bp->brokenFlag;

         bp->brokenFlag = a_pMeshes[0]->IsBottom(bp->position[0], vDist);

         //TODO: getNormal -> XMVECTOR
         XMFLOAT3 v3 = pHeightData->GetNormal(getx(vTexCoord), gety(vTexCoord));
         vNormal = XMLoadFloat3(&v3);
         if (bp->brokenFlag > 0)
         {
            float3 dir = a_pMeshes[0]->GetMoveDir();
            Broken(vNormal, dir, bp, getx(vDist), props);
            bp->NeedPhysics = 1;
            if (bp->brokenFlag == 1) bp->brokenFlag = -1;
            if (bp->brokenFlag > 1)
               bp->NeedPhysics = 2;
         }
         else
         {
            if (OldbrokenFlag > 0)
            {
               if (OldbrokenFlag == 1)
               {
                  bp->brokenFlag = -1;
                  bp->NeedPhysics = 1;
                  continue;
               }
               else
               {
                  bp->NeedPhysics = 2;
               }
            }
         }
         {
            if (bp->brokenFlag == 0 && bp->NeedPhysics != 2)
            {
               Animatin(bp, vTexCoord, a_pMeshes, props);
            }
         }
      }
      if (bp->NeedPhysics == 2)
      {
         float3 w = pAirData->GetAirValue(vTexCoord);

         Phisics(bp, w, dTime, a_pMeshes, props);
      }
   }//for (DWORD i = 0; i < numBlades; i++)

   UpdateBuffer();
}