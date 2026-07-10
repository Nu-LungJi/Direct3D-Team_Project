
#pragma once

#include "Component.h"

#include "ISerializable.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)
class CComModelInstance;
class CResModelAnim;


class ENGINE_DLL CComAnimator : public CComponent
{

public:
	void UpdateGUI() override;

public:
	typedef struct tagDesc : CComponent::DESC
	{
		std::string_view  sComTag;
	
	}DESC;


public:
	enum ANIMTYPE{
		ANIM, ACTION
	};

	enum EVENTTYPE {
		COLLIDER,
		SOUND,
		EFFECT,
		END_EVENTTYPE
	};

	struct ANIMSTRUCT
	{
		int32_t   iAnimIndex = -1;

		// 재생 시간
		_float	  fTrackPosition = 0.f;
		// 재생 속도
		_float    fSpeed = 1.f;
		// 재생 루프
		_bool	bLoop = true;
		_bool	bFinished = false;

		// 키 몇개 있는지
		std::vector<uint32_t>	KeyFrameIndices;

		void Reset() {
			iAnimIndex = -1;
			fTrackPosition = 0.f;
			fSpeed = 1.f;
			bLoop = true;
			bFinished = false;
			KeyFrameIndices.clear();
		}

		void SetAnim(int32_t iIndex, _bool isLoop = true, _float fAnimSpeed = 1.f)
		{
			iAnimIndex = iIndex;
			fTrackPosition = 0.f;
			fSpeed = fAnimSpeed;
			bLoop = isLoop;
			bFinished = false;
			KeyFrameIndices.clear();
		}

		void SetTrackPostion(_float _trackPos) {
			fTrackPosition = _trackPos;
		}

		_bool IsValid() const
		{
			return iAnimIndex >= 0;
		}




		//void Serialize(ISerializer& serializer) const
		//{
		//	WRITE_ALL(serializer, iAnimIndex, fTrackPosition, fSpeed, bLoop, bFinished, KeyFrameIndices);
		//	//serializer.Write("iAnimIndex", iAnimIndex);
		//	//serializer.Write("fTrackPosition", fTrackPosition);
		//	//serializer.Write("fSpeed", fSpeed);
		//	//serializer.Write("bLoop", bLoop);
		//	//serializer.Write("bFinished", bFinished);
		//	//serializer.Write("KeyFrameIndices", KeyFrameIndices);
		//}
		//void Deserialize(IDeserializer& deserializer)
		//{
		//	READ_ALL(deserializer, iAnimIndex, fTrackPosition, fSpeed, bLoop, bFinished, KeyFrameIndices);
		//	//deserializer.Read("iAnimIndex", iAnimIndex);
		//	//deserializer.Read("fTrackPosition", fTrackPosition);
		//	//deserializer.Read("fSpeed", fSpeed);
		//	//deserializer.Read("bLoop", bLoop);
		//	//deserializer.Read("bFinished", bFinished);
		//	//deserializer.Read("KeyFrameIndices", KeyFrameIndices);
		//}
	};
	 
	struct ACTIONSTRUCT : public ISerializable {
		std::vector<std::string> sNames;
		std::vector<int32_t>   iAnimIndexes;
		// 항상 breakPoint는 iAnimIndexes보다 -1만큼 작다.
		std::vector<_float>    fAnimBreakPoint;
		// 재생 시간
		_float	  fDuration = 0.f;
		_float	  fActionTrackPosition = 0.f;

		// Collider
		




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


	// AnimUPdate
	HRESULT	Update_Anim(_float fTimeDelta);
	HRESULT Update_Action(_float fTimeDelta);
	void	Play_Anim(int32_t iAnimIndex, _bool bLoop=false, _float fBlendDuration = 0.1f);
	void	Update_AnimState(_float fTimeDelta, ANIMSTRUCT& AnimState);
	void	Build_BoneMatrices_CPU(_float fTimeDelta);
	void	Sample_Channel_CPU( CResModelChanel* pChannel, _float fTrackPosition, uint32_t& iCurrentKeyFrameIndex, std::vector<_float4x4>& OutLocalBoneMatrices);
	_matrix Evaluate_ChannelMatrix_CPU(CResModelChanel* pChannel, _float fTrackPosition);



	_vector RemoveYRotation(_vector qRotation);
	void	Blend_Anim(_float fTimeDelta);

	void	SetTrackPosition(_float fTrackPosition);


public:
	_float3 GetRootMotionDelta() { return m_vRootMotionDelta; }
	ANIMSTRUCT& GetCurAnimState() { return  m_CurAnimState; }


private:
	CComModelInstance* m_pModelInstance;


private:
	_string			m_Comtag;
	ANIMTYPE		m_iPlayAnimationType{ ANIMTYPE::ANIM };
	int32_t		m_iPlayAnimationNum{ 0 };
	int32_t		m_iPlayAnimIndex{ 0 };


private:
	// 애니메이션 정보 (블렌딩 고려)
	ANIMSTRUCT m_CurAnimState;
	ANIMSTRUCT m_PrevAnimState;

	// 애니메이션 제어를 위한 멤버 변수들
	_bool		    m_bPlay{ false };

	_bool			m_bBlending = true;
	_float          m_fRatio{ 0.f };
	_float			m_fBlendTime = 0.f;
	_float			m_fBlendDuration;

	std::vector<_float4x4>				m_LocalBoneMatrices;
	std::vector<_float4x4>				m_BlendStartLocalMatrices;
private:
	_bool   m_bRootMotion{ true };
	int32_t m_iRootBoneIndex{ -1 };
	// 애니메이션 Local 기준 RootMotion
	_float3 m_vRootMotionDelta{ 0.f, 0.f, 0.f };

public:
	static UPtr<CComAnimator> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

	_bool GetFinish() const { return m_CurAnimState.bFinished; }

	_bool GetPlay() const { return m_bPlay; }
	void  SetPlay(_bool bPlay) { m_bPlay = bPlay;}

	_bool GetLoop() const { return m_CurAnimState.bLoop; }
	void  SetLoop(_bool _bLoop) { m_CurAnimState.bLoop = _bLoop; }
	_float GetPlayAnimRatio() const { return m_fRatio; }


public:

	
public:
	uint32_t GetAnimationTYPE() const { return ETOUI(m_iPlayAnimationType); }
	void SetAnimationTYPE(ANIMTYPE eType) { m_iPlayAnimationType = eType; }	

	uint32_t GetPlayAnimIndex() const { return m_CurAnimState.iAnimIndex; }
	void SetPlayAnimIndex(uint32_t iIndex) { m_iPlayAnimIndex = iIndex; }



};

NS_END
