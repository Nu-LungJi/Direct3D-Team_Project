#pragma once
#define DECLARE_DERIVED_TYPE(ChildType, ParentType) \
    static constexpr Engine::_string_id StaticType = Engine::STRID(#ChildType); \
    bool IsA(Engine::_string_id id) const override { \
        if (id == StaticType) return true; \
        return ParentType::IsA(id); \
    } \
    Engine::_string_id GetType() const override { return StaticType; } \
    Engine::_string_view GetTypeString() const override { return #ChildType; }

namespace Engine
{
	class _declspec(dllexport) CEngineBase
	{
		// 만약 Free()를 public으로 한다면 friend는 필요 없음
		// 하지만 CEngineBase를 상속받아 만드는것들은 스마트포인터로 Create한다 가정하므로
		// Free()를 public으로 만들 필요 없다 판단하여 protected로 만듦
		friend struct tagEngineBaseDeleter;
	protected:
		CEngineBase() = default;
		CEngineBase(const CEngineBase&) = default;
		virtual ~CEngineBase() {};
	protected:
		virtual void Free() { delete this; }
	public:
		static constexpr _string_id StaticType = STRID("CEngineBase");
		virtual _string_id GetType() const { return StaticType; }
		virtual _string_view GetTypeString() const { return "CEngineBase"; }
		virtual bool IsA(_string_id id) const { return id == StaticType; }
		template<typename T> requires std::derived_from<T, CEngineBase>
		bool Is() const { return IsA(T::StaticType); }
	};


	typedef struct _declspec(dllexport) tagEngineBaseDeleter
	{
		void operator()(CEngineBase* base) const { if (base) base->Free(); }
	} ENGINE_BASE_DELETER;

	template <typename T>
	using UPtr = std::unique_ptr<T, ENGINE_BASE_DELETER>;

	template <typename T>
	using SPtr = std::shared_ptr<T>;

	template <typename T>
	using WPtr = std::weak_ptr<T>;


	template<typename T>
	T* Cast(CEngineBase* obj)
	{
		if (obj && obj->IsA(T::StaticType))
		{
			return static_cast<T*>(obj);
		}
		return nullptr;
	}

	template<typename T, typename U>
	SPtr<T> Cast(const SPtr<U>& obj)
	{
		if (obj && obj->IsA(T::StaticType))
		{
			return std::static_pointer_cast<T>(obj); // SPtr 전용 캐스팅
		}
		return nullptr;
	}

	template<typename T, typename U>
	UPtr<T> Cast(UPtr<U>&& obj)
	{
		if (obj && obj->IsA(T::StaticType))
		{
			// 원본 UPtr에서 소유권을 포기(release)하고 새로운 타입의 UPtr로 감쌉니다.
			return UPtr<T>(static_cast<T*>(obj.release()));
		}
		// 주의: 캐스팅에 실패하면 nullptr이 반환되며, 원본 객체는 메모리에서 소멸됩니다!
		return nullptr;
	}

	template <typename T>
	UPtr<T> ToUPtr(T* p)
	{
		return UPtr<T>(p);
	}

	template <typename T>
	SPtr<T> ToSPtr(T* p)
	{
		return SPtr<T>(p, ENGINE_BASE_DELETER{});
	}

	template<typename Derived, typename Base>
	UPtr<Derived> static_uptr_cast(UPtr<Base>&& base)
	{
		return UPtr<Derived>(static_cast<Derived*>(base.release()));
	}

	template<typename Derived, typename Base>
	UPtr<Derived> dynamic_uptr_cast(UPtr<Base>&& base)
	{
		if (Derived* p = dynamic_cast<Derived*>(base.get()))
		{
			base.release();
			return UPtr<Derived>(p);
		}
		return nullptr;
	}
}

namespace CEngineBaseExample
{
	class CTest final : public Engine::CEngineBase
	{
	private:
		explicit CTest(uint32_t iData) :m_iData{ iData } {};
		~CTest() override = default;
	public:
		uint32_t GetData() const { return m_iData; }
	private:
		const uint32_t m_iData;
		// 만약 Free를 오버라이딩 한다면 부모Free도 호출 필요함
		void Free() override { Engine::CEngineBase::Free(); };
	public:
		static inline Engine::UPtr<CTest> CreateUPtr(uint32_t iData) { return Engine::ToUPtr(new CTest{ iData }); }
		static inline Engine::SPtr<CTest> CreateSPtr(uint32_t iData) { return Engine::ToSPtr(new CTest{ iData }); }
	};
	class CTest2 final : public Engine::CEngineBase {};

	inline void Example()
	{
		{
			Engine::UPtr<CTest> uPtr1{ CTest::CreateUPtr(1) };
			assert(uPtr1->GetData() == 1);
			Engine::UPtr<Engine::CEngineBase> uPtr2 = Engine::dynamic_uptr_cast<Engine::CEngineBase>(std::move(uPtr1));
			assert(uPtr2);
			Engine::UPtr<CTest> uPtr3 = Engine::static_uptr_cast<CTest>(std::move(uPtr2));

			// 캐스팅 실패 케이스
			Engine::UPtr<CTest2> uPtr4 = Engine::dynamic_uptr_cast<CTest2>(std::move(uPtr3));
			assert(uPtr4 == nullptr);
			assert(uPtr3->GetData() == 1);
		}

		Engine::WPtr<CTest> sPtrWeak{};
		{
			Engine::SPtr<CTest> sPtr{};
			{
				Engine::SPtr<CTest> sPtr1{ CTest::CreateSPtr(2) };
				assert(sPtr1->GetData() == 2);
				Engine::SPtr<Engine::CEngineBase> sPtr2 = std::dynamic_pointer_cast<Engine::CEngineBase>(sPtr1);
				Engine::SPtr<Engine::CEngineBase> sPtr3 = std::static_pointer_cast<Engine::CEngineBase>(sPtr2);
				sPtr = sPtr1;
			}
			assert(sPtr->GetData() == 2);
			sPtrWeak = sPtr;
			assert(sPtrWeak.expired() == false);
		}
		assert(sPtrWeak.expired() == true);
	}
}
