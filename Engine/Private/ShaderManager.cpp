#include "pch.h"
#include "ShaderManager.h"
CShaderManager::CShaderManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) : m_pDevice(pDevice), m_pContext(pContext){ }
CShaderManager::~CShaderManager()	{ }

UPtr<CShaderManager> CShaderManager::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) {
	return ToUPtr(new CShaderManager{ pDevice , pContext });
}