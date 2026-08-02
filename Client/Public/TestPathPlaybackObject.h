#pragma once

#include "Client_Defines.h"
#include "GameObject.h"
#include "PathPlaybackDefines.h"

NS_BEGIN(Engine)
class CComPathPlayback;
class CResPathPlayback;
NS_END

NS_BEGIN(Client)

class CTestPathPlaybackObject final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestPathPlaybackObject, CGameObject)

	enum class TEST_CASE : uint8_t
	{
		START_LOCAL_LINEAR,
		WORLD_LINEAR,
		CATMULL_FACE_DIRECTION,
		LOOP_RECTANGLE,
		PING_PONG_ROTATION,
		EVENT_TO_CUSTOM,
		EASING_SEGMENTS
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		TEST_CASE eTestCase{ TEST_CASE::START_LOCAL_LINEAR };
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float fPlaybackRate{ 1.f };
		_bool bAutoPlay{ true };
	};

private:
	CTestPathPlaybackObject();
	CTestPathPlaybackObject(const CTestPathPlaybackObject& Prototype);
	~CTestPathPlaybackObject() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

private:
	HRESULT BuildTestPathResource();
	StringID GetTestClipID() const;
	_bool RestartPlayback(
		PATH_PLAYBACK_DIRECTION eDirection =
			PATH_PLAYBACK_DIRECTION::FORWARD);
	void HandleCommittedEvents();
	void DrawDebugObject();

private:
	SPtr<CResPathPlayback> m_pPathResource{};
	CComPathPlayback* m_pComPathPlayback{};

	TEST_CASE m_eTestCase{ TEST_CASE::START_LOCAL_LINEAR };
	_float3 m_vInitialPosition{};
	_float4 m_vInitialRotation{ 0.f, 0.f, 0.f, 1.f };
	_float m_fPlaybackRate{ 1.f };
	_bool m_bCustomMovement{};

public:
	static UPtr<CTestPathPlaybackObject> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
