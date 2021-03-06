//--------------------------------------------------------------------------------------
// Constant buffers 
//--------------------------------------------------------------------------------------
cbuffer cEveryFrame
{
    float4x4 g_mWorld;
    float4x4 g_mViewProj;
    float4x4 g_mInvCamView;
    float4x4 g_mLightViewProj;
    float4x4 g_mNormalMatrix;
    float    g_fTime;
};
    
// Mesh material settings
float3 g_vKd;
float3 g_vKs;
float3 g_vKa;
float  g_nNs;

cbuffer cUserControlled
{
    float3 g_vTerrRGB;
    float4 g_vFogColor;// = float4(0.0, 0.3, 0.8, 1.0);
    float  g_fTerrTile;
    float  g_fHeightScale;
    float3 g_vTerrSpec = float3(202.0/255.0, 218.0/255.0, 50.0/255.0);
};

cbuffer cRarely
{
    float g_fTerrRadius;
    float g_fGrassRadius;
    float2 g_vPixSize = float2(0.0039, 0.0039);
};

cbuffer cImmutable
{
    float3 vLightDir;// = float3( 0.0, 0.9174, -0.2752 ); 
};

//--------------------------------------------------------------------------------------
// Texture and samplers
//--------------------------------------------------------------------------------------
Texture2D g_txShadowMap;
Texture2D g_txGrassDiffuse;
Texture2D g_txSandDiffuse;
Texture2D g_txMeshDiffuse;
Texture2D g_txSeatingT1;
Texture2D g_txSeatingT2;
Texture2D g_txSeatingT3;
Texture2D g_txHeightMap;
Texture2D g_txLightMap;
Texture2D g_txSkyBox;
Texture2D g_txGrassColor;

// Mesh textures
Texture2D g_txMeshMapKd;
Texture2D g_txMeshMapKs;

#include "Shaders/Samplers.fx"
#include "Shaders/Fog.fx"
#include "Shaders/States.fx"


//--------------------------------------------------------------------------------------
// Input and output structures 
//--------------------------------------------------------------------------------------

/* Terrain input structures */

struct TerrVSIn
{
    float3 vPos      : POSITION;
    float2 vTexCoord : TEXCOORD0;
};

struct TerrPSIn
{
    float4 vPos      : SV_Position;
    //float4 vShadowPos: TEXCOORD0;
    float4 vTexCoord : TEXCOORD1;
    //float3 vNormal   : NORMAL;
};

/* Sky box input */

struct VSSceneIn
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float2 tex : TEXTURE0;
};

struct PSSceneIn
{
    float4 pos : SV_Position;
    float3 tex : TEXTURE0;
};

struct PSCarIn
{
    float4 pos : SV_Position;
    float3 world_pos : TEXCOORD1;
    float3 norm : NORMAL;
    float2 tex : TEXTURE0;
};

//--------------------------------------------------------------------------------------
// Shaders
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Sky vertex shader
//--------------------------------------------------------------------------------------
PSSceneIn VSSkymain(VSSceneIn Input)
{
    PSSceneIn Output;
    float3 vWorldPos = Input.pos * 0.25;
    Output.pos = mul(float4(vWorldPos, 1.0), g_mViewProj);
    Output.tex.xy = 1.0 - Input.tex;
    float fY = vWorldPos.y;
    Output.tex.z = FogValue(350 - fY);
    return Output;
}

float4 PSSkymain(PSSceneIn Input): SV_Target0
{
	return lerp(g_txSkyBox.Sample(g_samLinear, Input.tex.xy), g_vFogColor, Input.tex.z);
}

PSCarIn VSCarmain(VSSceneIn Input)
{
    PSCarIn Output;
    
    float4 vWorldPos = mul(float4(Input.pos, 1.0), g_mWorld);
    //vWorldPos.y += 0.2;
    Output.world_pos = vWorldPos.xyz;
    Output.pos = mul(vWorldPos, g_mViewProj);
    Output.tex = Input.tex;
    Output.tex.y = 1.0 - Output.tex.y;
    Output.norm = normalize(mul((float3x3)g_mNormalMatrix, Input.norm));
    
    return Output;
}

float4 PSCarmain0(PSCarIn Input): SV_Target0
{
    float4 ka, kd, ks;
    float3 eye_dir = normalize(g_mInvCamView[3].xyz - Input.world_pos);

    ka = float4(g_vKa, 1.0f);
	
    if (g_vKd.x != 0 || g_vKd.y != 0 || g_vKd.z != 0)
        kd = float4(g_vKd, 1.0f);
    else
        kd = g_txMeshMapKd.Sample(g_samLinear, Input.tex);
        
    if (g_vKs.x != 0 || g_vKs.y != 0 || g_vKs.z != 0)
        ks = float4(g_vKs, 1.0f);
    else
        ks = g_txMeshMapKs.Sample(g_samLinear, Input.tex);
    
    float3 half = normalize(eye_dir - vLightDir);
    
    float3 refl = normalize(reflect(eye_dir, Input.norm));
    float2 sphere_tex_coords;
    
    sphere_tex_coords.x = refl.x / 2 + 0.5;
    sphere_tex_coords.y = refl.y / 2 + 0.5;

    float3 Ir = g_txSkyBox.Sample(g_samLinear, sphere_tex_coords).xyz;
    
    float Ia = 0.3, Id = 0.2, Is = 1.1;

    return float4(Ia * ka.xyz * kd.xyz, kd.w) +
        float4(Id * kd.xyz * max(dot(-vLightDir, Input.norm), 0), kd.w) +
        float4(ks.xyz * (Is * pow(max(dot(half, Input.norm), 0.0f), g_nNs) + Ir), 0.0f);
}


/* Mesh shaders */
TerrPSIn MeshVSMain( TerrVSIn Input )
{
    TerrPSIn Output;
    Output.vPos         = mul(mul(float4(Input.vPos, 1.0), g_mWorld), g_mViewProj);
    Output.vTexCoord.xy = Input.vTexCoord;
    return Output;    
}

float4 MeshPSMain( TerrPSIn Input ): SV_Target
{
    float4 vTexel = g_txMeshDiffuse.Sample(g_samLinear, Input.vTexCoord.xy );
    return vTexel;
}


/* Terrain shaders */

//float3 CalcNormal(float2 a_vTexCoord)
//{
//    const float2 dU = float2(g_vPixSize.x, 0);
//    const float2 dV = float2(0, g_vPixSize.y);
//  
//    float heightC = g_txHeightMap.SampleLevel(g_samLinear, a_vTexCoord, 0).r * g_fHeightScale;
//    float heightL = g_txHeightMap.SampleLevel(g_samLinear, a_vTexCoord - dU, 0).r * g_fHeightScale;
//    float heightR = g_txHeightMap.SampleLevel(g_samLinear, a_vTexCoord + dU, 0).r * g_fHeightScale;
//    float heightT = g_txHeightMap.SampleLevel(g_samLinear, a_vTexCoord - dV, 0).r * g_fHeightScale;
//    float heightB = g_txHeightMap.SampleLevel(g_samLinear, a_vTexCoord + dV, 0).r * g_fHeightScale;
//	
//    return normalize(float3(heightR - heightL, 1.0, heightB - heightT));
//}


TerrPSIn TerrainVSMain( TerrVSIn Input )
{
    TerrPSIn Output;
    float fY			= g_txHeightMap.SampleLevel(g_samLinear, Input.vTexCoord, 0).a * g_fHeightScale;
    float4 vWorldPos	= float4(Input.vPos + float3(0.0, fY, 0.0), 1.0);
    Output.vPos         = mul(vWorldPos, g_mViewProj);
    Output.vTexCoord.xy = Input.vTexCoord;
    Output.vTexCoord.z  = FogValue(length(vWorldPos - g_mInvCamView[3].xyz));
    Output.vTexCoord.w  = length(vWorldPos - g_mInvCamView[3].xyz);
    
    return Output;
}

float GetAlphaCoef(float2 vTexCoord)
{
    float3 vAlpha =  float3(g_txSeatingT1.Sample(g_samLinear, vTexCoord).r,
                            g_txSeatingT2.Sample(g_samLinear, vTexCoord).r,
                            g_txSeatingT3.Sample(g_samLinear, vTexCoord).r);
    return clamp(length(vAlpha), 0.0, 1.0);                            
}

float4 TerrainPSMain( TerrPSIn Input ): SV_Target
{
    float2 fDot = g_txLightMap.Sample(g_samLinear, Input.vTexCoord.xy).rg;
    float3 vTexel = g_txGrassDiffuse.Sample(g_samLinear, Input.vTexCoord.xy * 0.5*g_fTerrTile).xyz;
    vTexel *=  g_vTerrRGB;
    float3 vGrassColor = g_txGrassColor.Sample(g_samLinear, Input.vTexCoord.xy).xyz;

	float fTexDist = min(Input.vTexCoord.w/140.f, 1.0);
	
	//float3(0.04, 0.1, 0.01) - previous color	
    float3 vL = lerp(vGrassColor, vTexel, fTexDist) * max(0.8, (2.0 + 5.0 * fDot.y) * 0.5);

	float fLimDist = clamp((Input.vTexCoord.w - 140.0) / 20.0, 0.0, 1.0);
//    return lerp(float4(fDot.x * g_vTerrSpec, 1.0), g_vFogColor, Input.vTexCoord.z);
    return lerp(float4(fDot.x * fLimDist* g_vTerrSpec + (1.0 - fDot.x * fLimDist) * vL, 1.0), g_vFogColor, Input.vTexCoord.z);

}

/* Light Map Shaders */

TerrPSIn LightMapVSMain( TerrVSIn Input )
{
    TerrPSIn Output;
    Output.vPos          = float4(Input.vPos, 1.0);
    Output.vTexCoord.xy  = Input.vTexCoord;
    //Output.vTexCoord.x = 1.0 - Output.vTexCoord.x;
    return Output;
}

float4 LightMapPSMain( TerrPSIn Input ): SV_Target
{
    float2 vTransformedTC = (Input.vTexCoord * 2.0 - 1.0) * g_fTerrRadius;
    float4 vHeightData    = g_txHeightMap.Sample(g_samLinear, Input.vTexCoord.xy);
    vHeightData.xyz = vHeightData.xyz * 2.0 - 1.0;
    float3 vTerrPos = float3(vTransformedTC.x, vHeightData.a * g_fHeightScale, vTransformedTC.y);
    float fDot = 1.0 - dot(normalize(g_mInvCamView[3].xyz - vTerrPos), vHeightData.xyz);
    float fLightDot = -dot(vLightDir, vHeightData.xyz);
    fDot *= fDot;  
	fDot *= fDot;  
    fDot *= fDot;   
    fDot *= fDot;  
    return float4(fDot, fLightDot, 0.0, 0.0);
}

technique10 Render
{    
    pass RenderTerrainPass
    {
        SetVertexShader( CompileShader( vs_4_0, TerrainVSMain() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, TerrainPSMain() ) ); 
        SetBlendState( NonAlphaState, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepthTestWrite, 0 );
        //SetRasterizerState( EnableMSAACulling );
    }  

    pass RenderMeshPass
    {
        SetVertexShader( CompileShader( vs_4_0, MeshVSMain() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, MeshPSMain() ) ); 
        SetBlendState( NonAlphaState, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );   
        //SetRasterizerState( EnableMSAA );
    }

    pass RenderLightMapPass
    {
        SetVertexShader( CompileShader( vs_4_0, LightMapVSMain() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, LightMapPSMain() ) ); 
        SetBlendState( NonAlphaState, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );   
        //SetRasterizerState( EnableMSAA );
    }
}

technique10 RenderSkyBox
{
	pass RenderPass
    {
        SetVertexShader( CompileShader( vs_4_0, VSSkymain() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PSSkymain() ) ); 
        SetBlendState( NonAlphaState, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );   
    }
}

technique10 RenderCar
{
	pass RenderPass
    {
        SetVertexShader( CompileShader( vs_4_0, VSCarmain() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PSCarmain0() ) ); 
        //SetBlendState( NonAlphaState, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );   
    }	
}
