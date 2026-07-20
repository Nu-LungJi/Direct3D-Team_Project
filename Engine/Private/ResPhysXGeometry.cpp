#include "pch.h"
#include "ResPhysXGeometry.h"
#include "GameInstance.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")


using namespace physx;

NS_USING(Engine)

CResPhysXGeometry::CResPhysXGeometry(const _string& sPath)
	: CResource{ sPath }
{
}

CResPhysXGeometry::~CResPhysXGeometry()
{
}
