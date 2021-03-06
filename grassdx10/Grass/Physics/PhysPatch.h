#pragma once
//#pragma warning(disable:1563)//taking address of a temporary

#include "includes.h"
#include "GrassPatch.h"
#include "Wind.h"
#include "PhysMath.h"
#include "GrassProperties.h"
#include "Terrain.h"
#include <omp.h>

struct IndexMapData;

/* 20.07 
 * New members: 
 * float fTransparency - Transparency factor for lods 
 * void  TransferFromOtherLod (PhysPatch &a_PhysPatch, bool a_bLod0ToLod1) - function to build the patch from patch 
 of other lod (lod0 => lod1, lod1 => lod0 - both conversions are supported)
 * GrassPatch *m_pBasePatch - base patch of the same lod as current. Required for TransferFromOtherLod() function.
 */

/**
Class, representing grass physical model for one patch
*/
class PhysPatch
{
public:
    /**
    Simply creates all the arrays of maxBlades size
    param maxBlades maximum blades in patch
    */
    PhysPatch(GrassPatch *a_pGrassPatch);

    /**
    Destroys all the arrays
    */
    ~PhysPatch();

    /**
    Makes a physics step
    param sphereRadius sphere collider radius
    param spherePosition sphere collider position
    param dTime delta time in SECONDS(!!!)
    */
    void PhysPatch::UpdatePhysics(const float3 &viewPos, float physLodDst, bool collision, float3 spherePosition, float sphereRadius, float dTime, const std::vector<GrassPropsUnified> &grassProps, const IndexMapData &indexMapData, Mesh *a_pMeshes[], UINT a_iNumMeshes );
    void TransferFromOtherLod ( const PhysPatch &a_PhysPatch, bool a_bLod0ToLod1 );

    /**
    Reinits all data
    */
    void Reinit();

    /**
    */
    void SetTransform( const D3DXMATRIX * a_pMtx );

    /**
    */
    float3 PosToWorld(const float3 &v);
    float3 DirToWorld(const float3 &v);

    /**
    */
    void  IASetPhysVertexBuffer0();
    void  IASetAnimVertexBuffer0();

    /**
    */
    DWORD PhysVerticesCount();
    DWORD AnimVerticesCount();

public:
    /**
    Parameters
    */
	static float eTime;
	static float windStrength;
    static float fWindTexTile;
    static float hardness;
    static float mass;
	static float gravity;
	static float dampfing;
	static float maxAngle;
	static float maxBrokenTime;
	static bool transmitSpringMomentDownwards;
	static bool animation;
    static UINT uTickCount;

    static float fTerrRadius;
    static const WindData* pWindData;
	static float fHeightScale;
	static const TerrainHeightData *pHeightData;

    /**
    Data, used by physics solver
    */
    struct BladePhysData
    {
        float3 startPosition;
        float3 startDirection, startDirectionY;
        float  segmentHeight;
        float  segmentWidth;

        /* Transparency factor */
        float  fTransparency;

        int brokenFlag;
        /* Use Anim or phys */
        int NeedPhysics;
        float brokenTime;
        float physicTime;
        float3 g[NUM_SEGMENTS];   
        float3 w[NUM_SEGMENTS]; 
        float3x3 R[NUM_SEGMENTS];
        float3x3 T[NUM_SEGMENTS];
        float3 position[NUM_SEGMENTS];

        float3x3 A[NUM_SEGMENTS];
        float lerpCoef;

        int type;

        int grabbedMeshIndex;
        bool disableCollision;
    };

private:
    /**
    Output data
    */
    struct VertexPhysData
    {
        float3x3 R[NUM_SEGMENTS-1];
        float3 Position;
        float  fTransparency;
        float3 vColor;
    };

    typedef GrassVertex VertexAnimData;

    /**
    */
    void Broken(float3 &vNormal, float3 &dir, PhysPatch::BladePhysData *bp, float fDist, const GrassPropsUnified &props);
    void Animatin(PhysPatch::BladePhysData *bp, float2 &vTexCoord, Mesh *a_pMeshes[], const GrassPropsUnified &props);
    void RotateSegment(PhysPatch::BladePhysData *bp, int j, const float3 &psi, const GrassPropsUnified &props);

    DWORD numBlades;

    UINT m_dwVertexStride[2];
    UINT m_dwVertexOffset;
    UINT m_dwVerticesCount[2];

    GrassPatch *m_pBasePatch;
    BladePhysData *bladePhysData;

    //static Perlin perlin;
    const D3DXMATRIX *m_pTransform;
    D3DXMATRIX m_InvTransform;
    bool animationPass;

    /* directx10 variables */
    ID3D10Buffer *m_pPhysVertexBuffer;
    ID3D10Buffer *m_pAnimVertexBuffer;
    ID3D10Device *m_pD3DDevice;

    void GenerateBuffer();
    void UpdateBuffer();
};

#include "Mesh/Mesh.h"
