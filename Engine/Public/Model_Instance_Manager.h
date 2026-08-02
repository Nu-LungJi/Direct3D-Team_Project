#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CComAnimator;
class CComModelInstance;
class CComStaticModelInstance;
class CResModel;

#define MAX_INSTANCE_COUNT	512
#define MAX_BONE_COUNT		512



class CModel_Instance_Manager final : public CEngineBase, public IRenderable
{
private:
	CModel_Instance_Manager();
	~CModel_Instance_Manager() override;

public:
	HRESULT Initialize();


public:
	void Add_Instance(CComModelInstance* pModelInstance,CComAnimator* pAnimator,const _float4x4& WorldMatrix,uint32_t iFlags = 0);

	void Add_Instance(CComStaticModelInstance* pModelInstance, const _float4x4& WorldMatrix, uint32_t iFlags);


	void Add_Instance(CComModelInstance* pModelInstance,const GPU_ANIM_INSTANCE_DATA& InstanceData);
	void Add_Part_Instance(CComStaticModelInstance* pModelInstance, const GPU_PART_INSTANCE_DATA& InstanceData);

	void Add_Instance(CComStaticModelInstance* pModelInstance, const GPU_ANIM_INSTANCE_DATA& InstanceData);


public:
	const std::vector<MODEL_INSTANCE_BATCH*>& Get_ActiveBatches() const
	{
		return m_ActiveBatches;
	}

	_bool Has_ActiveBatch() const
	{
		return !m_ActiveBatches.empty();
	}

	uint32_t Get_TotalInstanceCount() const
	{
		return m_iTotalInstanceCount;
	}


	void Clear_Frame();

public:
	void UpdateGUI();

public:
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	bool HasRenderPass(RENDERPASS ePass) const override { return ePass == RENDERPASS::DEFAULT; };

	/*----------- 광윤 추가 -----------*/
public:
	HRESULT Render_ShadowInstanced(ID3D11DeviceContext* pContext, std::optional<CHandle> _LightHandle, _bool _bStaticBatch);
	HRESULT Render_ShadowBatch(ID3D11DeviceContext* pContext, const MODEL_INSTANCE_BATCH& Batch);
	HRESULT Update_BonePaletteBuffer(ID3D11DeviceContext* pContext, const MODEL_INSTANCE_BATCH& Batch);
	HRESULT	Update_ShadowInstanceBuffer(ID3D11DeviceContext* pContext);
	HRESULT Bind_SkinMeshConstantBuffer(ID3D11DeviceContext* pContext, SPtr<CResModel>& Model, uint32_t MeshIndex);

	_bool	Has_ActiveDynamicShadowBatch();

private:
	SPtr<CResCBuffer>						m_pResSkinMeshCBuffer{};

	std::vector<GPU_ANIM_INSTANCE_DATA>		m_ShadowFilteredInstances{};
	std::vector<uint32_t>					m_ShadowVisibleSourceIndices{};
	std::vector<_float4x4>					m_ShadowBonePaletteScratch{};
	/*---------------------------------*/

private:
	MODEL_INSTANCE_BATCH* Find_Or_Create_Batch(CComStaticModelInstance* pModelInstance, _bool bStaticModel);
	MODEL_INSTANCE_BATCH* Find_Or_Create_Batch(CComModelInstance* pModelInstance, _bool bStaticModel, uint32_t iEvaluationMode = 0);
	MODEL_INSTANCE_BATCH* Find_Or_Create_Part_Batch(CComStaticModelInstance* pModelInstance);

private:
	// 모델 리소스별 영구 Batch 저장소
	std::unordered_map<MODEL_INSTANCE_KEY, std::unique_ptr<MODEL_INSTANCE_BATCH>,MODEL_INSTANCE_KEY_HASH>m_InstanceBatches;
	// 이번 프레임에 실제 Instance가 들어온 Batch만 저장
	std::vector<MODEL_INSTANCE_BATCH*>m_ActiveBatches;

private:
	uint32_t m_iTotalInstanceCount = 0;

public:
	static UPtr<CModel_Instance_Manager> Create();
};

NS_END
