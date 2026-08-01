#include "pch.h"
#include "TestPathPlaybackObject.h"

#include "ComPathPlayback.h"
#include "DbgLineRender.h"
#include "ResPathPlayback.h"

NS_USING(Client)

static PATH_PLAYBACK_KEYFRAME TestPathPlaybackMakeKeyframe(
	_float fTime,
	const _float3& vPosition,
	PATH_PLAYBACK_INTERPOLATION eInterpolation =
		PATH_PLAYBACK_INTERPOLATION::LINEAR,
	const _float4& vRotation = { 0.f, 0.f, 0.f, 1.f },
	const StringID& sEventTag = {},
	PATH_PLAYBACK_EASING eEasing = PATH_PLAYBACK_EASING::LINEAR)
{
	PATH_PLAYBACK_KEYFRAME Keyframe{};
	Keyframe.fTime = fTime;
	Keyframe.vPosition = vPosition;
	Keyframe.vRotation = vRotation;
	Keyframe.ePositionInterpolation = eInterpolation;
	Keyframe.eEasing = eEasing;
	Keyframe.sEventTag = sEventTag;
	return Keyframe;
}

static _float4 TestPathPlaybackMakeYawRotation(_float fDegrees)
{
	_float4 Rotation{};
	XMStoreFloat4(
		&Rotation,
		XMQuaternionRotationAxis(
			XMVectorSet(0.f, 1.f, 0.f, 0.f),
			XMConvertToRadians(fDegrees)));
	return Rotation;
}

CTestPathPlaybackObject::CTestPathPlaybackObject() = default;

CTestPathPlaybackObject::CTestPathPlaybackObject(
	const CTestPathPlaybackObject& Prototype)
	: CGameObject{ Prototype }
	, m_pPathResource{ Prototype.m_pPathResource }
{
}

HRESULT CTestPathPlaybackObject::InitializePrototype(void* pArg)
{
	return BuildTestPathResource();
}

HRESULT CTestPathPlaybackObject::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<const DESC*>(pArg);
	if (!pDesc || !m_pPathResource ||
		FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	m_eTestCase = pDesc->eTestCase;
	m_vInitialPosition = pDesc->vInitialPosition;
	m_fPlaybackRate = std::max(0.f, pDesc->fPlaybackRate);

	GetTransform().SetPosition(m_vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	m_vInitialRotation = GetTransform().GetQuaternion();
	GetTransform().Update();

	CComPathPlayback::DESC ComponentDesc{};
	ComponentDesc.pPathResource = m_pPathResource;
	ComponentDesc.fPlaybackRate = m_fPlaybackRate;
	if (FAILED(AddComponentFromProto(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComPathPlayback,
		"ComPathPlayback",
		&ComponentDesc,
		&m_pComPathPlayback)) ||
		!m_pComPathPlayback)
	{
		return E_FAIL;
	}

	if (pDesc->bAutoPlay && !RestartPlayback())
		return E_FAIL;

	return S_OK;
}

void CTestPathPlaybackObject::FixedUpdate(_float fTimeDelta)
{
	if (m_bCustomMovement)
	{
		GetTransform().AddPosition(
			_float3{ 0.f, 1.5f * fTimeDelta, 0.f });
		return;
	}

	if (!m_pComPathPlayback)
		return;

	const PATH_PLAYBACK_STEP_RESULT Step =
		m_pComPathPlayback->EvaluateNext(fTimeDelta);
	if (!Step.bValid)
		return;

	GetTransform().SetPosition(Step.tTargetPose.vPosition);
	GetTransform().SetQuaternion(Step.tTargetPose.vRotation);

	if (!m_pComPathPlayback->CommitEvaluatedStep())
		return;

	HandleCommittedEvents();
}

void CTestPathPlaybackObject::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();
	DrawDebugObjectAndPath();
}

void CTestPathPlaybackObject::UpdateGUI()
{
	CGameObject::UpdateGUI();

	ImGui::Separator();
	ImGui::Text(
		"Path Test: %s",
		magic_enum::enum_name(m_eTestCase).data());
	ImGui::Text(
		"Move Owner: %s",
		m_bCustomMovement ? "Custom" : "PathPlayback");

	if (ImGui::Button("Restart Forward"))
		RestartPlayback(PATH_PLAYBACK_DIRECTION::FORWARD);
	ImGui::SameLine();
	if (ImGui::Button("Restart Reverse"))
		RestartPlayback(PATH_PLAYBACK_DIRECTION::REVERSE);

	if (!m_pComPathPlayback)
		return;

	if (ImGui::Button("Pause"))
		m_pComPathPlayback->Pause();
	ImGui::SameLine();
	if (ImGui::Button("Resume"))
		m_pComPathPlayback->Resume();
	ImGui::SameLine();
	if (ImGui::Button("Stop"))
	{
		m_bCustomMovement = false;
		m_pComPathPlayback->Stop();
	}

	if (ImGui::DragFloat(
		"Playback Rate",
		&m_fPlaybackRate,
		0.05f,
		0.f,
		5.f))
	{
		m_pComPathPlayback->SetPlaybackRate(m_fPlaybackRate);
	}
}

HRESULT CTestPathPlaybackObject::BuildTestPathResource()
{
	PATH_PLAYBACK_DATA Data{};

	{
		PATH_PLAYBACK_CLIP Clip{};
		Clip.sClipID = "StartLocalLinear";
		Clip.eCoordinateSpace =
			PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL;
		Clip.eRotationMode = PATH_PLAYBACK_ROTATION_MODE::KEEP;
		Clip.Keyframes = {
			TestPathPlaybackMakeKeyframe(0.f, { 0.f, 0.f, 0.f }),
			TestPathPlaybackMakeKeyframe(2.f, { 0.f, 0.f, 4.f }),
			TestPathPlaybackMakeKeyframe(4.f, { 0.f, 0.f, 8.f })
		};
		Data.Clips.push_back(std::move(Clip));
	}

	{
		PATH_PLAYBACK_CLIP Clip{};
		Clip.sClipID = "WorldLinear";
		Clip.eCoordinateSpace = PATH_PLAYBACK_COORDINATE_SPACE::WORLD;
		Clip.eRotationMode = PATH_PLAYBACK_ROTATION_MODE::RECORDED;
		Clip.Keyframes = {
			TestPathPlaybackMakeKeyframe(0.f, { 24.f, 103.f, 5.f }),
			TestPathPlaybackMakeKeyframe(
				2.f,
				{ 24.f, 106.f, 9.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				TestPathPlaybackMakeYawRotation(90.f)),
			TestPathPlaybackMakeKeyframe(
				4.f,
				{ 29.f, 103.f, 13.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				TestPathPlaybackMakeYawRotation(180.f))
		};
		Data.Clips.push_back(std::move(Clip));
	}

	{
		PATH_PLAYBACK_CLIP Clip{};
		Clip.sClipID = "CurveFacing";
		Clip.eCoordinateSpace =
			PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL;
		Clip.eRotationMode = PATH_PLAYBACK_ROTATION_MODE::FACE_DIRECTION;
		Clip.Keyframes = {
			TestPathPlaybackMakeKeyframe(
				0.f, { 0.f, 0.f, 0.f },
				PATH_PLAYBACK_INTERPOLATION::CATMULL_ROM),
			TestPathPlaybackMakeKeyframe(
				1.5f, { 3.f, 2.f, 3.f },
				PATH_PLAYBACK_INTERPOLATION::CATMULL_ROM),
			TestPathPlaybackMakeKeyframe(
				3.f, { -3.f, 1.f, 6.f },
				PATH_PLAYBACK_INTERPOLATION::CATMULL_ROM),
			TestPathPlaybackMakeKeyframe(
				4.5f, { 0.f, 0.f, 9.f },
				PATH_PLAYBACK_INTERPOLATION::CATMULL_ROM)
		};
		Data.Clips.push_back(std::move(Clip));
	}

	{
		PATH_PLAYBACK_CLIP Clip{};
		Clip.sClipID = "LoopRectangle";
		Clip.eCoordinateSpace =
			PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL;
		Clip.eRotationMode = PATH_PLAYBACK_ROTATION_MODE::FACE_DIRECTION;
		Clip.ePlayMode = PATH_PLAYBACK_MODE::LOOP;
		Clip.Keyframes = {
			TestPathPlaybackMakeKeyframe(0.f, { 0.f, 0.f, 0.f }),
			TestPathPlaybackMakeKeyframe(1.5f, { 5.f, 0.f, 0.f }),
			TestPathPlaybackMakeKeyframe(3.f, { 5.f, 0.f, 5.f }),
			TestPathPlaybackMakeKeyframe(4.5f, { 0.f, 0.f, 5.f }),
			TestPathPlaybackMakeKeyframe(6.f, { 0.f, 0.f, 0.f })
		};
		Data.Clips.push_back(std::move(Clip));
	}

	{
		PATH_PLAYBACK_CLIP Clip{};
		Clip.sClipID = "PingPongRotation";
		Clip.eCoordinateSpace =
			PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL;
		Clip.eRotationMode = PATH_PLAYBACK_ROTATION_MODE::RECORDED;
		Clip.ePlayMode = PATH_PLAYBACK_MODE::PING_PONG;
		Clip.Keyframes = {
			TestPathPlaybackMakeKeyframe(
				0.f,
				{ 0.f, 0.f, 0.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				TestPathPlaybackMakeYawRotation(0.f)),
			TestPathPlaybackMakeKeyframe(
				2.f,
				{ 0.f, 3.f, 4.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				TestPathPlaybackMakeYawRotation(90.f)),
			TestPathPlaybackMakeKeyframe(
				4.f,
				{ 0.f, 0.f, 8.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				TestPathPlaybackMakeYawRotation(180.f))
		};
		Data.Clips.push_back(std::move(Clip));
	}

	{
		PATH_PLAYBACK_CLIP Clip{};
		Clip.sClipID = "EventToCustom";
		Clip.eCoordinateSpace =
			PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL;
		Clip.eRotationMode = PATH_PLAYBACK_ROTATION_MODE::FACE_DIRECTION;
		Clip.Keyframes = {
			TestPathPlaybackMakeKeyframe(0.f, { 0.f, 0.f, 0.f }),
			TestPathPlaybackMakeKeyframe(
				1.f,
				{ 0.f, 0.f, 3.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				{ 0.f, 0.f, 0.f, 1.f },
				"HalfWay"),
			TestPathPlaybackMakeKeyframe(
				2.f,
				{ 0.f, 0.f, 6.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				{ 0.f, 0.f, 0.f, 1.f },
				"StartCustomMovement"),
			TestPathPlaybackMakeKeyframe(3.f, { 0.f, 0.f, 9.f })
		};
		Data.Clips.push_back(std::move(Clip));
	}

	{
		PATH_PLAYBACK_CLIP Clip{};
		Clip.sClipID = "EasingSegments";
		Clip.eCoordinateSpace =
			PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL;
		Clip.eRotationMode = PATH_PLAYBACK_ROTATION_MODE::KEEP;
		Clip.Keyframes = {
			TestPathPlaybackMakeKeyframe(
				0.f,
				{ 0.f, 0.f, 0.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				{ 0.f, 0.f, 0.f, 1.f },
				{},
				PATH_PLAYBACK_EASING::LINEAR),
			TestPathPlaybackMakeKeyframe(
				2.f,
				{ 0.f, 0.f, 4.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				{ 0.f, 0.f, 0.f, 1.f },
				{},
				PATH_PLAYBACK_EASING::EASE_IN),
			TestPathPlaybackMakeKeyframe(
				4.f,
				{ 0.f, 0.f, 8.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				{ 0.f, 0.f, 0.f, 1.f },
				{},
				PATH_PLAYBACK_EASING::EASE_OUT),
			TestPathPlaybackMakeKeyframe(
				6.f,
				{ 0.f, 0.f, 12.f },
				PATH_PLAYBACK_INTERPOLATION::LINEAR,
				{ 0.f, 0.f, 0.f, 1.f },
				{},
				PATH_PLAYBACK_EASING::EASE_IN_OUT),
			TestPathPlaybackMakeKeyframe(
				8.f,
				{ 0.f, 0.f, 16.f })
		};
		Data.Clips.push_back(std::move(Clip));
	}

	m_pPathResource = CResPathPlayback::CreateFromData(std::move(Data));
	return m_pPathResource ? S_OK : E_FAIL;
}

StringID CTestPathPlaybackObject::GetTestClipID() const
{
	switch (m_eTestCase)
	{
	case TEST_CASE::START_LOCAL_LINEAR:
		return "StartLocalLinear";
	case TEST_CASE::WORLD_LINEAR:
		return "WorldLinear";
	case TEST_CASE::CATMULL_FACE_DIRECTION:
		return "CurveFacing";
	case TEST_CASE::LOOP_RECTANGLE:
		return "LoopRectangle";
	case TEST_CASE::PING_PONG_ROTATION:
		return "PingPongRotation";
	case TEST_CASE::EVENT_TO_CUSTOM:
		return "EventToCustom";
	case TEST_CASE::EASING_SEGMENTS:
		return "EasingSegments";
	default:
		return {};
	}
}

_bool CTestPathPlaybackObject::RestartPlayback(
	PATH_PLAYBACK_DIRECTION eDirection)
{
	if (!m_pComPathPlayback)
		return false;

	m_bCustomMovement = false;
	GetTransform().SetPosition(m_vInitialPosition);
	GetTransform().SetQuaternion(m_vInitialRotation);
	GetTransform().Update();
	m_pComPathPlayback->SetPlaybackRate(m_fPlaybackRate);

	if (!m_pComPathPlayback->Play(GetTestClipID(), eDirection, true))
		return false;

	const PATH_PLAYBACK_POSE& Pose =
		m_pComPathPlayback->GetCurrentPose();
	GetTransform().SetPosition(Pose.vPosition);
	GetTransform().SetQuaternion(Pose.vRotation);
	GetTransform().Update();
	return true;
}

void CTestPathPlaybackObject::HandleCommittedEvents()
{
	if (!m_pComPathPlayback)
		return;

	for (const size_t iKeyframe :
		m_pComPathPlayback->GetReachedKeyframeIndicesThisCommit())
	{
		const PATH_PLAYBACK_KEYFRAME* pKeyframe =
			m_pComPathPlayback->GetKeyframe(iKeyframe);
		if (!pKeyframe || pKeyframe->sEventTag.hash == 0)
			continue;

		DEBUG_LOG_STR(
			std::string{ "[PathPlaybackTest][" } +
			std::string{ GetObjectTag() } + "] Event: " +
			pKeyframe->sEventTag.GetDbgStr() + "\n");

		if (pKeyframe->sEventTag == StringID{ "StartCustomMovement" })
		{
			m_pComPathPlayback->Interrupt();
			m_bCustomMovement = true;
			break;
		}
	}
}

void CTestPathPlaybackObject::DrawDebugObjectAndPath()
{
	auto* pDebug = CGameInstance::Get().GetDbgLineRender();
	if (!pDebug || !m_pComPathPlayback)
		return;

	const _float4 PreviousColor = pDebug->GetColor();
	const DBG_LINE_DEPTH_MODE PreviousDepth = pDebug->GetDepthMode();
	pDebug->SetDepthTest(false);

	static constexpr std::array<_float4, 7> Colors{
		_float4{ 0.2f, 1.f, 0.2f, 1.f },
		_float4{ 1.f, 0.85f, 0.15f, 1.f },
		_float4{ 0.2f, 0.8f, 1.f, 1.f },
		_float4{ 1.f, 0.3f, 1.f, 1.f },
		_float4{ 1.f, 0.45f, 0.2f, 1.f },
		_float4{ 1.f, 0.2f, 0.2f, 1.f },
		_float4{ 0.2f, 1.f, 0.85f, 1.f }
	};
	const size_t iColor = std::min(
		static_cast<size_t>(m_eTestCase),
		Colors.size() - 1);
	pDebug->SetColor(Colors[iColor]);
	pDebug->AddBox(
		{ 0.45f, 0.45f, 0.45f },
		GetTransform().GetLoadedWorldMatrix());

	const PATH_PLAYBACK_CLIP* pClip =
		m_pComPathPlayback->GetCurrentClip();
	if (pClip && pClip->Keyframes.size() >= 2)
	{
		for (const PATH_PLAYBACK_KEYFRAME& Keyframe : pClip->Keyframes)
		{
			const _float3 Point =
				TransformPathPositionForDebug(Keyframe.vPosition);
			pDebug->AddSphere(
				0.15f,
				XMMatrixTranslation(Point.x, Point.y, Point.z));
		}

		for (size_t i = 0; i + 1 < pClip->Keyframes.size(); ++i)
		{
			const auto& Left = pClip->Keyframes[i];
			const auto& Right = pClip->Keyframes[i + 1];
			const uint32_t iSampleCount =
				Left.ePositionInterpolation ==
					PATH_PLAYBACK_INTERPOLATION::CATMULL_ROM
				? 16u
				: 1u;

			_float3 Previous = TransformPathPositionForDebug(Left.vPosition);
			for (uint32_t iSample = 1;
				iSample <= iSampleCount;
				++iSample)
			{
				const _float fRatio =
					static_cast<_float>(iSample) /
					static_cast<_float>(iSampleCount);
				_vector Position{};
				if (Left.ePositionInterpolation ==
					PATH_PLAYBACK_INTERPOLATION::CATMULL_ROM)
				{
					const size_t iPrevious = i > 0 ? i - 1 : i;
					const size_t iNext =
						std::min(i + 2, pClip->Keyframes.size() - 1);
					Position = XMVectorCatmullRom(
						XMLoadFloat3(&pClip->Keyframes[iPrevious].vPosition),
						XMLoadFloat3(&Left.vPosition),
						XMLoadFloat3(&Right.vPosition),
						XMLoadFloat3(&pClip->Keyframes[iNext].vPosition),
						fRatio);
				}
				else
				{
					Position = XMVectorLerp(
						XMLoadFloat3(&Left.vPosition),
						XMLoadFloat3(&Right.vPosition),
						fRatio);
				}

				_float3 LocalPoint{};
				XMStoreFloat3(&LocalPoint, Position);
				const _float3 Current =
					TransformPathPositionForDebug(LocalPoint);
				pDebug->AddLine(Previous, Current);
				Previous = Current;
			}
		}
	}

	pDebug->SetColor(PreviousColor);
	pDebug->SetDepthMode(PreviousDepth);
}

_float3 CTestPathPlaybackObject::TransformPathPositionForDebug(
	const _float3& vPosition) const
{
	const PATH_PLAYBACK_CLIP* pClip = m_pComPathPlayback
		? m_pComPathPlayback->GetCurrentClip()
		: nullptr;
	if (!pClip || pClip->eCoordinateSpace ==
		PATH_PLAYBACK_COORDINATE_SPACE::WORLD)
	{
		return vPosition;
	}

	const _matrix AnchorWorld =
		XMMatrixRotationQuaternion(XMLoadFloat4(&m_vInitialRotation)) *
		XMMatrixTranslationFromVector(XMLoadFloat3(&m_vInitialPosition));
	_float3 Result{};
	XMStoreFloat3(
		&Result,
		XMVector3TransformCoord(XMLoadFloat3(&vPosition), AnchorWorld));
	return Result;
}

UPtr<CTestPathPlaybackObject> CTestPathPlaybackObject::Create()
{
	auto pInstance = ToUPtr(new CTestPathPlaybackObject{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CTestPathPlaybackObject");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CTestPathPlaybackObject::Clone(void* pArg)
{
	auto pInstance =
		ToUPtr(new CTestPathPlaybackObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CTestPathPlaybackObject");
		return nullptr;
	}
	return pInstance;
}
