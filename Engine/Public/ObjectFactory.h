#pragma once
#include "Engine_Defines.h"
#include <unordered_map>
#include <functional>

NS_BEGIN(Engine)

class CEngineBase;

class ENGINE_DLL CObjectFactory
{
public:
	// 엔진 코어에서 싱글톤으로 관리
	static CObjectFactory* GetInstance()
	{
		static CObjectFactory instance;
		return &instance;
	}

	// 반환형은 프로젝트의 설계(Raw 포인터, shared_ptr, UPtr 등)에 맞게 조정하세요.
	using CreatorFunc = std::function<CEngineBase* ()>;

	// ===================================================
	// 1. 객체 생성기 등록 (엔진 초기화 시 호출)
	// ===================================================
	template<typename T>
	void RegisterType()
	{
		// 매크로가 만들어준 고유 해시값(StaticType)을 Key로 사용합니다!
		_string_id typeId = T::StaticType;

		m_creators[typeId] = []() -> CEngineBase* {
			/* [중요 포인트]
			  CEngineBase의 생성자가 protected이므로, 파생 클래스들의 생성자도
			  protected일 확률이 높습니다.
			  만약 `new T()`가 막혀있다면, 각 클래스에 구현해두신
			  정적 생성 함수(예: T::Create())를 호출하도록 수정하시면 됩니다.
			*/
			// return T::Create().release(); // (스마트 포인터 방식인 경우)
			return new T(); // (Raw 포인터 방식인 경우)
			};
	}

	// ===================================================
	// 2. 해시값을 이용한 동적 객체 생성 (역직렬화 시 호출)
	// ===================================================
	CEngineBase* CreateObject(_string_id typeId)
	{
		auto it = m_creators.find(typeId);
		if (it != m_creators.end())
		{
			// 맵에 등록된 생성 람다 함수를 호출하여 메모리를 할당합니다.
			return it->second();
		}

		// 등록되지 않은 타입이면 nullptr 반환
		return nullptr;
	}

private:
	CObjectFactory() = default;
	~CObjectFactory() = default;

	std::unordered_map<_string_id, CreatorFunc> m_creators;
};

NS_END
