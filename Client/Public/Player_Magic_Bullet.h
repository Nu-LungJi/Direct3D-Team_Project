#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Client)

class CPlayer_Magic_Bullet : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Magic_Bullet, CGameObject)

public:
	typedef struct tag_Magic_Bullet_desc : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vStartPosition{};
		_float3 vEndPosition{};
		_float  fSpeed{ 10.f };
		_float  fCurveHeight{ 3.f };
		uint32_t iSampleCount{ 48 };
	}MAGIC_BULLET_DESC;

private:
	CPlayer_Magic_Bullet();
	~CPlayer_Magic_Bullet() override;

public:
	void UpdateGUI() override;
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	_float3 m_vStartPosition{};
	_float3 m_vEndPosition{};
	_float  m_fSpeed{};
	_float  m_fDistanceOnSegment{};
	size_t  m_iSplineIndex{};

	std::vector<_float3> m_Splines;

private:
	void BuildSpline(_float fCurveHeight, uint32_t iSampleCount);

public:
	static E::UPtr<CPlayer_Magic_Bullet> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
