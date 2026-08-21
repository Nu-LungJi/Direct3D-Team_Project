#pragma once

#include "Resource.h"
struct aiAnimation;


NS_BEGIN(Engine)
class CResModelChanel;
class ENGINE_DLL CResModelAnim final : public CResource
{
public:
	struct MORPH_KEY
	{
		_float fTrackPosition{};
		std::vector<uint32_t> TargetIndices{};
		std::vector<_float> Weights{};
	};
	struct MORPH_CHANNEL
	{
		_string sMeshName{};
		std::vector<MORPH_KEY> Keys{};
	};
	DECLARE_DERIVED_TYPE(CResModelAnim, CResource)
public:
	typedef struct tagDesc {
		std::string path{};
	}DESC;
private:
	explicit CResModelAnim(const _string& sPath);
	~CResModelAnim() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;


	SPtr<CResModelChanel> FindRootChannel(uint32_t iRootBoneIndex) const;

public:
	_float  GetDuration() const { return m_fDuration; }
	_float  GetTickPerSecond() const { return m_fTickPerSecond; }

	const std::string& GetAnimName() const { return m_AnimName; }
	void		 SetAnimName(std::string _name) { m_AnimName = _name; }

	const std::string& GetAnimPath() const { return m_AnimPath; }
	void	 SetAnimPath(std::string _path) { m_AnimPath = _path; }	

	uint32_t	GetNumChannel() const { return m_iNumChannels; };
	const std::vector<SPtr<CResModelChanel>>& GetChannels() const { return m_Channels; }
	CResModelChanel* GetChannelByBoneIndex(uint32_t iBoneIndex) const;
	_bool SampleMorphWeights(_float fTrackPosition,
		DirectX::XMUINT4& outIndices, _float4& outWeights) const;
	_bool HasMorphCurves() const { return !m_MorphChannels.empty(); }

private:
	// 런 타임 도중만 가지는 주소
	std::string			m_AnimPath;
	std::string			m_AnimName;

	// [LSY] 모든 Animator가 공유하는 불변 Clip 원본 데이터다.
	// 현재 재생 시간과 KeyFrame cursor는 CComAnimator::ANIMSTRUCT가 객체별로 가진다.
	_float				m_fDuration = {};
	_float				m_fTickPerSecond = {};

	/* 컨트롤해야하는 뼈의 갯수 */
	uint32_t							m_iNumChannels = {};
	std::vector<SPtr<CResModelChanel>>	m_Channels;

	std::vector<SPtr<CResModelChanel>>	m_ChannelsByBone;
	std::vector<MORPH_CHANNEL> m_MorphChannels{};
	_float3								m_vRootDelta;

public:

	static SPtr<CResModelAnim> Create(const _string& sPath = {});
};

NS_END
