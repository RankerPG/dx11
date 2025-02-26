#pragma once

#include "Object.h"

class CMesh;
class CTransform;
class CShader;
class CTexture;
class CFrustum;

class CSkyBox : public CObject
{
public:
	CSkyBox(ID3D11Device* p_Device, ID3D11DeviceContext* p_Context, COMHASHMAP* p_hashMap);
	~CSkyBox();

public:
	virtual void Init();
	virtual void Update(float p_deltaTime);
	virtual void LastUpdate(float p_deltaTime);
	virtual void Render(XMMATRIX* p_matAdd = nullptr, BOOL p_isUseMtrl = TRUE);

private:
	virtual void Release();

private:
	CMesh*			m_pMesh;
	CTransform*		m_pTransform;
	CShader*		m_pShader;
	CTexture*		m_pTexture;
	CFrustum*		m_pFrustum;

private: //constant buffer
	ID3D11Buffer*	m_pCB;

	XMFLOAT4X4		m_matWVP;
};