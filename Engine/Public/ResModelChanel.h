
#pragma once

#include "Resource.h"

NS_BEGIN(Engine)

class CResModel;
class CResModelBone;
class ENGINE_DLL CResModelChanel final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResModelChanel, CResource)
public:
	typedef struct tagDesc {
		_char* ptr;
		CResModel* pModel;
	}DESC;
private:
	explicit CResModelChanel(const _string& sPath);
	~CResModelChanel() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	void Update_TransformationMatrix(uint32_t& iCurrentKeyFrameIndex, _float fCurrentTrackPosition, const std::vector<SPtr<CResModelBone>>& Bones);

	uint32_t FindKeyFrameIndex(float fTrackPos);


private:
	char				m_szName[MAX_PATH] = {};
	int32_t				m_iBoneIndex = {};
	uint32_t			m_iNumKeyFrames = {};
	std::vector<KEYFRAME>	m_KeyFrames;

public:
	static SPtr<CResModelChanel> Create(const _string& sPath = {});
};

NS_END