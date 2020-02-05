#include <string>

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"

#include "main.h"

#include "camera.h"
#include "StateManager.h"

#pragma warning( disable : 4100 )

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CFirstPersonCamera          *g_Camera;               // A model viewing camera

CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = nullptr;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls

// Scene Effect vars
ID3DX11EffectMatrixVariable* g_mWorld = nullptr;
ID3DX11EffectMatrixVariable* g_mViewProj = nullptr;
ID3DX11EffectMatrixVariable* g_mInvCamView = nullptr;
ID3DX11EffectMatrixVariable* g_mLightViewProj = nullptr;
ID3DX11EffectMatrixVariable* g_mNormalMatrix = nullptr;
//ID3DX11EffectScalarVariable* g_fTime = nullptr;


// Direct3D 11 resources
ID3D11VertexShader*         g_pVertexShader11 = nullptr;
ID3D11PixelShader*          g_pPixelShader11 = nullptr;
ID3D11InputLayout*          g_pLayout11 = nullptr;
ID3D11SamplerState*         g_pSamLinear = nullptr;

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
#pragma pack(push,1)
struct CB_VS_PER_OBJECT
{
   XMFLOAT4X4  m_mWorldViewProjection;
   XMFLOAT4X4  m_mWorld;
   XMFLOAT4    m_MaterialAmbientColor;
   XMFLOAT4    m_MaterialDiffuseColor;
};

struct CB_VS_PER_FRAME
{
   XMFLOAT3    m_vLightDir;
   float       m_fTime;
   XMFLOAT4    m_LightDiffuse;
};
#pragma pack(pop)

ID3D11Buffer*                       g_pcbVSPerObject11 = nullptr;
ID3D11Buffer*                       g_pcbVSPerFrame11 = nullptr;


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
   
   WCHAR sStr[MAX_PATH];
   swprintf_s(sStr, MAX_PATH, L"Wind Strength: %.4f", g_fWindStrength);
   g_HUD.AddStatic(IDC_GRASS_WIND_LABEL, sStr, 20, iY += iYo, 140, 22);
   g_HUD.AddSlider(IDC_GRASS_WIND_FORCE_SLYDER, 20, iY += iYo, 135, 22, 0, 10000, (int)(g_fWindStrength * 10000));

   g_HUD.AddButton(IDC_TOGGLE_WIREFRAME, L"Toggle wire-frame (F4)", 25, iY += iYo, 125, 22, VK_F4);
   g_HUD.AddButton(IDC_TOGGLE_RENDERING_GRASS, L"Toggle rendering-grass (F5)", 25, iY += iYo, 125, 22, VK_F5);

   swprintf_s(sStr, MAX_PATH, L"Diffuse: (%.2f,%.2f,%.2f)", g_vTerrRGB.x, g_vTerrRGB.y, g_vTerrRGB.z);
   g_HUD.AddStatic(IDC_TERR_RGB_LABEL, sStr, 20, iY += iYo, 140, 22);
   g_HUD.AddSlider(IDC_TERR_R_SLYDER, 20, iY += iYo, 135, 22, 0, 100, (int)(g_vTerrRGB.x * 100));
   g_HUD.AddSlider(IDC_TERR_G_SLYDER, 20, iY += iYo, 135, 22, 0, 100, (int)(g_vTerrRGB.y * 100));
   g_HUD.AddSlider(IDC_TERR_B_SLYDER, 20, iY += iYo, 135, 22, 0, 100, (int)(g_vTerrRGB.z * 100));

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
   swprintf(lookAtStr, sizeof(lookAtStr), L"LookAt: X = %f, Y = %f, Z = % f", lookAt.x, lookAt.y, lookAt.z);

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

ID3D11DepthStencilState* m_depthDisabledStencilState;
ID3D11DepthStencilState* m_depthEnabledStencilState;
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
   hr = pd3dDevice->CreateDepthStencilState(&depthDisabledStencilDesc, &m_depthDisabledStencilState);
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
   hr = pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_depthEnabledStencilState);
   if (FAILED(hr))
   {
      return false;
   }
   
   // Set the depth stencil state.
   pd3dImmediateContext->OMSetDepthStencilState(m_depthEnabledStencilState, 1);


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
   g_GrassInitState.InitState[1].sSeatingTexPath = L"resources/SeatingType2.dds";
   g_GrassInitState.InitState[2].sSeatingTexPath = L"resources/SeatingType3.dds";
   
   g_GrassInitState.InitState[0].sSubTypesPath = L"config/T1SubTypes.cfg";
   g_GrassInitState.InitState[1].sSubTypesPath = L"config/T2SubTypes.cfg";
   g_GrassInitState.InitState[2].sSubTypesPath = L"config/T3SubTypes.cfg";
   g_GrassInitState.InitState[0].fCameraMeshDist = g_fCameraMeshDistMax;
   g_GrassInitState.InitState[1].fCameraMeshDist = g_fCameraMeshDistMax;
   g_GrassInitState.InitState[2].fCameraMeshDist = g_fCameraMeshDistMax;
   g_GrassInitState.sSceneEffectPath = L"Shaders/SceneEffect.fx";
   g_GrassInitState.sNoiseMapPath = L"resources/Noise.dds";
   g_GrassInitState.sGrassOnTerrainTexturePath = L"resources/grass512.dds";
   g_GrassInitState.fHeightScale = g_fHeightScale;
   g_GrassInitState.fTerrRadius = 400.0f;
   g_pGrassField = new GrassFieldManager(g_GrassInitState);
   g_pTerrTile = g_pGrassField->SceneEffect()->GetVariableByName("g_fTerrTile")->AsScalar();
   
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
   //    g_pGrassField->SetWindMapTile(4);
   g_pGrassField->SetSegMass(g_fMass);
   g_pGrassField->SetHardness(g_fHardness);
   g_pGrassField->SetGrassLodBias(g_fGrassLodBias);
   g_pGrassField->SetSubScatterGamma(g_fGrassSubScatterGamma);
   g_pGrassField->SetGrassAmbient(g_fGrassAmbient);
   g_pGrassField->SetWindStrength(g_fWindStrength);
   g_pGrassField->SetWindSpeed(g_fWindTexSpeed);
   //g_pGrassField->SetHeightScale(g_fHeightScale);
   g_pGrassField->SetQuality(g_fQuality);
   g_pGrassField->SetLowGrassDiffuse(vGrassSpecular);
   g_pGrassField->SetFogColor(vFogColor);
   g_pGrassField->SetWindBias(g_fWindBias);
   g_pGrassField->SetWindScale(g_fWindScale);
   
   //InitMeshes(pd3dDevice);
   
   // Setup the camera's view parameters
   float height_scale, grass_radius;
   Terrain* const terr = g_pGrassField->GetTerrain(&height_scale, &grass_radius);
   
   g_Camera = new LandscapeCamera(g_fCameraHeight, terr, height_scale, grass_radius);
   
   g_Camera->SetViewParams(XMLoadFloat3(&g_vCameraEyeStart), XMLoadFloat3(&g_vCameraAtStart));
   g_Camera->SetScalers(0.01f, g_fCameraSpeed /* g_fMeter*/);
   
   g_dbgWin.Initialize(pd3dDevice, g_windowWidth, g_windowHeight, g_pGrassField->GetAxesFanFlow()->GetShaderResourceView(), 10);
   
   return S_OK;
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

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
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
   XMMATRIX mViewProj;
   mViewProj = mul(mView, mProj);
   
   g_pGrassField->SetTime(g_fTime);
   
   g_pGrassField->SetViewProjMtx(mViewProj);
   g_pGrassField->SetViewMtx(mView);
   g_pGrassField->SetProjMtx(mProj);

   // Draw Grass
   XMVECTOR vCamDir = g_Camera->GetLookAtPt() - g_Camera->GetEyePt();
   
   g_pGrassField->Update(vCamDir, g_Camera->GetEyePt(), g_pMeshes, 0/*g_fNumOfMeshes*/, a_fElapsedTime);
    g_pGrassField->Render();
   
   
   if (GetGlobalStateManager().UseWireframe())
      GetGlobalStateManager().SetRasterizerState("EnableMSAACulling_Wire");
   else
      GetGlobalStateManager().SetRasterizerState("EnableMSAACulling");
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------

void TurnZBufferOn(ID3D11DeviceContext* pd3dImmediateContext)
{
   pd3dImmediateContext->OMSetDepthStencilState(m_depthEnabledStencilState, 1);
   return;
}


void TurnZBufferOff(ID3D11DeviceContext* pd3dImmediateContext)
{
   pd3dImmediateContext->OMSetDepthStencilState(m_depthDisabledStencilState, 1);
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
   g_dbgWin.SetOrthoMtx(mOrtho);
   g_dbgWin.SetWorldMtx(mWorld);
   
   g_dbgWin.Render(pd3dImmediateContext, 100, 100);
   TurnZBufferOn(pd3dImmediateContext);

   // Copy it over because we can't resolve on present at the moment
   ID3D11Resource* pRT;
   pOrigRT->GetResource(&pRT);
   D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
   pOrigRT->GetDesc(&rtDesc);
   pd3dImmediateContext->ResolveSubresource(pRT, D3D10CalcSubresource(0, 0, 1), g_pRenderTarget, D3D10CalcSubresource(0, 0,
      1),
      rtDesc.Format);
   SAFE_RELEASE(pRT);

   // Use our Old RT again
   aRTViews[0] = pOrigRT;
   pd3dImmediateContext->OMSetRenderTargets(1, aRTViews, pOrigDS);
   SAFE_RELEASE(pOrigRT);
   SAFE_RELEASE(pOrigDS);

   DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
   g_HUD.OnRender( fElapsedTime );
   g_SampleUI.OnRender( fElapsedTime );
   RenderText();
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
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
   g_DialogResourceManager.OnD3D11DestroyDevice();
   g_SettingsDlg.OnD3D11DestroyDevice();
   DXUTGetGlobalResourceCache().OnDestroyDevice();

   g_dbgWin.Shutdown();

   SAFE_DELETE( g_pTxtHelper );

   SAFE_RELEASE( g_pVertexShader11 );
   SAFE_RELEASE( g_pPixelShader11 );
   SAFE_RELEASE( g_pLayout11 );
   SAFE_RELEASE( g_pSamLinear );

   SAFE_RELEASE(g_pRenderTarget);
   SAFE_RELEASE(g_pRTRV);
   SAFE_RELEASE(g_pDSTarget);
   SAFE_RELEASE(g_pDSRV);

    // Delete additional render resources here...
   SAFE_DELETE(g_Camera);
   SAFE_DELETE(g_pGrassField);

   SAFE_RELEASE( g_pcbVSPerObject11 );
   SAFE_RELEASE( g_pcbVSPerFrame11 );
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
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
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
   WCHAR sStr[MAX_PATH] = { 0 };

   switch( nControlID )
   {
     case IDC_TOGGLE_WIREFRAME:
        GetGlobalStateManager().ToggleWireframe();
        break;
     
     case IDC_TOGGLE_RENDERING_GRASS:
        g_pGrassField->ToggleRenderingGrass();
        break;

     case IDC_GRASS_WIND_FORCE_SLYDER:
     {
        g_fWindStrength = (float)g_HUD.GetSlider(IDC_GRASS_WIND_FORCE_SLYDER)->GetValue() / 10000.0f;
        swprintf_s(sStr, MAX_PATH, L"Wind Strength: %.4f", g_fWindStrength);
        g_HUD.GetStatic(IDC_GRASS_WIND_LABEL)->SetText(sStr);
        g_pGrassField->SetWindStrength(g_fWindStrength);
        break;
     }
     case IDC_TERR_R_SLYDER:
     case IDC_TERR_G_SLYDER:
     case IDC_TERR_B_SLYDER:
     {
        g_vTerrRGB.x = (float)g_HUD.GetSlider(IDC_TERR_R_SLYDER)->GetValue() / 100.0f;
        g_vTerrRGB.y = (float)g_HUD.GetSlider(IDC_TERR_G_SLYDER)->GetValue() / 100.0f;
        g_vTerrRGB.z = (float)g_HUD.GetSlider(IDC_TERR_B_SLYDER)->GetValue() / 100.0f;
        swprintf_s(sStr, MAX_PATH, L"Diffuse: (%.2f,%.2f,%.2f)", g_vTerrRGB.x, g_vTerrRGB.y, g_vTerrRGB.z);
        g_HUD.GetStatic(IDC_TERR_RGB_LABEL)->SetText(sStr);
        XM_TO_V(g_vTerrRGB, vTerrRGB, 3);
        g_pGrassField->SetTerrRGB(vTerrRGB);
        break;
     }
   }
}
