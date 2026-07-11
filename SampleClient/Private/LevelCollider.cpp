#include "pch.h"
#include "LevelCollider.h"
#include "GameInstance.h"
#include "TestCollider.h"
#include "ComCollider.h"

NS_USING(Client)

CLevelCollider::CLevelCollider()
{
}

CLevelCollider::~CLevelCollider()
{
}

HRESULT CLevelCollider::Initialize()
{
	CGameInstance::Get().GameObjectAllReset();
	//"LEVEL_COLLIDER", "Prototype_GameObject_TestCollider"

	{
		CComCollider::DESC col1{};
		col1.eCollType = CollType::Box;
		col1.vCenter = {0.f, 1.f, 0.f};
		col1.vExtents = {1.f, 2.f, 1.f};

		CComCollider::DESC col2{};
		col2.eCollType = CollType::Sphere;
		col2.vCenter = {2.1f, 0.f, 0.f};
		col2.fRadius = 1.f;
		CComCollider::DESC col3{};
		col3.eCollType = CollType::Sphere;
		col3.vCenter = {-2.1f, 0.f, 0.f };
		col3.fRadius = 1.f;
		CComCollider::DESC col4{};
		col4.eCollType = CollType::Sphere;
		col4.vCenter = { 0.f, 0.f, 2.1f };
		col4.fRadius = 1.f;
		CComCollider::DESC col5{};
		col5.eCollType = CollType::Sphere;
		col5.vCenter = { 0.f, 0.f, -2.1f };
		col5.fRadius = 1.f;
		CTestCollider::DESC Desc{};
		Desc.bIsController = true;
		Desc.CollGroupID = "Coll_Tests";
		Desc.collInfos = { {"ComCollider1", col1},  {"ComCollider2", col2},  {"ComCollider3", col3},  {"ComCollider4", col4},  {"ComCollider5", col5} };
		Desc.sObjectTag = "ControllerTestColl";
		if (!(E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_COLLIDER", "Prototype_GameObject_TestCollider",
			"00_COLLIDERS", &Desc)))
		{
			return E_FAIL;
		}
	}


	{
		CComCollider::DESC col1{};
		col1.eCollType = CollType::Box;
		col1.vCenter = {};
		col1.vExtents = { 1.f, 1.f, 1.f };

		CTestCollider::DESC Desc{};
		Desc.CollGroupID = "Coll_Tests";
		Desc.collInfos = { {"ComCollider1", col1} };
		Desc.sObjectTag = "TestColl";
		if (auto handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_COLLIDER", "Prototype_GameObject_TestCollider",
			"01_COLLIDERS", &Desc))
		{
			if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(handle.value()))
			{
				pObj->GetTransform().SetPosition(_float3{ 3.f, 3.f, 3.f });
			}
		}
	}

	{
		CComCollider::DESC col1{};
		col1.eCollType = CollType::OrientedBox;
		col1.vCenter = {};
		col1.vExtents = { 1.f, 1.f, 1.f };
		XMStoreFloat4(&col1.quatOritented, XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, XMConvertToRadians(45.f), 0.f, 0.f)));

		CTestCollider::DESC Desc{};
		Desc.CollGroupID = "Coll_Tests";
		Desc.collInfos = { {"ComCollider1", col1} };
		Desc.sObjectTag = "TestColl";
		if (auto handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_COLLIDER", "Prototype_GameObject_TestCollider",
			"01_COLLIDERS", &Desc))
		{
			if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(handle.value()))
			{
				pObj->GetTransform().SetPosition(_float3{ -3.f, 3.f, 3.f });
			}
		}
	}

	{
		CComCollider::DESC col1{};
		col1.eCollType = CollType::Sphere;
		col1.vCenter = {};
		col1.fRadius = 1.f;

		CTestCollider::DESC Desc{};
		Desc.CollGroupID = "Coll_Tests";
		Desc.collInfos = { {"ComCollider1", col1} };
		Desc.sObjectTag = "TestColl";
		if (auto handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_COLLIDER", "Prototype_GameObject_TestCollider",
			"01_COLLIDERS", &Desc))
		{
			if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(handle.value()))
			{
				pObj->GetTransform().SetPosition(_float3{ -3.f, 3.f, -3.f });
			}
		}
	}

	{
		CComCollider::DESC col1{};
		col1.eCollType = CollType::Frustum;
		col1.vCenter = {};
		//col1.fRadius = 1.f;
		XMStoreFloat4x4(
			&col1.matFrustum,
			XMMatrixPerspectiveFovLH(
				XMConvertToRadians(45.f),
				1.f,
				0.1f,
				10.f));

		CTestCollider::DESC Desc{};
		Desc.CollGroupID = "Coll_Tests";
		Desc.collInfos = { {"ComCollider1", col1} };
		Desc.sObjectTag = "TestColl";
		if (auto handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_COLLIDER", "Prototype_GameObject_TestCollider",
			"01_COLLIDERS", &Desc))
		{
			if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(handle.value()))
			{
				pObj->GetTransform().SetPosition(_float3{ 3.f, 3.f, -3.f });
			}
		}
	}

	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
		Desc.vAt = { 0.f, 0.f, 0.f };
		Desc.vEye = { 0.f, 0.f, -5.f };
		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
		Desc.fFovY = 75.f;
		Desc.fNear = 0.1f;
		Desc.fFar = 100.f;
		Desc.sObjectTag = "FlyCam";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
			{
				MSG_BOX("MSG_BOX_123");
			}
			E::CGameInstance::Get().SetActiveCamera("FLY");
		}
	}
	return S_OK;
}

void CLevelCollider::Update(_float fTimeDelta)
{

}

HRESULT CLevelCollider::Render()
{
	return S_OK;
}

void CLevelCollider::UpdateGUI()
{
}

void CLevelCollider::FrameStart(_float fTimeDelta)
{
}

UPtr<CLevelCollider> CLevelCollider::Create()
{
	auto	pInstance = Engine::UPtr<CLevelCollider>(new CLevelCollider{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelCollider");
	}

	return pInstance;
}

void CLevelCollider::Free()
{
	CLevel::Free();
}
