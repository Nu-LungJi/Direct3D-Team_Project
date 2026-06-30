#include "pch.h"
#include "ResGeoShaderStreamOut.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CResGeoShaderStreamOut::Load(const std::any& arg)
{
	m_eState = STATE::LOADING;
	if (FAILED(CompileShader()))
	{
		MSG_BOX("GEOMETRY SHADER SO COMPILE FAILED");
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	ComPtr<ID3D11ShaderReflection> pReflector{};
	if (FAILED(D3DReflect(
		m_pBlob->GetBufferPointer(),
		m_pBlob->GetBufferSize(),
		IID_ID3D11ShaderReflection,
		(void**)&pReflector
	)))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	D3D11_SHADER_DESC shaderDesc;
	pReflector->GetDesc(&shaderDesc);

	std::vector<D3D11_SO_DECLARATION_ENTRY> soEntries;
	UINT totalComponentCount = 0;

	// 2. 출력 파라미터 개수만큼 순회
	for (UINT i = 0; i < shaderDesc.OutputParameters; ++i)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		pReflector->GetOutputParameterDesc(i, &paramDesc);

		D3D11_SO_DECLARATION_ENTRY entry = {};
		entry.Stream = 0;                             // 기본 스트림 0
		entry.SemanticName = paramDesc.SemanticName;  // "POSITION", "TEXCOORD" 등
		entry.SemanticIndex = paramDesc.SemanticIndex;
		entry.StartComponent = 0;                     // 시작 컴포넌트

		// 컴포넌트 개수 계산 (Mask 값 확인)
		// 1이면 float1, 3이면 float2, 7이면 float3, 15면 float4
		BYTE mask = paramDesc.Mask;
		BYTE count = 0;
		if (mask & 1) count++;
		if (mask & 2) count++;
		if (mask & 4) count++;
		if (mask & 8) count++;

		entry.ComponentCount = count;
		entry.OutputSlot = 0; // 데이터를 보낼 SO 버퍼 슬롯

		soEntries.push_back(entry);
	}

	UINT stride = 0;
	for (auto& entry : soEntries) stride += entry.ComponentCount * sizeof(float);

	UINT bufferStrides[] = { stride };
	UINT numStrides = 1;

	if (FAILED(m_pDevice->CreateGeometryShaderWithStreamOutput(
		m_pBlob->GetBufferPointer(),
		m_pBlob->GetBufferSize(),
		soEntries.data(),            // 리플렉션으로 만든 배열
		(UINT)soEntries.size(),      // 엔트리 개수
		bufferStrides,               // 출력 버퍼의 간격(Stride)
		numStrides,                  // 스트라이드 배열 개수
		D3D11_SO_NO_RASTERIZED_STREAM, // 픽셀 셰이더로 보내지 않을 때 사용
		nullptr,                     // Class Linkage
		&m_pGeometryShader)))        // 생성된 셰이더 저장
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	//if (FAILED(m_pDevice->CreateGeometryShader(m_pBlob->GetBufferPointer(),
	//	m_pBlob->GetBufferSize(), nullptr, &m_pGeometryShader)))
	//{
	//	m_eState = STATE::LOADFAIL;
	//	return E_FAIL;
	//}


	m_eState = STATE::LOADED;

	m_pBlob.Reset();
	m_pErrorBlob.Reset();
	return S_OK;
}

HRESULT CResGeoShaderStreamOut::Unload(const std::any& arg)
{
	m_eState = STATE::UNLOAD;
	return S_OK;
}

CResGeoShaderStreamOut::CResGeoShaderStreamOut(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResShader{ sPath, pDevice, pContext }
{
	m_sEntryPoint = "GSSOMain";
	m_sTarget = "gs_5_0";
}

CResGeoShaderStreamOut::~CResGeoShaderStreamOut()
{
}

SPtr<CResGeoShaderStreamOut> CResGeoShaderStreamOut::Create(const _string& sPath)
{
	return ToSPtr(new CResGeoShaderStreamOut{ sPath, CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
