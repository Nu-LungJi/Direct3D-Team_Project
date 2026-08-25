#pragma once
#include "Engine_Defines.h"
#include "Handle.h"
#include "SerializerInterface.h"


NS_BEGIN(Engine)

// 카메라 키프레임이 움직일 기준
enum class ECinematicCoordinateSpace
{
	World,
	TargetLocal
};

// 타깃이 컷신 중 움직일 때 카메라 경로도 함께 움직일지
enum class ECinematicBindingMode
{
	// 매 프레임 타깃의 현재 Transform을 사용
	Live, // Boss 이동 -> Boss 기준 카메라 경로도 같이 이동

	// Legacy
	// 컷신 시작 순간의 타깃 Transform을 저장하고 계속 사용
	Snapshot // 컷신 시작 위치 저장 -> Boss가 움직여도 카메라 경로는 고정
};

// 보간 방식
enum class ECinematicInterpolation
{
	Linear,
	CatmullRom
};

// 키프레임 데이터 (원본)
struct FCinematicCameraKeyframe : public ISerializable
{
	// Shot 내부 로컬 시간
	_float fTime{};
	_float3 vPosition{};
	_float4 vRotation{ 0.f, 0.f, 0.f, 1.f };
	_float fFovY{ 75.f };
	ECinematicInterpolation ePositionInterpolation{ ECinematicInterpolation::Linear };

	void Serialize(ISerializer& serializer) const override
	{
		serializer.Write("Time", fTime);
		serializer.Write("Position", vPosition);
		serializer.Write("Rotation", vRotation);
		serializer.Write("FovY", fFovY);
		serializer.Write("PositionInterpolation", ePositionInterpolation);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		deserializer.Read("Time", fTime);
		deserializer.Read("Position", vPosition);
		deserializer.Read("Rotation", vRotation);
		deserializer.Read("FovY", fFovY);
		deserializer.Read("PositionInterpolation", ePositionInterpolation);
	}
};

// (런타임) 특정시간의 계산결과pose
struct FCinematicCameraPose
{
	_float3 vPosition{};
	_float4 vRotation{ 0.f, 0.f,0.f,1.f };
	_float fFovY{ 75.f };
};

struct FCinematicCameraShot : public ISerializable
{
	StringID ShotID{};
	// 트랙 내 시간 중 시작시간
	_float fStartTime{};

	ECinematicCoordinateSpace eCoordinateSpace{ ECinematicCoordinateSpace::World };
	ECinematicBindingMode eBindingMode{ ECinematicBindingMode::Snapshot };

	std::vector<FCinematicCameraKeyframe> Keyframes{};


	void Serialize(ISerializer& serializer) const override
	{
		serializer.Write("ShotID", ShotID);
		serializer.Write("StartTime", fStartTime);
		serializer.Write("CoordinateSpace", eCoordinateSpace);
		serializer.Write("BindingMode", eBindingMode);
		serializer.Write("Keyframes", Keyframes);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		deserializer.Read("ShotID", ShotID);
		deserializer.Read("StartTime", fStartTime);
		deserializer.Read("CoordinateSpace", eCoordinateSpace);
		deserializer.Read("BindingMode", eBindingMode);
		deserializer.Read("Keyframes", Keyframes);

		SortKeyFrames();
	}

	void SortKeyFrames()
	{
		std::stable_sort(
			Keyframes.begin(),
			Keyframes.end(),
			[](const FCinematicCameraKeyframe& Left,
				const FCinematicCameraKeyframe& Right)
			{
				return Left.fTime < Right.fTime;
			});
	}
};

struct FCinematicCameraTrack : public ISerializable
{
	StringID TrackID{};
	std::vector<FCinematicCameraShot> Shots{};

	void Serialize(ISerializer& serializer) const override
	{
		serializer.Write("TrackID", TrackID);
		serializer.Write("Shots", Shots);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		deserializer.Read("TrackID", TrackID);
		deserializer.Read("Shots", Shots);

		SortShots();
	}

	void SortShots()
	{
		std::stable_sort(
			Shots.begin(),
			Shots.end(),
			[](const FCinematicCameraShot& Left,
				const FCinematicCameraShot& Right)
			{
				return Left.fStartTime < Right.fStartTime;
			});
	}
};

// 저장 전용 루트 데이터
struct FCinematicAssetData final : public ISerializable
{
	int iVersion{ 1 };
	StringID CinematicID{};
	FCinematicCameraTrack CameraTrack{};

	void Serialize(ISerializer& serializer) const override
	{
		serializer.Write("Version", iVersion);
		serializer.Write("CinematicID", CinematicID);
		serializer.Write("CameraTrack", CameraTrack);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		deserializer.Read("Version", iVersion);
		deserializer.Read("CinematicID", CinematicID);
		deserializer.Read("CameraTrack", CameraTrack);

		if (iVersion != 1)
			throw std::runtime_error(
				"Unsupported cinematic asset version");
	}
};

// 컷신 시작할 때 옵션
enum class ECinematicStartMode
{
	Immediate,
	Blend
};

// 컷신 끝나고 복귀 옵션
enum class ECinematicReturnMode
{
	Immediate,
	Blend
};

struct FCinematicPlayOptions
{
	ECinematicStartMode eStartMode{ ECinematicStartMode::Immediate };
	_float fStartBlendDuration{ 0.5f };

	ECinematicReturnMode eReturnMode { ECinematicReturnMode::Immediate };
	_float fReturnBlendDuration { 0.5f };

	// 지정하면 키프레임 회전 대신 이 오브젝트를 매 프레임 바라본다.
	std::optional<CHandle> LookAtTargetHandle{};
	// 타겟의 회전 기준 로컬 오프셋 (예: 캐릭터 머리 높이).
	_float3 vLookAtTargetLocalOffset{};
};

NS_END
