#pragma once
#include "WorldAgent.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CResVertexShader;
class CResComputeShader;
NS_END

NS_BEGIN(Client)
class CWorldAnimal : public CWorldAgent
{
public:
	DECLARE_DERIVED_TYPE(CWorldAnimal, CWorldAgent)

private:
	CWorldAnimal();
	~CWorldAnimal() override;

public:
	void UpdateGUI() override;
public:
	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initialize(void* pArg) override;
	void						PriorityUpdate(E::_float fTimeDelta) override;
	void						FixedUpdate(E::_float fTimeDelta) override;
	void						Update(E::_float fTimeDelta) override;
	void						LateUpdate(E::_float fTimeDelta) override;
	HRESULT					Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch) override;
public:
	void						Set_Gravity(_bool bGravity);
private:
	HRESULT					Bind_InstanceBuffer_CS(ID3D11DeviceContext* pContext);
	HRESULT					Bind_FinalBoneUAV_CS(ID3D11DeviceContext* pContext);
	HRESULT					Bind_FinalBoneSRV_VS(ID3D11DeviceContext* pContext);
	void						Unbind_GPUAnimation_CS(ID3D11DeviceContext* pContext);
	void						Unbind_GPUAnimation_VS(ID3D11DeviceContext* pContext);
private:
	SPtr<E::CResVertexShader>	m_pResVertexGPUSkinningInstancedShader{};
	SPtr<E::CResComputeShader>	m_pAnimComputeShader{};
public:
	static E::UPtr<CWorldAnimal> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
