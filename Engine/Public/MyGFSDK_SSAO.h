#pragma once
#include "Engine_Defines.h"

#include "GFSDK_SSAO.h"
class GFSDK_SSAO_Context_D3D11;
NS_BEGIN(Engine)

class CMyGFSDK_SSAO final: public CEngineBase
{
private:
	CMyGFSDK_SSAO();
	~CMyGFSDK_SSAO() override;

private:
	HRESULT Initialize();

public:
	void SetInputDepths(const GFSDK_SSAO_InputData_D3D11& input = {}) { m_GFSDK_SSAO_InputData = input; };
	void SetAOParameters(const GFSDK_SSAO_Parameters& param = {}) { m_GFSDK_SSAO_Parameters = param; };
	void SetRenderTarget(const GFSDK_SSAO_Output_D3D11& output = {}) { m_GFSDK_SSAO_Output = output; }
	HRESULT RenderAO();
private:
	GFSDK_SSAO_CustomHeap m_GFSDK_SSAO_CustomHeap{};
	GFSDK_SSAO_Context_D3D11* m_pGFSDK_SSAO_Context{};
	GFSDK_SSAO_InputData_D3D11 m_GFSDK_SSAO_InputData{};
	GFSDK_SSAO_Parameters m_GFSDK_SSAO_Parameters{};
	GFSDK_SSAO_Output_D3D11 m_GFSDK_SSAO_Output{};


public:
	static UPtr<CMyGFSDK_SSAO> Create();

private:
	void Free() override;
};

NS_END