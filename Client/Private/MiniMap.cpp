#include "MiniMap.h"
#include "pch.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "Level_Defines.h"
#include "PlayerThirdPersonCamera.h"
#include "TextureUI.h"
#include "Monster.h"

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
	ConfigureDefaultProfile();
	return S_OK;
}

void CMiniMap::PriorityUpdate(E::_float fTimeDelta)
{
	InitializeMonsterMarkerPool();
	SearchPlayerIcon();
}

void CMiniMap::Update(E::_float fTimeDelta)
{
	ConfigureDefaultProfile();

	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (!m_isActive)
	{
		m_bHasPreviousPlayerPosition = false;
		HideMonsterMarkers();
		return;
	}

	m_fSmokeTime = fmodf(m_fSmokeTime + fTimeDelta, 10000.f);

	CUIObject::Update(fTimeDelta);

	m_pComCButton->CheckPixelPerfectCollision(mousePos, true);

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}

	auto* pCamera = Cast<CPlayerThirdPersonCamera>(
		E::CGameInstance::Get().GetActiveCamera("PlayerCamera"));
	if (!pCamera)
	{
		m_bHasPreviousPlayerPosition = false;
		HideMonsterMarkers();
		return;
	}

	auto* pPlayer = E::CGameInstance::Get().GetGameObjectByHandle(
		pCamera->GetTargetHandle());
	if (!pPlayer)
	{
		m_bHasPreviousPlayerPosition = false;
		HideMonsterMarkers();
		return;
	}

	UpdateFogMovementOffset(pPlayer->GetTransform().GetPosition());
	UpdateWorldMapOffset(pPlayer->GetTransform().GetPosition());
	UpdateMonsterMarkers(fTimeDelta, pPlayer);

	XMVECTOR cameraLook = XMVectorSetY(
		pCamera->GetTransform().GetState(STATE::LOOK), 0.f);
	XMVECTOR playerLook = XMVectorSetY(
		pPlayer->GetTransform().GetState(STATE::LOOK), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(cameraLook)) < 0.000001f ||
		XMVectorGetX(XMVector3LengthSq(playerLook)) < 0.000001f)
		return;

	cameraLook = XMVector3Normalize(cameraLook);
	playerLook = XMVector3Normalize(playerLook);

	XMStoreFloat3(&m_cameraLook, cameraLook);
	XMStoreFloat3(&m_playerLook, playerLook);

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
		const _float renderAlpha =
			m_MiniMapProfile.Mode == MINIMAP_MODE::DUNGEON_FOG ?
			std::clamp(m_UIINFO.Alpha * m_MiniMapProfile.FogAlphaMultiplier,
				0.f, 1.f) :
			m_UIINFO.Alpha;
		perUI.color = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, renderAlpha };

		if (FAILED(m_pComCBufferPerUI->MapDiscard(pContext, &perUI, sizeof(perUI))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::UI), 1, m_pComCBufferPerUI->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::UI), 1, m_pComCBufferPerUI->GetAdressOfBuffer());
	}

	{
		E::CB_MINIMAP minimapBuffer{};
		minimapBuffer.mapOffset = tMapOffset;
		minimapBuffer.mapRotation = tRotation;
		minimapBuffer.mapScale = tScale;
		minimapBuffer.mapMode = static_cast<uint32_t>(m_MiniMapProfile.Mode);
		minimapBuffer.smokeIntensity = m_MiniMapProfile.SmokeIntensity;
		minimapBuffer.smokeSpeed = m_MiniMapProfile.SmokeSpeed;
		minimapBuffer.smokeTime = m_fSmokeTime;

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
			pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}

	{
		const auto& frameSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_HUD_MiniMap_Fade");
		const auto& minimapSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_MiniMapProfile.TextureTag);
		

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

void CMiniMap::SetMiniMapProfile(const MINIMAP_PROFILE& profile)
{
	m_MiniMapProfile = profile;
	m_UIINFO.Restag = profile.TextureTag;
	tScale = profile.MapScale;
	tMapOffset = {};
	m_bHasPreviousPlayerPosition = false;
}

void CMiniMap::AddBattleZone(const BATTLE_ZONE_INFO& battleZone)
{
	m_vBattleZones.push_back(battleZone);
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
	if (m_SearchPlayerIcon)
		return;

	for (auto pHandle : m_vChildren)
	{
		Engine::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(pHandle);
		if (!pUI)
			continue;

		const UI_INFO& pInfo = pUI->GetUIInfo();
		if (pInfo.Restag == "TEX_UI_T_HUD_MiniMap_PlayerBlip")
		{
			m_hPlayerIcon = pHandle;
			m_SearchPlayerIcon = true;
			return;
		}
	}
}

void CMiniMap::SetPlayerIconRot(_float rot)
{
	Engine::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(m_hPlayerIcon);
	if (!pUI)
	{
		m_SearchPlayerIcon = false;
		return;
	}

	pUI->SetLocalRot(rot);
	pUI->GetUIInfo().Rot = m_UIINFO.Rot + rot;
	pUI->CalcUICoord();
}

void CMiniMap::CalcDir()
{
	const _float cameraYaw = atan2f(m_cameraLook.x, m_cameraLook.z);
	const _float playerYaw = atan2f(m_playerLook.x, m_playerLook.z);

	// UI 좌표는 Y축이 아래를 향하므로 카메라 yaw와 같은 부호로 회전해야
	// 카메라 전방이 미니맵 위쪽을 향한다.
	m_UIINFO.Rot = XMConvertToDegrees(cameraYaw);
	CalcUICoord();

	// CUIObject combines child rotation as parent Rot + child LocalRot.
	SetPlayerIconRot(XMConvertToDegrees(-playerYaw));
}

void CMiniMap::InitializeMonsterMarkerPool()
{
	if (m_bMonsterMarkerPoolInitialized)
		return;

	m_bMonsterMarkerPoolInitialized = true;
	m_vMonsterMarkerHandles.reserve(MONSTER_MARKER_COUNT);

	const std::string currentLevel = _string("LEVEL_") +
		MagicEnumToStringView(static_cast<LEVEL>(
			E::CGameInstance::Get().GetCurrentLevelID())).data();

	for (size_t i = 0; i < MONSTER_MARKER_COUNT; ++i)
	{
		CTextureUI::UIOBJECT_DESC desc{};
		desc.sObjectTag = "MiniMap_MonsterMarker_" + std::to_string(i);
		desc.Name = desc.sObjectTag;
		desc.fX = m_UIINFO.fX;
		desc.fY = m_UIINFO.fY;
		desc.fSizeX = MONSTER_MARKER_SIZE;
		desc.fSizeY = MONSTER_MARKER_SIZE;
		desc.fAlpha = 0.f;
		desc.ResTag = "TEX_UI_T_MiniMap_AuthorityFigure";
		desc.ResWeight = m_UIINFO.Weight + 1;
		desc.UIType = ETOUI(UI_TYPE::TEXUI);

		auto markerHandle = E::CGameInstance::Get().AddGameObjectToLayer(
			currentLevel,
			"Prototype_GameObject_TextureUI",
			"Layer_UI",
			&desc);
		if (!markerHandle)
			continue;

		auto* pMarker = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextureUI>(*markerHandle);
		if (!pMarker)
			continue;

		pMarker->SetParent(GetHandle());
		pMarker->SetLocalPos({ 0.f, 0.f });
		pMarker->SetColor({ 1.f, 0.12f, 0.08f });
		pMarker->GetUIInfo().WeightOffset = 1;
		SetMonsterMarkerVisible(pMarker, false);

		AddChildren(*markerHandle);
		m_vMonsterMarkerHandles.push_back(*markerHandle);
	}
}

void CMiniMap::RefreshNearbyMonsters(E::CGameObject* pPlayer)
{
	m_vNearbyMonsterHandles.clear();
	if (!pPlayer || m_vMonsterMarkerHandles.empty())
		return;

	std::vector<E::PX_OVERLAP_RESULT> results{};
	const E::PX_OVERLAP_DESC overlapDesc{
		.tGeometry = {
			.eType = E::PX_QUERY_GEOMETRY_TYPE::SPHERE,
			.fRadius = MONSTER_DETECTION_RADIUS
		},
		.tPose = {
			.vPosition = pPlayer->GetTransform().GetPosition()
		},
		.tFilter = {
			.iQueryMask = ETOUI(COLLISION_LAYER::ENEMY_BODY)
		}
	};

	if (!E::CGameInstance::Get().GetPhysXManager()->
		OverlapMultiple(overlapDesc, results,
			static_cast<uint32_t>(MONSTER_MARKER_COUNT * 2)))
		return;

	for (const auto& result : results)
	{
		auto* pMonster = Cast<CMonster>(result.pGameObject);
		if (!pMonster || pMonster->GetPendingDestroy() ||
			pMonster->Get_CurrentHp() <= 0)
			continue;

		const CHandle monsterHandle = pMonster->GetHandle();
		if (std::find(m_vNearbyMonsterHandles.begin(),
			m_vNearbyMonsterHandles.end(), monsterHandle) !=
			m_vNearbyMonsterHandles.end())
			continue;

		m_vNearbyMonsterHandles.push_back(monsterHandle);
		if (m_vNearbyMonsterHandles.size() >=
			m_vMonsterMarkerHandles.size())
			break;
	}
}

void CMiniMap::UpdateMonsterMarkers(
	E::_float fTimeDelta, E::CGameObject* pPlayer)
{
	if (!m_bMonsterMarkerPoolInitialized || !pPlayer)
	{
		HideMonsterMarkers();
		return;
	}

	m_fMonsterSearchAcc += fTimeDelta;
	if (m_fMonsterSearchAcc >= MONSTER_SEARCH_INTERVAL)
	{
		m_fMonsterSearchAcc = 0.f;
		RefreshNearbyMonsters(pPlayer);
	}

	const _float3& playerPos = pPlayer->GetTransform().GetPosition();
	const _float mapRadius =
		0.5f * std::min(m_UIINFO.SizeX, m_UIINFO.SizeY);
	const _float markerRadius =
		0.5f * sqrtf(MONSTER_MARKER_SIZE * MONSTER_MARKER_SIZE * 2.f);
	const _float innerRadius = std::max(
		0.f, mapRadius - markerRadius - MINIMAP_BORDER_PADDING);
	const _float pixelPerWorldUnit =
		innerRadius / MONSTER_DETECTION_RADIUS;
	const _float detectionRadiusSq =
		MONSTER_DETECTION_RADIUS * MONSTER_DETECTION_RADIUS;

	for (size_t i = 0; i < m_vMonsterMarkerHandles.size(); ++i)
	{
		auto* pMarker = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextureUI>(m_vMonsterMarkerHandles[i]);
		if (!pMarker)
			continue;

		if (i >= m_vNearbyMonsterHandles.size())
		{
			SetMonsterMarkerVisible(pMarker, false);
			continue;
		}

		auto* pMonster = E::CGameInstance::Get().
			GetGameObjectByHandleT<CMonster>(m_vNearbyMonsterHandles[i]);
		if (!pMonster || pMonster->GetPendingDestroy() ||
			pMonster->Get_CurrentHp() <= 0)
		{
			SetMonsterMarkerVisible(pMarker, false);
			continue;
		}

		const _float3& monsterPos = pMonster->GetTransform().GetPosition();
		const _float dx = monsterPos.x - playerPos.x;
		const _float dz = monsterPos.z - playerPos.z;
		const _float distanceSq = dx * dx + dz * dz;

		if (distanceSq > detectionRadiusSq || innerRadius <= 0.f)
		{
			SetMonsterMarkerVisible(pMarker, false);
			continue;
		}

		pMarker->SetLocalPos({
			dx * pixelPerWorldUnit,
			-dz * pixelPerWorldUnit
		});

		XMVECTOR monsterLook = XMVectorSetY(
			pMonster->GetTransform().GetState(STATE::LOOK), 0.f);
		if (XMVectorGetX(XMVector3LengthSq(monsterLook)) > 0.000001f)
		{
			monsterLook = XMVector3Normalize(monsterLook);
			pMarker->SetLocalRot(XMConvertToDegrees(-atan2f(
				XMVectorGetX(monsterLook),
				XMVectorGetZ(monsterLook))));
		}

		SetMonsterMarkerVisible(pMarker, true);
	}
}

void CMiniMap::HideMonsterMarkers()
{
	for (const CHandle markerHandle : m_vMonsterMarkerHandles)
	{
		if (auto* pMarker = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextureUI>(markerHandle))
		{
			SetMonsterMarkerVisible(pMarker, false);
		}
	}
}

void CMiniMap::SetMonsterMarkerVisible(
	E::CUIObject* pMarker, _bool bVisible)
{
	if (!pMarker)
		return;

	pMarker->SetAlphaRatio(bVisible ? 1.f : 0.f);
	pMarker->SetAlpha(bVisible ? m_UIINFO.Alpha : 0.f);
	pMarker->SetActive(bVisible);
}

void CMiniMap::ConfigureDefaultProfile()
{
	const uint32_t currentLevelID =
		E::CGameInstance::Get().GetCurrentLevelID();
	if (m_iConfiguredLevel == currentLevelID)
		return;

	m_iConfiguredLevel = currentLevelID;
	MINIMAP_PROFILE profile{};
	profile.Mode = MINIMAP_MODE::DUNGEON_FOG;
	profile.TextureTag = "TEX_UI_T_MiniMapSmoke";
	profile.MapScale = 1.f;
	profile.SmokeIntensity = 0.9f;
	profile.SmokeSpeed = 8.f;
	profile.FogAlphaMultiplier = 1.75f;
	profile.PlayerScrollScale = 0.015f;

	SetMiniMapProfile(profile);
}

void CMiniMap::UpdateWorldMapOffset(const _float3& playerPosition)
{
	if (m_MiniMapProfile.Mode != MINIMAP_MODE::WORLD_MAP)
	{
		return;
	}

	const _float worldWidth =
		m_MiniMapProfile.WorldMaxXZ.x - m_MiniMapProfile.WorldMinXZ.x;
	const _float worldDepth =
		m_MiniMapProfile.WorldMaxXZ.y - m_MiniMapProfile.WorldMinXZ.y;
	if (fabsf(worldWidth) <= 0.000001f ||
		fabsf(worldDepth) <= 0.000001f)
	{
		tMapOffset = {};
		return;
	}

	const _float normalizedX = std::clamp(
		(playerPosition.x - m_MiniMapProfile.WorldMinXZ.x) / worldWidth,
		0.f, 1.f);
	const _float normalizedZ = std::clamp(
		(playerPosition.z - m_MiniMapProfile.WorldMinXZ.y) / worldDepth,
		0.f, 1.f);

	const _float mapU = m_MiniMapProfile.UVMin.x +
		(m_MiniMapProfile.UVMax.x - m_MiniMapProfile.UVMin.x) * normalizedX;
	const _float mapV = m_MiniMapProfile.UVMax.y +
		(m_MiniMapProfile.UVMin.y - m_MiniMapProfile.UVMax.y) * normalizedZ;

	tMapOffset = { mapU - 0.5f, mapV - 0.5f };
}

void CMiniMap::UpdateFogMovementOffset(const _float3& playerPosition)
{
	if (m_MiniMapProfile.Mode != MINIMAP_MODE::DUNGEON_FOG)
	{
		m_bHasPreviousPlayerPosition = false;
		return;
	}

	if (!m_bHasPreviousPlayerPosition)
	{
		m_vPreviousPlayerPosition = playerPosition;
		m_bHasPreviousPlayerPosition = true;
		return;
	}

	const _float deltaX = playerPosition.x - m_vPreviousPlayerPosition.x;
	const _float deltaZ = playerPosition.z - m_vPreviousPlayerPosition.z;
	m_vPreviousPlayerPosition = playerPosition;

	// Ignore teleports and large level-transition position changes.
	if (deltaX * deltaX + deltaZ * deltaZ > 100.f)
		return;

	tMapOffset.x = fmodf(tMapOffset.x +
		deltaX * m_MiniMapProfile.PlayerScrollScale, 1.f);
	tMapOffset.y = fmodf(tMapOffset.y -
		deltaZ * m_MiniMapProfile.PlayerScrollScale, 1.f);
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
