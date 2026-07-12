#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CComAnimator;
class CComModelInstance;
class CResModel;


class CModel_Instance_Manager final : public CEngineBase
{
private:
	CModel_Instance_Manager();
	~CModel_Instance_Manager() override;

public:
	HRESULT Initialize();


public:
	void Add_Instance(CComModelInstance* pModelInstance,CComAnimator* pAnimator,const _float4x4& WorldMatrix,uint32_t iFlags = 0);


	void Add_Instance(CComModelInstance* pModelInstance,const GPU_ANIM_INSTANCE_DATA& InstanceData);

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

private:
	MODEL_INSTANCE_BATCH* Find_Or_Create_Batch(CComModelInstance* pModelInstance);

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
