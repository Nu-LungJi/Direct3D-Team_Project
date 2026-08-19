#include "pch.h"
#include "Water.h"
#include "ResQuadTexBuffer.h"
#include "ResTexture2D.h"

NS_USING(Engine)

CWater::CWater()
{

}

CWater::CWater(const CWater& rhs)
	: CGameObject(rhs)
	, m_pQuadTexBuffer(rhs.m_pQuadTexBuffer)
	, m_pVertexShader(rhs.m_pVertexShader)
	, m_pPixelShader(rhs.m_pPixelShader)
{

}

CWater::~CWater()
{

}

HRESULT CWater::InitializePrototype(void* Arg)
{
	auto& gameInstance = CGameInstance::Get();

	// QuadTex Buffer
	if (!gameInstance.GetResourceFirst<CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex"))
	{
		auto buffer = gameInstance.AddResource(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex", CResQuadTexBuffer::Create());
		if (!buffer || FAILED(buffer->Load()))
			return E_FAIL;
	}

	// Vertex Shader
	if (!gameInstance.GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Water"))
	{
		auto shader = gameInstance.AddResourceT<CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_Water",
			"./ShaderFiles/Water/Shader_Water.hlsl");
		if (!shader || FAILED(shader->Load()))
			return E_FAIL;
	}

	// Pixel Shader 추가
	if (!gameInstance.GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Water"))
	{
		auto shader = gameInstance.AddResourceT<CResPixelShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"PS_Water",
			"./ShaderFiles/Water/Shader_Water.hlsl");
		if (!shader || FAILED(shader->Load()))
			return E_FAIL;
	}

	// Water Constant Buffer
	if (!gameInstance.GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Water"))
	{
		auto buffer = gameInstance.AddResource(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Water", CResCBuffer::Create());
		CResCBuffer::CBUFFER_DESC desc{ .byteWidth = sizeof(CB_WATER) };
		if (!buffer || FAILED(buffer->Load(desc)))
			return E_FAIL;
	}

	if (auto res = E::CGameInstance::Get().AddResourceT<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_WATER00", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/T_Water00_N.png")))
	{
		if (FAILED(res->Load()))
			return E_FAIL;
	}
	if (auto res = E::CGameInstance::Get().AddResourceT<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_WATER01", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/T_Water01_N.png")))
	{
		if (FAILED(res->Load()))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CWater::Initialize(void* Arg)
{
	if (FAILED(__super::Initialize(Arg)))
		return E_FAIL;

	auto& gameInstance = CGameInstance::Get();

	m_pQuadTexBuffer = gameInstance.GetResourceFirst<CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
	m_pVertexShader = gameInstance.GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Water");
	m_pPixelShader = gameInstance.GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Water");

	CComConstantBuffer::DESC objectBufferDesc{};
	objectBufferDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
	if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject",
		&objectBufferDesc, &m_pComCBufferPerObject)))
		return E_FAIL;

	CComConstantBuffer::DESC waterDesc{};
	waterDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_Water" };
	if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferWater",
		&waterDesc, &m_pComCBufferWater)))
		return E_FAIL;
	// TODO: 노멀 맵 텍스처 로드

	m_pNormalTex0 = gameInstance.GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_WATER00");
	if (m_pNormalTex0 == nullptr)
		return E_FAIL;

	m_pNormalTex1 = gameInstance.GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_WATER01");
	if (m_pNormalTex1 == nullptr)
		return E_FAIL;

	m_pSamplerState = gameInstance.GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	if (m_pSamplerState == nullptr)
		return E_FAIL;

	return S_OK;
}

void CWater::LateUpdate(_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
	m_fTime = std::fmod(m_fTime + std::max(0.f, fTimeDelta), 10000.f);
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::BLEND, this);
}

HRESULT CWater::Render(ID3D11DeviceContext* context, const RENDER_CTX& ctx)
{
	if (!m_pVertexShader || !m_pPixelShader || !m_pQuadTexBuffer)
		return E_FAIL;

	const _matrix world = GetTransform().GetLoadedCombinedWorldMatrix();

	CB_PER_OBJECT perObject{};
	XMStoreFloat4x4(&perObject.matWorld, world);
	XMStoreFloat4x4(&perObject.matWVP, world * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(context, &perObject, sizeof(perObject))))
		return E_FAIL;

	// 물 전용 상수 버퍼 업데이트 (시간, 색상, 스크롤 속도 등)
	CB_WATER waterData{};
	waterData.time = m_fTime;
	if (FAILED(m_pComCBufferWater->MapDiscard(context, &waterData, sizeof(waterData))))
		return E_FAIL;


	context->IASetInputLayout(m_pVertexShader->GetInputLayout().Get());
	context->VSSetShader(m_pVertexShader->GetVertexShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffer = m_pQuadTexBuffer->GetVertexBuffer().Get();
	const UINT stride = m_pQuadTexBuffer->GetVertexStride();
	const UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(m_pQuadTexBuffer->GetIndexBuffer().Get(), m_pQuadTexBuffer->GetIndexFormat(), 0);
	context->IASetPrimitiveTopology(m_pQuadTexBuffer->GetPrimitiveType());

	ID3D11Buffer* perObjectBuffer = m_pComCBufferPerObject->GetBuffer();
	ID3D11Buffer* waterConstantBuffer = m_pComCBufferWater->GetBuffer();
	context->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, &perObjectBuffer);
	context->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, &perObjectBuffer);
	context->PSSetConstantBuffers(11, 1, &waterConstantBuffer);

	context->PSSetShader(m_pPixelShader->GetPixelShader().Get(), nullptr, 0);

	// TODO: PSSetShaderResources를 통해 노멀 맵(t0, t1) 및 샘플러 바인딩
	context->PSSetShaderResources(0, 1, m_pNormalTex0->GetSRV().GetAddressOf());
	context->PSSetShaderResources(1, 1, m_pNormalTex1->GetSRV().GetAddressOf());

	context->PSSetSamplers(0, 1, m_pSamplerState->GetSamplerState().GetAddressOf());

	context->DrawIndexed(m_pQuadTexBuffer->GetNumIndices(), 0, 0);

	return S_OK;
}

void CWater::UpdateGUI()
{
	__super::UpdateGUI();
}

UPtr<CWater> CWater::Create()
{
	auto instance = ToUPtr(new CWater{});
	if (FAILED(instance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CWater");
		return nullptr;
	}
	return instance;
}

UPtr<CPrototype> CWater::Clone(void* pArg)
{
	auto instance = ToUPtr(new CWater{ *this });
	if (FAILED(instance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CWater");
		return nullptr;
	}
	return instance;
}


