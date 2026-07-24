#include "MiniMap.h"
#include "pch.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "Level_Defines.h"

NS_USING(Client)

CMiniMap::CMiniMap()
{
}

CMiniMap::~CMiniMap()
{
}

HRESULT CMiniMap::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CMiniMap::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	{
		/* Buffer */
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI" };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerUI", &Desc, &m_pComCBufferPerUI)))
		{
			return E_FAIL;
		};

		CComConstantBuffer::DESC mDesc{};
		mDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_MiniMap" };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerMiniMap", &mDesc, &m_pMinimapCBuffer)))
		{
			return E_FAIL;
		}; 


		/* Component */
		CComponent::DESC CDesc{};
		CDesc.pGameObject = this;

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_ButtonUI", "Com_Button", &CDesc, &m_pComCButton)))
		{
			return E_FAIL;
		};
	}

	m_UIINFO.UIType = ETOUI(UI_TYPE::MINIMAP);
	return S_OK;
}

void CMiniMap::PriorityUpdate(E::_float fTimeDelta)
{
	SearchPlayerIcon();
}

void CMiniMap::Update(E::_float fTimeDelta)
{
	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (!m_isActive)
		return;

	CUIObject::Update(fTimeDelta);

	m_pComCButton->CheckPixelPerfectCollision(mousePos, true);

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}

	_bool bA = CGameInstance::Get().KeyPressing(DIK_A);
	_bool bD = CGameInstance::Get().KeyPressing(DIK_D);
	_bool bW = CGameInstance::Get().KeyPressing(DIK_W);
	_bool bS = CGameInstance::Get().KeyPressing(DIK_S);
	if (bA) {
		//m_UIINFO.Rot -= 1.f;
		//CalcUICoord();
		_float rotationSpeed = XMConvertToRadians(-45.f);
		_float angle = rotationSpeed * fTimeDelta; // 프레임 속도 보정
	
		// 3. 현재 m_cameraLook의 X, Z 평면 성분 추출
		_float oldX = m_cameraLook.x;
		_float oldZ = m_cameraLook.z;

		m_cameraLook.x = oldX * cosf(angle) + oldZ * sinf(angle);
		m_cameraLook.z = -oldX * sinf(angle) + oldZ * cosf(angle);
	
		// 5. 방향 벡터의 크기를 항상 1로 유지하기 위해 정규화(Normalize)
		XMVECTOR vLook = XMLoadFloat3(&m_cameraLook);
		vLook = XMVector3Normalize(vLook);
		XMStoreFloat3(&m_cameraLook, vLook);
	}
	else if (bD)
	{
		//m_UIINFO.Rot += 1.f;
		//CalcUICoord();
		_float rotationSpeed = XMConvertToRadians(45.f);
		_float angle = rotationSpeed * fTimeDelta; // 프레임 속도 보정
	
		// 3. 현재 m_cameraLook의 X, Z 평면 성분 추출
		_float oldX = m_cameraLook.x;
		_float oldZ = m_cameraLook.z;
	
		// 4. 2D 회전 변환 행렬 공식 적용
		// 회전 방향이 반대라면 sin의 부호를 (+angle, -angle)로 서로 바꾸어 매칭하면 됩니다.
		m_cameraLook.x = oldX * cosf(angle) + oldZ * sinf(angle);
		m_cameraLook.z = -oldX * sinf(angle) + oldZ * cosf(angle);
	
		// 5. 방향 벡터의 크기를 항상 1로 유지하기 위해 정규화(Normalize)
		XMVECTOR vLook = XMLoadFloat3(&m_cameraLook);
		vLook = XMVector3Normalize(vLook);
		XMStoreFloat3(&m_cameraLook, vLook);
	}
	else if (bW)
		tMapOffset.y += 0.01f;
	else if (bS)
		tMapOffset.y -= 0.01f;

	CalcDir();
}

void CMiniMap::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	GetTransform().Update();
}

HRESULT CMiniMap::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_MiniMap");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_MiniMap");
	const auto& viBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get()
	};
	uint32_t strides[] = {
		viBuffer->GetVertexStride()
	};
	uint32_t offsets[] = {
		0
	};
	pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
	pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

	{
		E::CB_PER_UI perUI{};
		perUI.texCoord = { 0.f, 0.f };
		perUI.uvSize = { 0.f, 0.f };
		perUI.color = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha };

		if (FAILED(m_pComCBufferPerUI->MapDiscard(pContext, &perUI, sizeof(perUI))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
	}

	{
		E::CB_MINIMAP minimapBuffer{};
		minimapBuffer.mapOffset = tMapOffset;
		minimapBuffer.mapRotation = tRotation;
		minimapBuffer.mapScale = tScale;

		// 미니맵 전용 컴포넌트나 버퍼 오브젝트를 통해 MapDiscard 진행
		m_pMinimapCBuffer->MapDiscard(pContext, &minimapBuffer, sizeof(minimapBuffer));

		pContext->VSSetConstantBuffers(10, 1, m_pMinimapCBuffer->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(10, 1, m_pMinimapCBuffer->GetAdressOfBuffer());
	}

	{
		//auto pUICam = E::CGameInstance::Get().GetActiveUICamera();
		{
			auto pCbPerObject = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
			D3D11_MAPPED_SUBRESOURCE mappedSubResource;
			if (SUCCEEDED(pContext->Map(pCbPerObject->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
			{

				E::CB_PER_OBJECT cbPerObject{};
				cbPerObject.matWorld = *GetTransform().GetWorldMatrix();
				XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedWorldMatrix() * ctx.matProj);

				memcpy(mappedSubResource.pData, &cbPerObject, sizeof(cbPerObject));
				pContext->Unmap(pCbPerObject->GetCBuffer().Get(), 0);
			}
			pContext->VSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}

	{
		m_UIINFO.Restag = "TEX_UI_T_MapMini_Sanctuary_03_D";
		const auto& frameSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_HUD_MiniMap_Fade");
		const auto& minimapSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_UIINFO.Restag);
		

		ID3D11ShaderResourceView* srvs[2] = {
			frameSrv->GetSRV().Get(),      // t0: 프레임
			minimapSrv->GetSRV().Get(),   // t1: 맵
		};

		pContext->PSSetShaderResources(0, 2, srvs);
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	ID3D11Buffer* nullBuffer = nullptr;
	pContext->VSSetConstantBuffers(10, 1, &nullBuffer);
	pContext->PSSetConstantBuffers(10, 1, &nullBuffer);

	return S_OK;
}

void CMiniMap::PlayEffect(uint32_t uiState)
{
	if (m_pComTween == nullptr)
		return;

	if (uiState & ETOUI(UI_STATE::APPEAR))
	{
		ClearEffectTweens();
		if (Appear) Appear(this);
	}

	if (uiState & ETOUI(UI_STATE::DISAPPEAR))
	{
		ClearEffectTweens();
		if (Disappear) Disappear(this);
	}

	if (m_bInputLocked)
		return;
}

void CMiniMap::SearchPlayerIcon()
{
	if (!m_SearchPlayerIcon)
	{
		m_SearchPlayerIcon = true;
		for (auto pHandle : m_vChildren)
		{
			if (nullptr == E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(pHandle))
				return;

			Engine::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(pHandle);
			UI_INFO& pInfo = pUI->GetUIInfo();
			if (pInfo.Restag == "TEX_UI_T_HUD_MiniMap_PlayerBlip")
				m_hPlayerIcon = pHandle;
		}
	}
}

void CMiniMap::SetPlayerIconRot(_float rot)
{
	if (nullptr == E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(m_hPlayerIcon))
		return;

	Engine::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(m_hPlayerIcon);
	pUI->SetLocalRot(rot);
	pUI->CalcUICoord();
}

void CMiniMap::CalcDir()
{
	_float2 camLook2D = _float2(m_cameraLook.x, m_cameraLook.z);
	_float2 playerLook2D = _float2(m_playerLook.x, m_playerLook.z);

	XMVECTOR vCam = XMVector3Normalize(XMVectorSet(camLook2D.x, 0.f, camLook2D.y, 0.f));
	XMVECTOR vPlayer = XMVector3Normalize(XMVectorSet(playerLook2D.x, 0.f, playerLook2D.y, 0.f));

	_float camRadian = atan2f(XMVectorGetZ(vCam), XMVectorGetX(vCam)) - XM_PI / 2;
	_float playerRadian = atan2f(XMVectorGetZ(vPlayer), XMVectorGetX(vPlayer)) - XM_PI / 2;

	m_UIINFO.Rot = XMConvertToDegrees(camRadian);
	CalcUICoord();

	_float playerIconRotDegree = XMConvertToDegrees(playerRadian);
	SetPlayerIconRot(playerIconRotDegree);
}

E::UPtr<CMiniMap> CMiniMap::Create()
{
	auto pInstance = E::ToUPtr(new CMiniMap{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMiniMap");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CMiniMap::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CMiniMap{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMiniMap");
		return nullptr;
	}

	return pInstance;
}
