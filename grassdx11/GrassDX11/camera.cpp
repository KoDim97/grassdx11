#include "camera.h"

void LandscapeCamera::FrameMove(FLOAT fElapsedTime)
{
   if (DXUTGetGlobalTimer()->IsStopped()) {
      if (DXUTGetFPS() == 0.0f) fElapsedTime = 0;
      else fElapsedTime = 1.0f / DXUTGetFPS();
   }

   if (IsKeyDown(m_aKeys[CAM_RESET]))
      Reset();

   // Get keyboard/mouse/gamepad input
   GetInput(m_bEnablePositionMovement, (m_nActiveButtonMask & m_nCurrentButtonMask) || m_bRotateWithoutButtonDown, true);

   //// Get the mouse movement (if any) if the mouse button are down
   //if( (m_nActiveButtonMask & m_nCurrentButtonMask) || m_bRotateWithoutButtonDown )
   //    UpdateMouseDelta( fElapsedTime );

   // Get amount of velocity based on the keyboard input and drag (if any)
   UpdateVelocity(fElapsedTime);

   // Simple euler method to calculate position delta
   XMVECTOR mVelocity = XMLoadFloat3(&m_vVelocity);
   XMVECTOR vPosDelta = mVelocity * fElapsedTime;

   // If rotating the camera 
   if ((m_nActiveButtonMask & m_nCurrentButtonMask) ||
      m_bRotateWithoutButtonDown ||
      m_vGamePadRightThumb.x != 0 ||
      m_vGamePadRightThumb.z != 0)
   {
      // Update the pitch & yaw angle based on mouse movement
      float fYawDelta = m_vRotVelocity.x;
      float fPitchDelta = m_vRotVelocity.y;

      // Invert pitch if requested
      if (m_bInvertPitch)
         fPitchDelta = -fPitchDelta;

      m_fCameraPitchAngle += fPitchDelta;
      m_fCameraYawAngle += fYawDelta;

      // Limit pitch to straight up or straight down
      m_fCameraPitchAngle = __max(-XM_PI / 2.0f, m_fCameraPitchAngle);
      m_fCameraPitchAngle = __min(+XM_PI / 2.0f, m_fCameraPitchAngle);
   }

   // Make a rotation matrix based on the camera's yaw & pitch
   XMMATRIX mCameraRot;
   mCameraRot= XMMatrixRotationRollPitchYaw(m_fCameraPitchAngle, m_fCameraYawAngle, 0);

   // Transform vectors based on camera's rotation matrix
   XMVECTOR vWorldUp, vWorldAhead;

   XMFLOAT3 xmfLocalUp(0, 1, 0);
   XMVECTOR vLocalUp = XMLoadFloat3(&xmfLocalUp);

   XMFLOAT3 xmfLocalAhead(0, 0, 1);
   XMVECTOR vLocalAhead = XMLoadFloat3(&xmfLocalAhead);

   vWorldUp = XMVector3TransformCoord(vLocalUp, mCameraRot);
   vWorldAhead = XMVector3TransformCoord(vLocalAhead, mCameraRot);
   
   // Transform the position delta by the camera's rotation 
   XMVECTOR vPosDeltaWorld;
   if (!m_bEnableYAxisMovement)
   {
      // If restricting Y movement, do not include pitch
      // when transforming position delta vector.
      mCameraRot = XMMatrixRotationRollPitchYaw(0, m_fCameraYawAngle, 0);
   }
   vPosDeltaWorld = XMVector3TransformCoord(vPosDelta, mCameraRot);

   // Move the eye position 
   XMVECTOR mEye = XMLoadFloat3(&m_vEye);

   mEye += vPosDeltaWorld;
   if (m_bClipToBoundary)
      ConstrainToBoundary(mEye);

   XMStoreFloat3(&m_vEye, mEye);

   // Correct camera position by landscape height
   TerrainHeightData* pHD = m_pTerrain->HeightDataPtr();
   XMFLOAT2 vTexCoord = XMFLOAT2(m_vEye.x / m_fGrassRadius * 0.5f + 0.5f, m_vEye.z / m_fGrassRadius * 0.5f + 0.5f);

   float terrain_height = pHD->GetHeight(vTexCoord.x, vTexCoord.y) * m_fHeightScale;
   if (m_vEye.y < terrain_height + 1)
      m_vEye.y = terrain_height + 1;

   // Update the lookAt position based on the eye position 
   XMVECTOR mLookAt = XMLoadFloat3(&m_vLookAt);
   mLookAt = mEye + vWorldAhead;
   XMStoreFloat3(&m_vLookAt, mLookAt);

   // Update the view matrix
   XMMATRIX mView = XMMatrixLookAtLH(mEye, mLookAt, vWorldUp);
   XMStoreFloat4x4(&m_mView, mView);

   XMMATRIX mCameraWorld = XMMatrixInverse(NULL , mView);
   XMStoreFloat4x4(&m_mCameraWorld, mCameraWorld);
}


void HeightCamera::FrameMove (FLOAT fElapsedTime)
{
   if (DXUTGetGlobalTimer()->IsStopped())
   {
      if (DXUTGetFPS() == 0.0f)
         fElapsedTime = 0;
      else
         fElapsedTime = 1.0f / DXUTGetFPS();
   }

   if (IsKeyDown(m_aKeys[CAM_RESET]))
      Reset();

   // Change camera height
   if (IsKeyDown(m_aKeys[CAM_MOVE_UP]))
   {
      m_fDefaultHeight += 0.1f;
      if (m_fDefaultHeight > m_pMinMaxHeight.second)
         m_fDefaultHeight = m_pMinMaxHeight.second;
   }
   if (IsKeyDown(m_aKeys[CAM_MOVE_DOWN]))
   {
      m_fDefaultHeight -= 0.1f;
      if (m_fDefaultHeight < m_pMinMaxHeight.first)
         m_fDefaultHeight = m_pMinMaxHeight.first;
   }

   // Get keyboard/mouse/gamepad input
   GetInput(m_bEnablePositionMovement, (m_nActiveButtonMask & m_nCurrentButtonMask) || m_bRotateWithoutButtonDown, true);

   // Get amount of velocity based on the keyboard input and drag (if any)
   UpdateVelocity(fElapsedTime);

   // Simple euler method to calculate position delta
   XMVECTOR mVelocity = XMLoadFloat3(&m_vVelocity);
   XMVECTOR vPosDelta = mVelocity * fElapsedTime;

   // If rotating the camera 
   if ((m_nActiveButtonMask & m_nCurrentButtonMask) ||
      m_bRotateWithoutButtonDown ||
      m_vGamePadRightThumb.x != 0 ||
      m_vGamePadRightThumb.z != 0)
   {
      // Update the pitch & yaw angle based on mouse movement
      float fYawDelta = m_vRotVelocity.x;
      float fPitchDelta = m_vRotVelocity.y;

      // Invert pitch if requested
      if (m_bInvertPitch)
         fPitchDelta = -fPitchDelta;

      //m_fCameraPitchAngle += fPitchDelta;
      m_fCameraYawAngle += fYawDelta;

      // Limit pitch to straight up or straight down
      m_fCameraPitchAngle = __max(-PI / 2.0f, m_fCameraPitchAngle);
      m_fCameraPitchAngle = __min(+PI / 2.0f, m_fCameraPitchAngle);
   }

   // Make a rotation matrix based on the camera's yaw & pitch
   XMMATRIX mCameraRot = XMMatrixRotationRollPitchYaw(m_fCameraPitchAngle, m_fCameraYawAngle, 0);;
   

   // Transform vectors based on camera's rotation matrix
   XMVECTOR vWorldUp, vWorldAhead;
   XMVECTOR vLocalUp = create(0, 1, 0);
   XMVECTOR vLocalAhead = create(0, 0, 1);
   vWorldUp = XMVector3TransformCoord(vLocalUp, mCameraRot);
   vWorldAhead = XMVector3TransformCoord(vLocalAhead, mCameraRot);

   // Transform the position delta by the camera's rotation 
   XMVECTOR vPosDeltaWorld;
   if (!m_bEnableYAxisMovement)
   {
      // If restricting Y movement, do not include pitch
      // when transforming position delta vector.
      mCameraRot = XMMatrixRotationRollPitchYaw(0, m_fCameraYawAngle, 0);
   }
   vPosDeltaWorld = XMVector3TransformCoord(vPosDelta, mCameraRot);

   // Move the eye position 
   XMVECTOR eye = XMLoadFloat3(&m_vEye);
   eye += vPosDeltaWorld;
   if (m_bClipToBoundary)
      ConstrainToBoundary(eye);

   // Correct camera position by landscape height
   TerrainHeightData* pHD = m_pTerrain->HeightDataPtr();
   float2 vTexCoord = create(getx(eye) / m_fGrassRadius * 0.5f + 0.5f, getz(eye) / m_fGrassRadius * 0.5f + 0.5f);

   float terrain_height = pHD->GetHeight(getx(vTexCoord), gety(vTexCoord)) * m_fHeightScale;
   sety(eye, m_fDefaultHeight + terrain_height);
   if (gety(eye) < terrain_height + m_pMinMaxHeight.first)
      sety(eye, terrain_height + m_pMinMaxHeight.first);

   // Update the lookAt position based on the eye position 
   XMVECTOR lookat = eye + vWorldAhead;
   XMStoreFloat3(&m_vLookAt, lookat);

   // Update the view matrix
   XMMATRIX mv = XMMatrixLookAtLH(eye, lookat, vWorldUp);
   XMStoreFloat4x4(&m_mView, mv);
   XMStoreFloat3(&m_vEye, eye);

   mv = XMMatrixInverse(NULL, mv);
   XMStoreFloat4x4(&m_mCameraWorld, mv);
}

void MeshCamera::FrameMove(FLOAT fElapsedTime)
{
   if (DXUTGetGlobalTimer()->IsStopped())
   {
      if (DXUTGetFPS() == 0.0f)
         fElapsedTime = 0;
      else
         fElapsedTime = 1.0f / DXUTGetFPS();
   }

   if (IsKeyDown(m_aKeys[CAM_RESET]))
      Reset();

   // Change camera height
   if (IsKeyDown(m_aKeys[CAM_MOVE_UP]))
   {
      m_fDefaultHeight += 0.1f;
      if (m_fDefaultHeight > m_pMinMaxHeight.second)
         m_fDefaultHeight = m_pMinMaxHeight.second;
   }
   if (IsKeyDown(m_aKeys[CAM_MOVE_DOWN]))
   {
      m_fDefaultHeight -= 0.1f;
      if (m_fDefaultHeight < m_pMinMaxHeight.first)
         m_fDefaultHeight = m_pMinMaxHeight.first;
   }


   // Change mesh dist
   if (m_nMouseWheelDelta != m_nOldWheelMouseDelta)
   {
      m_fMeshDist -= 0.2f * ((m_nOldWheelMouseDelta - m_nMouseWheelDelta) / WHEEL_DELTA);
      if (m_fMeshDist < m_pMinMaxMeshDist.first)
         m_fMeshDist = m_pMinMaxMeshDist.first;
      if (m_fMeshDist > m_pMinMaxMeshDist.second)
         m_fMeshDist = m_pMinMaxMeshDist.second;
      m_nOldWheelMouseDelta = m_nMouseWheelDelta;
   }

   // Get position delta
   XMVECTOR mesh_pos_and_radius;
   XMVECTOR mesh_pos, mesh_dir;
   float mesh_radius;//, len;
   
   auto pos = m_pMesh->GetPosAndRadius();
   mesh_pos_and_radius = XMLoadFloat4(&pos);
   mesh_pos = mesh_pos_and_radius;
   mesh_radius = getw(mesh_pos_and_radius);

   XM_TO_V(m_vLookDir, ld, 3);
   if (length(ld) < 0.00001f)
      ld = m_pMesh->GetMoveDir();
   else
      ld += m_pMesh->GetMoveDir() * 0.5f;
   ld = normalize(ld);
   XMVECTOR vEye = mesh_pos - ld * m_fMeshDist;

   // Correct camera position by landscape height
   TerrainHeightData* pHD = m_pTerrain->HeightDataPtr();
   float2 vTexCoord = create(m_vEye.x / m_fGrassRadius * 0.5f + 0.5f, m_vEye.z / m_fGrassRadius * 0.5f + 0.5f);

   float terrain_height = pHD->GetHeight(getx(vTexCoord), gety(vTexCoord)) * m_fHeightScale;

   sety(vEye,__max(m_fDefaultHeight + terrain_height, gety(mesh_pos) + 0.5f));
   if (m_vEye.y < terrain_height + m_pMinMaxHeight.first)
      m_vEye.y = terrain_height + m_pMinMaxHeight.first;

   XMVECTOR la = mesh_pos;
   sety(la, gety(la) + 1.2f);

   XMMATRIX v = XMMatrixLookAtLH(vEye, la, create(0, 1, 0));

   XMStoreFloat4x4(&m_mView, v);
   v = XMMatrixInverse(NULL, v);
   XMStoreFloat4x4(&m_mCameraWorld, v);

   XMStoreFloat3(&m_vEye, vEye);
   XMStoreFloat3(&m_vLookDir, ld);
   XMStoreFloat3(&m_vLookAt, la);
}

float MeshCamera::GetMeshDist (void)
{
   return m_fMeshDist;
}