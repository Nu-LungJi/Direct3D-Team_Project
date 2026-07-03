#include "pch.h"
#include "GameInstance.h"

#include "TimeProvider.h"
#include "ImGuiManager.h"
#include "DInputManager.h"
#include "GraphicDevice.h"
#include "LevelManager.h"
#include "Level.h"
#include "SoundManager.h"
#include "FontManager.h"
#include "PrototypeManager.h"
#include "Prototype.h"
#include "ColliderManager.h"
#include "Collider.h"
#include "Renderer.h"
#include "ComConstantBuffer.h"
#include "FlyCamera.h"
#include "UICamera.h"
#include "ComBeHavior.h"
#include "AnimEdit_Manager.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Light.h"
#include "ComCollider.h"

NS_USING(Engine)

CGameInstance::CGameInstance()
{
}

CGameInstance::~CGameInstance()
{
}

HRESULT CGameInstance::InitializeEngine(const ENGINE_DESC& EngineDesc, ComPtr<ID3D11Device>& ppDevice, ComPtr<ID3D11DeviceContext>& ppContext)
{


	m_hWnd = EngineDesc.hWnd;
	m_vClientScreenSize.x = (float)EngineDesc.iWinSizeX;
	m_vClientScreenSize.y = (float)EngineDesc.iWinSizeY;


	m_pGraphicDevice = CGraphicDevice::Create(ppDevice, ppContext);
	if (m_pGraphicDevice == nullptr)
	{
		return E_FAIL;
	}

	m_pResourceManager = CResourceManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pResourceManager == nullptr)
	{
		return E_FAIL;
	}

	m_pSoundManager = CSoundManager::Create();
	if (m_pSoundManager == nullptr)
	{
		return E_FAIL;
	}

	if (FAILED(m_pGraphicDevice->ReadyDevice(EngineDesc.hWnd, EngineDesc.eWinMode, EngineDesc.iWinSizeX, EngineDesc.iWinSizeY)))
	{
		return E_FAIL;
	}


	if (FAILED(InitializeResources()))
	{
		return E_FAIL;
	}

	m_pPrototypeManager = CPrototypeManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pPrototypeManager == nullptr)
	{
		return E_FAIL;
	}

	if (FAILED(InitializePrototype()))
	{
		return E_FAIL;
	}

	m_pImguiManager = CImguiManager::Create(EngineDesc.hWnd, ppDevice.Get(), ppContext.Get());
	if (m_pImguiManager == nullptr)
	{
		return E_FAIL;
	}

	m_pLevelManager = CLevelManager::Create();
	if (m_pLevelManager == nullptr)
	{
		return E_FAIL;
	}

	m_pDInputManager = CDInputManager::Create(EngineDesc.hInstance, EngineDesc.hWnd);
	if (m_pDInputManager == nullptr)
	{
		return E_FAIL;
	}

	m_pTimeProvider = CTimeProvider::Create();
	if (m_pTimeProvider == nullptr)
	{
		return E_FAIL;
	}

	m_pWorkerManager = CWorkerManager::Create("Normal", 3);
	if (m_pWorkerManager == nullptr)
	{
		return E_FAIL;
	}

	m_pGameObjectManager = CGameObjectManager::Create();
	if (m_pGameObjectManager == nullptr)
	{
		return E_FAIL;
	}

	m_pRenderer = CRenderer::Create(ppDevice.Get(), ppContext.Get());
	if (m_pRenderer == nullptr)
	{
		return E_FAIL;
	}

	m_pCameraManager = CCameraManager::Create();
	if (m_pCameraManager == nullptr)
	{
		return E_FAIL;
	}

	m_pColliderManager = CColliderManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pColliderManager == nullptr)
	{
		return E_FAIL;
	}

	m_pAnimEdit_Manager = CAnimEdit_Manager::Create();
	if(m_pAnimEdit_Manager == nullptr)
	{
		return E_FAIL;
	}	

	//m_pLightManager = CLightManager::Create(ppDevice.Get(), ppContext.Get());
	//if (m_pLightManager == nullptr)
	//{
	//	return E_FAIL;
	//}


	//m_pParticleManager = CParticleManager::Create(ppDevice.Get(), ppContext.Get());
	//if (m_pParticleManager == nullptr)
	//{
	//	return E_FAIL;
	//}

	m_pFontManager = CFontManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pFontManager == nullptr)
	{
		return E_FAIL;
	}


    return S_OK;
}

void CGameInstance::UpdateGUI()
{
	ZoneScopedN("UpdateGUI");

	{
		ZoneScopedN("PrototypeManager_UpdateGUI");
		m_pPrototypeManager->UpdateGUI();
	}

	{
		ZoneScopedN("GameObjectManager_UpdateGUI");
		m_pGameObjectManager->UpdateGUI();
	}
	

	m_pAnimEdit_Manager->UpdateGUI();

	m_pWorkerManager->UpdateGUI();

	m_pResourceManager->UpdateGUI();


	m_pCameraManager->UpdateGUI();

	m_pLevelManager->UpdateGUI();

	m_pColliderManager->UpdateGUI();

	//m_pParticleManager->UpdateGUI();

	//m_pLightManager->UpdateGUI();


	m_pRenderer->UpdateGUI();

	m_pSoundManager->UpdateGUI();

	m_pImguiManager->Update_ImguiNodeEditor();
	if (ImGui::Button("ShaderRebuild"))
	{
		//TAG_RES_GRP_PERMANENT_SHADER
		if (auto resources = GetResource(TAG_RES_GRP_PERMANENT_SHADER))
		{
			for (auto& [_, res] : *resources)
			{
				if (!res.empty())
				{
					res.front()->Unload();
					res.front()->Load();
				}
			}
		}
	}
}

void CGameInstance::UpdateEngine(_float fTimeDelta)
{
	{
		ZoneScopedN("InputManager_Update");
		m_pDInputManager->Update_InputDev();
	}
	
	// TODO: 마우스 가두기 함수화하기
	{
		if (CGameInstance::Get().KeyDown(DIK_TAB))
		{

			m_bMouseFix = !m_bMouseFix;
			if (!m_bMouseFix)
			{
				ShowCursor(TRUE);
			}
			else
			{
				ShowCursor(FALSE);
			}
		}
		if (m_bMouseFix)
		{
			MouseFix();
		}
	}

	{
		ZoneScopedN("SoundManager_Update");
		m_pSoundManager->Update();
	}
	


	//m_pParticleManager->Update(fTimeDelta);
	m_pAnimEdit_Manager->Update(fTimeDelta);


	{
		ZoneScopedN("GameObjectManager_PriorityUpdate");
		m_pGameObjectManager->PriorityUpdate(fTimeDelta);
	}

	{
		ZoneScopedN("GameObjectManager_Update");
		m_pGameObjectManager->Update(fTimeDelta);
	}

	{
		ZoneScopedN("GameObjectManager_LateUpdate");
		m_pGameObjectManager->LateUpdate(fTimeDelta);
	}

	{
		ZoneScopedN("LevelManager_Update");
		m_pLevelManager->Update(fTimeDelta);
	}


	AddRenderObject(RENDERGROUP::COLLIDER, m_pColliderManager.get());
}

HRESULT CGameInstance::Draw()
{
	if (FAILED(m_pRenderer->Draw()))
	{
		return E_FAIL;
	}
    return S_OK;
}

//void CGameInstance::ClearResource(uint32_t iClearLevelIndex)
//{
//}

void CGameInstance::Release_Engine()
{
	m_pSoundManager.reset();
	m_pImguiManager.reset();
	m_pDInputManager.reset();
	m_pAnimEdit_Manager.reset();
	m_pGameObjectManager->AllReset();
	m_pLevelManager.reset();
	m_pColliderManager.reset();
	//m_pParticleManager.reset();
	m_pWorkerManager.reset();
	//m_pLightManager.reset();
	m_pCameraManager.reset();
	m_pPrototypeManager.reset();
	m_pGameObjectManager.reset();
	m_pRenderer.reset();
	m_pFontManager.reset();
	m_pResourceManager.reset();
	m_pGraphicDevice.reset();
}


void CGameInstance::FrameStart(_float fTimeDelta)
{
	m_pLevelManager->FrameStart(fTimeDelta);
	m_pGameObjectManager->FrameStart();
	m_pColliderManager->FrameStart();
}
void CGameInstance::FrameEnd(_float fTimeDelta) 
{
	m_pGameObjectManager->FrameEnd();
	m_pLevelManager->FrameEnd(fTimeDelta);

	m_pRenderer->FrameEnd();
	m_pColliderManager->FrameEnd();
}

void CGameInstance::MouseFix() const
{
	RECT rect;
	GetClientRect(CGameInstance::Get().GetHwnd(), &rect);
	POINT center;
	center.x = (rect.right - rect.left) / 2;
	center.y = (rect.bottom - rect.top) / 2;

	ClientToScreen(CGameInstance::Get().GetHwnd(), &center);
	SetCursorPos(center.x, center.y);
}

HRESULT CGameInstance::InitializeResources()
{
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_PASS) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_OBJECT) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI", E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_UI) })))
		{
			return E_FAIL;
		}
	}

	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP, CResSamplerState::Create()))
	{
		res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
			});
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_POINT_WRAP, CResSamplerState::Create()))
	{
		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;

		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		samplerDesc.MinLOD = 0.f;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		res->Load(samplerDesc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_POINT_WRAP_NOMIP, CResSamplerState::Create()))
	{
		res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = 0.0f,
			});
	}
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_SAHDOW, CResSamplerState::Create()))
	{
		D3D11_SAMPLER_DESC sampDesc{};
		sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.BorderColor[0] = 1.f;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;

		//sampDesc.MinLOD = -D3D11_FLOAT32_MAX;
		//sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		//sampDesc.MipLODBias = 0.f;
		//sampDesc.MaxAnisotropy = 1;
		if (FAILED(res->Load(sampDesc)))
		{
			return E_FAIL;
		}

		GetGraphicDeviceContext()->PSSetSamplers(4, 1, res->GetSamplerState().GetAddressOf());
	}
	//./ShaderFiles
	if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTex", "./ShaderFiles/QuadTex/QuadTex.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex", "./ShaderFiles/QuadTex/QuadTex.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadCol", "./ShaderFiles/QuadCol/QuadCol.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadCol", "./ShaderFiles/QuadCol/QuadCol.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_Filter", "./ShaderFiles/PostProcess/PS_PostProcess_Filter.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Particle", "./ShaderFiles/Particle/Shader_Particle_Compute.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}



	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex", E::CResQuadTexBuffer::Create()))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}




	//
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_BACKCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthClipEnable = TRUE;
		res->Load(desc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_FRONTCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_FRONT;
		desc.DepthClipEnable = TRUE;
		res->Load(desc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		desc.DepthBias = 10;
		desc.SlopeScaledDepthBias = 0.5f;
		desc.DepthBiasClamp = 0.0f;
		res->Load(desc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_WIREFRAME_NOCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_WIREFRAME;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;

		res->Load(desc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "RS_SOLID_BACKCULL_DEPTHBIAS", E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthClipEnable = TRUE;
		desc.DepthBias = -100;
		desc.SlopeScaledDepthBias = -1.0f;
		desc.DepthBiasClamp = 0.0f;
		res->Load(desc);
	}

	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "RS_SOLID_BLOCK_VOXEL_SHADOW", E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_FRONT;
		desc.DepthClipEnable = TRUE;
		res->Load(desc);
	}

	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "RS_ALPHATEST_BLOCK_VOXEL_SHADOW", E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		desc.DepthBias = 10;
		desc.SlopeScaledDepthBias = 0.5f;
		desc.DepthBiasClamp = 0.0f;
		res->Load(desc);
	}


	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND", E::CResBlendState::Create()))
	{
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		res->Load(blendDesc);
	}

	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_NO_DEPTHWRITE", E::CResDepthStencilState::Create()))
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; 
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
		res->Load(depthDesc);
	}

	// Test Model Load
	// 오류나서 제거
	if(true)
	{
		if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnmi", "./ShaderFiles/TestModel/Shader_VtxMesh.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}
		if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnmi", "./ShaderFiles/TestModel/Shader_VtxMesh.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}

		if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim", "./ShaderFiles/TestModel/Shader_VtxAnimMesh.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}
		if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim", "./ShaderFiles/TestModel/Shader_VtxAnimMesh.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}

	
		if (auto res = AddResourceT<E::CResTestModel>("TEST", "Model_Resource",
			CResTestModel::Create("./Resources/SampleClient/Models/LightObject/HorseStatue.fbx"))) {

			E::CResTestModel::DESC pDesc{};
			pDesc.eModelType = MODEL::NONANIM;
			pDesc.PreTransformMatrix = XMMatrixScaling(0.00001f, 0.00001f, 0.00001f);

			if (FAILED(res->Load(pDesc)))
			{
				return E_FAIL;
			}
		}

		//if (auto res = AddResourceT<E::CResTestModel>("TEST", "Model_Resource", CResTestModel::Create("./Resources/SampleClient/Models/ForkLift/ForkLift.FBX"))) {

		//	E::CResTestModel::DESC pDesc{};
		//	pDesc.eModelType = MODEL::NONANIM;
		//	pDesc.PreTransformMatrix = XMMatrixIdentity();

		//	if (FAILED(res->Load(pDesc)))
		//	{
		//		return E_FAIL;
		//	}
		//}
	}

	return S_OK;
}

HRESULT CGameInstance::InitializePrototype()
{
	if (AddPrototype("PERMANENT", "Prototype_Component_Transform", CComTransform::Create()))
	{
		return E_FAIL;
	}

	if (AddPrototype("PERMANENT", "Prototype_Component_ConstantBuffer", CComConstantBuffer::Create()))
	{
		return E_FAIL;
	}

	if (AddPrototype("PERMANENT", "Prototype_Component_ModelInstance", CComModelInstance::Create()))
	{
		return E_FAIL;
	}
	if (AddPrototype("PERMANENT", "Prototype_Component_Animator", CComAnimator::Create()))
	{
		return E_FAIL;
	}

	if (AddPrototype("CAMERAS", "Prototype_GameObject_FlyCamera", CFlyCamera::Create()))
	{
		return E_FAIL;
	}
	if (AddPrototype("CAMERAS", "Prototype_GameObject_UICamera", CUICamera::Create()))
	{
		return E_FAIL;
	}
	if (AddPrototype("BEHAVIOR", "Prototype_GameObject_BeHavior", CComBeHavior::Create()))
	{
		return E_FAIL;
	}

	if (AddPrototype("LIGHT", "Prototype_GameObject_Light", CLight::Create()))
	{
		return E_FAIL;
	}
	if (AddPrototype("COLLIDER", "Prototype_Component_Collider", CComCollider::Create()))
	{
		return E_FAIL;
	}

	//if (AddPrototype("CAMERAS", "Prototype_GameObject_PlayerCamera", CPlayerCamera::Create()))
	//{
	//	return E_FAIL;
	//}
	return S_OK;
}

#pragma region TIME_PROVIDER
_float CGameInstance::UpdateTimeProvider()
{
	return m_pTimeProvider->UpdateTimeProvider();
}
#pragma endregion

#pragma region IMGUI_MANAGER
void CGameInstance::ImguiNewFrame()
{
	m_pImguiManager->Update_Imgui();
}

void CGameInstance::ImguiEndFrameAndRender()
{
	m_pImguiManager->Render_Imgui();
}

_bool CGameInstance::ImguiWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return m_pImguiManager->WinProc(hWnd, message, wParam, lParam);
}

_bool CGameInstance::ImguiGetActive() const
{
	return m_pImguiManager->Get_Active();
}

void CGameInstance::ImguiSetActive(_bool bActive)
{
	m_pImguiManager->Set_Active(bActive);
}

void CGameInstance::ImguiEnableDocking(_bool bEnableDocking, _bool bEnableViewports)
{
	m_pImguiManager->EnableDocking(bEnableDocking, bEnableViewports);
}
#pragma endregion


#pragma region RESOURCE_MANAGER
SPtr<CResource> CGameInstance::AddResource(const StringID& sGroupTag, const StringID& sResTag, _string_id eAssetType, const _string& sPath, void* pArg)
{
	return m_pResourceManager->AddResource(sGroupTag, sResTag, eAssetType, sPath, pArg);
}
SPtr<CResource> CGameInstance::AddResource(const StringID& sGroupTag, const StringID& sResTag, SPtr<CResource> pAsset)
{
	return m_pResourceManager->AddResource(sGroupTag, sResTag, pAsset);
}
const std::vector<SPtr<CResource>>* CGameInstance::GetResource(const StringID& sGroupTag, const StringID& sResTag) const
{
	return m_pResourceManager->GetResource(sGroupTag, sResTag);
}
const std::unordered_map<StringID, std::vector<SPtr<CResource>>>* CGameInstance::GetResource(const StringID& sGroupTag) const
{
	return m_pResourceManager->GetResource(sGroupTag);
}
HRESULT CGameInstance::LoadResource(const StringID& sGroupTag)
{
	return m_pResourceManager->LoadResource(sGroupTag);
}
HRESULT CGameInstance::LoadResource(const StringID& sGroupTag, const StringID& sResTag)
{
	return m_pResourceManager->LoadResource(sGroupTag, sResTag);
}
HRESULT CGameInstance::UnLoadResource(const StringID& sGroupTag)
{
	return m_pResourceManager->UnLoadResource(sGroupTag);
}
HRESULT CGameInstance::UnLoadResource(const StringID& sGroupTag, const StringID& sResTag)
{
	return m_pResourceManager->UnLoadResource(sGroupTag, sResTag);
}
void CGameInstance::DelResource(const StringID& sGroupTag)
{
	m_pResourceManager->DelResource(sGroupTag);
}
void CGameInstance::DelResource(const StringID& sGroupTag, const StringID& sResTag)
{
	m_pResourceManager->DelResource(sGroupTag, sResTag);
}
#pragma endregion


#pragma region GRAPHIC_DEVICE
ComPtr<ID3D11Device> CGameInstance::GetGraphicDevice() const
{
	return m_pGraphicDevice->GetDevice();
}
ComPtr<ID3D11DeviceContext> CGameInstance::GetGraphicDeviceContext() const
{
	return  m_pGraphicDevice->GetContext();
}
ComPtr<ID3D11RenderTargetView> CGameInstance::GetBackBufferRTV() const
{
	return m_pGraphicDevice->GetBackBufferRTV();
}
ComPtr<ID3D11DepthStencilView> CGameInstance::GetBackBufferDSV() const
{
	return m_pGraphicDevice->GetBackBufferDSV();
}

HRESULT CGameInstance::ClearBackBufferView(const _float4* pClearColor)
{
	return m_pGraphicDevice->ClearBackBufferView(pClearColor);
}

HRESULT CGameInstance::ClearDepthStencilView()
{
	return m_pGraphicDevice->ClearDepthStencilView();
}

HRESULT CGameInstance::Present()
{
	ZoneScopedN("Present");
	return m_pGraphicDevice->Present();
}
#pragma endregion


#pragma region DINPUT_MANAGER
_bool CGameInstance::KeyPressing(_ubyte byKeyID) const
{
	return m_pDInputManager->KeyPressing(byKeyID);
}
_bool CGameInstance::KeyUp(_ubyte byKeyID) const
{
	return m_pDInputManager->KeyUp(byKeyID);
}
_bool CGameInstance::KeyDown(_ubyte byKeyID) const
{
	return m_pDInputManager->KeyDown(byKeyID);
}
int32_t CGameInstance::MouseMove(MOUSEMOVESTATE eMouseState) const
{
	return m_pDInputManager->MouseMove(eMouseState);
}
_bool CGameInstance::MousePressing(MOUSEKEYSTATE eMouseState) const
{
	return  m_pDInputManager->MousePressing(eMouseState);
}
_bool CGameInstance::MouseUp(MOUSEKEYSTATE eMouseState) const
{
	return  m_pDInputManager->MouseUp(eMouseState);
}
_bool CGameInstance::MouseDown(MOUSEKEYSTATE eMouseState) const
{
	return  m_pDInputManager->MouseDown(eMouseState);
}

#pragma endregion

#pragma region LEVEL_MANAGER
HRESULT CGameInstance::ChangeLevel(UPtr<CLevel> pNewLevel)
{
	return m_pLevelManager->ChangeLevel(std::move(pNewLevel));
}
void CGameInstance::RegisterLevelChangeFunc(const _string& ID, _Func func)
{
	m_pLevelManager->RegisterLevelChangeFunc(ID, func);
}
#pragma endregion


#pragma region SOUND_MANAGER
HRESULT CGameInstance::CreateSound(const _string& sPath, FMOD_SOUND** ppSound)
{
	return m_pSoundManager->CreateSound(sPath, ppSound);
}

HRESULT CGameInstance::SoundAddChannel(const StringID& channelTag, const std::pair<StringID, StringID>& soundResources)
{
	return m_pSoundManager->AddChannel(channelTag, soundResources);
}

HRESULT CGameInstance::SoundPlay(const StringID& channelTag, _float fVolume, _float fPitch)
{
	return m_pSoundManager->Play(channelTag, fVolume, fPitch);
}

void CGameInstance::SoundStop(const StringID& channelTag)
{
	m_pSoundManager->Stop(channelTag);
}

void CGameInstance::SoundPause(const StringID& channelTag, _bool bPause)
{
	m_pSoundManager->Pause(channelTag, bPause);
}

_bool CGameInstance::SoundGetVolume(const StringID& channelTag, _float& fVolume)
{
	return m_pSoundManager->GetVolume(channelTag, fVolume);
}

_bool CGameInstance::SoundSetVolume(const StringID& channelTag, _float fVolume)
{
	return m_pSoundManager->SetVolume(channelTag, fVolume);
}

_bool CGameInstance::SoundIsPlaying(const StringID& channelTag) const
{
	return m_pSoundManager->IsPlaying(channelTag);
}

void CGameInstance::SoundSetPitch(const StringID& channelTag, float fPitchRatio)
{
	m_pSoundManager->SetPitch(channelTag, fPitchRatio);
}

#pragma endregion


#pragma region FONT_MANAGER
void CGameInstance::FontDraw(const StringID& fontName, const _tchar* pText, const _float2& vPosition, float fScale, _fvector vColor, _float fRotation, const _float2& vOrigin)
{
	m_pFontManager->Draw(fontName, pText, vPosition, fScale, vColor, fRotation, vOrigin);
}
void CGameInstance::FontAddLateDraw(RENDERGROUP eRenderGroup, const StringID& fontName, const _wstring& pText, const _float2& vPosition, float fScale, _fvector vColor, _float fRotation, const _float2& vOrigin)
{
	m_pFontManager->AddLateDraw(eRenderGroup, fontName, pText, vPosition, fScale, vColor, fRotation, vOrigin);
}
_float2 CGameInstance::FontMeasureString(const StringID& fontName, const wchar_t* txt, float scale) const
{
	return m_pFontManager->MeasureString(fontName, txt, scale);
}
void CGameInstance::FontLateDraw(RENDERGROUP eRenderGroup)
{
	m_pFontManager->LateDraw(eRenderGroup);
}
#pragma endregion


#pragma region WORKER_MANAGER
void CGameInstance::WorkerEnqueue(_string_view svTaskName, _Func func)
{
	m_pWorkerManager->Enqueue(svTaskName, func);
}
#pragma endregion

#pragma region PROTOTYPE_MANAGER
HRESULT CGameInstance::AddPrototype(const StringID& svGroupTag, const StringID& svPrototypetag, UPtr<CPrototype> pPrototype)
{
	return m_pPrototypeManager->AddPrototype(svGroupTag, svPrototypetag, std::move(pPrototype));
}
UPtr<CPrototype> CGameInstance::ClonePrototype(const StringID& svGroupTag, const StringID& svPrototypetag, void* pArg)
{
	return m_pPrototypeManager->ClonePrototype(svGroupTag, svPrototypetag, pArg);
}
void CGameInstance::DelPrototype(const StringID& sGroupTag)
{
	m_pPrototypeManager->DelPrototype(sGroupTag);
}
#pragma endregion


#pragma region GAMEOBJECT_MANAGER
void CGameInstance::GameObjectAllReset()
{
	m_pGameObjectManager->AllReset();
}
std::optional<CHandle> CGameInstance::AddGameObjectToLayer(const StringID& iPrototypeLevelIndex, const StringID& svPrototypeTag, std::string_view sLayerName, void* pArg)
{
	return m_pGameObjectManager->AddGameObjectToLayer(iPrototypeLevelIndex, svPrototypeTag, sLayerName, pArg);
}
inline CGameObject* CGameInstance::GetGameObjectByHandle(const CHandle& handle)
{
	return m_pGameObjectManager->GetGameObjectByHandle(handle);
}
const std::vector<CHandle>* CGameInstance::GetGameObjectLayer(std::string_view sLayerName) const
{
	return m_pGameObjectManager->GetLayer(sLayerName);
}
const std::vector<CHandle>* CGameInstance::GetGameObjectLayer(std::string_view sLayerName, const StringID& iPrototypeLevelIndex, const StringID& svPrototypeTag, void* pArg)
{
	return m_pGameObjectManager->GetLayer(sLayerName, iPrototypeLevelIndex, svPrototypeTag, pArg);
}

const std::vector<std::pair<std::string, std::vector<CHandle>>>& CGameInstance::GetGameObjectLayers() const
{
	return m_pGameObjectManager->GetLayers();
}

void CGameInstance::DelGameObjectLayer(std::string_view sLayerName)
{
	return m_pGameObjectManager->DelLayer(sLayerName);
}
//std::optional<CHandle> CGameInstance::GetFreeHandle() const
//{
//	return m_pGameObjectManager->GetFreeHandle();
//}
#pragma endregion


#pragma region COLLIDER_MANAGER
void CGameInstance::AddColliderGroup(const StringID& groupTag, const CCollider* pCollider)
{
	m_pColliderManager->AddColliderGroup(groupTag, pCollider);
}
const std::vector<const CCollider*>* CGameInstance::GetColliderGroup(const StringID& groupTag) const
{
	return m_pColliderManager->GetColliderGroup(groupTag);
}
_bool CGameInstance::IntersectColl(const CCollider* pColl1, const CCollider* pColl2)
{
	return m_pColliderManager->IntersectColl(pColl1, pColl2);
}
const std::unordered_map<StringID, std::vector<const CCollider*>>* CGameInstance::GetColliders() const
{
	return m_pColliderManager->GetColliders();
}

#pragma endregion


#pragma region CAMERA_MANAGER
CCameraObject* CGameInstance::GetActiveCamera() const
{
	return m_pCameraManager->GetActiveCamera();
}
CCameraObject* CGameInstance::GetActiveCamera(const StringID& CameraID) const
{
	return m_pCameraManager->GetActiveCamera(CameraID);
}
HRESULT CGameInstance::SetActiveCamera(const StringID& CameraID)
{
	return m_pCameraManager->SetActiveCamera(CameraID);
}
CCameraObject* CGameInstance::GetCamera(const StringID& CameraID) const
{
	return m_pCameraManager->GetCamera(CameraID);
}
HRESULT CGameInstance::RegistCamera(const StringID& CameraID, const CHandle& handle)
{
	return m_pCameraManager->RegistCamera(CameraID, handle);
}

//const CCameraObject* CGameInstance::GetCameraObject(const StringID& GroupID) const
//{
//	return m_pCameraManager->GetCameraObject(GroupID);
//}
//HRESULT CGameInstance::SetCameraObject(const StringID& GroupID, const CHandle& handle)
//{
//	return m_pCameraManager->SetCameraObject(GroupID, handle);
//}

//CCameraObject* CGameInstance::GetActiveGameCamera() const
//{
//	return m_pCameraManager->GetActiveGameCamera();
//}
//
//HRESULT CGameInstance::SetActiveGameCamera(const StringID& CameraID)
//{
//	return m_pCameraManager->SetActiveGameCamera(CameraID);
//}
//
//CCameraObject* CGameInstance::GetActiveUICamera() const
//{
//	return m_pCameraManager->GetActiveUICamera();
//}
//
//HRESULT CGameInstance::SetActiveUICamera(const StringID& CameraID)
//{
//	return m_pCameraManager->SetActiveUICamera(CameraID);
//}
//
//CCameraObject* CGameInstance::GetActiveGameCamera(const StringID& CameraID) const
//{
//	return m_pCameraManager->GetActiveGameCamera(CameraID);
//}
//
//CCameraObject* CGameInstance::GetActiveUICamera(const StringID& CameraID) const
//{
//	return m_pCameraManager->GetActiveUICamera(CameraID);
//}
//
//CCameraObject* CGameInstance::GetGameCamera(const StringID& CameraID) const
//{
//	return m_pCameraManager->GetGameCamera(CameraID);
//}
//
//CCameraObject* CGameInstance::GetUICamera(const StringID& CameraID) const
//{
//	return m_pCameraManager->GetUICamera(CameraID);
//}
//
//HRESULT CGameInstance::RegistGameCamera(const StringID& CameraID, const CHandle& handle)
//{
//	return m_pCameraManager->RegistGameCamera(CameraID, handle);
//}
//
//HRESULT CGameInstance::RegistUICamera(const StringID& CameraID, const CHandle& handle)
//{
//	return m_pCameraManager->RegistUICamera(CameraID, handle);
//}

#pragma endregion


#pragma region RENDERER
HRESULT CGameInstance::AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject)
{
	return m_pRenderer->AddRenderObject(eRenderGroup, pRenderObject);
}
#pragma endregion

#pragma region ANIMEDIT_MANAGER
HRESULT CGameInstance::SetupTestModel() {
	return m_pAnimEdit_Manager->SetupTestModel();
}
#pragma endregion
