#pragma once

#include "Object.h"

class CTransform;
class CShader;
class CTexture;

class CRain : public CObject
{
public:
	CRain(ID3D11Device* p_Device, ID3D11DeviceContext* p_Context, COMHASHMAP* p_hashMap);
	~CRain();

public:
	virtual void Init();
	virtual void Update(float p_deltaTime);
	virtual void LastUpdate(float p_deltaTime);
	virtual void Render(XMMATRIX* p_matAdd = nullptr, BOOL p_isUseMtrl = TRUE);

private:
	virtual void Release();

private:
	CTransform*		m_pTransform;
	CShader*		m_pShader;
	CTexture*		m_pRandomTexture;

private: //constant buffer
	ID3D11Buffer*	m_pCB;
	ID3D11Buffer*	m_pCBMtrl;

	TRANSMATRIX		m_mat;
	MATERIAL		m_mtrl;
};