#pragma once

#include "Object.h"

class CMesh;
class CTransform;
class CShader;

class CSphere : public CObject
{
public:
	CSphere(ID3D11Device* p_Device, ID3D11DeviceContext* p_Context, COMHASHMAP* p_hashMap);
	~CSphere();

public:
	virtual void Init();
	virtual void Update(float p_deltaTime);
	virtual void Render();

private:
	virtual void Release();

private:
	CMesh* m_pMesh;
	CTransform* m_pTransform;
	CShader* m_pShader;

private: //constant buffer
	ID3D11Buffer* m_pCB;
	ID3D11Buffer* m_pCBMtrl;

	TRANSMATRIX		m_mat;
	MATERIAL		m_mtrl;
};

