#pragma once
#include "Component.h"
NS_BEGIN(Engine)

class ENGINE_DLL CComTransform final : public CComponent
{
public:
	DECLARE_DERIVED_TYPE(CComTransform, CComponent)

public:
	virtual void UpdateGUI();

private:
	explicit CComTransform();
	//explicit CComTransform(const CComTransform& rhs);
	~CComTransform() override;

public:
	_bool Update();

public:
	const _float3& GetPosition() const { return m_vPos; }
	const _float4& GetQuaternion() const { return m_vQuat; }
	const _float3& GetScale() const { return m_vScale; }

	_vector GetLoadedPostion() const { return XMLoadFloat3(&m_vPos); }
	_vector GetLoadedQuaternion() const { return XMLoadFloat4(&m_vQuat); }
	_vector GetLoadedScale() const { return XMLoadFloat3(&m_vScale); }

	void SetPosition(const _float3& vPos) { m_vPos = vPos; m_bDirty = true; }
	void SetPosition(_vector vPos) { XMStoreFloat3(&m_vPos, vPos); m_bDirty = true; }
	void SetScale(const _float3& vScale) { m_vScale = vScale; m_bDirty = true; }
	void SetScale(_vector vScale) { XMStoreFloat3(&m_vScale, vScale); m_bDirty = true; }
	void SetQuaternion(const _float4& vQuat) { SetQuaternion(XMLoadFloat4(&vQuat)); m_bDirty = true; }
	void SetQuaternion(_vector vQuat) { XMStoreFloat4(&m_vQuat, XMQuaternionNormalize(vQuat)); m_bDirty = true; }

	_vector GetState(STATE eState) const
	{
		switch (eState)
		{
		case STATE::RIGHT: // 기본 X축(1,0,0)을 현재 쿼터니언으로 회전
			return XMVector3Rotate(XMVectorSet(1.f, 0.f, 0.f, 0.f), XMLoadFloat4(&m_vQuat));
		case STATE::UP:    // 기본 Y축(0,1,0)을 현재 쿼터니언으로 회전
			return XMVector3Rotate(XMVectorSet(0.f, 1.f, 0.f, 0.f), XMLoadFloat4(&m_vQuat));
		case STATE::LOOK:  // 기본 Z축(0,0,1)을 현재 쿼터니언으로 회전
			return XMVector3Rotate(XMVectorSet(0.f, 0.f, 1.f, 0.f), XMLoadFloat4(&m_vQuat));
		case STATE::POSITION:
			return XMLoadFloat3(&m_vPos);
		}
		return XMVectorSet(0.f, 0.f, 0.f, 0.f);
	}

	void SetState(STATE eState, _vector vec)
	{
		switch (eState)
		{
		case STATE::RIGHT:
		{
			// 현재 UP, LOOK 벡터 가져오기
			_vector vUp = GetState(STATE::UP);
			_vector vLook = GetState(STATE::LOOK);

			// 정규화
			vec = XMVector3Normalize(vec);
			vUp = XMVector3Normalize(vUp);

			// RIGHT가 변경되면 LOOK도 재계산 (RIGHT × UP = LOOK)
			_vector vNewLook = XMVector3Cross(vUp, vec);
			vNewLook = XMVector3Normalize(vNewLook);

			// 다시 UP 재계산 (RIGHT × LOOK = UP)
			_vector vNewUp = XMVector3Cross(vNewLook, vec);
			vNewUp = XMVector3Normalize(vNewUp);

			// 3개의 축으로부터 쿼터니언 생성
			_matrix mat = XMMatrixIdentity();
			mat.r[0] = XMVectorSetW(vec, 0.f);       // RIGHT
			mat.r[1] = XMVectorSetW(vNewUp, 0.f);    // UP
			mat.r[2] = XMVectorSetW(vNewLook, 0.f);  // LOOK

			XMVECTOR quat = XMQuaternionRotationMatrix(mat);
			XMStoreFloat4(&m_vQuat, quat);
		}
		break;

		case STATE::UP:
		{
			_vector vRight = GetState(STATE::RIGHT);
			_vector vLook = GetState(STATE::LOOK);

			vec = XMVector3Normalize(vec);
			vRight = XMVector3Normalize(vRight);

			// UP이 변경되면 RIGHT도 재계산 (LOOK × UP = RIGHT)
			_vector vNewRight = XMVector3Cross(vec, vLook);
			vNewRight = XMVector3Normalize(vNewRight);

			// 다시 LOOK 재계산 (UP × RIGHT = LOOK)
			_vector vNewLook = XMVector3Cross(vNewRight, vec);
			vNewLook = XMVector3Normalize(vNewLook);

			_matrix mat = XMMatrixIdentity();
			mat.r[0] = XMVectorSetW(vNewRight, 0.f);  // RIGHT
			mat.r[1] = XMVectorSetW(vec, 0.f);        // UP
			mat.r[2] = XMVectorSetW(vNewLook, 0.f);   // LOOK

			XMVECTOR quat = XMQuaternionRotationMatrix(mat);
			XMStoreFloat4(&m_vQuat, quat);
		}
		break;

		case STATE::LOOK:
		{
			_vector vRight = GetState(STATE::RIGHT);
			_vector vUp = GetState(STATE::UP);

			vec = XMVector3Normalize(vec);
			vRight = XMVector3Normalize(vRight);

			// LOOK이 변경되면 RIGHT도 재계산 (UP × LOOK = RIGHT)
			_vector vNewRight = XMVector3Cross(vUp, vec);
			vNewRight = XMVector3Normalize(vNewRight);

			// 다시 UP 재계산 (LOOK × RIGHT = UP)
			_vector vNewUp = XMVector3Cross(vec, vNewRight);
			vNewUp = XMVector3Normalize(vNewUp);

			_matrix mat = XMMatrixIdentity();
			mat.r[0] = XMVectorSetW(vNewRight, 0.f);  // RIGHT
			mat.r[1] = XMVectorSetW(vNewUp, 0.f);     // UP
			mat.r[2] = XMVectorSetW(vec, 0.f);        // LOOK

			XMVECTOR quat = XMQuaternionRotationMatrix(mat);
			XMStoreFloat4(&m_vQuat, quat);
		}
		break;

		case STATE::POSITION:
		{
			XMStoreFloat3(&m_vPos, vec);
		}
		break;
		}
		m_bDirty = true;
	}

	void AddPosition(const _float3& vPos)
	{
		//m_vPos += vPos;
		
		XMStoreFloat3(&m_vPos, GetLoadedPostion() + XMLoadFloat3(&vPos));
		m_bDirty = true;
	}

	void AddPosition(_fvector vPos)
	{
		XMStoreFloat3(&m_vPos, GetLoadedPostion() + vPos);
		m_bDirty = true;
	}

	//{ return XMLoadFloat4(reinterpret_cast<const _float4*>(&GetWorldMatrix()->m[ETOUI(eState)][0])); }

	void AddRotation(_vector vAxis, _float fAngleDegree)
	{
		_vector vNormalizedAxis = XMVector3Normalize(vAxis);
		_vector qPrevRot = XMLoadFloat4(&m_vQuat);
		_vector qDeltaRot = XMQuaternionRotationAxis(vNormalizedAxis, XMConvertToRadians(fAngleDegree));
		_vector qNewRot = XMQuaternionMultiply(qPrevRot, qDeltaRot);
		XMStoreFloat4(&m_vQuat, XMQuaternionNormalize(qNewRot));

		m_bDirty = true;
	}

	void SetRotation(_vector vAxis, float fAngleDegree)
	{
		_vector qNewRot = XMQuaternionRotationAxis(vAxis, XMConvertToRadians(fAngleDegree));
		XMStoreFloat4(&m_vQuat, qNewRot);

		m_bDirty = true;
	}

	void AddQuaternion(_vector qDeltaRot)
	{
		_vector qPrevRot = XMLoadFloat4(&m_vQuat);
		_vector qNewRot = XMQuaternionMultiply(qPrevRot, qDeltaRot);
		XMStoreFloat4(&m_vQuat, XMQuaternionNormalize(qNewRot));
		m_bDirty = true;
	}

	const _float3& GetRotationEuler() const { return m_vEuler; }

	void SetRotationEuler(const _float3& vRotationAmount)
	{
		m_vEuler = vRotationAmount;
		_vector qDelta = XMQuaternionRotationRollPitchYaw(
			XMConvertToRadians(vRotationAmount.x),
			XMConvertToRadians(vRotationAmount.y),
			XMConvertToRadians(vRotationAmount.z)
		);
		XMStoreFloat4(&m_vQuat, XMQuaternionNormalize(qDelta));

		m_bDirty = true;
	}


	void AddRotationEuler(const _float3& vRotationAmount)
	{
		_vector qPrev = XMLoadFloat4(&m_vQuat);
		_vector qDelta = XMQuaternionRotationRollPitchYaw(
			XMConvertToRadians(vRotationAmount.x),
			XMConvertToRadians(vRotationAmount.y),
			XMConvertToRadians(vRotationAmount.z)
		);
		_vector qNew = XMQuaternionMultiply(qPrev, qDelta);
		XMStoreFloat4(&m_vQuat, XMQuaternionNormalize(qNew));

		m_bDirty = true;
	}

	void UpdateEulerFromQuat()
	{
		SimpleMath::Quaternion q = XMLoadFloat4(&m_vQuat);
		SimpleMath::Vector3 euler = q.ToEuler();
		m_vEuler.x = XMConvertToDegrees(euler.x);
		m_vEuler.y = XMConvertToDegrees(euler.y);
		m_vEuler.z = XMConvertToDegrees(euler.z);
	}

public:
	void GoStraight(_float fDist);
	void GoBackward(_float fDist);
	void GoLeft(_float fDist);
	void GoRight(_float fDist);

	void LookAt(_fvector vAt, _fvector vWorldUp = { 0.f, 1.f, 0.f, 0.f });
	void Chase(_fvector vGoal, _float fDist, _float fLimit = 0.f);

public:
	const _float4x4* GetWorldMatrix() const { return &m_WorldMatrix; }
	_matrix GetLoadedWorldMatrix() const { return XMLoadFloat4x4(&m_WorldMatrix); }
	const _float4x4* GetCombinedWorldMatrix() const { return &m_CombinedWorldMatrix; }
	_matrix GetLoadedCombinedWorldMatrix() const { return XMLoadFloat4x4(&m_CombinedWorldMatrix); }

private:
	_float3 m_vPos{};
	_float4 m_vQuat{ 0.f,0.f,0.f,1.f };
	_float3 m_vScale{ 1.f,1.f,1.f };

	_float3 m_vEuler{};

	//_bool m_bParentScaleNormalize{ false };

	_bool m_bDirty{ true };
public:
	void SetDirty(_bool b) { m_bDirty = b; }
	_bool GetDirty() const { return m_bDirty; }

public:
	void SetParentWorldMatrix(const std::optional<_float4x4>& mat) { m_ParentWorldMatrix = mat; m_bDirty = true; };
private:
	std::optional<_float4x4> m_ParentWorldMatrix{};

private:
	_float4x4 m_WorldMatrix{};
	_float4x4 m_CombinedWorldMatrix{};

public:
	static UPtr<CComTransform> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
