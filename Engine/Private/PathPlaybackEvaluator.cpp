#include "pch.h"
#include "PathPlaybackEvaluator.h"

NS_USING(Engine)

static constexpr _float PATH_PLAYBACK_EVALUATOR_EPSILON = 0.0001f;

_bool CPathPlaybackEvaluator::EvaluatePose(
	const PATH_PLAYBACK_CLIP& Clip,
	_float fElapsedTime,
	const CONTEXT& Context,
	PATH_PLAYBACK_POSE& OutPose,
	size_t* pOutSegmentIndex)
{
	if (Clip.Keyframes.size() < 2)
		return false;

	const auto& Keyframes = Clip.Keyframes;
	const _float fDuration =
		Keyframes.back().fTime - Keyframes.front().fTime;
	const _float fSampleTime = Keyframes.front().fTime +
		std::clamp(fElapsedTime, 0.f, fDuration);

	const auto Upper = std::upper_bound(
		Keyframes.begin(), Keyframes.end(), fSampleTime,
		[](_float fTime, const PATH_PLAYBACK_KEYFRAME& Keyframe)
		{
			return fTime < Keyframe.fTime;
		});
	const size_t iRight = Upper == Keyframes.end()
		? Keyframes.size() - 1
		: static_cast<size_t>(std::distance(Keyframes.begin(), Upper));
	const size_t iLeft = iRight > 0 ? iRight - 1 : 0;

	const auto& Left = Keyframes[iLeft];
	const auto& Right = Keyframes[iRight];
	const _float fSegmentDuration = Right.fTime - Left.fTime;
	const _float fLinearRatio =
		fSegmentDuration > PATH_PLAYBACK_EVALUATOR_EPSILON
		? std::clamp(
			(fSampleTime - Left.fTime) / fSegmentDuration,
			0.f, 1.f)
		: 0.f;
	const _float fRatio = ApplyEasing(fLinearRatio, Left.eEasing);

	_vector Position{};
	if (Left.ePositionInterpolation ==
		PATH_PLAYBACK_INTERPOLATION::CATMULL_ROM)
	{
		const size_t iPrevious = iLeft > 0 ? iLeft - 1 : iLeft;
		const size_t iNext = std::min(iRight + 1, Keyframes.size() - 1);
		Position = XMVectorCatmullRom(
			XMLoadFloat3(&Keyframes[iPrevious].vPosition),
			XMLoadFloat3(&Left.vPosition),
			XMLoadFloat3(&Right.vPosition),
			XMLoadFloat3(&Keyframes[iNext].vPosition),
			fRatio);
	}
	else
	{
		Position = XMVectorLerp(
			XMLoadFloat3(&Left.vPosition),
			XMLoadFloat3(&Right.vPosition),
			fRatio);
	}

	_vector Rotation = XMQuaternionSlerp(
		NormalizeQuaternionOrIdentity(Left.vRotation),
		NormalizeQuaternionOrIdentity(Right.vRotation),
		fRatio);

	if (Clip.eCoordinateSpace ==
		PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL)
	{
		const _vector AnchorRotation =
			NormalizeQuaternionOrIdentity(
				Context.tStartAnchorPose.vRotation);
		const _matrix AnchorWorld =
			XMMatrixRotationQuaternion(AnchorRotation) *
			XMMatrixTranslationFromVector(
				XMLoadFloat3(&Context.tStartAnchorPose.vPosition));
		Position = XMVector3TransformCoord(Position, AnchorWorld);

		const _matrix WorldRotation =
			XMMatrixRotationQuaternion(Rotation) *
			XMMatrixRotationQuaternion(AnchorRotation);
		Rotation = XMQuaternionNormalize(
			XMQuaternionRotationMatrix(WorldRotation));
	}

	XMStoreFloat3(&OutPose.vPosition, Position);
	XMStoreFloat4(&OutPose.vRotation, Rotation);

	if (Context.bHasCurrentObjectPose)
	{
		if (Clip.eRotationMode == PATH_PLAYBACK_ROTATION_MODE::KEEP)
		{
			OutPose.vRotation = Context.tCurrentObjectPose.vRotation;
		}
		else if (Clip.eRotationMode ==
			PATH_PLAYBACK_ROTATION_MODE::FACE_DIRECTION)
		{
			const _float3 vDirection{
				OutPose.vPosition.x - Context.tCurrentObjectPose.vPosition.x,
				OutPose.vPosition.y - Context.tCurrentObjectPose.vPosition.y,
				OutPose.vPosition.z - Context.tCurrentObjectPose.vPosition.z };
			OutPose.vRotation = MakeFacingQuaternion(
				vDirection,
				Context.tCurrentObjectPose.vRotation);
		}
	}

	if (pOutSegmentIndex)
		*pOutSegmentIndex = iLeft;
	return true;
}

_float CPathPlaybackEvaluator::ApplyEasing(
	_float fRatio,
	PATH_PLAYBACK_EASING eEasing)
{
	fRatio = std::clamp(fRatio, 0.f, 1.f);

	switch (eEasing)
	{
	case PATH_PLAYBACK_EASING::EASE_IN:
		return fRatio * fRatio;

	case PATH_PLAYBACK_EASING::EASE_OUT:
	{
		const _float fInverse = 1.f - fRatio;
		return 1.f - fInverse * fInverse;
	}

	case PATH_PLAYBACK_EASING::EASE_IN_OUT:
		return fRatio * fRatio * (3.f - 2.f * fRatio);

	case PATH_PLAYBACK_EASING::LINEAR:
	default:
		return fRatio;
	}
}

_vector CPathPlaybackEvaluator::NormalizeQuaternionOrIdentity(
	const _float4& Rotation)
{
	const _vector Quaternion = XMLoadFloat4(&Rotation);
	const _float fLengthSq = XMVectorGetX(XMQuaternionLengthSq(Quaternion));
	if (!std::isfinite(fLengthSq) ||
		fLengthSq <= PATH_PLAYBACK_EVALUATOR_EPSILON)
	{
		return XMQuaternionIdentity();
	}
	return XMQuaternionNormalize(Quaternion);
}

_float4 CPathPlaybackEvaluator::MakeFacingQuaternion(
	const _float3& Direction,
	const _float4& Fallback)
{
	_vector Look = XMLoadFloat3(&Direction);
	if (XMVectorGetX(XMVector3LengthSq(Look)) <=
		PATH_PLAYBACK_EVALUATOR_EPSILON)
	{
		return Fallback;
	}

	Look = XMVector3Normalize(Look);
	_vector Up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	if (std::abs(XMVectorGetX(XMVector3Dot(Look, Up))) >= 0.999f)
		Up = XMVectorSet(0.f, 0.f, 1.f, 0.f);

	const _vector Right = XMVector3Normalize(XMVector3Cross(Up, Look));
	Up = XMVector3Cross(Look, Right);

	_matrix RotationMatrix = XMMatrixIdentity();
	RotationMatrix.r[0] = XMVectorSetW(Right, 0.f);
	RotationMatrix.r[1] = XMVectorSetW(Up, 0.f);
	RotationMatrix.r[2] = XMVectorSetW(Look, 0.f);

	_float4 Result{};
	XMStoreFloat4(
		&Result,
		XMQuaternionNormalize(
			XMQuaternionRotationMatrix(RotationMatrix)));
	return Result;
}
