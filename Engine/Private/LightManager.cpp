#include "pch.h"
#include "LightManager.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "CollSphere.h"
#include "CollFrustum.h"

CLightManager::CLightManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) : m_pDevice(pDevice), m_pContext(pContext) {}
CLightManager::~CLightManager() { }

HRESULT CLightManager::Initialize_LightManager() {

	if (E::CGameInstance::Get().AddPrototype("PERMANENT", "Prototype_GameObject_Light", CLight::Create()))	return E_FAIL;


	if (m_pLightConstantBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Light", E::CResCBuffer::Create()))
	{
		if (FAILED(m_pLightConstantBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_LIGHT) })))    return E_FAIL;
	}

	m_pShadowComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PBR");
	if (nullptr == m_pShadowComputeShader)		return E_FAIL;

	m_pUAVComBinedOutput = CGameInstance::Get().Generate_UnorderedAccessView("ComBinedTex", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);
	
	if (m_pDirectionalLightVS = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Shadow_Direct", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pDirectionalLightVS->Load(CResShader::DESC{ .sEntryPoint = "VSMain_Final", .sTarget = "vs_5_0" })))    return E_FAIL;
	}
	if (m_pPointLightVS = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Shadow", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointLightVS->Load()))    return E_FAIL;
	}
	if (m_pPointLightGS = CGameInstance::Get().AddResourceT<E::CResGeometryShader>(TAG_RES_GRP_PERMANENT_SHADER, "GS_Shadow", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointLightGS->Load()))    return E_FAIL;
	}
	if (m_pPointLightPS = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Shadow", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointLightPS->Load()))    return E_FAIL;
	}
	
	{	// Generate Shadow Texture List Array
		_float2 ShadowMapResolution = CGameInstance::Get().GetClientScreenSize();

		uint32_t ScreenSizeX = ETOUI(ShadowMapResolution.x);
		uint32_t ScreenSizeY = ETOUI(ShadowMapResolution.y);
		uint32_t ShadowSize  = 1024;
		uint32_t CubeMapSize = 1024;

		m_pDirectionalShadowViewPort = CGameInstance::Get().Generate_ViewPort("VP_ShadowMap_Directional", ShadowSize, ShadowSize);
		m_pPointShadowViewPort = CGameInstance::Get().Generate_ViewPort("VP_ShadowMap_Point", CubeMapSize, CubeMapSize);

		if (FAILED(Generate_ShadowArray2D(m_pStaticDirectionalShadowList, ShadowSize, ShadowSize)))		return E_FAIL;
		if (FAILED(Generate_ShadowArray2D(m_pDynamicDirectionalShadowList, ShadowSize, ShadowSize)))	return E_FAIL;

		if (FAILED(Generate_ShadowArrayCube(m_pStaticPointShadowList, CubeMapSize, CubeMapSize)))		return E_FAIL;
		if (FAILED(Generate_ShadowArrayCube(m_pDynamicPointShadowList, CubeMapSize, CubeMapSize)))		return E_FAIL;
	}

#ifdef _DEBUG
	if (FAILED(Initialize_DebugRender()))	return E_FAIL;
#endif

	return S_OK;
}

VOID CLightManager::UpdateGUI() {
	{
		ImGui::Begin("Light Manager");

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(180.f / 255.f, 135.f / 255.f, 255.f / 255.f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(180.f / 255.f * 1.2f, 135.f / 255.f * 1.2f, 255.f / 255.f * 1.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(180.f / 255.f / 2.f, 135.f / 255.f / 2.f, 255.f / 255.f / 2.f, 1.0f));

		if (ImGui::Button("Generate Light", ImVec2(-FLT_MIN, 20))) {
			Add_PointLight({ 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }, 10.f, 10.f);
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
		float innerAttn = pSelectedLight->Get_LightInnerAttenuation();
		float outerAttn = pSelectedLight->Get_LightOuterAttenuation();

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

		if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f, "%.2f"))
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
			if (ImGui::DragFloat3("Position", &position.x, 0.1f, -100.0f, 100.0f, "%.2f"))
			{
				pSelectedLight->Set_LightPosition(position);
			}
			// 범위 조절
			if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 1000.0f, "%.2f"))
			{
				pSelectedLight->Set_LightRange(range);
			}
		}

		if (lightType == LIGHT_TYPE::SPOTLIGHT) {
			if (ImGui::SliderFloat("Inner Attenuation", &innerAttn, 0.0f, 75.0f, "%.1f도") && innerAttn < outerAttn)
			{
				pSelectedLight->Set_LightInnerAttenuation(innerAttn);
			}
			if (ImGui::SliderFloat("Outer Attenuation", &outerAttn, 0.0f, 75.0f, "%.1f도"))
			{
				pSelectedLight->Set_LightOuterAttenuation(outerAttn);
			}
		}
		ImGui::Separator();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 0.4f, 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.05f, 0.05f, 1.0f));

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

VOID CLightManager::Update(_float fTimeDelta) {
	if (CGameInstance::Get().KeyDown(DIK_P))
		Allocate_EffectLight({ 10.f, 3.f, 10.f }, 30.f, { 1.f, 0.f, 0.f }, 10.f, 1.f, {1.f, 0.f, 0.f});
}

HRESULT CLightManager::Capture_ShadowMap() {
	ZoneScopedN("Capture_ShadowMap");
	{
		SPtr<CResDepthStencilState> DepthWriteState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE");
		m_pContext->OMSetDepthStencilState(DepthWriteState->GetDepthStencilState().Get(), 0);
		
		m_pContext->OMSetRenderTargets(0, nullptr, nullptr);

		auto Rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, "RS_MULTIPLE_SHADOW");
		m_pContext->RSSetState(Rasterizer->GetRasterizerState().Get());
	}

	Update_ActiveLights();
	Allocate_ShadowSlot();
	Update_LightData();

	ID3D11Buffer* LightCB = m_pLightConstantBuffer->GetCBuffer().Get();

	for (uint32_t i = 0; i < m_pActiveShadowLightList.size(); ++i) {
		if (!m_pActiveShadowLightList[i]) continue;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(m_pActiveShadowLightList[i].value());
		if (nullptr == LightOBJ) continue;

		const auto ShadowSlot = LightOBJ->Get_ShadowSlotNumb();
		if (ShadowSlot == -1)	 continue;

		if (LightOBJ->Get_LightType() == LIGHT_TYPE::POINT) {
			m_pContext->IASetInputLayout(m_pPointLightVS->GetInputLayout().Get());
			m_pContext->VSSetShader(m_pPointLightVS->GetVertexShader().Get(), nullptr, 0);
			m_pContext->GSSetShader(m_pPointLightGS->GetGeometryShader().Get(), nullptr, 0);
			m_pContext->PSSetShader(m_pPointLightPS->GetPixelShader().Get(), nullptr, 0);

			m_pContext->RSSetViewports(1, &m_pPointShadowViewPort->GetViewPort());

			D3D11_MAPPED_SUBRESOURCE MRES = {};
			if (SUCCEEDED(m_pContext->Map(LightCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
			{
				m_pLightConstantVariable.CurrentShadowLightIndex = i;

				memcpy(MRES.pData, &m_pLightConstantVariable, sizeof(CB_LIGHT));
				m_pContext->Unmap(LightCB, 0);
			}
			m_pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, &LightCB);
			m_pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, &LightCB); 
			m_pContext->GSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, &LightCB);

			LightOBJ->Update_ObjectConstantBuffer(m_pContext.Get());

			if (LightOBJ->Is_StaticDirty()) {
				auto StaticDIRDSV = m_pStaticPointShadowList.DSVList[ShadowSlot];
				if (StaticDIRDSV) {
					m_pContext->ClearDepthStencilView(StaticDIRDSV.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
					m_pContext->OMSetRenderTargets(0, nullptr, StaticDIRDSV.Get());

					LightOBJ->Capture_ShadowMap(m_pContext.Get(), m_pRenderable_StaticObjectList);
				}
				LightOBJ->Set_StaticDirty(false);
			}

			auto DynamicDIRDSV = m_pDynamicPointShadowList.DSVList[ShadowSlot];
			if (DynamicDIRDSV) {
				m_pContext->ClearDepthStencilView(DynamicDIRDSV.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
				m_pContext->OMSetRenderTargets(0, nullptr, DynamicDIRDSV.Get());

				LightOBJ->Capture_ShadowMap(m_pContext.Get(), m_pRenderable_DynamicObjectList);
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
			if (SUCCEEDED(m_pContext->Map(m_pLightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
			{
				m_pLightConstantVariable.CurrentShadowLightIndex = i;

				memcpy(MRES.pData, &m_pLightConstantVariable, sizeof(CB_LIGHT));
				m_pContext->Unmap(m_pLightConstantBuffer->GetCBuffer().Get(), 0);
			}
			m_pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
			m_pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());

			LightOBJ->Update_ObjectConstantBuffer(m_pContext.Get());

			if (LightOBJ->Is_StaticDirty()) {
				auto StaticShadowDSV = m_pStaticDirectionalShadowList.DSVList[ShadowSlot];
				if (StaticShadowDSV) {
					m_pContext->ClearDepthStencilView(StaticShadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
					m_pContext->OMSetRenderTargets(0, nullptr, StaticShadowDSV.Get());

					LightOBJ->Capture_ShadowMap(m_pContext.Get(), m_pRenderable_StaticObjectList);
				}
				LightOBJ->Set_StaticDirty(false);
			}

			auto DynamicShadowDSV = m_pDynamicDirectionalShadowList.DSVList[ShadowSlot];
			if (DynamicShadowDSV) {
				m_pContext->ClearDepthStencilView(DynamicShadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
				m_pContext->OMSetRenderTargets(0, nullptr, DynamicShadowDSV.Get());

				LightOBJ->Capture_ShadowMap(m_pContext.Get(), m_pRenderable_DynamicObjectList);
			}
		}
	}
	m_pContext->OMSetRenderTargets(0, nullptr, nullptr);

	m_pContext->VSSetShader(nullptr, nullptr, 0);
	m_pContext->GSSetShader(nullptr, nullptr, 0);
	m_pContext->PSSetShader(nullptr, nullptr, 0);

	m_pRenderable_StaticObjectList.clear();
	m_pRenderable_DynamicObjectList.clear();

	auto Rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	m_pContext->RSSetState(Rasterizer->GetRasterizerState().Get());

	SPtr<CResDepthStencilState> DepthReadState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHREAD");
	m_pContext->OMSetDepthStencilState(DepthReadState->GetDepthStencilState().Get(), 0);

	return S_OK;
}

HRESULT CLightManager::Render_ObjectShadow() {
	ZoneScopedN("Render_ObjectShadow");
	{
		if (nullptr == m_pUAVComBinedOutput) return E_FAIL;

		ID3D11UnorderedAccessView* UAV[1] = { m_pUAVComBinedOutput->GetUAV().Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, UAV, nullptr);

		m_pContext->CSSetShader(m_pShadowComputeShader->GetComputeShader().Get(), nullptr, 0);

		m_pContext->RSSetViewports(1, &m_pDirectionalShadowViewPort->GetViewPort());
	}
	

	auto ActiveCamera = CGameInstance::Get().GetActiveCamera();
	if (nullptr == ActiveCamera) return E_FAIL;

	XMMATRIX InvViewProj = XMMatrixMultiply(XMMatrixInverse(nullptr, ActiveCamera->GetView()), XMMatrixInverse(nullptr, ActiveCamera->GetProj()));

	uint32_t LightCount = 0;
	CB_LIGHT LightBuffer{};

	XMStoreFloat4x4(&LightBuffer.g_InvViewProj, InvViewProj);
	for (auto& LightHandle : m_pEffectLightPool) {					// Effect Light Binding
		if (LightCount >= MAX_LIGHT_COUNT) break;

		if (!LightHandle) continue;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ || LightOBJ->Get_LightActivateState() == false) continue;


		LightBuffer.AffectedLight[LightCount].LightType = ETOUI(LightOBJ->Get_LightType());

		LightBuffer.AffectedLight[LightCount].LightDirection = LightOBJ->Get_LightDirection();
		LightBuffer.AffectedLight[LightCount].LightColor = LightOBJ->Get_LightColor();
		LightBuffer.AffectedLight[LightCount].LightIntensity = LightOBJ->Get_LightIntensity();
		LightBuffer.AffectedLight[LightCount].LightRange = LightOBJ->Get_LightRange();
		LightBuffer.AffectedLight[LightCount].Position = LightOBJ->Get_LightPosition();

		LightBuffer.AffectedLight[LightCount].ShadowSlot = -1;

		LightCount++;
	}
	for (auto&	 LightHandle : m_pActiveShadowLightList) {				// Normal Light Binding
		if (LightCount >= MAX_LIGHT_COUNT) break;
		if (!LightHandle) continue;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ) continue;

		const LIGHT_TYPE LightType = LightOBJ->Get_LightType();
		const bool bIsPointLight = (LightType == LIGHT_TYPE::POINT);

		LightBuffer.AffectedLight[LightCount].LightType = ETOUI(LightType);

		uint32_t LightMapCount = bIsPointLight ? MAX_LIGHT_MAPCOUNT : 1;
		for (int Face = 0; Face < LightMapCount; ++Face)
			LightBuffer.AffectedLight[LightCount].g_LightViewProj[Face] = LightOBJ->Get_LightViewProj(Face);

		LightBuffer.AffectedLight[LightCount].LightDirection	= LightOBJ->Get_LightDirection();
		LightBuffer.AffectedLight[LightCount].LightColor		= LightOBJ->Get_LightColor();
		LightBuffer.AffectedLight[LightCount].LightIntensity	= LightOBJ->Get_LightIntensity();
		LightBuffer.AffectedLight[LightCount].LightRange		= LightOBJ->Get_LightRange();
		LightBuffer.AffectedLight[LightCount].Position			= LightOBJ->Get_LightPosition();

		LightBuffer.AffectedLight[LightCount].InnerAttanuation	= cosf(XMConvertToRadians(LightOBJ->Get_LightInnerAttenuation()));
		LightBuffer.AffectedLight[LightCount].OuterAttanuation	= cosf(XMConvertToRadians(LightOBJ->Get_LightOuterAttenuation()));

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
	m_pContext->CSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
	m_pContext->GSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());

	Bind_ShadowResource();

	_float2 ShadowMapResolution = CGameInstance::Get().GetClientScreenSize();
	
	m_pContext->Dispatch((ETOUI(ShadowMapResolution.x) + 15) / 16, (ETOUI(ShadowMapResolution.y) + 15) / 16, 1);

	ID3D11ShaderResourceView* NullSRV[12] = { nullptr };
	m_pContext->CSSetShaderResources(0, 12, NullSRV);

	ID3D11UnorderedAccessView* NullUAV[1] = { nullptr };
	m_pContext->CSSetUnorderedAccessViews(0, 1, NullUAV, nullptr);

	UnBind_ShadowResource();

	return S_OK;
}

HRESULT CLightManager::Render_EffectLight() {
	ZoneScopedN("Render_EffectLight");
	CB_LIGHT LightBuffer{};
	uint32_t LightCount = 0;

	for (auto& LightHandle : m_pEffectLightPool) {
		if (LightCount >= MAX_LIGHT_COUNT) break;
		if (!LightHandle) continue;
		// Need Culling - Frustum & Distance
		auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ || LightOBJ->Get_LightActivateState() == false)	continue;

		// Distance Culling
		// Frustum Culling

		LightBuffer.AffectedLight[LightCount].LightColor = LightOBJ->Get_LightColor();
		LightBuffer.AffectedLight[LightCount].LightIntensity = LightOBJ->Get_LightIntensity();
		LightBuffer.AffectedLight[LightCount].LightRange = LightOBJ->Get_LightRange();
		LightBuffer.AffectedLight[LightCount].Position = LightOBJ->Get_LightPosition();

		LightCount++;
	}
	LightBuffer.LightCount = LightCount;

	D3D11_MAPPED_SUBRESOURCE MRES{};
	if (SUCCEEDED(m_pContext->Map(m_pLightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		memcpy(MRES.pData, &LightBuffer, sizeof(CB_LIGHT));
		m_pContext->Unmap(m_pLightConstantBuffer->GetCBuffer().Get(), 0);
	}
	m_pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());

	return S_OK;
}

VOID CLightManager::Bind_DynamicLight() {
	// 해당 함수(Bind_SceneLight)는 모델의 PBR 픽셀쉐이더를 Draw를 하기전에 CB_LIGHT_BUFFER를 채워주기 위한 용도. 그리기 연산은 수행하지 않음.

	CB_LIGHT LightBuffer{};
	uint32_t LightCount = 0;

	for (auto& LightHandle : m_LightHandleList) {
		if (LightCount >= MAX_LIGHT_COUNT) break;

		// Need Culling - Frustum & Distance
		auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ)	continue;

		// Distance Culling
		// Frustum Culling

		LightBuffer.AffectedLight[LightCount].LightType			= ETOUI(LightOBJ->Get_LightType());
		LightBuffer.AffectedLight[LightCount].LightDirection	= LightOBJ->Get_LightDirection();
		LightBuffer.AffectedLight[LightCount].LightColor		= LightOBJ->Get_LightColor();
		LightBuffer.AffectedLight[LightCount].LightIntensity	= LightOBJ->Get_LightIntensity();
		LightBuffer.AffectedLight[LightCount].LightRange		= LightOBJ->Get_LightRange();
		LightBuffer.AffectedLight[LightCount].Position			= LightOBJ->Get_LightPosition();
		LightBuffer.AffectedLight[LightCount].InnerAttanuation	= cosf(XMConvertToRadians(LightOBJ->Get_LightInnerAttenuation()));
		LightBuffer.AffectedLight[LightCount].OuterAttanuation	= cosf(XMConvertToRadians(LightOBJ->Get_LightOuterAttenuation()));

		LightCount++;
	}
	LightBuffer.LightCount = LightCount;

	D3D11_MAPPED_SUBRESOURCE MRES{};
	if (SUCCEEDED(m_pContext->Map(m_pLightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		memcpy(MRES.pData, &LightBuffer, sizeof(CB_LIGHT));
		m_pContext->Unmap(m_pLightConstantBuffer->GetCBuffer().Get(), 0);
	}

	m_pContext->GSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
	m_pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
}

std::optional<CHandle> CLightManager::Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity) {
	CLight::DESC LDesc{};
	if		(m_LightHandleList.size() < 10)     LDesc.sObjectTag = "Light_Clone00"	+ m_LightHandleList.size();
	else if (m_LightHandleList.size() < 100)    LDesc.sObjectTag = "Light_Clone0"	+ m_LightHandleList.size();
	else if (m_LightHandleList.size() < 1000)   LDesc.sObjectTag = "Light_Clone"	+ m_LightHandleList.size();

	auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
	if (!(LightHandle))	return std::nullopt;

	auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());

	LightOBJ->Change_LightType(LIGHT_TYPE::DIRECTIONAL);
	LightOBJ->Set_LightDirection(_Direction);
	LightOBJ->Set_LightColor(_Color);
	LightOBJ->Set_LightIntensity(_Intensity);

	m_LightHandleList.push_back(LightHandle.value());
	return LightHandle;
}
std::optional<CHandle> CLightManager::Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range) {
	CLight::DESC LDesc{};
	if		(m_LightHandleList.size() < 10)     LDesc.sObjectTag = "Light_Clone00"	+ m_LightHandleList.size();
	else if (m_LightHandleList.size() < 100)    LDesc.sObjectTag = "Light_Clone0"	+ m_LightHandleList.size();
	else if (m_LightHandleList.size() < 1000)   LDesc.sObjectTag = "Light_Clone"	+ m_LightHandleList.size();

	auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
	if (!LightHandle)			return std::nullopt;

	auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
	if (nullptr == LightOBJ)	return std::nullopt;

	LightOBJ->Change_LightType(LIGHT_TYPE::POINT);

	LightOBJ->Set_LightPosition(_Position);
	LightOBJ->Set_LightColor(_Color);
	LightOBJ->Set_LightIntensity(_Intensity);
	LightOBJ->Set_LightRange(_Range);

	m_LightHandleList.push_back(LightHandle.value());
	return LightHandle;
}
std::optional<CHandle> CLightManager::Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt) {
	CLight::DESC LDesc{};
	if		(m_LightHandleList.size() < 10)		LDesc.sObjectTag = "Light_Clone00"	+ m_LightHandleList.size();
	else if (m_LightHandleList.size() < 100)    LDesc.sObjectTag = "Light_Clone0"	+ m_LightHandleList.size();
	else if (m_LightHandleList.size() < 1000)   LDesc.sObjectTag = "Light_Clone"	+ m_LightHandleList.size();

	auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
	if (!LightHandle)	return std::nullopt;

	auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
	LightOBJ->Change_LightType(LIGHT_TYPE::SPOTLIGHT);

	LightOBJ->Set_LightPosition(_Position);
	LightOBJ->Set_LightColor(_Color);
	LightOBJ->Set_LightIntensity(_Intensity);
	LightOBJ->Set_LightRange(_Range);

	LightOBJ->Set_LightInnerAttenuation(_InnerAtt);
	LightOBJ->Set_LightOuterAttenuation(_OuterAtt);

	m_LightHandleList.push_back(LightHandle);
	return LightHandle;
}

HRESULT CLightManager::Add_ShadowRenderGroup(ACTORTYPE _ATYPE, CGameObject* pRenderObject) {
	if (nullptr == pRenderObject) return E_FAIL;

	_ATYPE == ACTORTYPE::DYNAMIC ? m_pRenderable_DynamicObjectList.push_back(pRenderObject) : m_pRenderable_StaticObjectList.push_back(pRenderObject);

	return S_OK;
}

VOID CLightManager::Bind_ShadowResource() {
	ID3D11ShaderResourceView* StaticDirectionalMapList[MAX_LIGHT_MAPCOUNT]  = { nullptr };
	ID3D11ShaderResourceView* DynamicDirectionalMapList[MAX_LIGHT_MAPCOUNT] = { nullptr };

	ID3D11ShaderResourceView* StaticPointLightMapList[MAX_LIGHT_MAPCOUNT]	= { nullptr };
	ID3D11ShaderResourceView* DynamicPointLightMapList[MAX_LIGHT_MAPCOUNT]	= { nullptr };

	uint32_t NormalLightCount = 0, PointLightCount = 0;

	for (uint32_t i = 0; i < m_pActiveShadowLightList.size(); ++i) {
		if (!m_pActiveShadowLightList[i]) continue;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(m_pActiveShadowLightList[i].value());
		if (nullptr == LightOBJ) continue;

		if (LightOBJ->Get_LightType() != LIGHT_TYPE::POINT) {
			if (NormalLightCount >= MAX_LIGHT_MAPCOUNT)	continue;

			auto StaticSRV	= m_pStaticDirectionalShadowList.SRV;
			auto DynamicSRV = m_pDynamicDirectionalShadowList.SRV;

			StaticDirectionalMapList[NormalLightCount]	= StaticSRV  ? StaticSRV.Get()  : nullptr;
			DynamicDirectionalMapList[NormalLightCount] = DynamicSRV ? DynamicSRV.Get() : nullptr;

			NormalLightCount++;
		}
		else {
			if (PointLightCount >= MAX_LIGHT_MAPCOUNT)	continue;

			auto StaticSRV	= m_pStaticPointShadowList.SRV;
			auto DynamicSRV = m_pDynamicPointShadowList.SRV;

			StaticPointLightMapList[PointLightCount]  = StaticSRV  ? StaticSRV.Get()  : nullptr;
			DynamicPointLightMapList[PointLightCount] = DynamicSRV ? DynamicSRV.Get() : nullptr;

			PointLightCount++;
		}
	}
	ID3D11ShaderResourceView* ShadowSRV[] = {
		m_pStaticDirectionalShadowList.SRV.Get(),
		m_pDynamicDirectionalShadowList.SRV.Get(),

		m_pStaticPointShadowList.SRV.Get(),
		m_pDynamicPointShadowList.SRV.Get()
	};

	m_pContext->CSSetShaderResources(9, 4, ShadowSRV);
}

VOID CLightManager::UnBind_ShadowResource() {
	ID3D11ShaderResourceView* NullSRV[4] = { nullptr };
	m_pContext->CSSetShaderResources(9, 4, NullSRV);
}

VOID CLightManager::Update_ActiveLights() {
	// 현재 프레임에 보이는 라이트 수집
	m_pActiveShadowLightList.clear();
	std::vector<LightData> CullingLight{};
	auto Camera = CGameInstance::Get().GetActiveCamera();
	if (nullptr == Camera) return;

	XMVECTOR CameraPos = Camera->GetTransform().GetLoadedPostion();

	for (auto& LightHandle : m_LightHandleList) {
		auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ)	continue;
		
		if (LightOBJ->Get_LightType() == LIGHT_TYPE::DIRECTIONAL) {
			CullingLight.push_back({ LightOBJ, 0.f });
			continue;
		}

		XMVECTOR CurrentPosition = LightOBJ->GetTransform().GetLoadedPostion();
		_float	 Distance = XMVectorGetX(XMVector3Length(XMVectorSubtract(CameraPos, CurrentPosition)));
		
		if (Distance <= 100.f)// && IsInFrustum(LightOBJ))
			CullingLight.push_back({ LightOBJ, Distance });
	}

	// 거리 기반 컬링 + 정렬(최단거리 순)
	std::sort(CullingLight.begin(), CullingLight.end(), [](const LightData& SRC, const LightData& DST) {
		return SRC.Distance < DST.Distance;
	});

	// 최대 MAX_LIGHT_COUNT 수만큼의 조명만 렌더링
	_float FinalActiveLightCount = std::min(MAX_LIGHT_COUNT, static_cast<int>(CullingLight.size()));
	for (uint32_t i = 0; i < FinalActiveLightCount; ++i) {
		m_pActiveShadowLightList.push_back(CullingLight[i].LightOBJ->GetHandle());
	}
}

_bool CLightManager::IsInFrustum(CLight* _LightOBJ) {
	auto ActiveCam = CGameInstance::Get().GetActiveCamera();
	if (nullptr == ActiveCam)			return false;

	auto ActiveCamCollider = ActiveCam->GetCollider().Get();
	if (nullptr == ActiveCamCollider)	return false;

	auto CameraFrustum = static_cast<CCollFrustum*>(ActiveCamCollider);

	auto LightCollider = (_LightOBJ->Get_LightType() == LIGHT_TYPE::POINT ? _LightOBJ->Get_SphereCollider() : _LightOBJ->Get_FrustumCollider());

	if (nullptr == LightCollider)		return false;

	return CameraFrustum->Intersect(*LightCollider.get());
}

HRESULT CLightManager::Generate_ShadowArray2D(SHADOW_ARRAY_2D& _SHAR, uint32_t _ResolutionX, uint32_t _ResolutionY) {
	D3D11_TEXTURE2D_DESC TEXDesc = {};
	TEXDesc.Width = _ResolutionX;
	TEXDesc.Height = _ResolutionY;
	TEXDesc.MipLevels = 1;
	TEXDesc.ArraySize = MAX_LIGHT_MAPCOUNT;
	TEXDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	TEXDesc.SampleDesc = { .Count = 1, .Quality = 0 };
	TEXDesc.Usage = D3D11_USAGE_DEFAULT;
	TEXDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(m_pDevice->CreateTexture2D(&TEXDesc, nullptr, _SHAR.TexBuffer.GetAddressOf()))) return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	SRVDesc.Texture2DArray.MostDetailedMip = 0;
	SRVDesc.Texture2DArray.MipLevels = 1;
	SRVDesc.Texture2DArray.FirstArraySlice = 0;
	SRVDesc.Texture2DArray.ArraySize = MAX_LIGHT_MAPCOUNT;
	
	if (FAILED(m_pDevice->CreateShaderResourceView(_SHAR.TexBuffer.Get(), &SRVDesc, _SHAR.SRV.GetAddressOf()))) return E_FAIL;
	
	for (uint32_t i = 0; i < MAX_LIGHT_MAPCOUNT; ++i) {
		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		DSVDesc.Texture2DArray.ArraySize = 1;
		DSVDesc.Texture2DArray.FirstArraySlice = i;
		DSVDesc.Texture2DArray.MipSlice = 0;

		if (FAILED(m_pDevice->CreateDepthStencilView(_SHAR.TexBuffer.Get(), &DSVDesc, _SHAR.DSVList[i].GetAddressOf()))) return E_FAIL;
	}
	
	return S_OK;
}
HRESULT CLightManager::Generate_ShadowArrayCube(SHADOW_ARRAY_CUBE& _SHAR, uint32_t _ResolutionX, uint32_t _ResolutionY) {
	uint32_t SurfaceCount = 6;
	
	D3D11_TEXTURE2D_DESC TEXDesc = {};
	TEXDesc.Width = _ResolutionX;
	TEXDesc.Height = _ResolutionY;
	TEXDesc.MipLevels = 1;
	TEXDesc.ArraySize = MAX_LIGHT_MAPCOUNT * SurfaceCount;
	TEXDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	TEXDesc.SampleDesc = { .Count = 1, .Quality = 0 };
	TEXDesc.Usage = D3D11_USAGE_DEFAULT;
	TEXDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	TEXDesc.CPUAccessFlags = 0;
	TEXDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	if (FAILED(m_pDevice->CreateTexture2D(&TEXDesc, nullptr, _SHAR.TexBuffer.GetAddressOf()))) return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
	SRVDesc.TextureCubeArray.MostDetailedMip = 0;
	SRVDesc.TextureCubeArray.MipLevels = 1;
	SRVDesc.TextureCubeArray.First2DArrayFace = 0;
	SRVDesc.TextureCubeArray.NumCubes = MAX_LIGHT_MAPCOUNT;

	if (FAILED(m_pDevice->CreateShaderResourceView(_SHAR.TexBuffer.Get(), &SRVDesc, _SHAR.SRV.GetAddressOf()))) return E_FAIL;

	for (uint32_t i = 0; i < MAX_LIGHT_MAPCOUNT; ++i) {
		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		DSVDesc.Texture2DArray.ArraySize = SurfaceCount;
		DSVDesc.Texture2DArray.FirstArraySlice = i * SurfaceCount;
		DSVDesc.Texture2DArray.MipSlice = 0;

		if (FAILED(m_pDevice->CreateDepthStencilView(_SHAR.TexBuffer.Get(), &DSVDesc, _SHAR.DSVList[i].GetAddressOf()))) return E_FAIL;
	}
	return S_OK;
}

HRESULT CLightManager::Initialize_EffectLight(uint32_t _PoolSize){
	m_pEffectLightPool.reserve(_PoolSize);

	for (uint32_t i = 0; i < _PoolSize; ++i) {
		CLight::DESC LDesc{};
		if		(m_pEffectLightPool.size() < 10)	LDesc.sObjectTag = "EffectLight00" + m_pEffectLightPool.size();
		else if (m_pEffectLightPool.size() < 100)   LDesc.sObjectTag = "EffectLight0" + m_pEffectLightPool.size();

		auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
		if (!LightHandle)			return E_FAIL;

		auto LightOBJ	 = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ)	return E_FAIL;

		LightOBJ->Change_LightType(LIGHT_TYPE::POINT);

		LightOBJ->Set_LightPosition({ 0.f, 0.f , 0.f });
		LightOBJ->Set_LightColor({ 1.f, 1.f, 1.f });
		LightOBJ->Set_LightIntensity(10.f);
		LightOBJ->Set_LightRange(10.f);
		LightOBJ->Set_EffectLight(true);

		m_pEffectLightPool.push_back(LightHandle);
	}

	return S_OK;
}

std::optional<CHandle> CLightManager::Allocate_EffectLight(XMVECTOR _WorldPos, _float _Intensity, _float3 _Color, _float _Range, _float _LifeTime, _float3 _Velocity){
	std::optional<CHandle> FinalLightHandle = std::nullopt;
	CLight* LightOBJ = nullptr;

	for (uint32_t i = 0; i < MAX_EFFECTLIGHT_COUNT; ++i) {
		uint32_t CircularIndex = (m_iLastAllocatedIndex + 1 + i) % MAX_EFFECTLIGHT_COUNT;
		auto LightHandle = m_pEffectLightPool[CircularIndex];
		if (!LightHandle)		 continue;

		LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ) continue;

		if (LightOBJ->Get_LightActivateState() == false) {
			LightOBJ->Set_LightActivateState(true);
			FinalLightHandle = m_pEffectLightPool[CircularIndex];
			m_iLastAllocatedIndex = CircularIndex;
			break;
		}
	}
	if (nullptr == LightOBJ) return std::nullopt;

	XMFLOAT3 WorldPosition{};
	XMStoreFloat3(&WorldPosition, _WorldPos);

	LightOBJ->Set_LightPosition(WorldPosition);
	LightOBJ->Set_LightIntensity(_Intensity);
	LightOBJ->Set_LightColor(_Color);
	LightOBJ->Set_LightRange(_Range);
	LightOBJ->Set_LightLifeTime(_LifeTime);
	LightOBJ->Set_LightVelocity(_Velocity);	

	return FinalLightHandle;
}

VOID CLightManager::Update_LightData() {
	m_pLightConstantVariable.LightCount = m_pActiveShadowLightList.size();
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

			const LIGHT_TYPE LightType = LightOBJ->Get_LightType();
			
			uint32_t LightMapCount = LightType == LIGHT_TYPE::POINT ? MAX_LIGHT_MAPCOUNT : 1;
			for (int Face = 0; Face < LightMapCount; ++Face) {
				m_pLightConstantVariable.AffectedLight[i].g_LightViewProj[Face] = LightOBJ->Get_LightViewProj(Face);
			}

			m_pLightConstantVariable.AffectedLight[i].LightType				= ETOUI(LightType);
			m_pLightConstantVariable.AffectedLight[i].LightDirection		= LightOBJ->Get_LightDirection();
			m_pLightConstantVariable.AffectedLight[i].LightColor			= LightOBJ->Get_LightColor();
			m_pLightConstantVariable.AffectedLight[i].LightIntensity		= LightOBJ->Get_LightIntensity();
			m_pLightConstantVariable.AffectedLight[i].LightRange			= LightOBJ->Get_LightRange();
			m_pLightConstantVariable.AffectedLight[i].Position				= LightOBJ->Get_LightPosition();

			m_pLightConstantVariable.AffectedLight[i].InnerAttanuation		= cosf(XMConvertToRadians(LightOBJ->Get_LightInnerAttenuation()));
			m_pLightConstantVariable.AffectedLight[i].OuterAttanuation		= cosf(XMConvertToRadians(LightOBJ->Get_LightOuterAttenuation()));

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
		m_pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
		m_pContext->GSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
		m_pContext->CSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
		m_pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::LIGHT), 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
	}
}

VOID CLightManager::Allocate_ShadowSlot(){
	uint32_t PLCount = 0, DLCount = 0;

	for (auto& LightHandle : m_pActiveShadowLightList) {
		if (!LightHandle) continue;

		auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ) continue;

		LightOBJ->Set_ShadowSlotNumb(-1);

		if (LightOBJ->Get_LightType() == LIGHT_TYPE::POINT) {
			if (PLCount < MAX_LIGHT_COUNT) LightOBJ->Set_ShadowSlotNumb(PLCount++);
		}
		else {
			if (DLCount < MAX_LIGHT_COUNT) LightOBJ->Set_ShadowSlotNumb(DLCount++);
		}
	}
}

#ifdef _DEBUG
HRESULT CLightManager::Initialize_DebugRender() {
	if (auto res = CGameInstance::Get().AddResource("LIGHT", "TEX2D_Icon_DirectionalLight", CResTexture2D::Create("./Resources/Engine/Texture/Debugging/Icon_DirectionalLight.png"))) {
		if (FAILED(res->Load())) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource("LIGHT", "TEX2D_Icon_PointLight", CResTexture2D::Create("./Resources/Engine/Texture/Debugging/Icon_PointLight.png"))) {
		if (FAILED(res->Load())) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource("LIGHT", "TEX2D_Icon_SpotLight", CResTexture2D::Create("./Resources/Engine/Texture/Debugging/Icon_SpotLight.png"))) {
		if (FAILED(res->Load())) return E_FAIL;
	}

	if (m_pResDebugVertexShader = CGameInstance::Get().AddResourceT<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTex_ICON", "./ShaderFiles/QuadTex/QuadTex.hlsl"))
	{
		if (FAILED(m_pResDebugVertexShader->Load(CResShader::DESC{ .sEntryPoint = "VSMain_BillBoard", .sTarget = "vs_5_0" })))			return E_FAIL;
	}

	m_pResDebugPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex");
	if (nullptr == m_pResDebugPixelShader)		return E_FAIL;

	m_pResLightTexBuffer = CGameInstance::Get().GetResourceFirst<CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
	if (nullptr == m_pResLightTexBuffer)	return E_FAIL;

	m_pResDirectionalLightTexture2D = CGameInstance::Get().GetResourceFirst<CResTexture2D>("LIGHT", "TEX2D_Icon_DirectionalLight");
	if (nullptr == m_pResDirectionalLightTexture2D)	return E_FAIL;

	m_pResPointLightTexture2D = CGameInstance::Get().GetResourceFirst<CResTexture2D>("LIGHT", "TEX2D_Icon_PointLight");
	if (nullptr == m_pResPointLightTexture2D)	return E_FAIL;

	m_pResSpotLightTexture2D = CGameInstance::Get().GetResourceFirst<CResTexture2D>("LIGHT", "TEX2D_Icon_SpotLight");
	if (nullptr == m_pResSpotLightTexture2D)	return E_FAIL;

	return S_OK;
}
HRESULT CLightManager::Render_DebugIcon() {

	m_pContext->IASetInputLayout(m_pResDebugVertexShader->GetInputLayout().Get());
	m_pContext->VSSetShader(m_pResDebugVertexShader->GetVertexShader().Get(), nullptr, 0);
	m_pContext->PSSetShader(m_pResDebugPixelShader->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffers[] = { m_pResLightTexBuffer->GetVertexBuffer().Get() };
	uint32_t strides[] = { m_pResLightTexBuffer->GetVertexStride() };
	uint32_t offsets[] = { 0 };

	m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	m_pContext->IASetIndexBuffer(m_pResLightTexBuffer->GetIndexBuffer().Get(), m_pResLightTexBuffer->GetIndexFormat(), 0);
	m_pContext->IASetPrimitiveTopology(m_pResLightTexBuffer->GetPrimitiveType());

	for (auto& LightHandle : m_LightHandleList) {
		auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
		if (nullptr == LightOBJ)			continue;

		LIGHT_TYPE LightType = LightOBJ->Get_LightType();

		{   // DEBUG : Light Position Icon
			if (LightType == LIGHT_TYPE::DIRECTIONAL) {
				m_pContext->PSSetShaderResources(0, 1, m_pResDirectionalLightTexture2D->GetSRV().GetAddressOf());
			}
			else if (LightType == LIGHT_TYPE::POINT) {
				m_pContext->PSSetShaderResources(0, 1, m_pResPointLightTexture2D->GetSRV().GetAddressOf());
			}
			else if (LightType == LIGHT_TYPE::SPOTLIGHT) {
				m_pContext->PSSetShaderResources(0, 1, m_pResSpotLightTexture2D->GetSRV().GetAddressOf());
			}
		}
		m_pContext->DrawIndexed(m_pResLightTexBuffer->GetNumIndices(), 0, 0);
	}
	m_pContext->PSSetShader(nullptr, nullptr, 0);

	ID3D11ShaderResourceView* pNullSRVs[1] = { nullptr };
	m_pContext->PSSetShaderResources(0, 1, pNullSRVs);

	return S_OK;
}
#endif

UPtr<CLightManager> CLightManager::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) {
	auto pInstance = ToUPtr(new CLightManager{ pDevice, pContext });
	if (FAILED(pInstance->Initialize_LightManager())) {
		MSG_BOX("Failed to Created : CLightManager");
		return nullptr;
	}
	return pInstance;
}
