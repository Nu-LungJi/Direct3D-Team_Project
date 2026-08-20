#include "pch.h"
#include "AnimationObject.h"

#include "ComAnimator.h"
#include "ComModelInstance.h"
#include "GameInstance.h"

NS_USING(Engine)

CAnimationObject::CAnimationObject(): CGameObject{}
{
}

CAnimationObject::CAnimationObject(const CAnimationObject& Prototype): CGameObject{ Prototype }
{


}

CAnimationObject::~CAnimationObject()
{
}

HRESULT CAnimationObject::Initialize(
	void* pArg)
{
	if (FAILED(
		CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	return S_OK;
}

void CAnimationObject::Update(_float fTimeDelta)
{
	CGameObject::Update(fTimeDelta);

}

void CAnimationObject::UpdateRenderVisibility(const BoundingBox& WorldBounds)
{


	const CCameraObject* MainCamera = CGameInstance::Get().GetActiveCamera();

	/*
	* 입력 Bounds를 직접 수정하지 않고 복사한다.
	*/
	BoundingBox MainViewBounds = WorldBounds;
	BoundingBox ShadowBounds = WorldBounds;

	const _float MaxExtent = std::max({
		WorldBounds.Extents.x,
		WorldBounds.Extents.y,
		WorldBounds.Extents.z
		});

	/*
	 * 작은 캐릭터도 최소 여유 범위를 보장하고,
	 * 큰 캐릭터는 크기에 비례해 Padding을 늘린다.
	 */
	const _float MainPadding =
		std::max(0.5f, MaxExtent * 0.1f);

	const _float ShadowPadding =
		std::max(1.f, MaxExtent * 0.25f);

	MainViewBounds.Extents.x += MainPadding;
	MainViewBounds.Extents.y += MainPadding;
	MainViewBounds.Extents.z += MainPadding;

	ShadowBounds.Extents.x += ShadowPadding;
	ShadowBounds.Extents.y += ShadowPadding;
	ShadowBounds.Extents.z += ShadowPadding;

	/*
	  1. Main View 판정
	 */

	m_bMainViewVisible =
		MainCamera == nullptr ||
		MainCamera->IntersectsViewVolume(WorldBounds);

	/*
	  2. Shadow Cascade 판정
	*/


	m_bShadowVisible = false;


	const CSM_DATA& CSM =
		CGameInstance::Get().Get_MainDirectionalLightData();

	if (!CSM.m_pLightHandle || !CSM.m_pShadowSRV)
		return;


	for(uint32_t i = 0; i < MAX_CASCADE_COUNT; ++i)
	{
		const _matrix CascadeViewProj = CGameInstance::Get().Get_CascadeShadowViewProj(i);

		if (IntersectsClipVolume(
			WorldBounds,
			CascadeViewProj))
		{
			m_bShadowVisible = true;
			break;
		}
	}



}

_bool CAnimationObject::IntersectsClipVolume(const BoundingBox& WorldBounds, _fmatrix ViewProj)
{
	_float4x4 VM{};
	XMStoreFloat4x4(&VM, ViewProj);

	_float4 Planes[6]{
		{
			// Left: x + w >= 0

			VM._11 + VM._14,
			VM._21 + VM._24,
			VM._31 + VM._34,
			VM._41 + VM._44
		},


		{
			// Right: -x + w >= 0
			-VM._11 + VM._14,
			-VM._21 + VM._24,
			-VM._31 + VM._34,
			-VM._41 + VM._44
		},

		{
			// Top: -y + w >= 0
			-VM._12 + VM._14,
			-VM._22 + VM._24,
			-VM._32 + VM._34,
			-VM._42 + VM._44
		},
		{
			// Bottom: y + w >= 0
			VM._12 + VM._14,
			VM._22 + VM._24,
			VM._32 + VM._34,
			VM._42 + VM._44
		},
		{
			// Near: z >= 0
			VM._13,
			VM._23,
			VM._33,
			VM._43
		},
		{
			// Far: -z + w >= 0
			-VM._13 + VM._14,
			-VM._23 + VM._24,
			-VM._33 + VM._34,
			-VM._43 + VM._44
		}

	};


	const _float3 Center = WorldBounds.Center;
	const _float3 Extent = WorldBounds.Extents; 

	for (_float4& Plane : Planes)
	{
		const _float Length = sqrtf(Plane.x * Plane.x + Plane.y * Plane.y + Plane.z * Plane.z);

		if(Length <= FLT_EPSILON)
			continue;

		Plane.x /= Length;
		Plane.y /= Length;
		Plane.z /= Length;
		Plane.w /= Length;

		/*
		* AABB를 평면 법선에 투영한 반지름.
		*/

		const _float Radius = Extent.x * fabsf(Plane.x) + Extent.y * fabsf(Plane.y) + Extent.z * fabsf(Plane.z);
		const _float Distance =Center.x * Plane.x +Center.y * Plane.y +Center.z * Plane.z +Plane.w;

		/*
		 * 박스 전체가 평면 바깥이면 Clip Volume과 겹치지 않는다.
		 */
		if (Distance + Radius < 0.f)
			return false;
	}


	return true;
}
