#pragma once

#include "includes.h"

class DebugWindow
{
private:
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT2 texture;
	};

public:
	bool Initialize (ID3D11Device*, int, int, ID3D11ShaderResourceView*, float);
	void Shutdown   (void);
	bool Render     (ID3D11DeviceContext*, int, int);

	int  GetIndexCount  (void);
	void SetOrthoMtx    (float4x4& a_mViewProj);
	void SetWorldMtx    (float4x4& a_mWorld);

private:
	bool InitializeBuffers (ID3D11Device*);
	void ShutdownBuffers   ();
	bool UpdateBuffers     (ID3D11DeviceContext*, int, int);
	void RenderBuffers     (ID3D11DeviceContext*);

	void CreateInputLayout (void);

private:
	ID3D11Device				*m_pDevice;
	ID3DX11EffectMatrixVariable *m_pOrthoEMV;
	ID3DX11EffectMatrixVariable* m_pTransformEMV;
	ID3DX11EffectShaderResourceVariable *pTexESRV;

	ID3D11InputLayout* m_pInputLayout;
	ID3D11ShaderResourceView* m_shaderResourceView;

	ID3DX11Effect *m_pEffect;
	ID3DX11EffectPass *m_pPass;
	ID3D11Buffer *m_vertexBuffer, *m_indexBuffer;
	int m_vertexCount, m_indexCount;
	int m_screenWidth, m_screenHeight;
	int m_bitmapWidth, m_bitmapHeight;
	int m_previousPosX, m_previousPosY;

	float m_fScale;

	ID3DX11EffectScalarVariable* m_fScaleESV;
};
