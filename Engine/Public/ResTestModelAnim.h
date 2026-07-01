#pragma once

#include "Resource.h"
struct aiAnimation;


NS_BEGIN(Engine)
class CResTestModelChanel;
class CResTestModel;
class ENGINE_DLL CResTestModelAnim final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResTestModelAnim, CResource)
public:
	typedef struct tagDesc {
		const aiAnimation* pAIAnimation;
		CResTestModel* pModel;
	}DESC;
private:
	explicit CResTestModelAnim(const _string& sPath);
	~CResTestModelAnim() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	_bool Update_TransformationMatrices(_float fTimeDelta, const std::vector<SPtr<CResTestModelBone>>& Bones, _bool isLoop);

private:
	/* 이 애니메이션의 총 길이. */
	_float				m_fDuration = {};
	_float				m_fTickPerSecond = {};
	_float				m_fCurrentTrackPosition = {};

	/* 컨트롤해야하는 뼈의 갯수 */
	uint32_t							m_iNumChannels = {};
	std::vector<SPtr<CResTestModelChanel>>	m_Channels;
	std::vector<uint32_t>					m_CurrentKeyFrameIndices;




public:
	static SPtr<CResTestModelAnim> Create(const _string& sPath = {});
};

NS_END