#include "pch.h"
#include "LightManager.h"
#include "GameInstance.h"
// LSY 변경: 기존 LightManager GUI를 대체하는 배치 전용 에디터를 연결한다.
#include "MapManager.h"
#include "MapMeshObject.h"
#include "OctreeNode.h"

#include "LightPlacementEditor.h"
#include "ComCollider.h"
#include "CollSphere.h"
#include "CollFrustum.h"
#include "DbgLineRender.h"

CLightManager::CLightManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) : m_pDevice(pDevice), m_pContext(pContext) {}
CLightManager::~CLightManager() { }

HRESULT CLightManager::Initialize_LightManager() {

	if (E::CGameInstance::Get().AddPrototype("PERMANENT", "Prototype_GameObject_Light", CLight::Create()))	return E_FAIL;

	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PBR", "./ShaderFiles/PBR/CS_PBR.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PBR_AlphaShadow", "./ShaderFiles/PBR/CS_PBR.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_Blend", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PBR_NonShadow", "./ShaderFiles/PBR/CS_PBR.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_NonShadow", .sTarget = "cs_5_0" })))    return E_FAIL;
	}

	if (m_pLightConstantBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_NormalLight", E::CResCBuffer::Create()))
	{
		if (FAILED(m_pLightConstantBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_LIGHT) })))    return E_FAIL;
	}
	if (m_pShadowConstantBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CS_ShadowLight", E::CResCBuffer::Create()))
	{
		if (FAILED(m_pShadowConstantBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_SHADOW) })))    return E_FAIL;
	}
	if (m_pEffectLightConstantBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CS_EffectLight", E::CResCBuffer::Create()))
	{
		if (FAILED(m_pEffectLightConstantBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_EFFECT_LIGHT) })))    return E_FAIL;
	}

	m_pShadowComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PBR");
	if (nullptr == m_pShadowComputeShader)		return E_FAIL;

	m_pNonShadowComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PBR_NonShadow");
	if (nullptr == m_pNonShadowComputeShader)	return E_FAIL;

	m_pUAVComBinedOutput = CGameInstance::Get().Generate_UnorderedAccessView("ComBinedTex", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);
	if (nullptr == m_pUAVComBinedOutput)		return E_FAIL;

	if (m_pInstancedDirectionalLightVS = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_InstancedShadow_Direct", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pInstancedDirectionalLightVS->Load(CResShader::DESC{ .sEntryPoint = "VSMain_InstancedDirectional", .sTarget = "vs_5_0" })))    return E_FAIL;
	}
	if (m_pInstancedPointLightVS = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_InstancedShadow_Point", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pInstancedPointLightVS->Load(CResShader::DESC{ .sEntryPoint = "VSMain_InstancedPoint", .sTarget = "vs_5_0" })))    return E_FAIL;
	}

	if (m_pDirectionalLightVS = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_NormalShadow_Direct", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pDirectionalLightVS->Load(CResShader::DESC{ .sEntryPoint = "VSMain_Final", .sTarget = "vs_5_0" })))    return E_FAIL;
	}
	if (m_pPointLightVS = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_NormalShadow_Point", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointLightVS->Load()))    return E_FAIL;
	}
	if (m_pPointFaceVS = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PointFace", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointFaceVS->Load(CResShader::DESC{ .sEntryPoint = "VSMain_PointFace", .sTarget = "vs_5_0" })))    return E_FAIL;
	}
	if (m_pInstancedPointFaceVS = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_InstancedPointFace", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pInstancedPointFaceVS->Load(CResShader::DESC{ .sEntryPoint = "VSMain_InstancedPointFace", .sTarget = "vs_5_0" })))    return E_FAIL;
	}
	if (m_pPointLightGS = CGameInstance::Get().AddResourceT<E::CResGeometryShader>(TAG_RES_GRP_PERMANENT_SHADER, "GS_Shadow", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointLightGS->Load()))    return E_FAIL;
	}
	if (m_pPointLightPS = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Shadow", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointLightPS->Load()))    return E_FAIL;
	}
	if (m_pPointFacePS = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PointFace", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointFacePS->Load(CResShader::DESC{ .sEntryPoint = "PSMain_PointFace", .sTarget = "ps_5_0" })))    return E_FAIL;
	}

	{	// Generate Shadow Texture List Array
		_float2 ShadowMapResolution = CGameInstance::Get().GetClientScreenSize();

		uint32_t ScreenSizeX = ETOUI(ShadowMapResolution.x);
		uint32_t ScreenSizeY = ETOUI(ShadowMapResolution.y);
		uint32_t ShadowSize  = 512;
		uint32_t CubeMapSize = 512;

		m_pDirectionalShadowViewPort = CGameInstance::Get().Generate_ViewPort("VP_ShadowMap_Directional", ShadowSize, ShadowSize);
		m_pPointShadowViewPort = CGameInstance::Get().Generate_ViewPort("VP_ShadowMap_Point", CubeMapSize, CubeMapSize);

		if (FAILED(Generate_ShadowArray2D(m_pStaticDirectionalShadowList, ShadowSize, ShadowSize)))		return E_FAIL;
		if (FAILED(Generate_ShadowArray2D(m_pDynamicDirectionalShadowList, ShadowSize, ShadowSize)))	return E_FAIL;

		if (FAILED(Generate_ShadowArrayCube(m_pStaticPointShadowList, CubeMapSize, CubeMapSize)))		return E_FAIL;
		if (FAILED(Generate_ShadowArrayCube(m_pDynamicPointShadowList, CubeMapSize, CubeMapSize)))		return E_FAIL;
	}

	// LSY 변경: 라이트 생성/편집/저장/로드 책임을 배치 에디터에 위임한다.
	m_pPlacementEditor = CLightPlacementEditor::Create(this);
	if (!m_pPlacementEditor) return E_FAIL;

	return S_OK;
}

VOID CLightManager::UpdateGUI() {
	// LSY 변경: 배치 에디터가 준비된 경우 기존 단순 LightManager GUI 대신 사용한다.
	if (m_pPlacementEditor)
	{
		m_pPlacementEditor->UpdateGUI();
		return;
	}

	{
		ImGui::Begin("Light Manager");

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(180.f / 255.f, 135.f / 255.f, 255.f / 255.f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(180.f / 255.f * 1.2f, 135.f / 255.f * 1.2f, 255.f / 255.f * 1.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(180.f / 255.f / 2.f, 135.f / 255.f / 2.f, 255.f / 255.f / 2.f, 1.0f));

		if (ImGui::Button("Generate Light", ImVec2(-FLT_MIN, 20))) {
			Add_PointLight({ 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }, 10.f, 10.f, 20.f);
		}

		ImGui::PopStyleColor(3);
		if (m_LightHandleList.empty()) {
			ImGui::End();
			return;
		}

		static int selectedLightIdx = 0;

		if (selectedLightIdx >= static_cast<int>(m_LightHandleList.size()))
			selectedLightIdx = 0;

		ImGui::Text("Light List");
		if (ImGui::BeginListBox("##Lights", ImVec2(-FLT_MIN, 100)))
		{
			int i = 0;
			for (auto iter = m_LightHandleList.begin(); iter != m_LightHandleList.end();)
			{
				auto LightObject = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>((*iter).value());
				if (nullptr == LightObject) {
					iter = m_LightHandleList.erase(iter);
					continue;
				}
				auto pComCollider = LightObject->GetComponent<CComCollider>("ComCollider_Sphere");
				if (pComCollider)
				{
					auto ColliderType = pComCollider->Get()->GetCollType();

					if (ColliderType == CollType::Sphere) {
						static_cast<CCollSphere*>((pComCollider->Get()))->SetLocalBoundingSphere({}, LightObject->Get_LightRange());
					}
				}
				auto pComCollider_FR = LightObject->GetComponent<CComCollider>("ComCollider_Frustum");
				if (pComCollider_FR)
				{
					auto ColliderType = pComCollider->Get()->GetCollType();

					if (ColliderType == CollType::Frustum) {
						auto LightPos = LightObject->Get_LightPosition();
						static_cast<CCollFrustum*>((pComCollider->Get()))->SetLocalFrustum(
							XMMatrixLookAtLH(XMLoadFloat3(&LightPos),
								LightObject->GetComponent<CComTransform>("Com_Transform")->GetState(STATE::LOOK),
								LightObject->GetComponent<CComTransform>("Com_Transform")->GetState(STATE::UP)));
					}
				}
				std::string lightName = "Light" + std::to_string(i);
				LIGHT_TYPE type = LightObject->Get_LightType();
				if (type == LIGHT_TYPE::DIRECTIONAL)    lightName += " [Directional]";
				else if (type == LIGHT_TYPE::POINT)          lightName += " [Point]";
				else                                         lightName += " [Spot]";

				const bool isSelected = (selectedLightIdx == i);
				if (ImGui::Selectable(lightName.c_str(), isSelected))
				{
					selectedLightIdx = i;
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();

				i++;
				iter++;
			}
			ImGui::EndListBox();
		}

		if (m_LightHandleList.size() == 0) {
			ImGui::End();
			return;
		}
		auto pSelectedLight = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(m_LightHandleList[selectedLightIdx].value());
		if (nullptr == pSelectedLight) {
			ImGui::End();
			return;
		}

		ImGui::Separator();
		ImGui::Text("Selected Light Details (Index: %d)", selectedLightIdx);

		// --- Getter로 현재 값들 가져오기 ---
		LIGHT_TYPE lightType = pSelectedLight->Get_LightType();
		XMFLOAT3 direction = pSelectedLight->Get_LightDirection();
		XMFLOAT3 color = pSelectedLight->Get_LightColor();
		float intensity = pSelectedLight->Get_LightIntensity();
		float range = pSelectedLight->Get_LightRange();
		XMFLOAT3 position = pSelectedLight->Get_LightPosition();

		float innerAttn = 0.f, outerAttn = 0.f;

		if		(lightType == LIGHT_TYPE::SPOTLIGHT) {
			innerAttn = pSelectedLight->Get_LightInnerAttenuation();
			outerAttn = pSelectedLight->Get_LightOuterAttenuation();
		}
		else if (lightType == LIGHT_TYPE::POINT) {
			innerAttn = pSelectedLight->Get_PointLightInnerAttenuation();
			outerAttn = pSelectedLight->Get_PointLightOuterAttenuation();
		}

		const char* lightTypeNames[] = { "Directional", "Point", "Spot" };
		int currentTypeIdx = static_cast<int>(lightType);
		if (ImGui::Combo("Light Type", &currentTypeIdx, lightTypeNames, IM_ARRAYSIZE(lightTypeNames)))
		{
			pSelectedLight->Set_LightType(static_cast<LIGHT_TYPE>(currentTypeIdx));
		}

		if (ImGui::ColorEdit3("Color", &color.x))
		{
			pSelectedLight->Set_LightColor(color);
		}

		if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 300.0f, "%.2f"))
		{
			pSelectedLight->Set_LightIntensity(intensity);
		}

		// 타입별 가변 속성 노출
		if (lightType == LIGHT_TYPE::DIRECTIONAL || lightType == LIGHT_TYPE::SPOTLIGHT)
		{
			// 방향 벡터 조절 (DragFloat3)
			if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -100.0f, 100.0f, "%.2f"))
			{
				pSelectedLight->Set_LightDirection(direction);
			}
		}

		if (lightType == LIGHT_TYPE::POINT || lightType == LIGHT_TYPE::SPOTLIGHT)
		{
			// 위치 조절
			if (ImGui::DragFloat3("Position", &position.x, 0.1f, -5000.0f, 5000.0f, "%.2f"))
			{
				pSelectedLight->Set_LightPosition(position);
			}
			// 범위 조절
			if (ImGui::SliderFloat("Inner Attenuation", &innerAttn, 0.0f, 100.0f, "%.1f") && innerAttn < outerAttn)
			{
				pSelectedLight->Set_PointLightInnerAttenuation(innerAttn);
			}
			if (ImGui::SliderFloat("Outer Attenuation", &outerAttn, 0.0f, 100.0f, "%.1f"))
			{
				pSelectedLight->Set_PointLightOuterAttenuation(outerAttn);
			}
		}

		if (lightType == LIGHT_TYPE::SPOTLIGHT) {
			if (ImGui::SliderFloat("Inner Attenuation", &innerAttn, 0.0f, 75.0f, "%.1f") && innerAttn < outerAttn)
			{
				pSelectedLight->Set_LightInnerAttenuation(innerAttn);
			}
			if (ImGui::SliderFloat("Outer Attenuation", &outerAttn, 0.0f, 75.0f, "%.1f"))
			{
				pSelectedLight->Set_LightOuterAttenuation(outerAttn);
			}
		}
		ImGui::Separator();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 0.4f, 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.05f,    0.05f, 1.0f));

		if (ImGui::Button("Delete Light", ImVec2(-FLT_MIN, 20))) {
			CHandle DeleteObjectHandle = pSelectedLight->GetHandle();
			pSelectedLight->SetPendingDestroyCascade();
			for (auto iter = m_LightHandleList.begin(); iter != m_LightHandleList.end();) {
				if (*iter == DeleteObjectHandle) {
					iter = m_LightHandleList.erase(iter);
					break;
				}
				else iter++;
			}
		}
		ImGui::PopStyleColor(3);

		ImGui::End();
	}
}

void CLightManager::SetActivePlacementLightGroup(
	std::string_view sGroup)
{
	// LSY 변경: 런타임 로더가 현재 레벨의 배치 그룹을 에디터에 알려
	// 저장/로드/삭제가 다른 레벨의 라이트에 영향을 주지 않게 한다.
	if (m_pPlacementEditor)
		m_pPlacementEditor->
			SetActivePlacementGroup(sGroup);
}

std::optional<CHandle>
CLightManager::FindPlacementLightHandleByAlias(
	std::string_view sGroup,
	std::string_view sAlias) const
{
	// LSY 변경: 별칭은 레벨별로 중복될 수 있으므로 배치 그룹까지 함께 비교한다.
	// 수명이 끝날 수 있는 CLight 포인터 대신 세대 검증이 가능한 핸들을 반환한다.
	if (sGroup.empty() || sAlias.empty())
		return std::nullopt;

	for (const auto& optionalHandle :
		m_LightHandleList)
	{
		if (!optionalHandle)
			continue;

		const CLight* light = CGameInstance::Get().
			GetGameObjectByHandleT<CLight>(
				*optionalHandle);
		if (!light)
			continue;

		if (light->Get_LightPlacementGroup() ==
				sGroup &&
			light->Get_LightAlias() == sAlias)
		{
			return *optionalHandle;
		}
	}

	return std::nullopt;
}

VOID CLightManager::Update(_float fTimeDelta) {
	Update_ActiveLights();

	DrawDebugEffectLights();
}

HRESULT CLightManager::Capture_ShadowMap() {
	ZoneScopedN("Capture_ShadowMap");
	{
		++m_iShadowFrameIndex;

		SPtr<CResDepthStencilState> DepthWriteState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE");
		m_pContext->OMSetDepthStencilState(DepthWriteState->GetDepthStencilState().Get(), 0);
		
		m_pContext->OMSetRenderTargets(0, nullptr, nullptr);

		auto Rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, "RS_MULTIPLE_SHADOW");
		m_pContext->RSSetState(Rasterizer->GetRasterizerState().Get());
	}

	Allocate_ShadowSlot();

	Update_LightData();

	Invalidate_DynamicShadowMaps();

	ID3D11Buffer* ShadowCB = m_pShadowConstantBuffer->GetCBuffer().Get();

	for (uint32_t i = 0; i < m_pActiveShadowLightList.size(); ++i) {
		if (!m_pActiveShadowLightList[i])	continue;

		auto LightOBJ	= CGameInstance::Get().GetGameObjectByHandleT<CLight>(m_pActiveShadowLightList[i].value());
		if (nullptr == LightOBJ)			continue;

		auto ShadowSlot = LightOBJ->Get_ShadowSlotNumb();
		if (ShadowSlot == -1)				continue;

		if (LightOBJ->Get_LightType() == LIGHT_TYPE::POINT) {
			if (!m_pPointFaceVS || !m_pInstancedPointFaceVS || !m_pPointFacePS)	{ UnBind_ShadowResource(); return E_FAIL; }

			m_pContext->IASetInputLayout(m_pPointFaceVS->GetInputLayout().Get());
			m_pContext->VSSetShader(m_pPointFaceVS->GetVertexShader().Get(), nullptr, 0);
			m_pContext->GSSetShader(nullptr, nullptr, 0);
			m_pContext->PSSetShader(m_pPointFacePS->GetPixelShader().Get(), nullptr, 0);
			 
			m_pContext->RSSetViewports(1, &m_pPointShadowViewPort->GetViewPort());

			auto UpdatePointFaceCB = [&](uint32_t Face) -> HRESULT{
					m_pShadowConstantVariable.CurrentShadowLightIndex = i;
					m_pShadowConstantVariable.CurrentPointFaceIndex = Face;

					D3D11_MAPPED_SUBRESOURCE MRES{};
					if (FAILED(m_pContext->Map(ShadowCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))	return E_FAIL;
					memcpy(MRES.pData, &m_pShadowConstantVariable, sizeof(CB_SHADOW));
					m_pContext->Unmap(ShadowCB, 0);

					return S_OK;
				};

			m_pContext->VSSetConstantBuffers(11, 1, &ShadowCB);
			m_pContext->PSSetConstantBuffers(11, 1, &ShadowCB);
			m_pContext->GSSetConstantBuffers(11, 1, &ShadowCB);

			LightOBJ->Update_ObjectConstantBuffer(m_pContext.Get());

			RENDER_CTX RCTX{};
			RCTX.pass = RENDERPASS::SHADOW;

			const _bool bStaticWasDirty = LightOBJ->Is_StaticDirty();
			const _bool bUpdateFinalThisFrame = bStaticWasDirty ||
				((m_iShadowFrameIndex + static_cast<uint64_t>(ShadowSlot)) % 2ull == 0ull);

			if (bStaticWasDirty) {
				Build_StaticShadowCasterList(m_pActiveShadowLightList[i]);
				for (uint32_t Face = 0; Face < POINT_SHADOW_FACE_COUNT; ++Face) {
					if (FAILED(UpdatePointFaceCB(Face))) { UnBind_ShadowResource();  return E_FAIL; }

					auto StaticFaceDSV = m_pStaticPointShadowList.FaceDSVList[ShadowSlot][Face];
					if (!StaticFaceDSV) { UnBind_ShadowResource();  return E_FAIL; }

					m_pContext->ClearDepthStencilView(StaticFaceDSV.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
					m_pContext->OMSetRenderTargets(0, nullptr, StaticFaceDSV.Get());

					if (FAILED(LightOBJ->Capture_ShadowMap(m_pContext.Get(), RCTX, m_pStaticShadowCasterScratch, static_cast<int32_t>(Face)))) { UnBind_ShadowResource();  return E_FAIL; }

					if (FAILED(Render_ShadowInstanced(m_pContext, m_pActiveShadowLightList[i], true, static_cast<int32_t>(Face)))) { UnBind_ShadowResource();  return E_FAIL; }
				}
				LightOBJ->Set_StaticDirty(false);
			}

			const _bool bFinalShadowDirty = bStaticWasDirty || LightOBJ->Is_DynamicDirty();

			if (bFinalShadowDirty && bUpdateFinalThisFrame) {
				if (FAILED(Copy_StaticShadowToFinal(LIGHT_TYPE::POINT, static_cast<uint32_t>(ShadowSlot)))) { UnBind_ShadowResource();  return E_FAIL; }

				for (uint32_t Face = 0; Face < POINT_SHADOW_FACE_COUNT; ++Face) {
					if (FAILED(UpdatePointFaceCB(Face))) { UnBind_ShadowResource();  return E_FAIL; }
					
					auto DynamicFaceDSV = m_pDynamicPointShadowList.FaceDSVList[ShadowSlot][Face];
					if (!DynamicFaceDSV) { UnBind_ShadowResource();  return E_FAIL; }

					m_pContext->OMSetRenderTargets(0, nullptr, DynamicFaceDSV.Get());

					if (FAILED(LightOBJ->Capture_ShadowMap(m_pContext.Get(), RCTX, m_pRenderable_DynamicObjectList, static_cast<int32_t>(Face)))) { UnBind_ShadowResource();  return E_FAIL; }

					if (FAILED(Render_ShadowInstanced(m_pContext, m_pActiveShadowLightList[i], false, static_cast<int32_t>(Face)))) { UnBind_ShadowResource();  return E_FAIL; }
				}
				LightOBJ->Set_DynamicDirty(false);
			}
			
			m_pContext->GSSetShader(nullptr, nullptr, 0);
			m_pContext->PSSetShader(nullptr, nullptr, 0);
		}
		else {
			m_pContext->IASetInputLayout(m_pDirectionalLightVS->GetInputLayout().Get());
			m_pContext->VSSetShader(m_pDirectionalLightVS->GetVertexShader().Get(), nullptr, 0);
			m_pContext->GSSetShader(nullptr, nullptr, 0);
			m_pContext->PSSetShader(nullptr, nullptr, 0);

			m_pContext->RSSetViewports(1, &m_pDirectionalShadowViewPort->GetViewPort());

			D3D11_MAPPED_SUBRESOURCE MRES = {};
			if (SUCCEEDED(m_pContext->Map(ShadowCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
			{
				m_pShadowConstantVariable.CurrentShadowLightIndex = i;

				memcpy(MRES.pData, &m_pShadowConstantVariable, sizeof(CB_SHADOW));
				m_pContext->Unmap(ShadowCB, 0);
			}
			m_pContext->VSSetConstantBuffers(11, 1, &ShadowCB);
			m_pContext->PSSetConstantBuffers(11, 1, &ShadowCB);

			LightOBJ->Update_ObjectConstantBuffer(m_pContext.Get());

			RENDER_CTX RCTX{};
			RCTX.pass = RENDERPASS::SHADOW;

			const _bool bStaticWasDirty = LightOBJ->Is_StaticDirty();

			if (bStaticWasDirty) {
				Build_StaticShadowCasterList(m_pActiveShadowLightList[i]);
				auto StaticShadowDSV = m_pStaticDirectionalShadowList.DSVList[ShadowSlot];
				if (StaticShadowDSV) {
					m_pContext->ClearDepthStencilView(StaticShadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
					m_pContext->OMSetRenderTargets(0, nullptr, StaticShadowDSV.Get());

					if (FAILED(LightOBJ->Capture_ShadowMap(m_pContext.Get(), RCTX, m_pStaticShadowCasterScratch, -1))) { UnBind_ShadowResource();  return E_FAIL; }

					if (FAILED(Render_ShadowInstanced(m_pContext, m_pActiveShadowLightList[i], true)))				 { UnBind_ShadowResource();  return E_FAIL; }
	
					LightOBJ->Set_StaticDirty(false);
				}
			}

			const _bool bFinalShadowDirty = bStaticWasDirty || LightOBJ->Is_DynamicDirty();
			if (bFinalShadowDirty){
				if (FAILED(Copy_StaticShadowToFinal(LightOBJ->Get_LightType(), static_cast<uint32_t>(ShadowSlot)))) { UnBind_ShadowResource();  return E_FAIL; }
				auto DynamicShadowDSV = m_pDynamicDirectionalShadowList.DSVList[ShadowSlot];
				if (DynamicShadowDSV) {
					m_pContext->OMSetRenderTargets(0, nullptr, DynamicShadowDSV.Get());

					if (FAILED(LightOBJ->Capture_ShadowMap(m_pContext.Get(), RCTX, m_pRenderable_DynamicObjectList, -1))) { UnBind_ShadowResource();  return E_FAIL; }

					if (FAILED(Render_ShadowInstanced(m_pContext, m_pActiveShadowLightList[i], false))) { UnBind_ShadowResource();  return E_FAIL; }
					
					LightOBJ->Set_DynamicDirty(false);
				}
			}
		}
	}

	UnBind_ShadowResource();

	return S_OK;
}
HRESULT CLightManager::Render_ShadowInstanced(const ComPtr<ID3D11DeviceContext>& pContext, std::optional<CHandle> _LightHandle, _bool _bStaticBatch, int32_t _PointFaceIndex) {
	if (!_LightHandle) return E_FAIL;

	if (_PointFaceIndex < -1 || _PointFaceIndex >= static_cast<int32_t>(POINT_SHADOW_FACE_COUNT))	return E_FAIL;

	auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(_LightHandle.value());
	if (nullptr == LightOBJ) return E_FAIL;

	auto LightType = LightOBJ->Get_LightType();

	const _bool bUsePointFace = LightType == LIGHT_TYPE::POINT && _PointFaceIndex >= 0;

	const auto& InstancedVertexShader = bUsePointFace ? m_pInstancedPointFaceVS : LightType == LIGHT_TYPE::POINT ? m_pInstancedPointLightVS : m_pInstancedDirectionalLightVS;
	if (nullptr == InstancedVertexShader) return E_FAIL;

	pContext->IASetInputLayout(InstancedVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(InstancedVertexShader->GetVertexShader().Get(), nullptr, 0);

	if (FAILED(CGameInstance::Get().Render_ShadowInstanced(pContext.Get(), _LightHandle, _bStaticBatch, _PointFaceIndex)))  return E_FAIL;

	const auto& OriginalVertexShader = bUsePointFace ? m_pPointFaceVS : LightType == LIGHT_TYPE::POINT ? m_pPointLightVS : m_pDirectionalLightVS;
	if (nullptr == OriginalVertexShader) return E_FAIL;

	pContext->IASetInputLayout(OriginalVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(OriginalVertexShader->GetVertexShader().Get(), nullptr, 0);

	return S_OK;
}
HRESULT CLightManager::Render_ObjectShadow() {
	ZoneScopedN("Render_ObjectShadow");

	auto ActiveCamera = CGameInstance::Get().GetActiveCamera();
	if (nullptr == ActiveCamera)			return E_FAIL;

	if (nullptr == m_pUAVComBinedOutput)	return E_FAIL;

	{
		ID3D11UnorderedAccessView* UAV[1] = { m_pUAVComBinedOutput->GetUAV().Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, UAV, nullptr);

		m_pContext->CSSetShader(m_pShadowComputeShader->GetComputeShader().Get(), nullptr, 0);
	}
	
	XMMATRIX InvViewProj = XMMatrixMultiply(XMMatrixInverse(nullptr, ActiveCamera->GetView()), XMMatrixInverse(nullptr, ActiveCamera->GetProj()));

	uint32_t LightCount = 0;
	CB_LIGHT LightBuffer{};

	XMStoreFloat4x4(&LightBuffer.g_InvViewProj, InvViewProj);

	for (auto&	 LightHandle : m_pActiveLightList) {				// Normal Light Binding
		if (LightCount >= MAX_LIGHT_COUNT) break;
		if (!LightHandle) continue;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ) continue;

		const LIGHT_TYPE LightType = LightOBJ->Get_LightType();
		const bool bIsPointLight = (LightType == LIGHT_TYPE::POINT);

		LightBuffer.AffectedLight[LightCount].LightType = ETOUI(LightType);

		uint32_t LightMapCount = bIsPointLight ? POINT_SHADOW_FACE_COUNT : 1;
		for (uint32_t Face = 0; Face < LightMapCount; ++Face)
			LightBuffer.AffectedLight[LightCount].g_LightViewProj[Face] = LightOBJ->Get_LightViewProj(Face);

		LightBuffer.AffectedLight[LightCount].LightDirection	= LightOBJ->Get_LightDirection();
		LightBuffer.AffectedLight[LightCount].LightColor		= LightOBJ->Get_LightColor();
		LightBuffer.AffectedLight[LightCount].LightIntensity	= LightOBJ->Get_LightIntensity();
		LightBuffer.AffectedLight[LightCount].LightRange		= LightOBJ->Get_LightRange();
		LightBuffer.AffectedLight[LightCount].Position			= LightOBJ->Get_LightPosition();

		if (bIsPointLight) {
			LightBuffer.AffectedLight[LightCount].InnerAttanuation = LightOBJ->Get_PointLightInnerAttenuation();
			LightBuffer.AffectedLight[LightCount].OuterAttanuation = LightOBJ->Get_PointLightOuterAttenuation();
		}
		else {
			LightBuffer.AffectedLight[LightCount].InnerAttanuation = cosf(XMConvertToRadians(LightOBJ->Get_LightInnerAttenuation()));
			LightBuffer.AffectedLight[LightCount].OuterAttanuation = cosf(XMConvertToRadians(LightOBJ->Get_LightOuterAttenuation()));
		}
		
		LightBuffer.AffectedLight[LightCount].ShadowSlot		= LightOBJ->Get_ShadowSlotNumb();

		LightCount++;
	}
	LightBuffer.LightCount = LightCount;

	D3D11_MAPPED_SUBRESOURCE MRES{};
	if (SUCCEEDED(m_pContext->Map(m_pLightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		memcpy(MRES.pData, &LightBuffer, sizeof(CB_LIGHT));
		m_pContext->Unmap(m_pLightConstantBuffer->GetCBuffer().Get(), 0);
	}
	else { return E_FAIL; }

	m_pContext->CSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
	m_pContext->GSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());

	Bind_ShadowResource();

	Update_EffectLightData();

	_float2 ShadowMapResolution = CGameInstance::Get().GetClientScreenSize();
	
	m_pContext->Dispatch((ETOUI(ShadowMapResolution.x) + 15) / 16, (ETOUI(ShadowMapResolution.y) + 15) / 16, 1);

	UnBind_ShadowResource();

	return S_OK;
}

HRESULT CLightManager::Render_ObjectNonShadow(){
	auto ActiveCamera = CGameInstance::Get().GetActiveCamera();
	if (nullptr == ActiveCamera)			return E_FAIL;

	if (nullptr == m_pUAVComBinedOutput)	return E_FAIL;

	{
		ID3D11UnorderedAccessView* UAV[1] = { m_pUAVComBinedOutput->GetUAV().Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, UAV, nullptr);

		m_pContext->CSSetShader(m_pNonShadowComputeShader->GetComputeShader().Get(), nullptr, 0);

		m_pContext->RSSetViewports(1, &m_pDirectionalShadowViewPort->GetViewPort());
	}

	XMMATRIX InvViewProj = XMMatrixMultiply(XMMatrixInverse(nullptr, ActiveCamera->GetView()), XMMatrixInverse(nullptr, ActiveCamera->GetProj()));

	uint32_t LightCount = 0;
	CB_LIGHT LightBuffer{};

	XMStoreFloat4x4(&LightBuffer.g_InvViewProj, InvViewProj);

	for (auto& LightHandle : m_pActiveLightList) {				// Normal Light Binding
		if (LightCount >= MAX_LIGHT_COUNT) break;
		if (!LightHandle) continue;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ) continue;

		const LIGHT_TYPE LightType = LightOBJ->Get_LightType();
		const bool bIsPointLight = (LightType == LIGHT_TYPE::POINT);

		LightBuffer.AffectedLight[LightCount].LightType = ETOUI(LightType);

		uint32_t LightMapCount = bIsPointLight ? POINT_SHADOW_FACE_COUNT : 1;
		for (int Face = 0; Face < LightMapCount; ++Face)
			LightBuffer.AffectedLight[LightCount].g_LightViewProj[Face] = LightOBJ->Get_LightViewProj(Face);

		LightBuffer.AffectedLight[LightCount].LightDirection = LightOBJ->Get_LightDirection();
		LightBuffer.AffectedLight[LightCount].LightColor = LightOBJ->Get_LightColor();
		LightBuffer.AffectedLight[LightCount].LightIntensity = LightOBJ->Get_LightIntensity();
		LightBuffer.AffectedLight[LightCount].LightRange = LightOBJ->Get_LightRange();
		LightBuffer.AffectedLight[LightCount].Position = LightOBJ->Get_LightPosition();

		if (bIsPointLight) {
			LightBuffer.AffectedLight[LightCount].InnerAttanuation = LightOBJ->Get_PointLightInnerAttenuation();
			LightBuffer.AffectedLight[LightCount].OuterAttanuation = LightOBJ->Get_PointLightOuterAttenuation();
		}
		else {
			LightBuffer.AffectedLight[LightCount].InnerAttanuation = cosf(XMConvertToRadians(LightOBJ->Get_LightInnerAttenuation()));
			LightBuffer.AffectedLight[LightCount].OuterAttanuation = cosf(XMConvertToRadians(LightOBJ->Get_LightOuterAttenuation()));
		}
		LightBuffer.AffectedLight[LightCount].ShadowSlot = -1;

		LightCount++;
	}
	LightBuffer.LightCount = LightCount;

	D3D11_MAPPED_SUBRESOURCE MRES{};
	if (SUCCEEDED(m_pContext->Map(m_pLightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		memcpy(MRES.pData, &LightBuffer, sizeof(CB_LIGHT));
		m_pContext->Unmap(m_pLightConstantBuffer->GetCBuffer().Get(), 0);
	}
	else { return E_FAIL; }
	m_pContext->CSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
	m_pContext->GSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());

	Update_EffectLightData();

	_float2 ShadowMapResolution = CGameInstance::Get().GetClientScreenSize();

	m_pContext->Dispatch((ETOUI(ShadowMapResolution.x) + 15) / 16, (ETOUI(ShadowMapResolution.y) + 15) / 16, 1);

	UnBind_ShadowResource();

	return S_OK;
}

std::optional<CHandle> CLightManager::Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity) {
	CLight::DESC LDesc{};
	// LSY 변경: 문자열 리터럴 포인터 연산을 제거하고 실제 인덱스 문자열로 태그를 만든다.
	LDesc.sObjectTag =
		"Light_Clone" + std::to_string(m_LightHandleList.size());

	auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
	if (!(LightHandle))	return std::nullopt;

	auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());

	LightOBJ->Set_LightType(LIGHT_TYPE::DIRECTIONAL);
	LightOBJ->Set_LightDirection(_Direction);
	LightOBJ->Set_LightColor(_Color);
	LightOBJ->Set_LightIntensity(_Intensity);
	LightOBJ->InvalidateAllShadow();

	m_LightHandleList.push_back(LightHandle.value());

	return LightHandle;
}
std::optional<CHandle> CLightManager::Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _InnerRange, _float _OuterRange) {
	CLight::DESC LDesc{};
	// LSY 변경: 문자열 리터럴 포인터 연산을 제거하고 실제 인덱스 문자열로 태그를 만든다.
	LDesc.sObjectTag =
		"Light_Clone" + std::to_string(m_LightHandleList.size());

	auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
	if (!LightHandle)			return std::nullopt;

	auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
	if (nullptr == LightOBJ)	return std::nullopt;

	LightOBJ->Set_LightType(LIGHT_TYPE::POINT);

	LightOBJ->Set_LightPosition(_Position);
	LightOBJ->Set_LightColor(_Color);
	LightOBJ->Set_LightIntensity(_Intensity);

	LightOBJ->Set_PointLightInnerAttenuation(_InnerRange);
	LightOBJ->Set_PointLightOuterAttenuation(_OuterRange);

	LightOBJ->InvalidateAllShadow();

	m_LightHandleList.push_back(LightHandle.value());
	return LightHandle;
}
std::optional<CHandle> CLightManager::Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt) {
	CLight::DESC LDesc{};
	// LSY 변경: 문자열 리터럴 포인터 연산을 제거하고 실제 인덱스 문자열로 태그를 만든다.
	LDesc.sObjectTag =
		"Light_Clone" + std::to_string(m_LightHandleList.size());

	auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
	if (!LightHandle)	return std::nullopt;

	auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
	LightOBJ->Set_LightType(LIGHT_TYPE::SPOTLIGHT);

	LightOBJ->Set_LightPosition(_Position);
	LightOBJ->Set_LightColor(_Color);
	LightOBJ->Set_LightIntensity(_Intensity);
	LightOBJ->Set_LightRange(_Range);

	LightOBJ->Set_LightInnerAttenuation(_InnerAtt);
	LightOBJ->Set_LightOuterAttenuation(_OuterAtt);

	LightOBJ->InvalidateAllShadow();

	m_LightHandleList.push_back(LightHandle);
	return LightHandle;
}

_bool CLightManager::Remove_Light(const CHandle& hLight)
{
	// LSY 변경: 에디터/로더가 지정한 라이트만 안전하게 제거할 수 있도록
	// 관리자 목록과 활성 그림자 목록을 함께 정리한다.
	const size_t previousSize = m_LightHandleList.size();
	std::erase_if(
		m_LightHandleList,
		[&hLight](const std::optional<CHandle>& handle)
		{
			return handle && *handle == hLight;
		});

	if (previousSize == m_LightHandleList.size())
		return false;

	std::erase_if(
		m_pActiveShadowLightList,
		[&hLight](const std::optional<CHandle>& handle)
		{
			return handle && *handle == hLight;
		});

	std::erase_if(
		m_pActiveLightList,
		[&hLight](const std::optional<CHandle>& handle)
		{
			return handle && *handle == hLight;
		});

	if (CLight* light = CGameInstance::Get().
		GetGameObjectByHandleT<CLight>(hLight))
	{
		light->SetPendingDestroyCascade();
	}

	return true;
}

size_t CLightManager::Remove_PlacementLightGroup(
	std::string_view sGroup)
{
	// LSY 변경: 레벨별 배치 그룹만 제거하여 하드코딩 또는 다른 레벨의
	// 라이트가 함께 삭제되는 문제를 방지한다.
	if (sGroup.empty())
		return 0;

	std::vector<CHandle> handles{};
	handles.reserve(m_LightHandleList.size());
	for (const auto& optionalHandle : m_LightHandleList)
	{
		if (!optionalHandle)
			continue;

		CLight* light = CGameInstance::Get().
			GetGameObjectByHandleT<CLight>(*optionalHandle);
		if (light &&
			std::string_view{
				light->Get_LightPlacementGroup() } ==
				sGroup)
		{
			handles.push_back(*optionalHandle);
		}
	}

	size_t removedCount{};
	for (const CHandle& handle : handles)
	{
		if (Remove_Light(handle))
			++removedCount;
	}
	return removedCount;
}

HRESULT CLightManager::AddShadowRenderGroup(ACTORTYPE _ATYPE, IRenderable* pRenderObject) {
	if (nullptr == pRenderObject) return E_FAIL;

	_ATYPE == ACTORTYPE::DYNAMIC ? m_pRenderable_DynamicObjectList.push_back(pRenderObject) : m_pRenderable_StaticObjectList.push_back(pRenderObject);

	return S_OK;
}

VOID	CLightManager::Bind_ShadowResource() {

	ID3D11ShaderResourceView* ShadowSRV[] = {
		// Directional + Spot Static ShadowMap
		m_pStaticDirectionalShadowList.SRV.Get(),
		// Directional + Spot Dynamic ShadowMap
		m_pDynamicDirectionalShadowList.SRV.Get(),

		// Point Static ShadowMap
		m_pStaticPointShadowList.SRV.Get(),
		// Point Dynamic ShadowMap
		m_pDynamicPointShadowList.SRV.Get()
	};

	m_pContext->CSSetShaderResources(9, 4, ShadowSRV);
}

VOID	CLightManager::UnBind_ShadowResource() {
	ID3D11ShaderResourceView* NullSRV[13] = { nullptr };
	m_pContext->CSSetShaderResources(0, 13, NullSRV);

	ID3D11UnorderedAccessView* NullUAV[1] = { nullptr };
	m_pContext->CSSetUnorderedAccessViews(0, 1, NullUAV, nullptr);

	m_pContext->OMSetRenderTargets(0, nullptr, nullptr);

	m_pContext->VSSetShader(nullptr, nullptr, 0);
	m_pContext->GSSetShader(nullptr, nullptr, 0);
	m_pContext->CSSetShader(nullptr, nullptr, 0);
	m_pContext->PSSetShader(nullptr, nullptr, 0);

	m_pRenderable_StaticObjectList.clear();
	m_pRenderable_DynamicObjectList.clear();

	auto Rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	m_pContext->RSSetState(Rasterizer->GetRasterizerState().Get());

	SPtr<CResDepthStencilState> DepthReadState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHREAD");
	m_pContext->OMSetDepthStencilState(DepthReadState->GetDepthStencilState().Get(), 0);
}

VOID	CLightManager::Update_ActiveLights() {
	const auto PreviousActiveLightList = m_pActiveLightList;
	auto PreviouslyActive = [&PreviousActiveLightList](const std::optional<CHandle>& LightHandle) {
			return std::find(PreviousActiveLightList.begin(), PreviousActiveLightList.end(), LightHandle) != PreviousActiveLightList.end();
		};

	m_pActiveShadowLightList.clear();
	m_pActiveLightList.clear();
	std::vector<LightData> CullingLight{};

	auto Camera = CGameInstance::Get().GetActiveCamera();
	if (nullptr == Camera) return;

	XMVECTOR CameraPos = Camera->GetTransform().GetLoadedPostion();

	for (auto& LightHandle : m_LightHandleList) {
		if (!LightHandle) continue;
		auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ || LightOBJ->Get_LightActivateState() == false)	continue;
		
		if (LightOBJ->Get_LightType() == LIGHT_TYPE::DIRECTIONAL) {
			CullingLight.push_back({ LightHandle, 0.f });
			continue;
		}

		if (!IsInFrustum(LightOBJ)) continue;

		XMVECTOR CurrentPosition = LightOBJ->GetTransform().GetLoadedPostion();
		_float	 DistanceSQ = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(CameraPos, CurrentPosition)));

		if (PreviouslyActive(LightHandle))
		{
			constexpr _float ActiveRetentionRatio = 0.85f;
			DistanceSQ *= ActiveRetentionRatio;
		}

		  
		CullingLight.push_back({ LightHandle, DistanceSQ });
	}

	// 거리 기반 컬링 + 정렬(최단거리 순)
	std::stable_sort(CullingLight.begin(), CullingLight.end(), [](const LightData& SRC, const LightData& DST) {
		return SRC.DistanceSQ < DST.DistanceSQ;
	});

	// 최대 MAX_LIGHT_COUNT 수만큼의 조명만 렌더링
	const uint32_t FinalActiveLightCount = std::min<uint32_t>(MAX_LIGHT_COUNT, static_cast<uint32_t>(CullingLight.size()));
	for (uint32_t i = 0; i < FinalActiveLightCount; ++i) {
		const auto& LightHandle = CullingLight[i].LightHandle;
		if (!LightHandle) continue;

		auto LightOBJ =CGameInstance::Get().GetGameObjectByHandleT<CLight>(*LightHandle);
		if (!LightOBJ) continue;

		m_pActiveLightList.push_back(LightHandle);

		if (LightOBJ->Get_LightShadowCast() == false) continue;

		m_pActiveShadowLightList.push_back(LightHandle);
	}
}

_bool	CLightManager::IsInFrustum(CLight* _LightOBJ) {
	auto ActiveCam = CGameInstance::Get().GetActiveCamera();
	if (nullptr == ActiveCam)			return false;

	auto ActiveCamCollider = ActiveCam->GetViewVolumeCollider();
	if (nullptr == ActiveCamCollider)	return false;

	auto LightCollider = (_LightOBJ->Get_LightType() == LIGHT_TYPE::POINT ? _LightOBJ->Get_SphereCollider() : _LightOBJ->Get_FrustumCollider());
	if (nullptr == LightCollider)		return false;

	return ActiveCamCollider->Intersect(*LightCollider.get());
}

HRESULT CLightManager::Copy_StaticShadowToFinal(LIGHT_TYPE _LightType, uint32_t _ShadowSlot) {
	if (_ShadowSlot >= MAX_SHADOW_LIGHT_COUNT) return E_FAIL;

	m_pContext->OMSetRenderTargets(0, nullptr, nullptr);

	ID3D11ShaderResourceView* NullShadowSRVs[4]{};
	m_pContext->CSSetShaderResources(9, 4, NullShadowSRVs);

	constexpr uint32_t MipLevels = 1;

	if (_LightType == LIGHT_TYPE::POINT)
	{
		ID3D11Texture2D* SourceTexture = m_pStaticPointShadowList.TexBuffer.Get();
		ID3D11Texture2D* DestinationTexture = m_pDynamicPointShadowList.TexBuffer.Get();

		if (!SourceTexture || !DestinationTexture) return E_FAIL;

		for (uint32_t Face = 0; Face < POINT_SHADOW_FACE_COUNT; ++Face) {
			const uint32_t ArraySlice = _ShadowSlot * POINT_SHADOW_FACE_COUNT + Face;
			const uint32_t Subresource =D3D11CalcSubresource(0, ArraySlice, MipLevels);

			m_pContext->CopySubresourceRegion(DestinationTexture, Subresource, 0, 0, 0, SourceTexture, Subresource, nullptr);
		}
	}
	else
	{
		ID3D11Texture2D* SourceTexture = m_pStaticDirectionalShadowList.TexBuffer.Get();
		ID3D11Texture2D* DestinationTexture = m_pDynamicDirectionalShadowList.TexBuffer.Get();

		if (!SourceTexture || !DestinationTexture) return E_FAIL;

		const uint32_t Subresource =D3D11CalcSubresource(0, _ShadowSlot, MipLevels);

		m_pContext->CopySubresourceRegion(DestinationTexture, Subresource, 0, 0, 0, SourceTexture, Subresource, nullptr);
	}

	return S_OK;
}

#pragma region EFFECT_LIGHT
VOID CLightManager::Update_EffectLightData() {
	if (!m_pEffectLightConstantBuffer)	return;

	CB_EFFECT_LIGHT ELightBuffer{};
	uint32_t LightCount = 0;

	for (const auto& LightHandle : m_pEffectLightPool) {
		if (LightCount >= MAX_EFFECTLIGHT_COUNT)	break;

		if (!LightHandle) continue;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());

		if (nullptr == LightOBJ || !LightOBJ->Is_EffectLight() || !LightOBJ->Get_LightActivateState()) continue;
	
		EFFECT_LIGHT& ELight = ELightBuffer.EffectLight[LightCount];
		ELight.Position = LightOBJ->Get_LightPosition();
		ELight.LightIntensity = LightOBJ->Get_LightIntensity();
		ELight.LightColor = LightOBJ->Get_LightColor();
		ELight.InnerAttanuation = LightOBJ->Get_PointLightInnerAttenuation();
		ELight.OuterAttanuation = LightOBJ->Get_PointLightOuterAttenuation();

		++LightCount;
	}
	ELightBuffer.LightCount = LightCount;

	D3D11_MAPPED_SUBRESOURCE MRES = {};
	auto Buffer = m_pEffectLightConstantBuffer->GetCBuffer().Get();
	if (SUCCEEDED(m_pContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		memcpy(MRES.pData, &ELightBuffer, sizeof(CB_EFFECT_LIGHT));
		m_pContext->Unmap(Buffer, 0);
	}
	else { return; }

	m_pContext->CSSetConstantBuffers(11, 1, m_pEffectLightConstantBuffer->GetCBuffer().GetAddressOf());
	
}
void CLightManager::ClearEffectLightPool()
{
	// LSY 변경: 레벨 전환 시 이전 레벨의 핸들이 풀 앞부분에 남아
	// 새로 생성한 라이트가 할당되지 않는 문제를 방지한다.
	for (const auto& optionalHandle :
		m_pEffectLightPool)
	{
		if (!optionalHandle)
			continue;

		// LSY 변경: 일반 라이트 목록에 함께 등록된 경우에는 목록과 객체를
		// 같이 정리하고, 이펙트 전용 풀에만 있는 객체도 반드시 파괴 예약한다.
		if (!Remove_Light(*optionalHandle))
		{
			if (CGameObject* object =
				CGameInstance::Get().
					GetGameObjectByHandle(
						*optionalHandle))
			{
				object->
					SetPendingDestroyCascade();
			}
		}
	}

	m_pEffectLightPool.clear();
	m_iEffectLightPoolSize = 0;
	m_iLastAllocatedIndex = 0;
}

VOID CLightManager::Build_StaticShadowCasterList(std::optional<CHandle> _LightHandle) {
	m_pStaticShadowCasterScratch.clear();

	if (!_LightHandle) return;

	auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(_LightHandle.value());
	if (nullptr == LightOBJ) return;

	m_pStaticShadowCasterScratch.insert(
		m_pStaticShadowCasterScratch.end(),
		m_pRenderable_StaticObjectList.begin(),
		m_pRenderable_StaticObjectList.end());

	const auto& MapChunks = CGameInstance::Get().GetMapChunks();

	for (const auto& [Coord, Chunk] : MapChunks)
	{
		if (Chunk.loadState != EChunkLoadState::Loaded) continue;

		const BoundingBox& ChunkBounds = Chunk.octreeNode ? Chunk.octreeNode->GetCullingBoundingBox() : Chunk.bounds;

		if (!LightOBJ->Intersects_ShadowBounds(ChunkBounds))	continue;

		for (const CHandle& ObjectHandle : Chunk.hObjects)
		{
			CMapMeshObject* pMapObject = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(ObjectHandle);
			if (nullptr == pMapObject) continue;

			BoundingBox ObjectBounds{};

			if (pMapObject->GetShadowBounds(ObjectBounds)) {
				if (!LightOBJ->Intersects_ShadowBounds(ObjectBounds)) continue;
			}

			m_pStaticShadowCasterScratch.push_back(pMapObject);
		}
	}
}

VOID CLightManager::Notify_StaticShadowSceneChanged(const BoundingBox& ChangedBounds) {
	for (const auto& LightHandle : m_LightHandleList)
	{
		if (!LightHandle)	continue;

		CLight* pLight = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());

		if (nullptr == pLight || pLight->Is_EffectLight() || !pLight->Get_LightActivateState() ||
			!pLight->Get_LightShadowCast())	continue;
		
		if (pLight->Intersects_ShadowBounds(ChangedBounds))
			pLight->Set_StaticDirty(true);
	}
}

HRESULT CLightManager::Initialize_EffectLight(
	uint32_t _PoolSize)
{
	// LSY 변경: 이펙트 풀은 레벨 수명에 맞춰 완전히 재구축한다.
	ClearEffectLightPool();

	if (_PoolSize == 0)
		return E_INVALIDARG;

	m_pEffectLightPool.reserve(_PoolSize);
	m_iEffectLightPoolSize = _PoolSize;
	m_iLastAllocatedIndex = _PoolSize - 1;

	for (uint32_t i = 0; i < _PoolSize; ++i)
	{
		CLight::DESC desc{};
		desc.sObjectTag =
			"EffectLight_" + std::to_string(i);

		auto lightHandle =
			CGameInstance::Get().
				AddGameObjectToLayer(
					"PERMANENT",
					"Prototype_GameObject_Light",
					"LightLayer",
					&desc);
		if (!lightHandle)
		{
			ClearEffectLightPool();
			return E_FAIL;
		}

		m_pEffectLightPool.push_back(
			*lightHandle);

		CLight* light = CGameInstance::Get().
			GetGameObjectByHandleT<CLight>(
				*lightHandle);
		if (!light)
		{
			ClearEffectLightPool();
			return E_FAIL;
		}

		light->Set_LightType(LIGHT_TYPE::POINT);
		light->Set_EffectLight(true);

		// LSY 변경: 생성 직후 첫 프레임에 풀 라이트가 모두 활성화되는 것을 막는다.
		light->Reset_Light();
	}

	return S_OK;
}

std::optional<CHandle> CLightManager::Allocate_EffectLight(XMVECTOR _WorldPos, _float _Intensity, _float3 _Color,  _float _InnerRange, _float _OuterRange, _float _LifeTime, _float3 _Velocity) {
	// LSY 변경: 고정 상수가 아니라 현재 레벨에서 실제 생성된 풀 크기를 사용한다.
	const size_t poolSize = m_pEffectLightPool.size();

	if (poolSize == 0) {
		DEBUG_LOG("[EffectLight] Allocation failed: pool is empty.\n");
		return std::nullopt;
	}

	std::optional<CHandle> selectedHandle{};
	CLight* selectedLight = nullptr;

	for (size_t i = 0; i < poolSize; ++i) {
		const size_t circularIndex =
			(static_cast<size_t>(m_iLastAllocatedIndex) + 1 + i) % poolSize;

		const auto& lightHandle = m_pEffectLightPool[circularIndex];
		if (!lightHandle)
			continue;

		auto* light = CGameInstance::Get()
			.GetGameObjectByHandleT<CLight>(*lightHandle);

		if (!light || light->Get_LightActivateState())
			continue;

		selectedHandle = *lightHandle;
		selectedLight = light;
		m_iLastAllocatedIndex = static_cast<uint32_t>(circularIndex);
		break;
	}

	if (!selectedHandle || !selectedLight) {
		DEBUG_LOG("[EffectLight] Allocation failed: no inactive light is available.\n");
		return std::nullopt;
	}

	XMFLOAT3 worldPosition{};
	XMStoreFloat3(&worldPosition, _WorldPos);

	selectedLight->Set_LightPosition(worldPosition);
	selectedLight->Set_LightIntensity(_Intensity);
	selectedLight->Set_LightColor(_Color);
	selectedLight->Set_PointLightInnerAttenuation(_InnerRange);
	selectedLight->Set_PointLightOuterAttenuation(_OuterRange);
	selectedLight->Set_LightLifeTime(_LifeTime);
	selectedLight->Set_LightVelocity(_Velocity);

	selectedLight->Set_LightActivateState(true);

	return selectedHandle;
}
_bool CLightManager::IsActiveShadowLight(std::optional<CHandle>& _Handle) {
	return std::ranges::any_of(m_pActiveShadowLightList,[&_Handle](const std::optional<CHandle>& activeHandle) {
			return activeHandle && *activeHandle == _Handle;
		});
}
HRESULT CLightManager::Reset_EffectLight(const std::optional<CHandle>& _Handle) {
	auto iter = std::find(m_pEffectLightPool.begin(), m_pEffectLightPool.end(), _Handle);
	if (iter == m_pEffectLightPool.end()) return E_FAIL;

	auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>((*iter).value());
	if (nullptr == LightOBJ) return E_FAIL;

	LightOBJ->Reset_Light();

	return S_OK;
}

HRESULT CLightManager::Reset_AllEffectLight() {
	for (auto& LightHandle : m_pEffectLightPool) {
		if (!LightHandle) return E_FAIL;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ)return E_FAIL;

		LightOBJ->Reset_Light();
	}

	return S_OK;
}

HRESULT CLightManager::Transform_EffectLight(const std::optional<CHandle>& _Handle, XMFLOAT3 _Position){
	auto iter = std::find(m_pEffectLightPool.begin(), m_pEffectLightPool.end(), _Handle);
	if (iter == m_pEffectLightPool.end()) return E_FAIL;

	auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>((*iter).value());
	if (nullptr == LightOBJ) return E_FAIL;

	LightOBJ->Set_LightPosition(_Position);

	return S_OK;
}
VOID	CLightManager::Clear_DynamicLightList() {
	m_LightHandleList.clear();
	m_pActiveShadowLightList.clear(); 
	m_pActiveLightList.clear(); 

	m_PointShadowSlotOwners.fill(std::nullopt);
	m_2DShadowSlotOwners.fill(std::nullopt);
}
HRESULT CLightManager::Transform_EffectLight(const std::optional<CHandle>& _Handle, XMVECTOR _Position) {
	auto iter = std::find(m_pEffectLightPool.begin(), m_pEffectLightPool.end(), _Handle);
	if (iter == m_pEffectLightPool.end()) return E_FAIL;

	auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>((*iter).value());
	if (nullptr == LightOBJ) return E_FAIL;

	LightOBJ->Set_LightPosition(_Position);

	return S_OK;
}

void CLightManager::DrawDebugEffectLights()
{
	// LSY 변경: ColliderManager에 다시 등록하지 않고 활성 이펙트 라이트만
	// 전용 풀에서 직접 읽어 영향 범위를 시각화한다.
	if (!m_bEffectLightDebugVisible)
		return;

	CDbgLineRender* debug =
		CGameInstance::Get().GetDbgLineRender();
	if (!debug)
		return;

	const _float4 previousColor =
		debug->GetColor();
	const DBG_LINE_DEPTH_MODE previousDepth =
		debug->GetDepthMode();

	debug->SetDepthTest(
		m_bEffectLightDebugDepthTest);
	debug->SetColor(
		{ 1.f, 0.f, 1.f, 1.f });

	for (const auto& optionalHandle :
		m_pEffectLightPool)
	{
		if (!optionalHandle)
			continue;

		CLight* light = CGameInstance::Get().
			GetGameObjectByHandleT<CLight>(
				*optionalHandle);
		if (!light ||
			!light->Get_LightActivateState())
		{
			continue;
		}

		const _float3 position =
			light->Get_LightPosition();

		debug->AddCross(position, 0.2f);
		debug->AddSphere(
			std::max(
				light->Get_PointLightOuterAttenuation(),
				0.02f),
			XMMatrixTranslation(
				position.x,
				position.y,
				position.z));
	}

	debug->SetColor(previousColor);
	debug->SetDepthMode(previousDepth);
}
#pragma endregion
HRESULT CLightManager::Generate_ShadowArray2D(SHADOW_ARRAY_2D& _SHAR, uint32_t _ResolutionX, uint32_t _ResolutionY) {
	D3D11_TEXTURE2D_DESC TEXDesc = {};
	TEXDesc.Width = _ResolutionX;
	TEXDesc.Height = _ResolutionY;
	TEXDesc.MipLevels = 1;
	TEXDesc.ArraySize = MAX_SHADOW_LIGHT_COUNT;
	TEXDesc.Format = DXGI_FORMAT_R16_TYPELESS;//DXGI_FORMAT_R32_TYPELESS;
	TEXDesc.SampleDesc = { .Count = 1, .Quality = 0 };
	TEXDesc.Usage = D3D11_USAGE_DEFAULT;
	TEXDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(m_pDevice->CreateTexture2D(&TEXDesc, nullptr, _SHAR.TexBuffer.GetAddressOf()))) return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R16_UNORM;//DXGI_FORMAT_R32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	SRVDesc.Texture2DArray.MostDetailedMip = 0;
	SRVDesc.Texture2DArray.MipLevels = 1;
	SRVDesc.Texture2DArray.FirstArraySlice = 0;
	SRVDesc.Texture2DArray.ArraySize = MAX_SHADOW_LIGHT_COUNT;
	
	if (FAILED(m_pDevice->CreateShaderResourceView(_SHAR.TexBuffer.Get(), &SRVDesc, _SHAR.SRV.GetAddressOf()))) return E_FAIL;
	
	for (uint32_t i = 0; i < MAX_SHADOW_LIGHT_COUNT; ++i) {
		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format = DXGI_FORMAT_D16_UNORM;//DXGI_FORMAT_D32_FLOAT;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		DSVDesc.Texture2DArray.ArraySize = 1;
		DSVDesc.Texture2DArray.FirstArraySlice = i;
		DSVDesc.Texture2DArray.MipSlice = 0;

		if (FAILED(m_pDevice->CreateDepthStencilView(_SHAR.TexBuffer.Get(), &DSVDesc, _SHAR.DSVList[i].GetAddressOf()))) return E_FAIL;
	}
	
	return S_OK;
}
HRESULT CLightManager::Generate_ShadowArrayCube(SHADOW_ARRAY_CUBE& _SHAR, uint32_t _ResolutionX, uint32_t _ResolutionY) {
	D3D11_TEXTURE2D_DESC TEXDesc = {};
	TEXDesc.Width = _ResolutionX;
	TEXDesc.Height = _ResolutionY;
	TEXDesc.MipLevels = 1;
	TEXDesc.ArraySize = MAX_SHADOW_LIGHT_COUNT * POINT_SHADOW_FACE_COUNT;
	TEXDesc.Format = DXGI_FORMAT_R16_TYPELESS; //DXGI_FORMAT_R32_TYPELESS;
	TEXDesc.SampleDesc = { .Count = 1, .Quality = 0 };
	TEXDesc.Usage = D3D11_USAGE_DEFAULT;
	TEXDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	TEXDesc.CPUAccessFlags = 0;
	TEXDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	if (FAILED(m_pDevice->CreateTexture2D(&TEXDesc, nullptr, _SHAR.TexBuffer.GetAddressOf()))) return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R16_UNORM;//DXGI_FORMAT_R32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
	SRVDesc.TextureCubeArray.MostDetailedMip = 0;
	SRVDesc.TextureCubeArray.MipLevels = 1;
	SRVDesc.TextureCubeArray.First2DArrayFace = 0;
	SRVDesc.TextureCubeArray.NumCubes = MAX_SHADOW_LIGHT_COUNT;

	if (FAILED(m_pDevice->CreateShaderResourceView(_SHAR.TexBuffer.Get(), &SRVDesc, _SHAR.SRV.GetAddressOf()))) return E_FAIL;

	for (uint32_t i = 0; i < MAX_SHADOW_LIGHT_COUNT; ++i) {
		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format = DXGI_FORMAT_D16_UNORM;//DXGI_FORMAT_D32_FLOAT;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		DSVDesc.Texture2DArray.ArraySize = POINT_SHADOW_FACE_COUNT;
		DSVDesc.Texture2DArray.FirstArraySlice = i * POINT_SHADOW_FACE_COUNT;
		DSVDesc.Texture2DArray.MipSlice = 0;

		if (FAILED(m_pDevice->CreateDepthStencilView(_SHAR.TexBuffer.Get(), &DSVDesc, _SHAR.DSVList[i].GetAddressOf()))) return E_FAIL;
	
		for (uint32_t Face = 0; Face < POINT_SHADOW_FACE_COUNT; ++Face) {
			D3D11_DEPTH_STENCIL_VIEW_DESC FaceDSVDesc{};
			FaceDSVDesc.Format = DXGI_FORMAT_D16_UNORM;
			FaceDSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			FaceDSVDesc.Texture2DArray.ArraySize = 1;
			FaceDSVDesc.Texture2DArray.FirstArraySlice = i * POINT_SHADOW_FACE_COUNT + Face;
			FaceDSVDesc.Texture2DArray.MipSlice = 0;

			if (FAILED(m_pDevice->CreateDepthStencilView(_SHAR.TexBuffer.Get(), &FaceDSVDesc, _SHAR.FaceDSVList[i][Face].GetAddressOf()))) return E_FAIL;
		}
	}
	return S_OK;
}

VOID	CLightManager::Update_LightData() {
	m_pLightConstantVariable.LightCount = std::min<uint32_t>(MAX_LIGHT_COUNT, static_cast<uint32_t>(m_pActiveShadowLightList.size()));
	uint32_t PLSlotIndex = 0, DLSlotIndex = 0;

	if (auto ActiveCamera = CGameInstance::Get().GetActiveCamera()) {
		XMMATRIX InvViewProj = XMMatrixMultiply(XMMatrixInverse(nullptr, ActiveCamera->GetView()), XMMatrixInverse(nullptr, ActiveCamera->GetProj()));
		XMStoreFloat4x4(&m_pLightConstantVariable.g_InvViewProj, InvViewProj);
	}

	{
		for (uint32_t i = 0; i < m_pActiveShadowLightList.size(); ++i) {
			if (i >= MAX_LIGHT_COUNT) break;
			if (!m_pActiveShadowLightList[i]) continue;

			auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(m_pActiveShadowLightList[i].value());
			if (nullptr == LightOBJ) continue;

			LightOBJ->PrepareShadowMapMatrices();

			const LIGHT_TYPE LightType = LightOBJ->Get_LightType();
			_bool bIsPointLight = (LightType == LIGHT_TYPE::POINT);
			
			uint32_t LightMapCount = bIsPointLight ? POINT_SHADOW_FACE_COUNT : 1;
			for (int Face = 0; Face < LightMapCount; ++Face) {
				m_pLightConstantVariable.AffectedLight[i].g_LightViewProj[Face] = LightOBJ->Get_LightViewProj(Face);
			}

			m_pLightConstantVariable.AffectedLight[i].LightType				= ETOUI(LightType);
			m_pLightConstantVariable.AffectedLight[i].LightDirection		= LightOBJ->Get_LightDirection();
			m_pLightConstantVariable.AffectedLight[i].LightColor			= LightOBJ->Get_LightColor();
			m_pLightConstantVariable.AffectedLight[i].LightIntensity		= LightOBJ->Get_LightIntensity();
			m_pLightConstantVariable.AffectedLight[i].LightRange			= LightOBJ->Get_LightRange();
			m_pLightConstantVariable.AffectedLight[i].Position				= LightOBJ->Get_LightPosition();

			if (bIsPointLight) {
				m_pLightConstantVariable.AffectedLight[i].InnerAttanuation = LightOBJ->Get_PointLightInnerAttenuation();
				m_pLightConstantVariable.AffectedLight[i].OuterAttanuation = LightOBJ->Get_PointLightOuterAttenuation();
			}
			else {
				m_pLightConstantVariable.AffectedLight[i].InnerAttanuation = cosf(XMConvertToRadians(LightOBJ->Get_LightInnerAttenuation()));
				m_pLightConstantVariable.AffectedLight[i].OuterAttanuation = cosf(XMConvertToRadians(LightOBJ->Get_LightOuterAttenuation()));
			}
			m_pLightConstantVariable.AffectedLight[i].ShadowSlot			= LightOBJ->Get_ShadowSlotNumb();
		}
	}
	{
		D3D11_MAPPED_SUBRESOURCE MRES = {};
		if (SUCCEEDED(m_pContext->Map(m_pLightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
		{
			memcpy(MRES.pData, &m_pLightConstantVariable, sizeof(CB_LIGHT));
			m_pContext->Unmap(m_pLightConstantBuffer->GetCBuffer().Get(), 0);
		}
		else { return; }
		m_pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
		m_pContext->GSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
		m_pContext->CSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
		m_pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
	}
}

VOID	CLightManager::Allocate_ShadowSlot(){
	constexpr uint32_t MAX_ACTIVE_POINT_SHADOWS = 2;
	constexpr uint32_t MAX_ACTIVE_2D_SHADOWS = 3;
	std::vector<std::optional<CHandle>> ShadowCandidates;
	ShadowCandidates.reserve(MAX_ACTIVE_POINT_SHADOWS + MAX_ACTIVE_2D_SHADOWS);

	uint32_t PointShadowCount = 0;
	uint32_t Shadow2DCount = 0;

	for (const auto& LightHandle : m_pActiveShadowLightList)
	{
		if (!LightHandle)	continue;

		CLight* LightOBJ =CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());

		if (!LightOBJ || !LightOBJ->Get_LightActivateState() || !LightOBJ->Get_LightShadowCast())	continue;


		if (LightOBJ->Get_LightType() == LIGHT_TYPE::POINT) {
			if (PointShadowCount >=	MAX_ACTIVE_POINT_SHADOWS)	continue;
			++PointShadowCount;
		}
		else {
			if (Shadow2DCount >= MAX_ACTIVE_2D_SHADOWS)			continue;
			++Shadow2DCount;
		}

		ShadowCandidates.push_back(LightHandle);
	}
	auto IsShadowCandidate = [&ShadowCandidates](const std::optional<CHandle>& Handle) {
			return std::find(ShadowCandidates.begin(), ShadowCandidates.end(), Handle) != ShadowCandidates.end();
		};
	auto ReleaseInvalidOwners = [this, &IsShadowCandidate](std::array<std::optional<CHandle>, MAX_SHADOW_LIGHT_COUNT>& Owners, _bool bPointSlot) {
		for (uint32_t Slot = 0; Slot < MAX_SHADOW_LIGHT_COUNT; ++Slot) {
			auto& Owner = Owners[Slot];

			if (!Owner)	continue;

			CLight* LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(Owner.value());

			if (!LightOBJ) {
				Owner.reset(); continue;
			}

			const _bool bCorrectType = bPointSlot ? LightOBJ->Get_LightType() == LIGHT_TYPE::POINT : LightOBJ->Get_LightType() != LIGHT_TYPE::POINT;
			const _bool bKeepSlot = LightOBJ->Get_LightActivateState() && LightOBJ->Get_LightShadowCast() && bCorrectType && IsShadowCandidate(Owner);

			if (bKeepSlot)	continue;

			if (LightOBJ->Get_ShadowSlotNumb() == static_cast<int32_t>(Slot))	LightOBJ->Set_ShadowSlotNumb(-1);

			Owner.reset();
		}
	};

	ReleaseInvalidOwners(m_PointShadowSlotOwners, true);
	ReleaseInvalidOwners(m_2DShadowSlotOwners, false);

	for (const auto& LightHandle : ShadowCandidates) {
		if (!LightHandle) continue;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ) continue;

		std::array<std::optional<CHandle>, MAX_SHADOW_LIGHT_COUNT>& Owners = LightOBJ->Get_LightType() == LIGHT_TYPE::POINT ? m_PointShadowSlotOwners : m_2DShadowSlotOwners;

		auto OwnerIter = std::find(Owners.begin(), Owners.end(), LightHandle);

		if (OwnerIter != Owners.end()) {
			const int32_t slot = static_cast<int32_t>(std::distance(Owners.begin(), OwnerIter));

			LightOBJ->Set_ShadowSlotNumb(slot);
			continue;
		}
		auto EmptyIter = std::find(Owners.begin(), Owners.end(), std::nullopt);
		if (EmptyIter == Owners.end()) {
			LightOBJ->Set_ShadowSlotNumb(-1);
			continue;
		}

		const int32_t NewSlot = static_cast<int32_t>(std::distance(Owners.begin(), EmptyIter));
		*EmptyIter = *LightHandle;

		LightOBJ->Set_ShadowSlotNumb(NewSlot);

		LightOBJ->Set_StaticDirty(true);
		LightOBJ->Set_DynamicDirty(true);
	}
}

VOID CLightManager::Invalidate_DynamicShadowMaps(){
	const auto& ActiveBatchList = CGameInstance::Get().Get_ActiveBatches();

	for (auto LightHandle : m_pActiveShadowLightList) {
		if (!LightHandle) continue;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ) continue;

		_bool bHasCasterNow = false;

		for (auto Renderable : m_pRenderable_DynamicObjectList)
		{
			if (!Renderable) continue;

			BoundingBox Bounds{};

			if (!Renderable->GetShadowBounds(Bounds)) {
				bHasCasterNow = true;
				break;
			}

			if (LightOBJ->Intersects_ShadowBounds(Bounds)) {
				bHasCasterNow = true;
				break;
			}
		}

		if (!bHasCasterNow) {
			for (const MODEL_INSTANCE_BATCH* Batch : ActiveBatchList) {
				if (!Batch || Batch->Instances.empty() || Batch->bModelStatic || Batch->bGPUSkinned)	continue;

				const size_t InstanceCount = Batch->Instances.size();

				for (size_t i = 0; i < InstanceCount; ++i) {
					if (i >= Batch->ShadowBounds.size() || !Batch->ShadowBounds[i].has_value()) {
						bHasCasterNow = true;
						break;
					}

					if (LightOBJ->Intersects_ShadowBounds(Batch->ShadowBounds[i].value())) {
						bHasCasterNow = true;
						break;
					}
				}

				if (bHasCasterNow)	break;
			}
		}
		if (bHasCasterNow || LightOBJ->Had_DynamicShadowCaster())	LightOBJ->Set_DynamicDirty(true);

		LightOBJ->Set_HadDynamicShadowCaster(bHasCasterNow);
	}
}

UPtr<CLightManager> CLightManager::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) {
	auto pInstance = ToUPtr(new CLightManager{ pDevice, pContext });
	if (FAILED(pInstance->Initialize_LightManager())) {
		MSG_BOX("Failed to Created : CLightManager");
		return nullptr;
	}
	return pInstance;
}
