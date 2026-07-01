
#pragma once

#include "Resource.h"

struct aiNodeAnim;

NS_BEGIN(Engine)

class CResTestModel;
class CResTestModelBone;
class ENGINE_DLL CResTestModelChanel final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResTestModelChanel, CResource)
public:
	typedef struct tagDesc {
		const aiNodeAnim* pAIChannel;
		CResTestModel* pModel;
	}DESC;
private:
	explicit CResTestModelChanel(const _string& sPath);
	~CResTestModelChanel() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	void Update_TransformationMatrix(uint32_t& iCurrentKeyFrameIndex, _float fCurrentTrackPosition, const std::vector<SPtr<CResTestModelBone>>& Bones);


private:
	char				m_szName[MAX_PATH] = {};
	int32_t				m_iBoneIndex = {};
	uint32_t			m_iNumKeyFrames = {};
	std::vector<KEYFRAME>	m_KeyFrames;

public:
	static SPtr<CResTestModelChanel> Create(const _string& sPath = {});
};

NS_END