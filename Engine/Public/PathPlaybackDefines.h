#pragma once

#include "SerializerInterface.h"

NS_BEGIN(Engine)

inline constexpr int PATH_PLAYBACK_DATA_VERSION = 2;

enum class PATH_PLAYBACK_STATE : uint8_t
{
	IDLE,
	PLAYING,
	PAUSED,
	COMPLETED,
	INTERRUPTED
};

enum class PATH_PLAYBACK_INTERPOLATION : uint8_t
{
	LINEAR,
	CATMULL_ROM
};

enum class PATH_PLAYBACK_EASING : uint8_t
{
	LINEAR,
	EASE_IN,
	EASE_OUT,
	EASE_IN_OUT
};

enum class PATH_PLAYBACK_COORDINATE_SPACE : uint8_t
{
	WORLD,
	START_LOCAL
};

enum class PATH_PLAYBACK_ROTATION_MODE : uint8_t
{
	KEEP,
	RECORDED,
	FACE_DIRECTION
};

enum class PATH_PLAYBACK_MODE : uint8_t
{
	ONCE,
	LOOP,
	PING_PONG
};

enum class PATH_PLAYBACK_DIRECTION : uint8_t
{
	FORWARD,
	REVERSE
};

enum class PATH_PLAYBACK_FINISH_BEHAVIOR : uint8_t
{
	HOLD_LAST,
	RESET_TO_START
};

struct PATH_PLAYBACK_KEYFRAME final : public ISerializable
{
	_float fTime{};
	_float3 vPosition{};
	_float4 vRotation{ 0.f, 0.f, 0.f, 1.f };
	PATH_PLAYBACK_INTERPOLATION ePositionInterpolation{
		PATH_PLAYBACK_INTERPOLATION::LINEAR };
	PATH_PLAYBACK_EASING eEasing{ PATH_PLAYBACK_EASING::LINEAR };
	StringID sEventTag{};

	void Serialize(ISerializer& Serializer) const override
	{
		Serializer.Write("Time", fTime);
		Serializer.Write("Position", vPosition);
		Serializer.Write("Rotation", vRotation);
		Serializer.Write("PositionInterpolation", ePositionInterpolation);
		Serializer.Write("Easing", eEasing);
		Serializer.Write("EventTag", sEventTag);
	}

	void Deserialize(IDeserializer& Deserializer) override
	{
		Deserializer.Read("Time", fTime);
		Deserializer.Read("Position", vPosition);
		Deserializer.Read("Rotation", vRotation);
		Deserializer.Read("PositionInterpolation", ePositionInterpolation);
		Deserializer.Read("Easing", eEasing);
		Deserializer.Read("EventTag", sEventTag);
	}
};

struct PATH_PLAYBACK_CLIP final : public ISerializable
{
	StringID sClipID{};
	PATH_PLAYBACK_COORDINATE_SPACE eCoordinateSpace{
		PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL };
	PATH_PLAYBACK_ROTATION_MODE eRotationMode{
		PATH_PLAYBACK_ROTATION_MODE::RECORDED };
	PATH_PLAYBACK_MODE ePlayMode{ PATH_PLAYBACK_MODE::ONCE };
	PATH_PLAYBACK_FINISH_BEHAVIOR eFinishBehavior{
		PATH_PLAYBACK_FINISH_BEHAVIOR::HOLD_LAST };
	std::vector<PATH_PLAYBACK_KEYFRAME> Keyframes{};

	void SortKeyframes()
	{
		std::stable_sort(
			Keyframes.begin(), Keyframes.end(),
			[](const PATH_PLAYBACK_KEYFRAME& Left,
				const PATH_PLAYBACK_KEYFRAME& Right)
			{
				return Left.fTime < Right.fTime;
			});
	}

	void Serialize(ISerializer& Serializer) const override
	{
		Serializer.Write("ClipID", sClipID);
		Serializer.Write("CoordinateSpace", eCoordinateSpace);
		Serializer.Write("RotationMode", eRotationMode);
		Serializer.Write("PlayMode", ePlayMode);
		Serializer.Write("FinishBehavior", eFinishBehavior);
		Serializer.Write("Keyframes", Keyframes);
	}

	void Deserialize(IDeserializer& Deserializer) override
	{
		Deserializer.Read("ClipID", sClipID);
		Deserializer.Read("CoordinateSpace", eCoordinateSpace);
		Deserializer.Read("RotationMode", eRotationMode);
		Deserializer.Read("PlayMode", ePlayMode);
		Deserializer.Read("FinishBehavior", eFinishBehavior);
		Deserializer.Read("Keyframes", Keyframes);
		SortKeyframes();
	}
};

struct PATH_PLAYBACK_DATA final : public ISerializable
{
	int iVersion{ PATH_PLAYBACK_DATA_VERSION };
	std::vector<PATH_PLAYBACK_CLIP> Clips{};

	void SortKeyframes()
	{
		for (auto& Clip : Clips)
			Clip.SortKeyframes();
	}

	void Serialize(ISerializer& Serializer) const override
	{
		Serializer.Write("Version", iVersion);
		Serializer.Write("Clips", Clips);
	}

	void Deserialize(IDeserializer& Deserializer) override
	{
		Deserializer.Read("Version", iVersion);
		Deserializer.Read("Clips", Clips);
		SortKeyframes();
	}
};

struct PATH_PLAYBACK_POSE
{
	_float3 vPosition{};
	_float4 vRotation{ 0.f, 0.f, 0.f, 1.f };
};

struct PATH_PLAYBACK_STEP_RESULT
{
	_bool bValid{};
	PATH_PLAYBACK_POSE tTargetPose{};
	_float fStartElapsedTime{};
	_float fTargetElapsedTime{};
	size_t iTargetSegmentIndex{};
	PATH_PLAYBACK_DIRECTION eTargetDirection{
		PATH_PLAYBACK_DIRECTION::FORWARD };
	_bool bWouldComplete{};
	_bool bWouldWrap{};
};

NS_END
