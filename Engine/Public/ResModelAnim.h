#pragma once

#include "Resource.h"
struct aiAnimation;


NS_BEGIN(Engine)
class CResModelChanel;
class CResModel;
class ENGINE_DLL CResModelAnim final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResModelAnim, CResource)
public:
	typedef struct tagDesc {
		CResModel* pModel;
		std::string& path;
	}DESC;
private:
	explicit CResModelAnim(const _string& sPath);
	~CResModelAnim() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	_bool Update_TransformationMatrices(_float fTimeDelta, const std::vector<SPtr<CResModelBone>>& Bones, _bool isLoop);
	_bool ExtractRootMotionDelta(_float fPrevTrackPosition, _float fCurrTrackPosition, uint32_t iRootBoneIndex, _float3& vOutDelta);
	SPtr<CResModelChanel> FindRootChannel(uint32_t iRootBoneIndex);
	void RebuildCurrentKeyFrameIndices();

public:
	_float  GetDuration() const { return m_fDuration; }
	_float  GetTickPerSecond() const { return m_fTickPerSecond; }
	_float  GetCurrentTrackPosition() const { return m_fCurrentTrackPosition; }

	void    SetDuration(_float fDuration) { m_fDuration = fDuration; }
	void    SetTickPerSecond(_float fTickPerSecond) { m_fTickPerSecond = fTickPerSecond; }
	void    SetCurrentTrackPosition(_float fCurrentTrackPosition) ;

	std::string& GetAnimName() { return m_AnimName; }
	void		 SetAnimName(std::string _name) { m_AnimName = _name; }

	std::string& GetAnimPath() { return m_AnimPath; }
	void	 SetAnimPath(std::string _path) { m_AnimPath = _path; }	

	uint32_t	GetNumChannel() { return m_iNumChannels; };
	std::vector<SPtr<CResModelChanel>>& GetChannels() { return m_Channels; }


	int32_t     GetRootBoneIndex() { return m_iRootBoneIndex; }

private:
	// 런 타임 도중만 가지는 주소
	std::string			m_AnimPath;
	std::string			m_AnimName;

	/* 이 애니메이션의 총 길이. */
	_float				m_fDuration = {};
	_float				m_fTickPerSecond = {};
	_float				m_fCurrentTrackPosition = {};

	/* 컨트롤해야하는 뼈의 갯수 */
	uint32_t							m_iNumChannels = {};
	std::vector<SPtr<CResModelChanel>>	m_Channels;
	std::vector<uint32_t>					m_CurrentKeyFrameIndices;

	int32_t								m_iRootBoneIndex{};
	_float3								m_vRootDelta;

public:

	static SPtr<CResModelAnim> Create(const _string& sPath = {});
};

NS_END
