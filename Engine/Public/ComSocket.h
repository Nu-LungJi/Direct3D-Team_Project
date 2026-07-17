

#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class CComAnimator;
class CComModelInstance;

class ENGINE_DLL CComSocket : public CComponent
{
public:
	typedef struct tagDesc : CComponent::DESC
	{
		CHandle	m_pOwner;
		std::string sModelInstanceName;
		std::string sAnimationName;
		uint32_t iBoneIndex;
		_float4 m_fOffset;

	}DESC;
public:
	DECLARE_DERIVED_TYPE(CComSocket, CComponent)

public:
	virtual void UpdateGUI() override;
	_bool Get_Socket_MatrixAtPose(int32_t iAnimIndex, _float fTrackPosition, _float4x4& OutSocketMatrix) const;

	void BuildBoneChain(const CResModel& model) const;

private:
	explicit CComSocket();
	~CComSocket() override;


private:
	HRESULT Initialize(void* pArg) override;
	_float4x4& Get_Socket_Matrix();





public:
	static UPtr<CComSocket> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	CHandle					m_pOwner{};
	mutable  std::vector<uint32_t> m_BoneChain;
	uint32_t		m_iBoneIndex;
	_float4			m_fOffset;
	_float4x4		m_SocketMatrix{};

	StringID		m_sAnimatorName;
	StringID		m_sModelInstanceName;
		
};

NS_END
