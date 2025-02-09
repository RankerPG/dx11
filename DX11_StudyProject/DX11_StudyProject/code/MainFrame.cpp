#include "MainFrame.h"
#include "device.h"
#include "timer.h"
#include "box.h"
#include "terrain.h"
#include "shader.h"
#include "FigureMesh.h"
#include "Transform.h"
#include "Camera.h"
#include "Sphere.h"
#include "Input.h"
#include "Visible.h"
#include "Texture.h"
#include "Lake.h"
#include "Trees.h"
#include "Frustum.h"
#include "skybox.h"
#include "EnvSphere.h"
#include "DCMCreator.h"
#include "Particle.h"

XMMATRIX g_matView, g_matViewWorld, g_matProj;
UINT g_dwRenderCnt;
BOOL g_isReflect = TRUE;

CMainFrame::CMainFrame(CDevice* p_Device)
	: m_pGraphicDevice(p_Device)
	, m_pInput(CInput::Get_Instance())
	, m_pDevice(nullptr)
	, m_pContext(nullptr)
	, m_pState{ nullptr, nullptr, nullptr, nullptr, nullptr }
	, m_pSampler{ nullptr, nullptr }
	, m_pBlend{ nullptr, nullptr, nullptr }
	, m_pDepthStencil{ nullptr, nullptr, nullptr, nullptr }
	, m_pCamera(nullptr)
	, m_pMainTimer(new CTimer())
	, m_pBox(nullptr)
	, m_pTerrain(nullptr)
	, m_pSphere(nullptr)
	, m_pVisible(nullptr)
	, m_pLake(nullptr)
	, m_pTrees(nullptr)
	, m_pSkyBox(nullptr)
	, m_pEnvSphere(nullptr)
	, m_pFrustum(nullptr)
	, m_pCreator(nullptr)
	, m_arrShader{ nullptr, }
	, m_pCBLight(nullptr)
	, m_pCBPointLight(nullptr)
	, m_pCBPerFrame(nullptr)
	, m_pCBMtrl(nullptr)
	, m_pCBTess(nullptr)
	, m_Light(LIGHT())
	, m_PointLight(POINTLIGHT())
	, m_PerFrame(PERFRAME())
	, m_ShadowMtrl(MATERIAL())
	, m_Tess(TESS())
	, m_dwFrameCnt(0)
	, m_fElapsedTime(0.f)
	, m_isWireFrame(FALSE)
{
}

CMainFrame::~CMainFrame()
{
	Release();
}

void CMainFrame::Init()
{
	Create_Device();
	Create_Components();

	Create_RasterizerState();
	Create_SamplerState();
	Create_BlendState();
	Create_DepthStencilState();

	Update_RasterizerState(RASTERIZER::CULLBACK);
	Update_SamplerState(SAMPLER::WRAP);
	Update_BlendState(BLEND::NONALPHA);

	Create_Object();
}

void CMainFrame::Update()
{
	m_pMainTimer->Tick();
	Calculate_FPS();
	float MainTime = m_pMainTimer->Get_MainTime();
	float deltaTime = m_pMainTimer->Get_DeltaTime();

	Update_Input();

	m_pSkyBox->Update(MainTime);

	m_pCamera->Update(MainTime);
	m_pBox->Update(deltaTime);
	m_pTerrain->Update(deltaTime);
	m_pSphere->Update(deltaTime);
	m_pVisible->Update(deltaTime);
	m_pLake->Update(deltaTime);
	m_pTrees->Update(deltaTime);
	m_pEnvSphere->Update(deltaTime);
}

void CMainFrame::Last_Update()
{
	float MainTime = m_pMainTimer->Get_MainTime();
	float deltaTime = m_pMainTimer->Get_DeltaTime();

	g_dwRenderCnt = 0;

	// 절두체 평면 갱신
	m_pFrustum->Update_Frustum(&g_matView, &g_matProj);

	m_pSkyBox->LastUpdate(MainTime);

	// 객체별 컬링 판정
	m_pCamera->LastUpdate(MainTime);
	m_pBox->LastUpdate(deltaTime);
	m_pTerrain->LastUpdate(deltaTime);
	m_pSphere->LastUpdate(deltaTime);
	m_pVisible->LastUpdate(deltaTime);
	m_pLake->LastUpdate(deltaTime);
	m_pTrees->LastUpdate(deltaTime);
	m_pEnvSphere->LastUpdate(deltaTime);
}

void CMainFrame::Render()
{
	m_pCreator->Update_RenderTarget(XMFLOAT3(-5.f, 2.f, 0.f), this);

	m_pGraphicDevice->Begin_Render();

	Render_Default();

	// 동적 큐브 맵 객체 따로 렌더
	m_pEnvSphere->Render();

	Render_Stencil();

	Render_Shadow();

	Render_Color();

	m_pGraphicDevice->End_Render();
}

void CMainFrame::Release()
{
	m_mapComponent.clear();

	SAFE_RELEASE(m_pCBLight);
	SAFE_RELEASE(m_pCBPointLight);
	SAFE_RELEASE(m_pCBPerFrame);
	SAFE_RELEASE(m_pCBMtrl);
	SAFE_RELEASE(m_pCBTess);

	for (int i = 0; i < 5; ++i)
	{
		SAFE_RELEASE(m_pState[i]);
	}

	for (int i = 0; i < (int)SAMPLER::SMP_END; ++i)
	{
		SAFE_RELEASE(m_pSampler[i]);
	}

	for (int i = 0; i < (int)BLEND::BLEND_END; ++i)
	{
		SAFE_RELEASE(m_pBlend[i]);
	}

	for (int i = 0; i < (int)DEPTHSTENCIL::DS_END; ++i)
	{
		SAFE_RELEASE(m_pDepthStencil[i]);
	}

	SAFE_RELEASE(m_pContext);
	SAFE_RELEASE(m_pDevice);
}

void CMainFrame::Calculate_FPS()
{
	++m_dwFrameCnt;
	m_fElapsedTime += m_pMainTimer->Get_DeltaTime();

	if (m_fElapsedTime >= 1.f)
	{
		float fFrameTime = 1000.f / m_dwFrameCnt;

		wchar_t wText[128] = L"";
		_stprintf_s(wText, L"FPS : %d / Frame Time : %f(ms) / Rendering Cnt : %d", m_dwFrameCnt, fFrameTime, g_dwRenderCnt);
		SetWindowText(g_hWnd, wText);

		m_dwFrameCnt = 0;
		m_fElapsedTime = 0.f;
	}
}

void CMainFrame::Create_Device()
{
	if (FALSE == m_pGraphicDevice->Init_Device(1280, 720))
	{
		m_pGraphicDevice->Release_Instance();
		return;
	}

	//전역 행렬 초기화
	g_matView = XMMatrixIdentity();
	g_matProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.f / 720.f, 0.1f, 1000.f);

	//
	m_pDevice = m_pGraphicDevice->Get_Device();
	m_pContext = m_pGraphicDevice->Get_Context();
	m_pDevice->AddRef();
	m_pContext->AddRef();
}

void CMainFrame::Create_Components()
{
	// 컴포넌트 생성
	m_pFrustum = CFrustum::Create_Frustum(m_pDevice, m_pContext);
	m_mapComponent.insert(make_pair("Frustum", m_pFrustum));

	m_pCreator = CDCMCreator::Create_DCM(m_pDevice, m_pContext);
	m_mapComponent.insert(make_pair("DCMCreator", m_pCreator));

	// 쉐이더
	CShader* pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/Default.fx", "vs_main", "ps_main", 0);
	m_mapComponent.insert(make_pair("DefaultFX", pShader));

	m_arrShader[(int)SHADER::LIGHT] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/Light.fx", "vs_main", "ps_main", 1);
	m_mapComponent.insert(make_pair("LightFX", pShader));

	pShader->Create_ConstantBuffer(&m_Light, sizeof(LIGHT), &m_pCBLight);
	pShader->Create_ConstantBuffer(&m_PointLight, sizeof(POINTLIGHT), &m_pCBPointLight);
	pShader->Create_ConstantBuffer(&m_PerFrame, sizeof(PERFRAME), &m_pCBPerFrame);
	pShader->Create_ConstantBuffer(&m_ShadowMtrl, sizeof(MATERIAL), &m_pCBMtrl);
	pShader->Create_ConstantBuffer(&m_Tess, sizeof(TESS), &m_pCBTess);

	XMVECTOR vDir = XMVectorSet(1.f, -1.f, 1.f, 0.f); //XMVectorSet(0.f, -0.8f, 1.f, 0.f); 
	vDir = XMVector3Normalize(-vDir);
	XMStoreFloat3A(&m_Light.direction, vDir);
	m_Light.diffuse = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	m_Light.ambient = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	m_Light.specular = XMFLOAT4(1.f, 1.f, 1.f, 1.f);

	m_PointLight.position = XMFLOAT3A(0.f, 5.f, 0.f);
	m_PointLight.Range = 20.f;
	m_PointLight.diffuse = XMFLOAT4(1.f, 0.f, 0.f, 1.f);
	m_PointLight.ambient = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	m_PointLight.specular = XMFLOAT4(1.f, 0.f, 0.f, 1.f);

	m_PerFrame.fogColor = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	m_PerFrame.fogStart = 50.f;
	m_PerFrame.fogRange = 100.f;

	m_ShadowMtrl.diffuse = XMFLOAT4(0.f, 0.f, 0.f, 0.5f);
	m_ShadowMtrl.ambient = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	m_ShadowMtrl.specular = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	m_ShadowMtrl.Env = XMFLOAT4(0.f, 0.f, 0.f, 0.f);

	m_Tess.maxDistance = 1.f;
	m_Tess.minDistance = 30.f;
	m_Tess.maxFactor = 5.f;
	m_Tess.minFactor = 1.f;
	m_Tess.heightScale.x = 0.25f;

	m_arrShader[(int)SHADER::TEXTURE] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/Texture.fx", "vs_main", "ps_main", 2);
	m_mapComponent.insert(make_pair("TextureFX", pShader));
	// 상수 버퍼는 라이트 쉐이더와 공유

	m_arrShader[(int)SHADER::GEOMETRY] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/Geometry.fx", "vs_main", "ps_main", 2);
	pShader->Create_GeometryShader(L"./Shader/Geometry.fx", "gs_main", "gs_5_0");
	m_mapComponent.insert(make_pair("GeometryFX", pShader));

	m_arrShader[(int)SHADER::BILLBORAD] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/Billboard.fx", "vs_main", "ps_main", 3);
	pShader->Create_GeometryShader(L"./Shader/Billboard.fx", "gs_main", "gs_5_0");
	m_mapComponent.insert(make_pair("BillboardFX", pShader));
	
	m_arrShader[(int)SHADER::INSTANCE] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/Instancing.fx", "vs_main", "ps_main", 4);
	m_mapComponent.insert(make_pair("InstanceFX", pShader));
	
	m_arrShader[(int)SHADER::SKYBOX] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/Skybox.fx", "vs_main", "ps_main");
	m_mapComponent.insert(make_pair("SkyboxFX", pShader));

	m_arrShader[(int)SHADER::ENVMAP] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/EnvMapping.fx", "vs_main", "ps_main", 2);
	m_mapComponent.insert(make_pair("EnvMapFX", pShader));

	m_arrShader[(int)SHADER::NORMALMAP] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/NormalMapping.fx", "vs_main", "ps_main", 5);
	m_mapComponent.insert(make_pair("NrmMapFX", pShader));

	m_arrShader[(int)SHADER::DISPLACEMENT] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/Displacement.fx", "vs_main", "ps_main", 5);
	pShader->Create_HullShader(L"./Shader/Displacement.fx", "HS", "hs_5_0");
	pShader->Create_DomainShader(L"./Shader/Displacement.fx", "DS", "ds_5_0");
	m_mapComponent.insert(make_pair("DisplacementFX", pShader));

	m_arrShader[(int)SHADER::WAVE] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/wave.fx", "vs_main", "ps_main", 5);
	pShader->Create_HullShader(L"./Shader/wave.fx", "HS", "hs_5_0");
	pShader->Create_DomainShader(L"./Shader/wave.fx", "DS", "ds_5_0");
	m_mapComponent.insert(make_pair("WaveFX", pShader));

	m_arrShader[(int)SHADER::SO_RAIN] = pShader = CShader::Create_Shader(m_pDevice, m_pContext, L"./Shader/SO_Rain.fx", "vs_main", nullptr, 6);
	pShader->Create_GeometryShader(L"./Shader/SO_Rain.fx", "StreamOutGS", "gs_5_0");
	m_mapComponent.insert(make_pair("SORainFX", pShader));
	
	// 메쉬
	CMesh* pMesh = CFigureMesh::Create_FigureMesh(m_pDevice, m_pContext);
	m_mapComponent.insert(make_pair("CubeNrmMesh", pMesh));

	pMesh = CFigureMesh::Create_FigureMesh(m_pDevice, m_pContext, 1);
	m_mapComponent.insert(make_pair("SphereMesh", pMesh));

	pMesh = CFigureMesh::Create_FigureMesh(m_pDevice, m_pContext, 2);
	m_mapComponent.insert(make_pair("TerrainMesh", pMesh));

	pMesh = CFigureMesh::Create_FigureMesh(m_pDevice, m_pContext, 3);
	m_mapComponent.insert(make_pair("CubeTexMesh", pMesh));

	pMesh = CFigureMesh::Create_FigureMesh(m_pDevice, m_pContext, 4);
	m_mapComponent.insert(make_pair("SphereTexMesh", pMesh));

	pMesh = CFigureMesh::Create_FigureMesh(m_pDevice, m_pContext, 5);
	m_mapComponent.insert(make_pair("TerrainTexMesh", pMesh));
	
	pMesh = CFigureMesh::Create_FigureMesh(m_pDevice, m_pContext, 6);
	m_mapComponent.insert(make_pair("QuadNrmMesh", pMesh));

	pMesh = CFigureMesh::Create_FigureMesh(m_pDevice, m_pContext, 7);
	m_mapComponent.insert(make_pair("SphereNrmMesh", pMesh));

	pMesh = CFigureMesh::Create_FigureMesh(m_pDevice, m_pContext, 8);
	m_mapComponent.insert(make_pair("TileQuadNrmMesh", pMesh));

	// 트랜스폼
	CTransform* pTransform = CTransform::Create_Transform(m_pDevice, m_pContext);
	m_mapComponent.insert(make_pair("Transform", pTransform));

	// 텍스쳐
	CTexture* pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/WoodBox.dds", FALSE);
	m_mapComponent.insert(make_pair("BoxTexture", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/Floor.jpg");
	m_mapComponent.insert(make_pair("CubeTexture", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/Floor_N.jpg");
	m_mapComponent.insert(make_pair("CubeTexture_N", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/WireFence.dds", FALSE);
	m_mapComponent.insert(make_pair("WireTexture", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/earth.bmp");
	m_mapComponent.insert(make_pair("EarthTexture", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/earth_N.png");
	m_mapComponent.insert(make_pair("EarthTexture_N", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/stones.dds", FALSE);
	m_mapComponent.insert(make_pair("TerrainTexture", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/stones_N.dds", FALSE);
	m_mapComponent.insert(make_pair("TerrainTexture_N", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/Water1.dds", FALSE);
	m_mapComponent.insert(make_pair("WaterTexture", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/Water2.dds", FALSE);
	m_mapComponent.insert(make_pair("WaterTexture_2", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/waves0.dds", FALSE);
	m_mapComponent.insert(make_pair("WaveTexture_N", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/waves1.dds", FALSE);
	m_mapComponent.insert(make_pair("WaveTexture_N2", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/tree%d.dds", FALSE, 4);
	m_mapComponent.insert(make_pair("TreeTexture", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext, L"./Texture/skybox.dds", FALSE);
	m_mapComponent.insert(make_pair("SkyTexture", pTexture));

	pTexture = CTexture::Create_Texture(m_pDevice, m_pContext);
	m_mapComponent.insert(make_pair("RandomTexture", pTexture));
}

void CMainFrame::Create_Object()
{
	// 오브젝트 생성
	m_pMainTimer->Reset();

	m_pCamera.reset(new CCamera(m_pDevice, m_pContext, &m_mapComponent));
	m_pCamera->Init();

	m_pBox.reset(new CBox(m_pDevice, m_pContext, &m_mapComponent));
	m_pBox->Init();

	m_pTerrain.reset(new CTerrain(m_pDevice, m_pContext, &m_mapComponent));
	m_pTerrain->Init();

	m_pSphere.reset(new CSphere(m_pDevice, m_pContext, &m_mapComponent));
	m_pSphere->Init();

	m_pVisible.reset(new CVisible(m_pDevice, m_pContext, &m_mapComponent));
	m_pVisible->Init();

	m_pLake.reset(new CLake(m_pDevice, m_pContext, &m_mapComponent));
	m_pLake->Init();

	m_pTrees.reset(new CTrees(m_pDevice, m_pContext, &m_mapComponent));
	m_pTrees->Init();

	m_pSkyBox.reset(new CSkyBox(m_pDevice, m_pContext, &m_mapComponent));
	m_pSkyBox->Init();

	m_pEnvSphere.reset(new CEnvSphere(m_pDevice, m_pContext, &m_mapComponent));
	m_pEnvSphere->Init();
}

void CMainFrame::Create_RasterizerState()
{
	D3D11_RASTERIZER_DESC rd;
	ZeroMemory(&rd, sizeof(D3D11_RASTERIZER_DESC));

	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_BACK;
	rd.FrontCounterClockwise = FALSE;
	rd.DepthClipEnable = TRUE;

	if (FAILED(m_pDevice->CreateRasterizerState(&rd, &m_pState[0])))
	{
		MessageBox(g_hWnd, L"Create RasterizerState Failed!!", 0, 0);
		return;
	}

	rd.FrontCounterClockwise = TRUE;

	if (FAILED(m_pDevice->CreateRasterizerState(&rd, &m_pState[1])))
	{
		MessageBox(g_hWnd, L"Create RasterizerState Failed!!", 0, 0);
		return;
	}

	rd.FillMode = D3D11_FILL_WIREFRAME;
	rd.FrontCounterClockwise = FALSE;
	if (FAILED(m_pDevice->CreateRasterizerState(&rd, &m_pState[2])))
	{
		MessageBox(g_hWnd, L"Create RasterizerState Failed!!", 0, 0);
		return;
	}

	rd.FrontCounterClockwise = TRUE;

	if (FAILED(m_pDevice->CreateRasterizerState(&rd, &m_pState[3])))
	{
		MessageBox(g_hWnd, L"Create RasterizerState Failed!!", 0, 0);
		return;
	}

	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	rd.FrontCounterClockwise = FALSE;

	if (FAILED(m_pDevice->CreateRasterizerState(&rd, &m_pState[4])))
	{
		MessageBox(g_hWnd, L"Create RasterizerState Failed!!", 0, 0);
		return;
	}
}

void CMainFrame::Create_SamplerState()
{
	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));

	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.MaxAnisotropy = 1;
	sd.MinLOD = 0;
	sd.MaxLOD = 0;
	sd.MipLODBias = 0;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.BorderColor[0] = 1;
	sd.BorderColor[1] = 1;
	sd.BorderColor[2] = 1;
	sd.BorderColor[3] = 1;

	sd.MaxLOD = D3D11_FLOAT32_MAX;
	sd.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	
	if (FAILED(m_pDevice->CreateSamplerState(&sd, &m_pSampler[0])))
	{
		MessageBox(g_hWnd, L"Create SamplerState Failed!!", 0, 0);
		return;
	}

	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

	if (FAILED(m_pDevice->CreateSamplerState(&sd, &m_pSampler[1])))
	{
		MessageBox(g_hWnd, L"Create SamplerState Failed!!", 0, 0);
		return;
	}
}

void CMainFrame::Create_BlendState()
{
	D3D11_BLEND_DESC bd;
	ZeroMemory(&bd, sizeof(D3D11_BLEND_DESC));

	bd.AlphaToCoverageEnable = TRUE;
	bd.IndependentBlendEnable = FALSE;

	D3D11_RENDER_TARGET_BLEND_DESC rtbd;
	ZeroMemory(&rtbd, sizeof(D3D11_RENDER_TARGET_BLEND_DESC));

	rtbd.BlendEnable = FALSE;
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlend = D3D11_BLEND_ONE;
	rtbd.DestBlend = D3D11_BLEND_ZERO;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	
	bd.RenderTarget[0] = rtbd;

	if (FAILED(m_pDevice->CreateBlendState(&bd, &m_pBlend[0])))
	{
		MessageBox(g_hWnd, L"Create BlendState Failed!!", 0, 0);
		return;
	}

	//
	rtbd.BlendEnable = TRUE;
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;


	bd.RenderTarget[0] = rtbd;

	if (FAILED(m_pDevice->CreateBlendState(&bd, &m_pBlend[1])))
	{
		MessageBox(g_hWnd, L"Create BlendState Failed!!", 0, 0);
		return;
	}

	//
	rtbd.BlendEnable = TRUE;
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlend = D3D11_BLEND_ONE;
	rtbd.DestBlend = D3D11_BLEND_ONE;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;


	bd.RenderTarget[0] = rtbd;

	if (FAILED(m_pDevice->CreateBlendState(&bd, &m_pBlend[2])))
	{
		MessageBox(g_hWnd, L"Create BlendState Failed!!", 0, 0);
		return;
	}
}

void CMainFrame::Create_DepthStencilState()
{
	D3D11_DEPTH_STENCIL_DESC dsd;
	ZeroMemory(&dsd, sizeof(D3D11_DEPTH_STENCIL_DESC));
	
	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;
	
	dsd.StencilEnable = FALSE;

	if (FAILED(m_pDevice->CreateDepthStencilState(&dsd, &m_pDepthStencil[0])))
	{
		MessageBox(g_hWnd, L"Create DepthStencilState Failed!!", 0, 0);
		return;
	}

	//
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	
	dsd.StencilEnable = TRUE;
	dsd.StencilReadMask = 0xff;
	dsd.StencilWriteMask = 0xff;

	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	dsd.BackFace = dsd.FrontFace;

	if (FAILED(m_pDevice->CreateDepthStencilState(&dsd, &m_pDepthStencil[1])))
	{
		MessageBox(g_hWnd, L"Create DepthStencilState Failed!!", 0, 0);
		return;
	}

	//
	dsd.DepthEnable = FALSE;

	dsd.StencilEnable = TRUE;
	dsd.StencilReadMask = 0xff;
	dsd.StencilWriteMask = 0xff;

	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	dsd.BackFace = dsd.FrontFace;

	if (FAILED(m_pDevice->CreateDepthStencilState(&dsd, &m_pDepthStencil[2])))
	{
		MessageBox(g_hWnd, L"Create DepthStencilState Failed!!", 0, 0);
		return;
	}

	//
	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

	dsd.StencilEnable = TRUE;
	dsd.StencilReadMask = 0xff;
	dsd.StencilWriteMask = 0xff;

	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	dsd.BackFace = dsd.FrontFace;

	if (FAILED(m_pDevice->CreateDepthStencilState(&dsd, &m_pDepthStencil[3])))
	{
		MessageBox(g_hWnd, L"Create DepthStencilState Failed!!", 0, 0);
		return;
	}

	//
	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	dsd.StencilEnable = FALSE;

	dsd.BackFace = dsd.FrontFace;

	if (FAILED(m_pDevice->CreateDepthStencilState(&dsd, &m_pDepthStencil[4])))
	{
		MessageBox(g_hWnd, L"Create DepthStencilState Failed!!", 0, 0);
		return;
	}
}

void CMainFrame::Update_RasterizerState(RASTERIZER p_eRS)
{
	if (RASTERIZER::CULLBACK == p_eRS)
	{
		if (TRUE == m_isWireFrame)
		{
			m_pContext->RSSetState(m_pState[2]);
		}
		else
		{
			m_pContext->RSSetState(m_pState[0]);
		}
	}
	else if (RASTERIZER::CULLFRONT == p_eRS)
	{
		if (TRUE == m_isWireFrame)
		{
			m_pContext->RSSetState(m_pState[3]);
		}
		else
		{
			m_pContext->RSSetState(m_pState[1]);
		}
	}
	else if (RASTERIZER::CULLNONE == p_eRS)
	{
		m_pContext->RSSetState(m_pState[4]);
	}
}

void CMainFrame::Update_SamplerState(SAMPLER p_eSS)
{
	if (SAMPLER::WRAP == p_eSS)
	{
		m_pContext->DSSetSamplers(0, 1, &m_pSampler[0]);
		m_pContext->PSSetSamplers(0, 1, &m_pSampler[0]);
	}
	else if (SAMPLER::CLAMP == p_eSS)
	{
		m_pContext->DSSetSamplers(0, 1, &m_pSampler[1]);
		m_pContext->PSSetSamplers(0, 1, &m_pSampler[1]);
	}
}

void CMainFrame::Update_BlendState(BLEND p_eBS)
{
	float factor[] = { 0.f, 0.f, 0.f, 0.f };

	if (BLEND::NONALPHA == p_eBS)
	{
		m_pContext->OMSetBlendState(m_pBlend[0], factor, 0xffffffff);
	}
	else if (BLEND::ALPHA == p_eBS)
	{
		m_pContext->OMSetBlendState(m_pBlend[1], factor, 0xffffffff);
	}
	else
	{
		m_pContext->OMSetBlendState(m_pBlend[2], factor, 0xffffffff);
	}
}

void CMainFrame::Update_DepthStencilState(DEPTHSTENCIL eDS)
{
	if (DEPTHSTENCIL::DEPTH == eDS)
	{
		m_pContext->OMSetDepthStencilState(m_pDepthStencil[0], 1);
	}
	else if (DEPTHSTENCIL::STENCILON == eDS)
	{
		m_pContext->OMSetDepthStencilState(m_pDepthStencil[1], 1);
	}
	else if (DEPTHSTENCIL::DRAWSTENCIL == eDS)
	{
		m_pContext->OMSetDepthStencilState(m_pDepthStencil[2], 1);
	}
	else if (DEPTHSTENCIL::SHADOWON == eDS)
	{
		m_pContext->OMSetDepthStencilState(m_pDepthStencil[3], 0);
	}
	else if (DEPTHSTENCIL::DEPTHOFF == eDS)
	{
		m_pContext->OMSetDepthStencilState(m_pDepthStencil[4], 0);
	}
}

void CMainFrame::Update_ShaderArray()
{
	for (int i = 0; i < (int)SHADER::SHADER_END; ++i)
	{
		if (nullptr == m_arrShader[i])
			continue;

		XMStoreFloat3A(&m_PerFrame.viewPos, m_pCamera->Get_ViewPos());
		m_arrShader[i]->Update_ConstantBuffer(&m_Light, sizeof(LIGHT), m_pCBLight, 1);
		XMStoreFloat3(&m_PointLight.position, m_pVisible->Get_PointLightPos());
		m_arrShader[i]->Update_ConstantBuffer(&m_PointLight, sizeof(POINTLIGHT), m_pCBPointLight, 4);
		m_arrShader[i]->Update_ConstantBuffer(&m_PerFrame, sizeof(PERFRAME), m_pCBPerFrame, 3);
		m_arrShader[i]->Update_ConstantBuffer(&m_Tess, sizeof(TESS), m_pCBTess, 5);
	}
}

void CMainFrame::Update_Shader(int p_shaderNum)
{
	XMStoreFloat3A(&m_PerFrame.viewPos, m_pCamera->Get_ViewPos());
	m_arrShader[p_shaderNum]->Update_ConstantBuffer(&m_Light, sizeof(LIGHT), m_pCBLight, 1);
	XMStoreFloat3(&m_PointLight.position, m_pVisible->Get_PointLightPos());
	m_arrShader[p_shaderNum]->Update_ConstantBuffer(&m_PointLight, sizeof(POINTLIGHT), m_pCBPointLight, 4);
	m_arrShader[p_shaderNum]->Update_ConstantBuffer(&m_PerFrame, sizeof(PERFRAME), m_pCBPerFrame, 3);
	m_arrShader[p_shaderNum]->Update_ConstantBuffer(&m_Tess, sizeof(TESS), m_pCBTess, 5);
}

void CMainFrame::Update_Input()
{
	if (m_pInput->Get_DIKPressState(DIK_SPACE))
	{
		m_isWireFrame ^= TRUE;

		Update_RasterizerState(RASTERIZER::CULLBACK);
	}

	if (m_pInput->Get_DIKPressState(DIK_LSHIFT))
	{
		if (TRUE == m_pMainTimer->Get_isStopped())
		{
			m_pMainTimer->Start();
		}
		else
		{
			m_pMainTimer->Stop();
		}
	}

	if (m_pInput->Get_DIKPressState(DIK_1))
	{
		g_isReflect ^= TRUE;
	}
}

void CMainFrame::Render_Default()
{
	Update_RasterizerState(RASTERIZER::CULLNONE);
	Update_DepthStencilState(DEPTHSTENCIL::DEPTHOFF);

	Update_ShaderArray();

	// 그리기
	m_pSkyBox->Render();
	
	Update_RasterizerState(RASTERIZER::CULLBACK);
	Update_DepthStencilState(DEPTHSTENCIL::DEPTH);

	m_pTerrain->Render();
	
	if (TRUE == m_pSphere->Get_Visible())
	{
		m_pSphere->Render();
	}

	if (TRUE == m_pBox->Get_Visible())
	{
		m_pBox->Render();
	}

	if (TRUE == m_pBox->Get_Visible())
	{
		Update_RasterizerState(RASTERIZER::CULLFRONT);

		m_pBox->Render();

		Update_RasterizerState(RASTERIZER::CULLBACK);
	}

	Update_RasterizerState(RASTERIZER::CULLNONE);

	Update_SamplerState(SAMPLER::CLAMP);

	m_pTrees->Render();

	Update_RasterizerState(RASTERIZER::CULLBACK);

	Update_SamplerState(SAMPLER::WRAP);
}

void CMainFrame::Render_CubeMap()
{
	Update_RasterizerState(RASTERIZER::CULLNONE);
	Update_BlendState(BLEND::NONALPHA);
	Update_DepthStencilState(DEPTHSTENCIL::DEPTHOFF);

	Update_ShaderArray();

	// 그리기
	m_pSkyBox->Render();

	Update_RasterizerState(RASTERIZER::CULLBACK);
	Update_DepthStencilState(DEPTHSTENCIL::DEPTH);

	m_pTerrain->Render();
	m_pSphere->Render();

	// 알파 블렌딩
	Update_BlendState(BLEND::ALPHA);

	m_pBox->Render();

	Update_RasterizerState(RASTERIZER::CULLFRONT);

	m_pBox->Render();

	Update_RasterizerState(RASTERIZER::CULLNONE);

	Update_SamplerState(SAMPLER::CLAMP);

	m_pTrees->Render();

	Update_SamplerState(SAMPLER::WRAP);

	m_pLake->Render();
}

void CMainFrame::Render_Stencil()
{
	// 스텐실
	Update_DepthStencilState(DEPTHSTENCIL::STENCILON);

	Update_BlendState(BLEND::ALPHA);

	m_pLake->Render();

	// 반사 객체 그리기
	Update_DepthStencilState(DEPTHSTENCIL::DRAWSTENCIL);

	XMVECTOR stencilPlane = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMMATRIX R = XMMatrixReflect(stencilPlane);

	XMVECTOR litDir = XMLoadFloat3A(&m_Light.direction);
	XMVECTOR litReflectDir = XMVector3TransformNormal(-litDir, R);
	XMStoreFloat3A(&m_Light.direction, litReflectDir);

	Update_Shader((int)SHADER::TEXTURE);
	Update_Shader((int)SHADER::GEOMETRY);

	m_pBox->Render(&R);
	m_pSphere->Render(&R);

	Update_RasterizerState(RASTERIZER::CULLFRONT);

	m_pBox->Render(&R);

	Update_RasterizerState(RASTERIZER::CULLBACK);

	m_pEnvSphere->Render(&R);

	Update_DepthStencilState(DEPTHSTENCIL::DEPTH);

	XMStoreFloat3A(&m_Light.direction, litDir);

	Update_BlendState(BLEND::ALPHA);
	m_pLake->Render();

	Update_Shader((int)SHADER::TEXTURE);
	Update_Shader((int)SHADER::GEOMETRY);
}

void CMainFrame::Render_Shadow()
{
	m_pGraphicDevice->Clear_Stencil();

	Update_DepthStencilState(DEPTHSTENCIL::SHADOWON);

	XMVECTOR vShadow = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMVECTOR vShadowLit = XMLoadFloat3A(&m_Light.direction);
	XMMATRIX matShadow = XMMatrixShadow(vShadow, vShadowLit);
	matShadow *= XMMatrixTranslation(0.f, 0.01f, 0.f);

	m_arrShader[(int)SHADER::TEXTURE]->Update_ConstantBuffer(&m_ShadowMtrl, sizeof(MATERIAL), m_pCBMtrl, 2);
	m_arrShader[(int)SHADER::ENVMAP]->Update_ConstantBuffer(&m_ShadowMtrl, sizeof(MATERIAL), m_pCBMtrl, 2);
	m_arrShader[(int)SHADER::NORMALMAP]->Update_ConstantBuffer(&m_ShadowMtrl, sizeof(MATERIAL), m_pCBMtrl, 2);

	Update_BlendState(BLEND::ALPHA);

	m_pSphere->Render(&matShadow, FALSE);
	m_pBox->Render(&matShadow, FALSE);

	Update_SamplerState(SAMPLER::CLAMP);
	m_pTrees->Render(&matShadow, FALSE);
	Update_SamplerState(SAMPLER::WRAP);

	m_pEnvSphere->Render(&matShadow, FALSE);

	Update_RasterizerState(RASTERIZER::CULLBACK);

	Update_DepthStencilState(DEPTHSTENCIL::DEPTH);

	Update_BlendState(BLEND::NONALPHA);
}

void CMainFrame::Render_Color()
{
	// 컬러 객체
	Update_BlendState(BLEND::NONALPHA);

	if (TRUE == m_pVisible->Get_Visible())
	{
		m_pVisible->Render();
	}
}
