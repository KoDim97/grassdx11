#include <string>

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"

#include "DXUTcamera.h"

#include "main.h"

#include "camera.h"
#include "StateManager.h"

#include "CopterController.h"
#include "Copter.h"

#include "Car.h"

#pragma warning( disable : 4100 )

using namespace DirectX;

/* Camera types */
#define CAMERA_NORMAL 0
#define CAMERA_TERRAIN 1
#define CAMERA_MESH 2
#define CAMERA_COPTER 3

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

CFirstPersonCamera          *g_Camera;               // A model viewing camera

CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = nullptr;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls

Copter                    *copter;
CopterController           copterController;

int  GetCameraType         (void);
void ToggleToMeshCamera    (void);
void ToggleToTerrainCamera (void);
void ToggleToNormalCamera  (void);
void InitCarMesh           (void);

bool isDbgUiRendered = false;

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#ifdef _DEBUG
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );

    InitApp();

    DXUTInit( true, true, nullptr ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"GrassDX11" );

    // Only require 10-level hardware, change to D3D_FEATURE_LEVEL_11_0 to require 11-class hardware
    // Switch to D3D_FEATURE_LEVEL_9_x for 10level9 hardware
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, g_windowWidth, g_windowHeight );

    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


void InitMeshes (ID3D11Device* dev, ID3D11DeviceContext* devcon)
{
   XMFLOAT4 vPosAndRad;

   vPosAndRad.x = 0.0f;
   vPosAndRad.z = 0.0f;
   vPosAndRad.w = 1.2f;
   vPosAndRad.y = 0.0f;

   g_fNumOfMeshes = 0;

   //ID3D10Device* pd3dDevice = DXUTGetD3D10Device();
   //float height_scale, grass_radius;
   //Terrain * const terr = g_pGrassField->GetTerrain(&height_scale, &grass_radius);
   //
   //g_pMeshes[0] = new Car(dev, devcon, g_pGrassField->SceneEffect(),
   //    create(0, 0, 0, 0),
   //    terr, height_scale, grass_radius,
   //    g_fCarFrontWidth, g_fCarHeight, g_fCarLength, 0.0f);
   //g_fNumOfMeshes = 1;

}



//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
   g_SettingsDlg.Init( &g_DialogResourceManager );
   g_HUD.Init( &g_DialogResourceManager );
   g_SampleUI.Init( &g_DialogResourceManager );

   g_HUD.SetCallback( OnGUIEvent );
   int iY = 30;
   int iYo = 26;
   
   g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 25, iY += iYo, 125, 22, VK_F2);

   WCHAR sStr[MAX_PATH];
   
   swprintf_s(sStr, MAX_PATH, L"Cam speed scale: %.4f", g_fCameraSpeed);
   g_HUD.AddStatic(IDC_CAM_SPEED_SCALE_LABEL, sStr, 20, iY += iYo, 180, 22);
   g_HUD.AddSlider(IDC_CAM_SPEED_SCALE_SLYDER, 20, iY += iYo, 185, 22, 0, 1000, (int)(g_fCameraSpeed * 1000));

   swprintf_s(sStr, MAX_PATH, L"Wind Strength: %.4f", g_fWindStrength);
   g_HUD.AddStatic(IDC_GRASS_WIND_LABEL, sStr, 20, iY += iYo, 180, 22);
   g_HUD.AddSlider(IDC_GRASS_WIND_FORCE_SLYDER, 20, iY += iYo, 185, 22, 0, 10000, (int)(g_fWindStrength * 10000));

   swprintf_s(sStr, MAX_PATH, L"Flow Strength: %.4f", g_fMaxFlowStrength);
   g_HUD.AddStatic(IDC_GRASS_MAX_FLOW_STRENGTH_LABEL, sStr, 20, iY += iYo, 180, 22);
   g_HUD.AddSlider(IDC_GRASS_MAX_FLOW_STRENGTH_SLYDER, 20, iY += iYo, 185, 22, 0, 10000, (int)(g_fMaxFlowStrength * 10000));

   swprintf_s(sStr, MAX_PATH, L"Fan radius: %.4f", g_fFanRadius / 2);
   g_HUD.AddStatic(IDC_FAN_RADIUS_LABEL, sStr, 20, iY += iYo, 180, 22);
   g_HUD.AddSlider(IDC_FAN_RADIUS_SLYDER, 20, iY += iYo, 185, 22, 0, 1000, (int)(g_fFanRadius * 1000));

   swprintf_s(sStr, MAX_PATH, L"Copter scale: %.4f", g_fCopterScale);
   g_HUD.AddStatic(IDC_COPTER_SCALE_LABEL, sStr, 20, iY += iYo, 180, 22);
   g_HUD.AddSlider(IDC_COPTER_SCALE_SLYDER, 20, iY += iYo, 185, 22, 0, 1000, (int)(g_fCopterScale * 10000));

   swprintf_s(sStr, MAX_PATH, L"Angle speed: %.4f", g_fAngleSpeed);
   g_HUD.AddStatic(IDC_FAN_ANGLE_SPEED_LABEL, sStr, 20, iY += iYo, 180, 22);
   g_HUD.AddSlider(IDC_FAN_ANGLE_SPEED_SLYDER, 20, iY += iYo, 185, 22, 0, 100, (int)(g_fAngleSpeed * 100));

   g_HUD.AddButton(IDC_TOGGLE_WIREFRAME, L"Toggle wire-frame (F4)", 25, iY += iYo, 125, 22, VK_F4);
   g_HUD.AddButton(IDC_TOGGLE_RENDERING_GRASS, L"Toggle rendering-grass (F5)", 25, iY += iYo, 125, 22, VK_F5);
   
   g_HUD.AddButton(IDC_TOGGLE_RENDERING_DBG_WIN, L"Toggle rendering-dbg win (F6)", 25, iY += iYo, 125, 22, VK_F6);
   g_HUD.AddButton(IDC_TOGGLE_DBG_WIN_SLICE, L"Toggle dbg win slice (F7)", 25, iY += iYo, 125, 22, VK_F7);
   g_HUD.AddButton(IDC_FIX_CAMERA, L"Fix cam (F8)", 25, iY += iYo, 125, 22, VK_F8);

   
   swprintf_s(sStr, MAX_PATH, L"Diffuse: (%.2f,%.2f,%.2f)", g_vTerrRGB.x, g_vTerrRGB.y, g_vTerrRGB.z);
   g_HUD.AddStatic(IDC_TERR_RGB_LABEL, sStr, 20, iY += iYo, 140, 22);
   g_HUD.AddSlider(IDC_TERR_R_SLYDER, 20, iY += iYo, 135, 22, 0, 100, (int)(g_vTerrRGB.x * 100));
   g_HUD.AddSlider(IDC_TERR_G_SLYDER, 20, iY += iYo, 135, 22, 0, 100, (int)(g_vTerrRGB.y * 100));
   g_HUD.AddSlider(IDC_TERR_B_SLYDER, 20, iY += iYo, 135, 22, 0, 100, (int)(g_vTerrRGB.z * 100));

   swprintf_s(sStr, MAX_PATH, L"Dir: (%.2f,%.2f,%.2f)", g_vDir.x, g_vDir.y, g_vDir.z);
   g_HUD.AddStatic(IDC_FLOW_DIR_LABEL, sStr, 20, iY += iYo, 140, 22);
   g_HUD.AddSlider(IDC_FLOW_DIR_X_SLYDER, 20, iY += iYo, 135, 22, -100, 100, (int)(g_vDir.x * 100));
   g_HUD.AddSlider(IDC_FLOW_DIR_Y_SLYDER, 20, iY += iYo, 135, 22, -100, 100, (int)(g_vDir.y * 100));
   g_HUD.AddSlider(IDC_FLOW_DIR_Z_SLYDER, 20, iY += iYo, 135, 22, -100, 100, (int)(g_vDir.z * 100));

   CDXUTComboBox* pComboBox = NULL;
   g_HUD.AddStatic(IDC_STATIC, L"Camera Type", 20, iY += iYo, 105, 25);
   g_HUD.AddComboBox(IDC_CAMERA_TYPE, 20, iY += iYo, 140, 24, VK_F4, false, &pComboBox);
   if (pComboBox)
   {
      pComboBox->SetDropHeight(40);
      pComboBox->AddItem(L"Normal  camera", IntToPtr(CAMERA_NORMAL));
      pComboBox->AddItem(L"Terrain camera", IntToPtr(CAMERA_TERRAIN));
      pComboBox->AddItem(L"Mesh    camera", IntToPtr(CAMERA_MESH));
      pComboBox->AddItem(L"Copter  camera", IntToPtr(CAMERA_COPTER));
   }

   g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text.
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( Colors::Yellow );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

   XMVECTOR vEye = g_Camera->GetEyePt();
   XMFLOAT3 eye;
   XMStoreFloat3(&eye, vEye);

   XMVECTOR vLookAt = g_Camera->GetLookAtPt();
   XMFLOAT3 lookAt;
   XMStoreFloat3(&lookAt, vLookAt);

   WCHAR eyeStr[100];
   swprintf(eyeStr, sizeof(eyeStr), L"Eye: X = %f, Y = %f, Z = % f", eye.x, eye.y, eye.z);

   WCHAR lookAtStr[100];
   swprintf(lookAtStr, sizeof(lookAtStr), L"Look to: X = %f, Y = %f, Z = % f", lookAt.x - eye.x, lookAt.y - eye.y, lookAt.z - eye.z);

   g_pTxtHelper->DrawTextLine(eyeStr);
   g_pTxtHelper->DrawTextLine(lookAtStr);

   g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output,
                                       const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
   return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------

ID3D11DepthStencilState* g_depthStencilStateDisabled;
ID3D11DepthStencilState* g_depthStencilStateEnabled;
XMMATRIX m_orthoMatrix;

HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
   HRESULT hr;
   auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();

   D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc;
   D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
   
   // Create an orthographic projection matrix for 2D rendering.
   m_orthoMatrix = DirectX::XMMatrixOrthographicLH((float)g_windowWidth, (float)g_windowHeight, 0.1, 1000.0);
   
   // Now create a second depth stencil state which turns off the Z buffer for 2D rendering.  The only difference is 
   // that DepthEnable is set to false, all other parameters are the same as the other depth stencil state.
   depthDisabledStencilDesc.DepthEnable = false;
   depthDisabledStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
   depthDisabledStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
   depthDisabledStencilDesc.StencilEnable = true;
   depthDisabledStencilDesc.StencilReadMask = 0xFF;
   depthDisabledStencilDesc.StencilWriteMask = 0xFF;
   depthDisabledStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
   depthDisabledStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
   depthDisabledStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
   depthDisabledStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
   depthDisabledStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
   depthDisabledStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
   depthDisabledStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
   depthDisabledStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
   // Create the state using the device.
   hr = pd3dDevice->CreateDepthStencilState(&depthDisabledStencilDesc, &g_depthStencilStateDisabled);
   if (FAILED(hr))
   {
      return false;
   }
   
   // Clear the second depth stencil state before setting the parameters.
   ZeroMemory(&depthDisabledStencilDesc, sizeof(depthDisabledStencilDesc));
   
   // Set up the description of the stencil state.
   depthStencilDesc.DepthEnable = true;
   depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
   depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
   
   depthStencilDesc.StencilEnable = true;
   depthStencilDesc.StencilReadMask = 0xFF;
   depthStencilDesc.StencilWriteMask = 0xFF;
   
   // Stencil operations if pixel is front-facing.
   depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
   depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
   depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
   depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
   
   // Stencil operations if pixel is back-facing.
   depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
   depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
   depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
   depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
   
   // Create the depth stencil state.
   hr = pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &g_depthStencilStateEnabled);
   if (FAILED(hr))
   {
      return false;
   }
   
   // Set the depth stencil state.
   pd3dImmediateContext->OMSetDepthStencilState(g_depthStencilStateEnabled, 1);


    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_SettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

   // Load camera parameters
   std::ifstream InFile;
   
   InFile.open("config/camera.ini");
   InFile >> g_fCameraHeight;
   InFile >> g_fCameraHeightMin;
   InFile >> g_fCameraHeightMax;
   InFile >> g_fCameraMeshDist;
   InFile >> g_fCameraMeshDistMin;
   InFile >> g_fCameraMeshDistMax;
   InFile.close();
   InFile.clear();

   // Create rasterizer states
   GetGlobalStateManager().SetDevice(pd3dDevice, pd3dImmediateContext); 
   
   D3D11_RASTERIZER_DESC CurrentRasterizerState;
   CurrentRasterizerState.FillMode = D3D11_FILL_SOLID;
   CurrentRasterizerState.CullMode = D3D11_CULL_NONE;
   CurrentRasterizerState.FrontCounterClockwise = true;
   CurrentRasterizerState.DepthBias = false;
   CurrentRasterizerState.DepthBiasClamp = 0;
   CurrentRasterizerState.SlopeScaledDepthBias = 0;
   CurrentRasterizerState.DepthClipEnable = true;
   CurrentRasterizerState.ScissorEnable = false;
   CurrentRasterizerState.MultisampleEnable = true;
   CurrentRasterizerState.AntialiasedLineEnable = false;
   GetGlobalStateManager().AddRasterizerState("EnableMSAA", CurrentRasterizerState);
   
   CurrentRasterizerState.CullMode = D3D11_CULL_FRONT;
   GetGlobalStateManager().AddRasterizerState("EnableMSAACulling", CurrentRasterizerState);
   
   CurrentRasterizerState.CullMode = D3D11_CULL_NONE;
   CurrentRasterizerState.FillMode = D3D11_FILL_WIREFRAME;
   GetGlobalStateManager().AddRasterizerState("EnableMSAA_Wire", CurrentRasterizerState);
   
   CurrentRasterizerState.CullMode = D3D11_CULL_FRONT;
   CurrentRasterizerState.FillMode = D3D11_FILL_WIREFRAME;
   GetGlobalStateManager().AddRasterizerState("EnableMSAACulling_Wire", CurrentRasterizerState);
   
   // Create grass field
   g_GrassInitState.InitState[0].fMaxQuality = 0.0f;//0.7f;
   g_GrassInitState.InitState[0].dwBladesPerPatchSide = 20;
   g_GrassInitState.InitState[0].dwPatchesPerSide = 37;//40;//43;//45;//32;//50;
   g_GrassInitState.InitState[0].fMostDetailedDist = 2.0f;//* g_fMeter;
   g_GrassInitState.InitState[0].fLastDetailedDist = 140.0;//85;//150.0f;// * g_fMeter;
   g_GrassInitState.InitState[0].fGrassRadius = 140.0;//85;//150.0f;// * g_fMeter;
   g_GrassInitState.InitState[0].pD3DDevice = pd3dDevice;
   g_GrassInitState.InitState[0].pD3DDeviceCtx = pd3dImmediateContext;
   
   g_GrassInitState.InitState[0].uNumCollidedPatchesPerMesh = 10;
   g_GrassInitState.InitState[0].uMaxColliders = MAX_NUM_MESHES;
   
   g_GrassInitState.InitState[1] = g_GrassInitState.InitState[0];
   g_GrassInitState.InitState[2] = g_GrassInitState.InitState[0];
   //some differences...
   g_GrassInitState.InitState[0].sLowGrassTexPath = L"resources/LowGrass.dds";
   g_GrassInitState.InitState[0].sIndexTexPath = L"resources/IndexType1.dds";
   g_GrassInitState.InitState[1].sIndexTexPath = L"resources/IndexType2.dds";
   g_GrassInitState.InitState[2].sIndexTexPath = L"resources/IndexType3.dds";
   //g_GrassInitState.InitState[1].dwBladesPerPatchSide       = 2;
   g_GrassInitState.InitState[2].dwBladesPerPatchSide = 2;
   g_GrassInitState.InitState[0].sTexPaths.push_back(L"resources/GrassType1.dds");
   g_GrassInitState.InitState[0].sTexPaths.push_back(L"resources/GrassType1_1.dds");
   //g_GrassInitState.InitState[1].sTexPaths.push_back(L"resources/GrassType2.dds");
   g_GrassInitState.InitState[1].sTexPaths.push_back(L"resources/GrassType2_1.dds");
   g_GrassInitState.InitState[2].sTexPaths.push_back(L"resources/GrassType3.dds");
   g_GrassInitState.InitState[2].sTopTexPaths.push_back(L"resources/Top1.dds");
   g_GrassInitState.InitState[2].sTopTexPaths.push_back(L"resources/Top2.dds");
   g_GrassInitState.InitState[2].sTopTexPaths.push_back(L"resources/Top3.dds");
   g_GrassInitState.InitState[2].sTopTexPaths.push_back(L"resources/Top4.dds");
   g_GrassInitState.InitState[2].sTopTexPaths.push_back(L"resources/Top5.dds");
   g_GrassInitState.InitState[2].sTopTexPaths.push_back(L"resources/Top6.dds");
   g_GrassInitState.InitState[2].sTopTexPaths.push_back(L"resources/Top7.dds");
   g_GrassInitState.InitState[0].sEffectPath = L"Shaders/GrassType1.fx";
   g_GrassInitState.InitState[1].sEffectPath = L"Shaders/GrassType2.fx";
   g_GrassInitState.InitState[2].sEffectPath = L"Shaders/GrassType3.fx";
   g_GrassInitState.InitState[0].sSeatingTexPath = L"resources/SeatingType1.dds";
   //g_GrassInitState.InitState[1].sSeatingTexPath = L"resources/SeatingType2.dds";
   g_GrassInitState.InitState[2].sSeatingTexPath = L"resources/SeatingType3.dds";
   
   g_GrassInitState.InitState[0].sSubTypesPath = L"config/T1SubTypes.cfg";
   g_GrassInitState.InitState[1].sSubTypesPath = L"config/T2SubTypes.cfg";
   g_GrassInitState.InitState[2].sSubTypesPath = L"config/T3SubTypes.cfg";
   g_GrassInitState.InitState[0].fCameraMeshDist = g_fCameraMeshDistMax;
   g_GrassInitState.InitState[1].fCameraMeshDist = g_fCameraMeshDistMax;
   g_GrassInitState.InitState[2].fCameraMeshDist = g_fCameraMeshDistMax;
   g_GrassInitState.sSceneEffectPath = L"Shaders/SceneEffect.fx";
   g_GrassInitState.sNoiseMapPath = L"resources/Noise.dds";
   g_GrassInitState.sGrassOnTerrainTexturePath = L"resources/g.dds";
   g_GrassInitState.fHeightScale = g_fHeightScale;
   g_GrassInitState.fTerrRadius = 400.0f;
   g_pGrassField = new GrassFieldManager(g_GrassInitState);
   g_pTerrTile = g_pGrassField->SceneEffect()->GetVariableByName("g_fTerrTile")->AsScalar();
   
   g_pSkyBoxESRV = g_pGrassField->SceneEffect()->GetVariableByName("g_txSkyBox")->AsShaderResource();
   g_pSkyboxTechnique = g_pGrassField->SceneEffect()->GetTechniqueByName("RenderSkyBox");
   g_pSkyViewProjEMV = g_pGrassField->SceneEffect()->GetVariableByName("g_mViewProj")->AsMatrix();

   // Define our scene vertex layout
   const D3D11_INPUT_ELEMENT_DESC SkyBoxLayout[] =
   {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
   };

   int iNumElements = sizeof(SkyBoxLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC);
   D3DX11_PASS_DESC PassDesc;
   ID3DX11EffectPass* pPass;
   g_pSkyboxPass = g_pSkyboxTechnique->GetPassByIndex(0);
   g_pSkyboxPass->GetDesc(&PassDesc);
   V_RETURN(pd3dDevice->CreateInputLayout(SkyBoxLayout, iNumElements, PassDesc.pIAInputSignature,
      PassDesc.IAInputSignatureSize, &g_pSkyVertexLayout));
   g_MeshSkybox.Create(pd3dDevice, L"resources\\skysphere.sdkmesh");
   g_pSkyBoxESRV->SetResource(nullptr);

   /*Loading colors*/
   InFile.open("config/colors.ini");
   InFile >> g_vFogColor.x >> g_vFogColor.y >> g_vFogColor.z;
   InFile >> g_vTerrRGB.x >> g_vTerrRGB.y >> g_vTerrRGB.z;
   InFile >> g_vGrassSpecular.x >> g_vGrassSpecular.y >> g_vGrassSpecular.z;
   InFile.close();

   /****************/
   XM_TO_V(g_vTerrRGB, vTerrRGB, 3);
   XM_TO_V(g_vGrassSpecular, vGrassSpecular, 4);
   XM_TO_V(g_vFogColor, vFogColor, 4);

   g_pGrassField->SetTerrRGB(vTerrRGB);
   g_pTerrTile->SetFloat(g_fTerrTile);
   g_pGrassField->SetWindMapTile(6);
   //g_pGrassField->SetWindMapTile(4);
   g_pGrassField->SetSegMass(g_fMass);
   g_pGrassField->SetHardness(g_fHardness);
   g_pGrassField->SetGrassLodBias(g_fGrassLodBias);
   g_pGrassField->SetSubScatterGamma(g_fGrassSubScatterGamma);
   g_pGrassField->SetGrassAmbient(g_fGrassAmbient);
   g_pGrassField->SetWindStrength(g_fWindStrength);
   g_pGrassField->SetWindSpeed(g_fWindTexSpeed);
   g_pGrassField->SetQuality(g_fQuality);
   XMVECTOR lgd = create(0.29 * 2, 1, 0.08, 2);
   g_pGrassField->SetLowGrassDiffuse(lgd);
   g_pGrassField->SetFogColor(vFogColor);
   g_pGrassField->SetWindBias(g_fWindBias);
   g_pGrassField->SetWindScale(g_fWindScale);
   
   // Setup the camera's view parameters
   float height_scale, grass_radius;
   Terrain* const terr = g_pGrassField->GetTerrain(&height_scale, &grass_radius);
   
   // Setup global flow parameters (some of them unused, just for debug)
   g_pGrassField->GetFlowManager()->SetMaxHorizFlow(g_fMaxFlowStrength);
   g_pGrassField->GetFlowManager()->SetShift(0.005f);
   g_pGrassField->GetFlowManager()->m_pAxesFanFlow->SetHeightMap(terr->HeightMapSRV());
   g_pGrassField->GetFlowManager()->m_pAxesFanFlow->SetHeightScale(g_fHeightScale);
   //InitMeshes(pd3dDevice);
   
   g_Camera = new LandscapeCamera(g_fCameraHeight, terr, height_scale, grass_radius);
   copterController.SetupCamera(g_Camera);
   copterController.InitHeightCtrl(terr, grass_radius, height_scale);

   g_Camera->SetViewParams(XMLoadFloat3(&g_vCameraEyeStart), XMLoadFloat3(&g_vCameraAtStart));
   g_Camera->SetScalers(0.01f, g_fCameraSpeed /* g_fMeter*/);
   
   copter = new Copter(pd3dDevice, pd3dImmediateContext, g_pGrassField->m_pSceneEffect, g_pGrassField->GetFlowManager());
   copterController.SetOwner(copter);

   InitCarMesh();

   //g_dbgWin = new DebugWindow(pd3dDevice, g_windowWidth, g_windowHeight, g_pGrassField->GetWind()->GetMap(), 10);
   //g_dbgWin = new DebugWindow(pd3dDevice, g_windowWidth, g_windowHeight, g_pGrassField->m_pShadowMapping->m_pSRV, 0.1 / 4);
   //g_dbgWin = new DebugWindow(pd3dDevice, g_windowWidth, g_windowHeight, g_pGrassField->m_pSceneTex->GetShaderResourceView(), 0.5);
   //g_dbgWin = new DebugWindow(pd3dDevice, g_windowWidth, g_windowHeight, terr->HeightMapSRV(), 1);
   g_dbgWin = new DebugWindow(pd3dDevice, g_windowWidth, g_windowHeight, g_pGrassField->m_pMixer->m_shaderResourceView, 1);
   //g_dbgWin = new DebugWindow(pd3dDevice, g_windowWidth, g_windowHeight, g_pGrassField->GetFlowManager()->m_pAxesFanFlow->m_shaderResourceView, 1);
   //g_dbgWin->ToggleRender();


   return S_OK;
}



//
static XMFLOAT4 pos(18.0f, 0.0f, 8.0f, 0.0f);
static XMFLOAT3 vCarDir(0.0f, 0.0f, 1.0f);
static float fFi = 0.0f, fTimeEmul = 0.0f;
static float fTimeRot[5] = { 3.0f, 4.0f, 10.0f, 11.0f, 1000.0f };
static float fF[5] = { 0.0f, 0.0001f, 0.0f, -0.0001f, 0.0f }, fw = 0.0f;
static int Indx = 0;

void UpdateMeshes (float a_fElapsedTime)
{
  // if (GetCameraType() == CAMERA_MESH)
   {
     
      // Rotate car
      float d = powf(0.98f, a_fElapsedTime * 10.0f);
      if (d > 0.98f) d = 0.98f;
      if (fabs(g_fCarRotAccel) > 0.0001)
      {
         g_fCarRotVel += g_fCarRotAccel * a_fElapsedTime;
         g_fCarRotVel *= d;
         if (g_fCarRotVel > 7.0f * g_fCarVelocity * g_fCarRotMaxVel) 
            g_fCarRotVel = 7.0f * g_fCarVelocity * g_fCarRotMaxVel;
         if (g_fCarRotVel < 7.0f * g_fCarVelocity * g_fCarRotMinVel) 
            g_fCarRotVel = 7.0f * g_fCarVelocity * g_fCarRotMinVel;
      }
      else
      {
         if (g_fCarRotVel > 0.0001)
         {
            g_fCarRotVel -= g_fCarRotForce * a_fElapsedTime;
            g_fCarRotVel *= d;
            if (g_fCarRotVel < 0.0f) g_fCarRotVel = 0.0f;
         } 
         else
            if (g_fCarRotVel < 0.0001)
            {
               g_fCarRotVel += g_fCarRotForce * a_fElapsedTime;
               g_fCarRotVel *= d;
               if (g_fCarRotVel > 0.0f) g_fCarRotVel = 0.0f;
            }
      }
      float3x3 mrot;
      mrot = XMMatrixRotationY(g_fCarRotVel);
      g_vCarDir = XMVector3TransformCoord(g_vCarDir, mrot);

      sety(g_vCarDir, 0.0);
      g_vCarDir = normalize(g_vCarDir);

      // Update car velocity
      g_fCarVelocity += g_fCarAccel;
      if (g_fCarVelocity < g_fCarMinVelocity)
         g_fCarVelocity = g_fCarMinVelocity;
      else if (g_fCarVelocity > g_fCarMaxVelocity)
         g_fCarVelocity = g_fCarMaxVelocity;

      // Update car pos
      for (UINT i = 0; i < g_fNumOfMeshes; i++)
      {
         pos.x += getx(g_vCarDir) * g_fCarVelocity;
         pos.z += getz(g_vCarDir) * g_fCarVelocity;
         g_pMeshes[i]->SetPosAndRadius(pos);
      }

      g_fCarVelocity *= 0.99;
   }
}



//--------------------------------------------------------------------------------------
// Update the MSAA sample count combo box for this format
//--------------------------------------------------------------------------------------
void UpdateMSAASampleCounts(ID3D11Device* pd3dDevice, DXGI_FORMAT fmt)
{
   CDXUTComboBox* pComboBox = NULL;
   bool bResetSampleCount = false;
   UINT iHighestSampleCount = 0;

   pComboBox = g_HUD.GetComboBox(IDC_SAMPLE_COUNT);
   if (!pComboBox)
      return;

   pComboBox->RemoveAllItems();

   WCHAR val[10];
   for (UINT i = 1; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; i++)
   {
      UINT Quality;
      if (SUCCEEDED(pd3dDevice->CheckMultisampleQualityLevels(fmt, i, &Quality)) &&
         Quality > 0)
      {
         swprintf_s(val, 10, L"%d", i);
         pComboBox->AddItem(val, IntToPtr(i));
         iHighestSampleCount = i;
      }
      else if (g_MSAASampleCount == i)
      {
         bResetSampleCount = true;
      }
   }

   if (bResetSampleCount)
      g_MSAASampleCount = iHighestSampleCount;

   pComboBox->SetSelectedByData(IntToPtr(g_MSAASampleCount));
}



HRESULT CreateRenderTarget(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceCtx, UINT uiWidth, UINT uiHeight, UINT uiSampleCount,
   UINT uiSampleQuality)
{
   HRESULT hr = S_OK;

   SAFE_RELEASE(g_pRenderTarget);
   SAFE_RELEASE(g_pRTRV);
   SAFE_RELEASE(g_pDSTarget);
   SAFE_RELEASE(g_pDSRV);

   ID3D11RenderTargetView* pOrigRT = NULL;
   ID3D11DepthStencilView* pOrigDS = NULL;
   pd3dDeviceCtx->OMGetRenderTargets(1, &pOrigRT, &pOrigDS);

   D3D11_RENDER_TARGET_VIEW_DESC DescRTV;
   pOrigRT->GetDesc(&DescRTV);
   SAFE_RELEASE(pOrigRT);
   SAFE_RELEASE(pOrigDS);

   D3D11_TEXTURE2D_DESC dstex;
   dstex.Width = uiWidth;
   dstex.Height = uiHeight;
   dstex.MipLevels = 1;
   dstex.Format = DescRTV.Format;
   dstex.SampleDesc.Count = uiSampleCount;
   dstex.SampleDesc.Quality = uiSampleQuality;
   dstex.Usage = D3D11_USAGE_DEFAULT;
   dstex.BindFlags = D3D11_BIND_RENDER_TARGET;
   dstex.CPUAccessFlags = 0;
   dstex.MiscFlags = 0;
   dstex.ArraySize = 1;
   V_RETURN(pd3dDevice->CreateTexture2D(&dstex, NULL, &g_pRenderTarget));

   // Create the render target view
   D3D11_RENDER_TARGET_VIEW_DESC DescRT;
   DescRT.Format = dstex.Format;
   DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
   V_RETURN(pd3dDevice->CreateRenderTargetView(g_pRenderTarget, &DescRT, &g_pRTRV));

   // Create depth stencil texture.
   dstex.Width = uiWidth;
   dstex.Height = uiHeight;
   dstex.MipLevels = 1;
   dstex.Format = DXGI_FORMAT_D32_FLOAT;
   dstex.SampleDesc.Count = uiSampleCount;
   dstex.SampleDesc.Quality = uiSampleQuality;
   dstex.Usage = D3D11_USAGE_DEFAULT;
   dstex.BindFlags = D3D11_BIND_DEPTH_STENCIL;
   dstex.CPUAccessFlags = 0;
   dstex.MiscFlags = 0;
   V_RETURN(pd3dDevice->CreateTexture2D(&dstex, NULL, &g_pDSTarget));

   // Create the depth stencil view
   D3D11_DEPTH_STENCIL_VIEW_DESC DescDS;
   DescDS.Format = DXGI_FORMAT_D32_FLOAT;
   DescDS.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
   DescDS.Flags = 0;
   V_RETURN(pd3dDevice->CreateDepthStencilView(g_pDSTarget, &DescDS, &g_pDSRV));

   return hr;
}



//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_SettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera->SetProjParams( 70.4f * ( 3.14159f / 180.0f ), fAspectRatio, 0.1f, 1000.0f );
    

   //g_Camera->SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
   //g_Camera->SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 300, 0 );
    g_HUD.SetSize( 200, 200 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300 );
    g_SampleUI.SetSize( 170, 300 );

   // Update the sample count
    UpdateMSAASampleCounts( pd3dDevice, pBackBufferSurfaceDesc->Format );

   // Create a multi-sample render target
   auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
   g_BackBufferWidth = pBackBufferSurfaceDesc->Width;
   g_BackBufferHeight = pBackBufferSurfaceDesc->Height;
   V_RETURN(CreateRenderTarget(pd3dDevice, pd3dImmediateContext, g_BackBufferWidth, g_BackBufferHeight, g_MSAASampleCount, 0));

    return S_OK;
}


void RenderGrass(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceCtx, XMMATRIX& mView, XMMATRIX& mProj, float a_fElapsedTime)
{
   copterController.UpdatePhysics();
   copterController.UpdateCamera();
   copter->UpdateFromTransform(copterController.transform);
   
   XMMATRIX mViewProj;
   mViewProj = mul(mView, mProj);

   g_pGrassField->SetTime(g_fTime);

   g_pGrassField->SetViewProjMtx(mViewProj);
   g_pGrassField->SetViewMtx(mView);
   g_pGrassField->SetProjMtx(mProj);


   UpdateMeshes(a_fElapsedTime);
   // Draw Grass
   XMVECTOR vCamDir = g_Camera->GetLookAtPt() - g_Camera->GetEyePt();
   //if (g_RotCamController.isFixed) {
   //   XMVECTOR pos = (g_Camera->GetEyePt() + g_RotCamController.delta);
   //   V_TO_XM(pos, xpos, 3);
   //   g_pGrassField->GetFlowManager()->fans[0].position = xpos;
   //}

   g_pGrassField->Update(vCamDir, g_Camera->GetEyePt(), g_pMeshes, g_fNumOfMeshes, a_fElapsedTime, g_fTime);
   g_pGrassField->Render(copter, (Car*)g_pMeshes[0]);

   //for (auto* mesh : g_pMeshes) {
   //   if (mesh != nullptr) {
   //      mesh->Render();
   //   }
   //}
   
   pd3dDeviceCtx->IASetInputLayout(g_pSkyVertexLayout);
   g_pSkyViewProjEMV->SetMatrix((float*)& mViewProj);

   if (GetGlobalStateManager().UseWireframe())
      GetGlobalStateManager().SetRasterizerState("EnableMSAACulling_Wire");
   else
      GetGlobalStateManager().SetRasterizerState("EnableMSAACulling");


   pd3dDeviceCtx->IASetInputLayout(g_pSkyVertexLayout);
   g_pSkyboxPass->Apply(0, pd3dDeviceCtx);
   g_MeshSkybox.Render(pd3dDeviceCtx, 0);
   g_pSkyBoxESRV->SetResource(g_MeshSkybox.GetMaterial(0)->pDiffuseRV11);
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------

void TurnZBufferOn(ID3D11DeviceContext* pd3dImmediateContext)
{
   pd3dImmediateContext->OMSetDepthStencilState(g_depthStencilStateEnabled, 1);
   return;
}


void TurnZBufferOff(ID3D11DeviceContext* pd3dImmediateContext)
{
   pd3dImmediateContext->OMSetDepthStencilState(g_depthStencilStateDisabled, 1);
   return;
}


void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext )
{   
   HRESULT hr;
   // If the settings dialog is being shown, then render it instead of rendering the app's scene
   if( g_SettingsDlg.IsActive() )
   {
       g_SettingsDlg.OnRender( fElapsedTime );
       return;
   }       

    float ClearColor[4] = { 0.0, 0.3f, 0.8f, 0.0 };
   ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
   pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D10_CLEAR_DEPTH, 1.0, 0);

   // Set our render target since we can't present multisampled ref
   ID3D11RenderTargetView* pOrigRT;
   ID3D11DepthStencilView* pOrigDS;
   pd3dImmediateContext->OMGetRenderTargets(1, &pOrigRT, &pOrigDS);

   ID3D11RenderTargetView* aRTViews[1] = { g_pRTRV };
   pd3dImmediateContext->OMSetRenderTargets(1, aRTViews, g_pDSRV);

   // Clear the render target and DSV
   pd3dImmediateContext->ClearRenderTargetView(g_pRTRV, ClearColor);
   pd3dImmediateContext->ClearDepthStencilView(g_pDSRV, D3D11_CLEAR_DEPTH, 1.0, 0);

   g_fTime += fElapsedTime;
   // Get the projection & view matrix from the camera class
   XMMATRIX mView = g_Camera->GetViewMatrix();
   XMMATRIX mProj = g_Camera->GetProjMatrix();
   XMMATRIX mWorld = g_Camera->GetWorldMatrix();
   
   // Render grass
   RenderGrass(pd3dDevice, pd3dImmediateContext, mView, mProj, fElapsedTime);
   TurnZBufferOff(pd3dImmediateContext);
   
   XMMATRIX mViewProj;
   mViewProj = mul(mView, mProj);
   XMMATRIX mOrtho = XMMatrixTranspose(m_orthoMatrix);
   g_dbgWin->SetOrthoMtx(mOrtho);
   g_dbgWin->SetWorldMtx(mWorld);
   
   if (isDbgUiRendered)
      g_dbgWin->Render(pd3dImmediateContext, 100, 100);
   TurnZBufferOn(pd3dImmediateContext);

   // Copy it over because we can't resolve on present at the moment
   ID3D11Resource* pRT;
   pOrigRT->GetResource(&pRT);
   D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
   pOrigRT->GetDesc(&rtDesc);
   pd3dImmediateContext->ResolveSubresource(pRT, D3D11CalcSubresource(0, 0, 1), g_pRenderTarget, D3D11CalcSubresource(0, 0,
      1),
      rtDesc.Format);
   SAFE_RELEASE(pRT);

   // Use our Old RT again
   aRTViews[0] = pOrigRT;
   pd3dImmediateContext->OMSetRenderTargets(1, aRTViews, pOrigDS);
   SAFE_RELEASE(pOrigRT);
   SAFE_RELEASE(pOrigDS);

   DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
   if (isDbgUiRendered)
   {
      g_HUD.OnRender(fElapsedTime);
      g_SampleUI.OnRender(fElapsedTime);
      RenderText();
   }
   DXUT_EndPerfEvent();

   static ULONGLONG timefirst = GetTickCount64();
   if ( GetTickCount64() - timefirst > 5000 )
   {    
       OutputDebugString( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
       OutputDebugString( L"\n" );
       timefirst = GetTickCount64();
   }
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
   g_DialogResourceManager.OnD3D11ReleasingSwapChain();
   
   SAFE_RELEASE(g_pRTRV);
   SAFE_RELEASE(g_pDSRV);
   SAFE_RELEASE(g_pDSTarget);
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
   g_DialogResourceManager.OnD3D11DestroyDevice();
   g_SettingsDlg.OnD3D11DestroyDevice();
   DXUTGetGlobalResourceCache().OnDestroyDevice();

   SAFE_DELETE(g_pTxtHelper);

   SAFE_RELEASE(g_pRenderTarget);
   SAFE_RELEASE(g_pRTRV);
   SAFE_RELEASE(g_pDSTarget);
   SAFE_RELEASE(g_pDSRV);

   SAFE_RELEASE(g_depthStencilStateDisabled);
   SAFE_RELEASE(g_depthStencilStateEnabled);

   // Delete additional render resources here...
   SAFE_DELETE(g_Camera);
   SAFE_DELETE(g_dbgWin);
   SAFE_DELETE(g_pGrassField);


   SAFE_RELEASE(g_pSkyVertexLayout);
   g_MeshSkybox.Destroy();

   for (int i = 0; i < g_fNumOfMeshes; i++) {
      SAFE_DELETE(g_pMeshes[i]);
   }
   g_fNumOfMeshes = 0;
   //PrintMemoryLeaks(L"mem.txt");
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
   pDeviceSettings->d3d11.SyncInterval = 0;
   // For the first device created if its a REF device, optionally display a warning dialog box
   //static bool s_bFirstTime = true;
   //if (s_bFirstTime)
   //{
   //   s_bFirstTime = false;
   //   if ((DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
   //      (DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
   //         pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE))
   //      DXUTDisplaySwitchingToREFWarning(pDeviceSettings->ver);
   //}

   // Disable MSAA settings from the settings dialog
   g_SettingsDlg.GetDialogControl()->GetComboBox(DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT)->SetEnabled(false);
   g_SettingsDlg.GetDialogControl()->GetComboBox(DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY)->SetEnabled(false);
   g_SettingsDlg.GetDialogControl()->GetStatic(DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT_LABEL)->SetEnabled(false);
   g_SettingsDlg.GetDialogControl()->GetStatic(DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY_LABEL)->SetEnabled(false);
   return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
   // Update the camera's position based on user input 
   g_Camera->FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
   // Pass messages to dialog resource manager calls so GUI state is updated correctly
   *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
   if( *pbNoFurtherProcessing )
       return 0;

   // Pass messages to settings dialog if its active
   if( g_SettingsDlg.IsActive() )
   {
       g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
       return 0;
   }

   // Give the dialogs a chance to handle the message first
   *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
   if( *pbNoFurtherProcessing )
       return 0;
   *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
   if( *pbNoFurtherProcessing )
       return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
   if (g_Camera != NULL) {
      g_Camera->HandleMessages(hWnd, uMsg, wParam, lParam);
   }

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
   if (bKeyDown)
   {
      switch (nChar)
      {
      case 74: //j
         copterController.OnLeft();
         break;
      case 75: //k
         copterController.OnBackward();
         break;
      case 76: //l
         copterController.OnRight();
         break;
      case 73://i
         copterController.OnForward();
         break;
      case 85://u
         copterController.OnRLeft();
         break;
      case 79://o
         copterController.OnRRight();
         break;
      case 219://[
         copterController.OnDetorque();
         break;
      case 221://]
         copterController.OnTorque();
         break;
      case VK_BACK:
         isDbgUiRendered = !isDbgUiRendered;
         break;
      case VK_SUBTRACT:
         ToggleToNormalCamera();
         copterController.UnfixCam();
         break;
      case VK_DIVIDE:
         ToggleToTerrainCamera();
         copterController.UnfixCam();
         break;
      case VK_ADD:
         ToggleToNormalCamera();
         copterController.FixCam();
         break;
      case VK_MULTIPLY:
         ToggleToMeshCamera();
         break;
      case 70:
         g_fCarRotAccel = -g_fCarRotForce;
         break;
      case 72:
         g_fCarRotAccel = g_fCarRotForce;
         break;
      case 84:
         g_fCarAccel = g_fCarForce;
         break;
      case 71:
         g_fCarAccel = -g_fCarForce * 2;
         break;
      }
   }
   else
   {
      if (nChar == 72 || nChar == 70)
         g_fCarRotAccel = 0;
      else if (nChar == 84 || nChar == 71)
         g_fCarAccel = 0;//-g_fCarForce;

      switch (nChar)
      {
      case 74: //j
         copterController.NonLeft();
         break;
      case 75: //k
         copterController.NonBackward();
         break;
      case 76: //l
         copterController.NonRight();
         break;
      case 73://i
         copterController.NonForward();
         break;
      case 85://u
         copterController.NonRLeft();
         break;
      case 79://o
         copterController.NonRRight();
         break;
      case 219://[
         copterController.NonDetorque();
         break;
      case 221://]
         copterController.NonTorque();
         break;
      }
   }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
   WCHAR sStr[MAX_PATH] = { 0 };

   switch( nControlID )
   {
       case IDC_CHANGEDEVICE:
          g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive());
          break;
      
      case IDC_TOGGLE_WIREFRAME:
         GetGlobalStateManager().ToggleWireframe();
         break;
      
      case IDC_TOGGLE_RENDERING_GRASS:
         g_pGrassField->ToggleRenderingGrass();
         break;
      
      case IDC_TOGGLE_RENDERING_DBG_WIN:
         g_dbgWin->ToggleRender();
         break;
      
      case IDC_TOGGLE_DBG_WIN_SLICE:
         g_dbgWin->ToggleSlice();
         break;
      
      //case IDC_FIX_CAMERA:
      //   g_RotCamController.isFixed = !g_RotCamController.isFixed;
      //   if (g_RotCamController.isFixed) {
      //      XMFLOAT3 position = g_pGrassField->GetFlowManager()->fans[0].position;
      //      XM_TO_V(position, pos, 3);
      //      g_RotCamController.delta = pos - g_Camera->GetEyePt();
      //   }
      //   break;
      
      case IDC_GRASS_WIND_FORCE_SLYDER:
      {
         g_fWindStrength = (float)g_HUD.GetSlider(IDC_GRASS_WIND_FORCE_SLYDER)->GetValue() / 10000.0f;
         swprintf_s(sStr, MAX_PATH, L"Wind Strength: %.4f", g_fWindStrength);
         g_HUD.GetStatic(IDC_GRASS_WIND_LABEL)->SetText(sStr);
         g_pGrassField->SetWindStrength(g_fWindStrength);
         break;
      }
      
      case IDC_GRASS_MAX_FLOW_STRENGTH_SLYDER:
      {
         g_fMaxFlowStrength = (float)g_HUD.GetSlider(IDC_GRASS_MAX_FLOW_STRENGTH_SLYDER)->GetValue() / 1000.0f;
         swprintf_s(sStr, MAX_PATH, L"Flow Horiz Strength: %.4f", g_fMaxFlowStrength);
         g_HUD.GetStatic(IDC_GRASS_MAX_FLOW_STRENGTH_LABEL)->SetText(sStr);
         g_pGrassField->GetFlowManager()->SetMaxHorizFlow(g_fMaxFlowStrength);
         //((Car*)g_pMeshes[0])->param = g_fMaxFlowStrength;
         break;
      }
      
      case IDC_FAN_RADIUS_SLYDER:
      {
         g_fFanRadius = (float)g_HUD.GetSlider(IDC_FAN_RADIUS_SLYDER)->GetValue() / 100.0f;
         swprintf_s(sStr, MAX_PATH, L"Fan Radius: %.4f", g_fFanRadius * 2);
         g_HUD.GetStatic(IDC_FAN_RADIUS_LABEL)->SetText(sStr);
         g_pGrassField->GetFlowManager()->fans[0].radius = g_fFanRadius;
         g_pGrassField->GetFlowManager()->fans[1].radius = g_fFanRadius;
         g_pGrassField->GetFlowManager()->fans[2].radius = g_fFanRadius;
         g_pGrassField->GetFlowManager()->fans[3].radius = g_fFanRadius;
         break;
      }
      
      case IDC_CAM_SPEED_SCALE_SLYDER:
      {
         g_fCameraSpeed = (float)g_HUD.GetSlider(IDC_CAM_SPEED_SCALE_SLYDER)->GetValue() / 1000.0f;
         swprintf_s(sStr, MAX_PATH, L"Cam speed scale: %.4f", g_fCameraSpeed * 60);
         g_HUD.GetStatic(IDC_CAM_SPEED_SCALE_LABEL)->SetText(sStr);
         g_Camera->SetScalers(0.01, g_fCameraSpeed * 60);
         break;
      }
      
      
      case IDC_COPTER_SCALE_SLYDER:
      {
         g_fCopterScale = (float)g_HUD.GetSlider(IDC_COPTER_SCALE_SLYDER)->GetValue() / 100.0f;
         swprintf_s(sStr, MAX_PATH, L"Copter scale: %.4f", g_fCopterScale);
         g_HUD.GetStatic(IDC_COPTER_SCALE_LABEL)->SetText(sStr);
         copter->UpdateScale(g_fCopterScale);
         break;
      }
      
      case IDC_FAN_ANGLE_SPEED_SLYDER:
      {
         g_fAngleSpeed = (float)g_HUD.GetSlider(IDC_FAN_ANGLE_SPEED_SLYDER)->GetValue();
         swprintf_s(sStr, MAX_PATH, L"Angle Speed: %.4f", g_fAngleSpeed);
         g_HUD.GetStatic(IDC_FAN_ANGLE_SPEED_LABEL)->SetText(sStr);
         g_pGrassField->GetFlowManager()->fans[0].angleSpeed = g_fAngleSpeed;
         g_pGrassField->GetFlowManager()->fans[1].angleSpeed = g_fAngleSpeed;
         g_pGrassField->GetFlowManager()->fans[2].angleSpeed = g_fAngleSpeed;
         g_pGrassField->GetFlowManager()->fans[3].angleSpeed = g_fAngleSpeed;
         break;
      }
      
      
      case IDC_TERR_R_SLYDER:
      case IDC_TERR_G_SLYDER:
      case IDC_TERR_B_SLYDER:
      {
         g_vTerrRGB.x = (float)g_HUD.GetSlider(IDC_TERR_R_SLYDER)->GetValue() / 200.0f;
         g_vTerrRGB.y = (float)g_HUD.GetSlider(IDC_TERR_G_SLYDER)->GetValue() / 200.0f;
         g_vTerrRGB.z = (float)g_HUD.GetSlider(IDC_TERR_B_SLYDER)->GetValue() / 200.0f;
         swprintf_s(sStr, MAX_PATH, L"Diffuse: (%.2f,%.2f,%.2f)", g_vTerrRGB.x, g_vTerrRGB.y, g_vTerrRGB.z);
         g_HUD.GetStatic(IDC_TERR_RGB_LABEL)->SetText(sStr);
         XM_TO_V(g_vTerrRGB, vTerrRGB, 3);
         setw(vTerrRGB, 1);
         g_pGrassField->SetTerrRGB(vTerrRGB);
         //vTerrRGB *= 2;
         //g_pGrassField->SetLowGrassDiffuse(vTerrRGB);
         break;
      }
      
      //case IDC_FLOW_DIR_X_SLYDER:
      //case IDC_FLOW_DIR_Y_SLYDER:
      //case IDC_FLOW_DIR_Z_SLYDER:
      //{
      //   g_vDir.x = (float)g_HUD.GetSlider(IDC_FLOW_DIR_X_SLYDER)->GetValue() / 100.0f;
      //   g_vDir.y = (float)g_HUD.GetSlider(IDC_FLOW_DIR_Y_SLYDER)->GetValue() / 100.0f;
      //   g_vDir.z = (float)g_HUD.GetSlider(IDC_FLOW_DIR_Z_SLYDER)->GetValue() / 100.0f;
      //   swprintf_s(sStr, MAX_PATH, L"Dir: (%.2f,%.2f,%.2f)", g_vDir.x, g_vDir.y, g_vDir.z);
      //   g_HUD.GetStatic(IDC_FLOW_DIR_LABEL)->SetText(sStr);
      //   g_pGrassField->m_pFlowManager->fans[0].direction = g_vDir;
      //   break;
      //}
   }
}


void InitCarMesh (void) 
{
   float height_scale;
   float grass_radius;
   Terrain* const terr = g_pGrassField->GetTerrain(&height_scale, &grass_radius);

   if (g_fNumOfMeshes == 0)
   {
      ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
      ID3D11DeviceContext* pd3dDeviceCtx = DXUTGetD3D11DeviceContext();

      g_pMeshes[0] = new Car(pd3dDevice, pd3dDeviceCtx, g_pGrassField->SceneEffect(),
         create(0, 0, 0, 0),
         terr, height_scale, grass_radius,
         g_fCarFrontWidth, g_fCarHeight, g_fCarLength, 0.0f);
      g_fNumOfMeshes = 1;
      
      UpdateMeshes(0.01f);
   }
}


void BeforeCameraChange(void)
{
   SAFE_DELETE(g_Camera);
   copterController.UnfixCam();
}


void AfterCameraChange (void)
{
   copterController.SetupCamera(g_Camera);
   g_Camera->SetProjParams(60.4f * (3.14159f / 180.0f), 1.33f, 0.1f, 1000.0f /*g_fMeter*/);
   g_Camera->SetViewParams(XMLoadFloat3(&g_vCameraEyeStart), XMLoadFloat3(&g_vCameraAtStart));
   g_Camera->SetScalers(0.01f, g_fCameraSpeed);
}


void ToggleToNormalCamera (void)
{
   BeforeCameraChange();

   float height_scale;
   float grass_radius;
   Terrain* const terr = g_pGrassField->GetTerrain(&height_scale, &grass_radius);

   g_Camera = new LandscapeCamera(g_fCameraHeight, terr, height_scale, grass_radius);
   AfterCameraChange();
}


void ToggleToTerrainCamera (void)
{
   BeforeCameraChange();

   float height_scale;
   float grass_radius;
   Terrain* const terr = g_pGrassField->GetTerrain(&height_scale, &grass_radius);

   g_Camera = new HeightCamera(g_fCameraHeight,
      std::pair<float, float>(g_fCameraHeightMin, g_fCameraHeightMax),
      terr, height_scale, grass_radius);
   AfterCameraChange();
}


void ToggleToMeshCamera (void) {
   BeforeCameraChange();

   float height_scale;
   float grass_radius;
   Terrain* const terr = g_pGrassField->GetTerrain(&height_scale, &grass_radius);

   g_Camera = new MeshCamera(g_fCameraMeshDist,
      std::pair<float, float>(g_fCameraMeshDistMin, g_fCameraMeshDistMax),
      g_fCameraHeight,
      std::pair<float, float>(g_fCameraHeightMin, g_fCameraHeightMax),
      g_pMeshes[0], terr, height_scale, grass_radius);
   AfterCameraChange();
}


int GetCameraType (void) 
{
   if (dynamic_cast<MeshCamera*>(g_Camera) != NULL)
      return CAMERA_MESH;

   if (dynamic_cast<LandscapeCamera*>(g_Camera) != NULL)
      return CAMERA_NORMAL;

   if (dynamic_cast<HeightCamera*>(g_Camera) != NULL)
      return CAMERA_TERRAIN;
}
