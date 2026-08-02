#include "pch.h"
#include "GameOverMask.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "Level_Defines.h"

NS_USING(Client)

CGameOverMask::CGameOverMask()
{
}

CGameOverMask::~CGameOverMask()
{
}

HRESULT CGameOverMask::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CGameOverMask::Initialize(void* pArg)
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

		/* Component */
		CComponent::DESC CDesc{};
		Desc.pGameObject = this;

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};
	}

	return S_OK;
}

void CGameOverMask::PriorityUpdate(E::_float fTimeDelta)
{
}

void CGameOverMask::Update(E::_float fTimeDelta)
{
	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (!m_isActive)
		return;

	CUIObject::Update(fTimeDelta);

	m_fAccTime += fTimeDelta;

	//m_pComCButton->CheckPixelPerfectCollision(mousePos, true);

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}
}

void CGameOverMask::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	CUIObject::LateUpdate(fTimeDelta);
}

HRESULT CGameOverMask::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	const auto& viBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_GAMEOVERMASK");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_GAMEOVERMASK");

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
		perUI.texCoord = { 0.f, m_fAccTime };
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
		{
			auto pCbPerObject = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
			D3D11_MAPPED_SUBRESOURCE mappedSubResource;
			if (SUCCEEDED(pContext->Map(pCbPerObject->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
			{
				E::CB_PER_OBJECT cbPerObject{};

				_matrix matWVP = GetTransform().GetLoadedWorldMatrix() * ctx.matProj;
				XMStoreFloat4x4(&cbPerObject.matWVP, matWVP);

				memcpy(mappedSubResource.pData, &cbPerObject, sizeof(cbPerObject));
				pContext->Unmap(pCbPerObject->GetCBuffer().Get(), 0);
			}
			pContext->VSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}

	{
		const auto& maskSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_GameOverMask");
		const auto& glitterSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_SharpGlitterMask");
		const auto& smokeNoiseSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_SmokesNoiseMask");
		const auto& distortSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_CloudDistortion");

		ID3D11ShaderResourceView* srvs[4] = {
			maskSrv->GetSRV().Get(),
			glitterSrv->GetSRV().Get(),
			smokeNoiseSrv->GetSRV().Get(),
			distortSrv->GetSRV().Get(),
		};

		pContext->PSSetShaderResources(0, 4, srvs);
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	return S_OK;
}

E::UPtr<CGameOverMask> CGameOverMask::Create()
{
	auto pInstance = E::ToUPtr(new CGameOverMask{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CGameOverMask");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CGameOverMask::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CGameOverMask{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CGameOverMask");
		return nullptr;
	}

	return pInstance;
}
