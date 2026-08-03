#pragma once

#include <DirectXMath.h>
#include <cstdint>
#include <vector>

struct ID3D11ShaderResourceView;

namespace Engine
{
	struct NVCLOTH_FABRIC_HANDLE
	{
		// [LSY] 매니저가 소유하는 공유 Fabric을 식별한다. 0은 유효하지 않은 핸들이다.
		uint64_t iValue{};

		constexpr explicit operator bool() const
		{
			return iValue != 0;
		}
	};

	struct NVCLOTH_CLOTH_HANDLE
	{
		// [LSY] 매니저가 소유하는 개별 Cloth 인스턴스를 식별한다. 0은 유효하지 않은 핸들이다.
		uint64_t iValue{};

		constexpr explicit operator bool() const
		{
			return iValue != 0;
		}
	};

	struct NVCLOTH_FABRIC_DESC
	{
		// [LSY] Cloth 오브젝트 로컬 공간의 쿠킹용 파티클 위치다.
		std::vector<DirectX::XMFLOAT3> vecPositions{};

		// [LSY] 삼각형 목록 인덱스다. 원소 개수는 반드시 3의 배수여야 한다.
		std::vector<uint32_t> vecIndices{};

		// [LSY] 파티클별 역질량이다. 0이면 고정되고, 양수이면 물리적으로 움직인다.
		// 비어 있으면 모든 파티클을 동적 파티클로 취급한다.
		std::vector<float> vecInverseMasses{};

		// [LSY] Fabric 쿠커가 수직·수평 제약 Phase를 분류할 때 사용하는 기준 방향이다.
		// 런타임에 Cloth를 떨어뜨리는 중력값은 NVCLOTH_CLOTH_DESC::vGravity에서 설정한다.
		DirectX::XMFLOAT3 vGravity{ 0.f, -1.f, 0.f };

		// [LSY] 메시 표면을 따라 계산하는 Geodesic Tether를 생성한다.
		// 직선거리 방식보다 복잡한 형상에서 안정적이지만 Fabric 쿠킹 비용이 증가한다.
		bool bUseGeodesicTether{ true };
	};

	struct NVCLOTH_FABRIC_INFO
	{
		// [LSY] 쿠킹된 공유 Fabric의 통계 정보다. 디버깅과 비용 확인에 사용한다.
		uint32_t iParticleCount{};
		uint32_t iPhaseCount{};
		uint32_t iConstraintCount{};
		uint32_t iTetherCount{};
		uint32_t iTriangleCount{};
	};

	struct NVCLOTH_CLOTH_DESC
	{
		// [LSY] 이 Cloth 인스턴스가 공유할 쿠킹 결과다.
		NVCLOTH_FABRIC_HANDLE hFabric{};

		// [LSY] Cloth 인스턴스의 초기 파티클 위치다. Fabric 쿠킹 위치와 달라도 되지만
		// 파티클 개수는 Fabric과 반드시 일치해야 한다.
		std::vector<DirectX::XMFLOAT3> vecPositions{};

		// [LSY] 인스턴스에서 사용할 파티클별 역질량이다. 0은 고정, 양수는 동적이다.
		std::vector<float> vecInverseMasses{};

		// [LSY] 시뮬레이션 중력 가속도다.
		DirectX::XMFLOAT3 vGravity{ 0.f, -9.81f, 0.f };

		// [LSY] 축별 속도 감쇠율이다. 높을수록 움직임이 빨리 잦아들지만 펄럭임도 줄어든다.
		DirectX::XMFLOAT3 vDamping{ 0.05f, 0.05f, 0.05f };

		// [LSY] Cloth 기준 Transform의 선형 이동이 파티클 관성에 반영되는 비율이다.
		// 0이면 기준점 이동을 관성으로 전달하지 않고, 1이면 전부 전달한다.
		DirectX::XMFLOAT3 vLinearInertia{ 1.f, 1.f, 1.f };

		// [LSY] Cloth 기준 Transform의 회전이 파티클 관성에 반영되는 비율이다.
		DirectX::XMFLOAT3 vAngularInertia{ 1.f, 1.f, 1.f };

		// [LSY] 기준 Transform 회전에서 발생하는 원심 관성의 축별 반영 비율이다.
		DirectX::XMFLOAT3 vCentrifugalInertia{ 1.f, 1.f, 1.f };

		// [LSY] 초당 Solver 반복 목표 빈도다. 높을수록 안정적이지만 연산 비용이 증가한다.
		float fSolverFrequency{ 120.f };

		// [LSY] 제약 강성을 시간 간격에 맞게 평가하는 기준 빈도다.
		float fStiffnessFrequency{ 120.f };

		// [LSY] Fabric의 각 Phase에 적용할 기본 제약 강성이다.
		float fPhaseStiffness{ 1.f };

		// [LSY] 압축·신장 한계를 벗어난 제약에 추가로 적용할 강성 배율이다.
		float fPhaseStiffnessMultiplier{ 1.f };

		// [LSY] 제약의 허용 압축 비율이다. 1은 원래 길이를 기준으로 한다.
		float fCompressionLimit{ 1.f };

		// [LSY] 제약의 허용 신장 비율이다. 1은 원래 길이를 기준으로 한다.
		float fStretchLimit{ 1.f };

		// [LSY] 애니메이션 Motion Constraint 구체가 파티클을 제한하는 강도다.
		float fMotionConstraintStiffness{ 1.f };

		// [LSY] 동적 삼각형 중심에 충돌 계산 전용 가상 파티클을 추가한다.
		// 실제 시뮬레이션·렌더 파티클 수를 늘리지 않고 충돌 누락을 줄이지만
		// 충돌 검사 비용은 증가한다.
		bool bUseVirtualParticles{};
	};

	struct NVCLOTH_ANIMATION_CONSTRAINT_DESC
	{
		// [LSY] 현재 애니메이션으로 계산한 Cloth 시뮬레이션 로컬 공간의 목표 위치다.
		// 아래 두 배열은 Cloth 파티클 수와 정확히 일치해야 한다.
		std::vector<DirectX::XMFLOAT3> vecTargetPositions{};

		// [LSY] 각 파티클이 애니메이션 목표 위치에서 벗어날 수 있는 최대 반경이다.
		// 0이면 목표 위치에 고정되고, 값이 클수록 물리적으로 자유롭게 움직인다.
		std::vector<float> vecMaxDistances{};

		// [LSY] 선택적인 파티클별 Backstop 구체 정보다. Cloth 시뮬레이션 로컬 공간이며
		// 파티클은 해당 Separation 구체 내부로 들어갈 수 없다.
		// 두 배열은 모두 비어 있거나 Cloth 파티클 수와 정확히 일치해야 한다.
		std::vector<DirectX::XMFLOAT3>
			vecSeparationCenters{};
		std::vector<float> vecSeparationRadii{};

		// [LSY] 시뮬레이션 전에 역질량이 0인 고정 파티클을 애니메이션 목표로 이동시킨다.
		bool bUpdateFixedParticles{ true };

		// [LSY] 고정 파티클의 Previous 위치도 목표 위치로 맞춰 가짜 속도를 제거한다.
		// 최초 프레임이나 순간이동 직후에만 사용해야 정상 부착 이동의 관성이 유지된다.
		bool bResetPreviousParticles{};
	};

	struct NVCLOTH_COLLISION_SPHERE
	{
		// [LSY] Cloth 시뮬레이션 로컬 공간의 구 중심과 반지름이다.
		DirectX::XMFLOAT3 vCenter{};
		float fRadius{};
	};

	struct NVCLOTH_COLLISION_CAPSULE
	{
		// [LSY] vecSpheres의 두 구를 연결해 캡슐을 구성하는 인덱스다.
		// 두 구 사이의 선분 영역도 충돌 영역에 포함된다.
		uint32_t iSphere0{};
		uint32_t iSphere1{};
	};

	struct NVCLOTH_COLLISION_PLANE
	{
		// [LSY] Cloth 시뮬레이션 로컬 공간의 평면 방정식이다.
		// dot(vNormal, position) + fDistance = 0 형태로 사용한다.
		DirectX::XMFLOAT3 vNormal{ 0.f, 1.f, 0.f };
		float fDistance{};
	};

	struct NVCLOTH_COLLISION_CONVEX
	{
		// [LSY] i번째 비트가 vecPlanes[i] 포함 여부를 나타낸다.
		// 32비트 평면 마스크로 Convex를 표현하므로 최대 32개 평면을 사용할 수 있다.
		uint32_t iPlaneMask{};
	};

	struct NVCLOTH_COLLISION_DESC
	{
		// [LSY] 한 Cloth에 적용할 충돌 기본 도형 목록이다.
		std::vector<NVCLOTH_COLLISION_SPHERE> vecSpheres{};
		std::vector<NVCLOTH_COLLISION_CAPSULE> vecCapsules{};
		std::vector<NVCLOTH_COLLISION_PLANE> vecPlanes{};
		std::vector<NVCLOTH_COLLISION_CONVEX> vecConvexes{};

		// [LSY] 이전·현재 위치를 함께 사용해 빠르게 움직이는 충돌체의 관통을 줄인다.
		// 활성화하면 충돌 안정성이 좋아지지만 비용이 증가한다.
		bool bContinuousCollision{ true };

		// [LSY] 충돌 해결 중 적용하는 파티클 질량 보정값이다.
		// 높이면 충돌 수렴에 도움이 되지만 지나치면 비물리적인 반응이 발생할 수 있다.
		float fCollisionMassScale{ 10.f };

		// [LSY] Cloth와 충돌 도형 사이의 마찰 계수다. 0은 마찰 없음, 1은 최대값이다.
		float fFriction{ 0.2f };
	};

	struct NVCLOTH_RENDER_PARTICLE_VIEW
	{
		// [LSY] 매니저가 소유하며 렌더 구간에 빌려 쓰는 SRV다. 호출자가 Release하면 안 된다.
		// DX11 Cloth는 네이티브 파티클 버퍼를 사용하고, CPU Cloth는 시뮬레이션 스텝마다
		// GPU 버퍼로 업로드한다. 셰이더는 i번째 파티클을 i * 16 바이트에서 읽는다.
		ID3D11ShaderResourceView* pSRV{};
		uint32_t iParticleCount{};
	};
}
