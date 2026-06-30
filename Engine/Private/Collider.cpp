#include "pch.h"
#include "Collider.h"
#include "CollBox.h"

NS_USING(Engine)

CCollider::CCollider(CollType eCollType)
	: m_eCollType(eCollType)
{
}

CCollider::~CCollider()
{
}

void CCollider::Free()
{
	//if (m_pInnerHint2)
	//{
	//	free(m_pInnerHint2);
	//}
	CEngineBase::Free();
}
