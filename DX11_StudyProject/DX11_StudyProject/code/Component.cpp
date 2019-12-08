#include "Component.h"

CComponent::CComponent(ID3D11Device* p_Device, ID3D11DeviceContext* p_Context)
	: m_pDevice(p_Device)
	, m_pContext(p_Context)
{
	m_pDevice->AddRef();
	m_pContext->AddRef();
}

CComponent::~CComponent()
{
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pContext);
}