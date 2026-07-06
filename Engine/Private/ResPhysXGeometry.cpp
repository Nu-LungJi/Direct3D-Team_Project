#include "pch.h"
#include "ResPhysXGeometry.h"
#include "GameInstance.h"

#ifdef _DEBUG
// 라이브러리 설정 전후로 매크로 잠시 해제
#undef new
#endif

#include "PxPhysicsAPI.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif


using namespace physx;

NS_USING(Engine)

CResPhysXGeometry::CResPhysXGeometry(const _string& sPath)
	: CResource{ sPath }
{
}

CResPhysXGeometry::~CResPhysXGeometry()
{
}
