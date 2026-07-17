#include "pch.h"
#include "LightManager.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "CollSphere.h"
#include "CollFrustum.h"

CLightManager::CLightManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) : m_pDevice(pDevice), m_pContext(pContext) {}
CLightManager::~CLightManager() { }

HRESULT CLightManager::Initialize_LightManager() {

	if (m_pLightConstantBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Light", E::CResCBuffer::Create()))
	{
		if (FAILED(m_pLightConstantBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_LIGHT) })))    return E_FAIL;
	}
	m_pQuadBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
	if (nullptr == m_pQuadBuffer)			return E_FAIL;		

	m_pResLightTexBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
	if (nullptr == m_pResLightTexBuffer)	return E_FAIL;

	m_pPBRComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PBR");
	if (nullptr == m_pPBRComputeShader)		return E_FAIL;

	// 2K Resolution
	uint32_t ShadowMapResolutionX = { 1280 * 2 };
	uint32_t ShadowMapResolutionY = { 720 * 2 };

	m_pUAVComBinedOutput = CGameInstance::Get().Generate_UnorderedAccessView("ComBinedTex", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);

	m_pShadowViewPort = CGameInstance::Get().Generate_ViewPort("VP_ShadowMap", ShadowMapResolutionX, ShadowMapResolutionY);

	if (m_pPointLightVS = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Shadow_Point", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointLightVS->Load()))    return E_FAIL;
	}
	if (m_pDirectionalLightVS = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Shadow_Direct", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pDirectionalLightVS->Load(CResShader::DESC{ .sEntryPoint = "VSMain_Final", .sTarget = "vs_5_0" })))    return E_FAIL;
	}
	if (m_pPointLightPS = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Shadow", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointLightPS->Load()))    return E_FAIL;
	}
	if (m_pPointLightGS = CGameInstance::Get().AddResourceT<E::CResGeometryShader>(TAG_RES_GRP_PERMANENT_SHADER, "GS_Shadow", "./ShaderFiles/RayMarching/US_Shadow.hlsl"))
	{
		if (FAILED(m_pPointLightGS->Load()))    return E_FAIL;
	}

	StaticShadowMapList.resize(MAX_LIGHT_MAPCOUNT, nullptr);
	DynamicShadowMapList.resize(MAX_LIGHT_MAPCOUNT, nullptr);
	NullList.resize(MAX_LIGHT_MAPCOUNT, nullptr);

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
				auto LightObject = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(*iter);
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
		auto pSelectedLight = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(m_LightHandleList[selectedLightIdx]);
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
			if (ImGui::SliderFloat("Inner Attenuation", &innerAttn, 0.0f, 90.0f, "%.1f도") && innerAttn < outerAttn)
			{
				pSelectedLight->Set_LightInnerAttenuation(innerAttn);
			}
			if (ImGui::SliderFloat("Outer Attenuation", &outerAttn, 0.0f, 90.0f, "%.1f도"))
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

}

HRESULT CLightManager::Capture_ShadowMap() {
	ZoneScopedN("Capture_ShadowMap");

	SPtr<CResDepthStencilState> DepthWriteState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE");
	m_pContext->OMSetDepthStencilState(DepthWriteState->GetDepthStencilState().Get(), 0);

	ID3D11RenderTargetView* NullRTV = { nullptr };
	m_pContext->OMSetRenderTargets(1, &NullRTV, nullptr);

	Update_ActiveLights();

	for (uint32_t i = 0; i < m_pActiveShadowLightList.size(); ++i) {
		auto LightOBJ = m_pActiveShadowLightList[i];

		const LIGHT_TYPE LightType = LightOBJ->Get_LightType();
		const bool bIsPointLight = (LightType == LIGHT_TYPE::POINT);

		if (bIsPointLight) {
			m_pContext->IASetInputLayout(m_pPointLightVS->GetInputLayout().Get());
			m_pContext->VSSetShader(m_pPointLightVS->GetVertexShader().Get(), nullptr, 0);
			m_pContext->GSSetShader(m_pPointLightGS->GetGeometryShader().Get(), nullptr, 0);
			m_pContext->PSSetShader(m_pPointLightPS->GetPixelShader().Get(), nullptr, 0);
		}
		else {
			m_pContext->IASetInputLayout(m_pDirectionalLightVS->GetInputLayout().Get());
			m_pContext->VSSetShader(m_pDirectionalLightVS->GetVertexShader().Get(), nullptr, 0);
			m_pContext->GSSetShader(nullptr, nullptr, 0);
			m_pContext->PSSetShader(nullptr, nullptr, 0);
		}

		m_pContext->RSSetViewports(1, &m_pShadowViewPort->GetViewPort());

		D3D11_MAPPED_SUBRESOURCE MRES = {};
		if (SUCCEEDED(m_pContext->Map(m_pLightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
		{
			CB_LIGHT CBLight{};
			if (bIsPointLight) {
				for (int Face = 0; Face < 6; ++Face) {
					CBLight.AffectedLight[i].g_LightViewProj[Face] = LightOBJ->Get_LightViewProj(Face);
				}
			}
			else {
				CBLight.AffectedLight[i].g_LightViewProj[0] = LightOBJ->Get_LightViewProj();
			}

			CBLight.AffectedLight[i].LightType = ETOUI(LightType);
			CBLight.LightCount = m_pActiveShadowLightList.size();
			CBLight.CurrentLightIndex = i;

			memcpy(MRES.pData, &CBLight, sizeof(CB_LIGHT));
			m_pContext->Unmap(m_pLightConstantBuffer->GetCBuffer().Get(), 0);
		}

		m_pContext->VSSetConstantBuffers(4, 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
		m_pContext->CSSetConstantBuffers(4, 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());
		m_pContext->GSSetConstantBuffers(4, 1, m_pLightConstantBuffer->GetCBuffer().GetAddressOf());

		LightOBJ->Update_ObjectConstantBuffer(m_pContext.Get());

		if (LightOBJ->Is_StaticDirty()) {
			auto StaticShadowDSV = LightOBJ->Get_StaticShadowDSV();
			if (StaticShadowDSV) {
				m_pContext->ClearDepthStencilView(StaticShadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
				m_pContext->OMSetRenderTargets(1, &NullRTV, StaticShadowDSV.Get());

				LightOBJ->Capture_ShadowMap(m_pContext.Get(), &m_pRenderable_StaticObjectList, nullptr);
			}
			LightOBJ->Set_StaticDirty(false);
		}

		auto DynamicShadowDSV = LightOBJ->Get_DynamicShadowDSV();
		if (DynamicShadowDSV) {
			m_pContext->ClearDepthStencilView(DynamicShadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
			m_pContext->OMSetRenderTargets(1, &NullRTV, DynamicShadowDSV.Get());

			LightOBJ->Capture_ShadowMap(m_pContext.Get(), nullptr, &m_pRenderable_DynamicObjectList);
		}

		if (bIsPointLight) {
			m_pContext->GSSetShader(nullptr, nullptr, 0);
			m_pContext->PSSetShader(nullptr, nullptr, 0);
		}
	}
	m_pContext->OMSetRenderTargets(1, &NullRTV, nullptr);

	m_pContext->VSSetShader(nullptr, nullptr, 0);
	m_pContext->GSSetShader(nullptr, nullptr, 0);
	m_pContext->PSSetShader(nullptr, nullptr, 0);

	m_pRenderable_StaticObjectList.clear();
	m_pRenderable_DynamicObjectList.clear();

	return S_OK;
}
HRESULT CLightManager::Render_ObjectShadow(const ComPtr<ID3D11ShaderResourceView>& _Diffuse, const ComPtr<ID3D11ShaderResourceView>& _Normal, const ComPtr<ID3D11ShaderResourceView>& _SMRO,
	const ComPtr<ID3D11ShaderResourceView>& _Emissive, const ComPtr<ID3D11ShaderResourceView> _Ambient, const ComPtr<ID3D11ShaderResourceView> _Depth) {
	ZoneScopedN("Render_ObjectShadow");

	m_pContext->CSSetShader(m_pPBRComputeShader->GetComputeShader().Get(), nullptr, 0);

	ID3D11UnorderedAccessView* pUAVs[1] = { m_pUAVComBinedOutput->GetUAV().Get() };
	m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);

	auto LightConstantBuffer = CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Light");

	auto ActiveCamera = CGameInstance::Get().GetActiveCamera();
	if (nullptr == ActiveCamera) return E_FAIL;

	XMMATRIX InvViewProj = XMMatrixMultiply(XMMatrixInverse(nullptr, ActiveCamera->GetView()), XMMatrixInverse(nullptr, ActiveCamera->GetProj()));

	ID3D11ShaderResourceView* pMainSRVs[6] = {
		_Diffuse.Get(), _Normal.Get(), _SMRO.Get(), _Emissive.Get(), _Ambient.Get(), _Depth.Get()
	};

	m_pContext->CSSetShaderResources(0, 6, pMainSRVs);

	ID3D11ShaderResourceView* pIBLSRVs[3] = {
		m_pIrridianceSRV.Get(),
		m_pPreFilterSRV.Get(),
		m_pLUTSRV.Get()
	};

	m_pContext->CSSetShaderResources(8, 3, pIBLSRVs);

	uint32_t LightCount = 0;
	CB_LIGHT LightBuffer{};

	XMStoreFloat4x4(&LightBuffer.g_InvViewProj, InvViewProj);

	for (auto&	 LightHandle : m_LightHandleList) {
		if (LightCount >= MAX_LIGHT_COUNT) break;

		auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle);
		if (nullptr == LightOBJ)			continue;

		LightBuffer.AffectedLight[LightCount].LightType = ETOUI(LightOBJ->Get_LightType());
		LightBuffer.AffectedLight[LightCount].g_LightViewProj[0] = LightOBJ->Get_LightViewProj();

		LightBuffer.AffectedLight[LightCount].LightDirection = LightOBJ->Get_LightDirection();
		LightBuffer.AffectedLight[LightCount].LightColor = LightOBJ->Get_LightColor();
		LightBuffer.AffectedLight[LightCount].LightIntensity = LightOBJ->Get_LightIntensity();
		LightBuffer.AffectedLight[LightCount].LightRange = LightOBJ->Get_LightRange();

		LightBuffer.AffectedLight[LightCount].Position = LightOBJ->Get_LightPosition();

		LightBuffer.AffectedLight[LightCount].InnerAttanuation = cosf(XMConvertToRadians(LightOBJ->Get_LightInnerAttenuation()));
		LightBuffer.AffectedLight[LightCount].OuterAttanuation = cosf(XMConvertToRadians(LightOBJ->Get_LightOuterAttenuation()));

		LightCount++;
	}
	LightBuffer.LightCount = LightCount;

	D3D11_MAPPED_SUBRESOURCE MRES;
	if (SUCCEEDED(m_pContext->Map(LightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		memcpy(MRES.pData, &LightBuffer, sizeof(CB_LIGHT));
		m_pContext->Unmap(LightConstantBuffer->GetCBuffer().Get(), 0);
	}

	Bind_ShadowResource();

	m_pContext->CSSetConstantBuffers(4, 1, LightConstantBuffer->GetCBuffer().GetAddressOf());

	uint32_t ScreenResolutionX = { 1280 };
	uint32_t ScreenResolutionY = { 720 };

	m_pContext->Dispatch((ScreenResolutionX + 15) / 16, (ScreenResolutionY + 15) / 16, 1);

	ID3D11ShaderResourceView* NullSRVs[11] = { nullptr };
	m_pContext->CSSetShaderResources(0, 11, NullSRVs);

	ID3D11UnorderedAccessView* NullUAV[1] = { nullptr };
	m_pContext->CSSetUnorderedAccessViews(0, 1, NullUAV, nullptr);

	UnBind_ShadowResource();

	return S_OK;
}

VOID CLightManager::Bind_EnviromentLight() {
	//m_pContext->PSSetShaderResources(4, 1, &m_IrridianceSRV);
	//m_pContext->PSSetShaderResources(5, 1, &m_PreFilterSRV);
	//m_pContext->PSSetShaderResources(6, 1, &m_LUTSRV);
}

VOID CLightManager::Bind_DynamicLight() {
	// 해당 함수(Bind_SceneLight)는 모델의 PBR 픽셀쉐이더를 Draw를 하기전에 CB_LIGHT_BUFFER를 채워주기 위한 용도. 그리기 연산은 수행하지 않음.

	CB_LIGHT LightBuffer{};
	uint32_t LightCount = 0;

	for (auto& LightHandle : m_LightHandleList) {
		if (LightCount >= MAX_LIGHT_COUNT) break;

		// Need Culling - Frustum & Distance
		auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle);
		if (nullptr == LightOBJ)	continue;

		// Distance Culling
		// Frustum Culling

		LightBuffer.AffectedLight[LightCount].LightType = ETOUI(LightOBJ->Get_LightType());
		LightBuffer.AffectedLight[LightCount].LightDirection = LightOBJ->Get_LightDirection();
		LightBuffer.AffectedLight[LightCount].LightColor = LightOBJ->Get_LightColor();
		LightBuffer.AffectedLight[LightCount].LightIntensity = LightOBJ->Get_LightIntensity();
		LightBuffer.AffectedLight[LightCount].LightRange = LightOBJ->Get_LightRange();
		LightBuffer.AffectedLight[LightCount].Position = LightOBJ->Get_LightPosition();
		LightBuffer.AffectedLight[LightCount].InnerAttanuation = cosf(XMConvertToRadians(LightOBJ->Get_LightInnerAttenuation()));
		LightBuffer.AffectedLight[LightCount].OuterAttanuation = cosf(XMConvertToRadians(LightOBJ->Get_LightOuterAttenuation()));

		LightCount++;
	}
	LightBuffer.LightCount = LightCount;

	auto LightConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Light");
	D3D11_MAPPED_SUBRESOURCE MRES;
	if (SUCCEEDED(m_pContext->Map(LightConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		memcpy(MRES.pData, &LightBuffer, sizeof(CB_LIGHT));
		m_pContext->Unmap(LightConstantBuffer->GetCBuffer().Get(), 0);
	}

	m_pContext->PSSetConstantBuffers(4, 1, LightConstantBuffer->GetCBuffer().GetAddressOf());
}

VOID CLightManager::Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity) {
	CLight::DESC LDesc{};
	if (m_LightHandleList.size() < 10)     LDesc.sObjectTag = "Light_Clone00" + m_LightHandleList.size();
	else if (m_LightHandleList.size() < 100)    LDesc.sObjectTag = "Light_Clone0" + m_LightHandleList.size();
	else if (m_LightHandleList.size() < 1000)   LDesc.sObjectTag = "Light_Clone" + m_LightHandleList.size();

	auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
	if (!(LightHandle))	return;

	auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
	LightOBJ->Change_LightType(m_pContext.Get(), LIGHT_TYPE::DIRECTIONAL);
	LightOBJ->Set_LightDirection(_Direction);
	LightOBJ->Set_LightColor(_Color);
	LightOBJ->Set_LightIntensity(_Intensity);

	m_LightHandleList.push_back(LightHandle.value());
}
VOID CLightManager::Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range) {
	CLight::DESC LDesc{};
	if (m_LightHandleList.size() < 10)     LDesc.sObjectTag = "Light_Clone00" + m_LightHandleList.size();
	else if (m_LightHandleList.size() < 100)    LDesc.sObjectTag = "Light_Clone0" + m_LightHandleList.size();
	else if (m_LightHandleList.size() < 1000)   LDesc.sObjectTag = "Light_Clone" + m_LightHandleList.size();

	auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
	if (!LightHandle)			return;

	auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
	if (nullptr == LightOBJ)	return;

	LightOBJ->Change_LightType(m_pContext.Get(), LIGHT_TYPE::POINT);

	LightOBJ->Set_LightPosition(_Position);
	LightOBJ->Set_LightColor(_Color);
	LightOBJ->Set_LightIntensity(_Intensity);
	LightOBJ->Set_LightRange(_Range);

	m_LightHandleList.push_back(LightHandle.value());

	
}
VOID CLightManager::Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt) {
	CLight::DESC LDesc{};
	if		(m_LightHandleList.size() < 10)		LDesc.sObjectTag = "Light_Clone00"	+ m_LightHandleList.size();
	else if (m_LightHandleList.size() < 100)    LDesc.sObjectTag = "Light_Clone0"	+ m_LightHandleList.size();
	else if (m_LightHandleList.size() < 1000)   LDesc.sObjectTag = "Light_Clone"	+ m_LightHandleList.size();

	auto LightHandle = E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_Light", "LightLayer", &LDesc);
	if (!LightHandle)	return;

	auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value());
	LightOBJ->Change_LightType(m_pContext.Get(), LIGHT_TYPE::SPOTLIGHT);

	LightOBJ->Set_LightPosition(_Position);
	LightOBJ->Set_LightColor(_Color);
	LightOBJ->Set_LightIntensity(_Intensity);
	LightOBJ->Set_LightRange(_Range);

	LightOBJ->Set_LightInnerAttenuation(_InnerAtt);
	LightOBJ->Set_LightOuterAttenuation(_OuterAtt);

	m_LightHandleList.push_back(LightHandle.value());
}

HRESULT CLightManager::Add_ShadowRenderGroup(ACTORTYPE _ATYPE, CGameObject* pRenderObject) {
	if (nullptr == pRenderObject) return E_FAIL;

	if (_ATYPE == ACTORTYPE::DYNAMIC) {
		m_pRenderable_DynamicObjectList.push_back(pRenderObject);
	}
	else {
		m_pRenderable_StaticObjectList.push_back(pRenderObject);
	}
	return S_OK;
}

VOID CLightManager::Bind_ShadowResource() {
	for (uint32_t i = 0; i < m_pActiveShadowLightList.size(); ++i) {
		if (i >= MAX_LIGHT_MAPCOUNT)	break;

		auto LightOBJ = m_pActiveShadowLightList[i];
		if (LightOBJ->Get_LightType() != LIGHT_TYPE::POINT) {
			StaticShadowMapList[i] = LightOBJ->Get_StaticShadowSRV().Get();
			DynamicShadowMapList[i] = LightOBJ->Get_DynamicShadowSRV().Get();
		}
	}
	m_pContext->CSSetShaderResources(6, 1, StaticShadowMapList.data());
	m_pContext->CSSetShaderResources(7, 1, DynamicShadowMapList.data());
}

VOID CLightManager::UnBind_ShadowResource() {
	m_pContext->CSSetShaderResources(6, MAX_LIGHT_MAPCOUNT, NullList.data());
	m_pContext->CSSetShaderResources(7, MAX_LIGHT_MAPCOUNT, NullList.data());
}

VOID CLightManager::Update_ActiveLights() {
	// 현재 프레임에 보이는 라이트 수집
	m_pActiveShadowLightList.clear();
	std::vector<LightData> CullingLight{};
	auto Camera = CGameInstance::Get().GetActiveCamera();
	if (nullptr == Camera) return;

	XMVECTOR CameraPos = Camera->GetTransform().GetLoadedPostion();

	for (auto& LightHandle : m_LightHandleList) {
		auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle);
		if (nullptr == LightOBJ)	continue;
		
		if (LightOBJ->Get_LightType() == LIGHT_TYPE::DIRECTIONAL) {
			CullingLight.push_back({ LightOBJ, 0.f });
			continue;
		}

		XMVECTOR CurrentPosition = LightOBJ->GetTransform().GetLoadedPostion();
		_float	 Distance = XMVectorGetX(XMVector3Length(XMVectorSubtract(CameraPos, CurrentPosition)));
		
		if (Distance <= 50.f )// && IsInFrustum(LightOBJ))
			CullingLight.push_back({ LightOBJ, Distance });
	}

	// 거리 기반 컬링 + 정렬
	std::sort(CullingLight.begin(), CullingLight.end(), [](const LightData& SRC, const LightData& DST) {
		return SRC.Distance < DST.Distance;
	});

	// 최대 MAX_LIGHT_COUNT 수만큼의 조명만 렌더링
	_float FinalActiveLightCount = std::min(MAX_LIGHT_COUNT, static_cast<int>(CullingLight.size()));
	for (uint32_t i = 0; i < FinalActiveLightCount; ++i) {
		m_pActiveShadowLightList.push_back(CullingLight[i].LightOBJ);
	}
}

_bool CLightManager::IsInFrustum(CLight* _LightOBJ) {
	auto CamCollider = CGameInstance::Get().GetActiveCamera()->GetCollider().Get();
	if (nullptr == CamCollider) return false;

	auto CameraFrustum = static_cast<CCollFrustum*>(CamCollider);

	auto LightCollider = (_LightOBJ->Get_LightType() == LIGHT_TYPE::POINT ? _LightOBJ->Get_SphereCollider() : _LightOBJ->Get_FrustumCollider());

	if (nullptr == LightCollider) return false;

	return CameraFrustum->Intersect(*LightCollider.get());
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
		auto LightOBJ = E::CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle);
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
