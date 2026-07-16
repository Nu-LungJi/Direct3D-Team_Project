

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
		StringID& sModelInstanceName;
		StringID& sAnimationName;
		uint32_t iBoneIndex;
		_float4 m_fOffset;

	}DESC;
public:
	DECLARE_DERIVED_TYPE(CComSocket, CComponent)

public:
	virtual void UpdateGUI() override;
	// Gets this socket at an arbitrary animation pose without changing playback.
	_bool Get_Socket_MatrixAtPose(int32_t iAnimIndex, _float fTrackPosition, _float4x4& OutSocketMatrix) const;
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
	CComModelInstance*		m_ComModelInstance{};
	CComAnimator*			m_ComAnimator{};

	uint32_t		m_iBoneIndex;
	_float4			m_fOffset;
	_float4x4		m_SocketMatrix{};

};

NS_END
