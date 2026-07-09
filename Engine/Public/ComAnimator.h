
#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class CComModelInstance;
class CComAnimMontage;
class CResModelAnim;


class ENGINE_DLL CComAnimator : public CComponent
{
public:
	typedef struct tagDesc : CComponent::DESC
	{
		std::string_view  sComTag;
	
	}DESC;

public:
	enum ANIMTYPE{
		ANIM, MONTAGE
	};



public:
	DECLARE_DERIVED_TYPE(CComAnimator, CComponent)


private:
	explicit CComAnimator();
	~CComAnimator() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	HRESULT Update(_float fTimeDelta);

	HRESULT Play_AnimationMontage(_float fTimeDelta, int32_t iAnimMontageIndex);
	 

public:
	HRESULT AnimEditor_Play_AnimResource(_float fTimeDelta, uint32_t iModelAnimNum);
	HRESULT AnimEditor_Play_AnimMontage(_float fTimeDelta, const std::string& strAnimMontageName);   


public:
	uint32_t GetAnimationTYPE() const { return ETOUI(m_iPlayAnimationType); }
	void SetAnimationTYPE(ANIMTYPE eType) { m_iPlayAnimationType = eType; }	

	uint32_t GetPlayAnimIndex() const { return m_iPlayAnimIndex; }
	void SetPlayAnimIndex(uint32_t iIndex) { m_iPlayAnimIndex = iIndex; }

	_float GetPlayAnimRatio() const { return m_fRatio; }

	_bool GetPlay() const { return m_bPlay; }
	void  SetPlay(_bool bPlay) { m_bPlay = bPlay; }

	_bool GetFinish() const { return m_bFinish; }


	_bool GetLoop() const { return m_bLoop; }
	void  SetLoop(_bool _bLoop) { m_bLoop = _bLoop; }


	std::unordered_map<int32_t, CComAnimMontage*>& GetAnimMontages(){return m_mapAnimMontages;}

	int32_t Get_CurrAnimMontageIndex() { return m_iAnimMontageIndex; }
	void SetCurrentAnimMontageIndex(int32_t iIndex) { m_iAnimMontageIndex = iIndex; }
private:
	CComModelInstance* m_pModelInstance;
	std::unordered_map<int32_t, CComAnimMontage*> m_mapAnimMontages;
	int32_t		   m_iAnimMontageIndex{ -1 };

private:
	_string			m_Comtag;
	ANIMTYPE		m_iPlayAnimationType{ ANIMTYPE::ANIM };
	uint32_t		m_iPlayAnimationNum{ 0 };
	uint32_t		m_iPlayAnimIndex{ 0 };
	uint32_t		m_iPlayAnimMonatgueIndex{ 0 };

private:
	_bool		    m_bPlay{ false };
	_bool			m_bLoop{ false };
	_bool		    m_bFinish{ false };
	_float          m_fRatio{ 0.f };

	
public:
	static UPtr<CComAnimator> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
