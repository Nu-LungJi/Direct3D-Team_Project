#pragma once
#include <concepts>
#include <type_traits>

// [LSY] CEngineBase 계층 밖의 보조 기반 타입에 고유 런타임 타입 ID를 부여한다.
#define DECLARE_RUNTIME_TYPE(Type) \
    using RuntimeTypeOwner = Type; \
    static constexpr Engine::_string_id StaticType = Engine::RuntimeTypeID<Type>();

// [LSY] 기존 단일 주 상속 계층은 IsA 체인을 그대로 사용한다.
#define DECLARE_DERIVED_TYPE(ChildType, ParentType) \
    using RuntimeTypeOwner = ChildType; \
    static constexpr Engine::_string_id StaticType = Engine::STRID(#ChildType); \
    bool IsA(Engine::_string_id id) const override { \
        static_assert(std::same_as<ChildType, std::remove_cvref_t<decltype(*this)>>, \
            "DECLARE_DERIVED_TYPE ChildType must match the declaring class."); \
        if (id == StaticType) return true; \
        return ParentType::IsA(id); \
    } \
    Engine::_string_id GetType() const override { return StaticType; } \
    Engine::_string_view GetTypeString() const override { return #ChildType; }

// [LSY] 주 상속 계층과 함께 조회할 보조 기반 타입들을 등록한다.
#define DECLARE_DERIVED_TYPE_WITH_BASES(ChildType, ParentType, ...) \
    DECLARE_DERIVED_TYPE(ChildType, ParentType) \
    void* QueryAdditionalType(Engine::_string_id id) noexcept override { \
        if (void* pType = Engine::QueryAdditionalTypes<ChildType, __VA_ARGS__>(this, id)) \
            return pType; \
        return ParentType::QueryAdditionalType(id); \
    } \
    const void* QueryAdditionalType(Engine::_string_id id) const noexcept override { \
        if (const void* pType = Engine::QueryAdditionalTypes<ChildType, __VA_ARGS__>(this, id)) \
            return pType; \
        return ParentType::QueryAdditionalType(id); \
    }

namespace Engine
{
	class CEngineBase;

	template<typename T>
	consteval _string_id RuntimeTypeID()
	{
		// [LSY] namespace까지 포함된 컴파일러 타입명을 사용해 동일한 짧은 클래스명 충돌을 피한다.
		// 런타임 조회 전용 ID이며 파일·네트워크 데이터로 영속화하지 않는다.
		return STRID(__FUNCSIG__);
	}

	template<typename T>
	concept RuntimeType = requires
	{
		typename T::RuntimeTypeOwner;
		{ T::StaticType } -> std::convertible_to<_string_id>;
	} && std::same_as<T, typename T::RuntimeTypeOwner>;

	template<typename TOwner, typename TAdditional, typename... TRest>
	void* QueryAdditionalTypes(TOwner* pOwner, _string_id id) noexcept
	{
		static_assert(RuntimeType<TAdditional>,
			"Additional base types must declare DECLARE_RUNTIME_TYPE.");
		static_assert(!std::is_base_of_v<CEngineBase, TAdditional>,
			"Additional base types must not derive from CEngineBase.");
		static_assert(std::is_convertible_v<TOwner*, TAdditional*>,
			"Additional types must be public, unambiguous bases of the owner.");

		if (id == TAdditional::StaticType)
			return static_cast<TAdditional*>(pOwner);

		if constexpr (sizeof...(TRest) > 0)
			return QueryAdditionalTypes<TOwner, TRest...>(pOwner, id);

		return nullptr;
	}

	template<typename TOwner, typename TAdditional, typename... TRest>
	const void* QueryAdditionalTypes(const TOwner* pOwner, _string_id id) noexcept
	{
		static_assert(RuntimeType<TAdditional>,
			"Additional base types must declare DECLARE_RUNTIME_TYPE.");
		static_assert(!std::is_base_of_v<CEngineBase, TAdditional>,
			"Additional base types must not derive from CEngineBase.");
		static_assert(std::is_convertible_v<const TOwner*, const TAdditional*>,
			"Additional types must be public, unambiguous bases of the owner.");

		if (id == TAdditional::StaticType)
			return static_cast<const TAdditional*>(pOwner);

		if constexpr (sizeof...(TRest) > 0)
			return QueryAdditionalTypes<TOwner, TRest...>(pOwner, id);

		return nullptr;
	}

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
		using RuntimeTypeOwner = CEngineBase;
		static constexpr _string_id StaticType = STRID("CEngineBase");
		virtual _string_id GetType() const { return StaticType; }
		virtual _string_view GetTypeString() const { return "CEngineBase"; }
		virtual bool IsA(_string_id id) const { return id == StaticType; }
		// [LSY] 다중 상속 포인터 오프셋이 적용된 실제 보조 기반 타입 주소를 반환한다.
		virtual void* QueryAdditionalType(_string_id) noexcept { return nullptr; }
		virtual const void* QueryAdditionalType(_string_id) const noexcept { return nullptr; }

		template<RuntimeType T>
		bool Is() const
		{
			if constexpr (std::derived_from<T, CEngineBase>)
				return IsA(T::StaticType);

			return QueryAdditionalType(T::StaticType) != nullptr;
		}
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


	template<RuntimeType T>
	T* Cast(CEngineBase* obj)
	{
		if (!obj)
			return nullptr;

		if constexpr (std::derived_from<T, CEngineBase>)
		{
			if (!obj->IsA(T::StaticType))
				return nullptr;

			return static_cast<T*>(obj);
		}

		return static_cast<T*>(obj->QueryAdditionalType(T::StaticType));
	}

	template<RuntimeType T>
	const T* Cast(const CEngineBase* obj)
	{
		if (!obj)
			return nullptr;

		if constexpr (std::derived_from<T, CEngineBase>)
		{
			if (!obj->IsA(T::StaticType))
				return nullptr;

			return static_cast<const T*>(obj);
		}

		return static_cast<const T*>(
			obj->QueryAdditionalType(T::StaticType));
	}

	template<RuntimeType T, typename U>
		requires std::derived_from<U, CEngineBase>
	SPtr<T> Cast(const SPtr<U>& obj)
	{
		T* pAdjusted = Cast<T>(obj.get());
		if (!pAdjusted)
			return {};

		// [LSY] 보조 기반 타입의 실제 주소를 사용하면서 기존 control block과 deleter는 공유한다.
		return SPtr<T>{ obj, pAdjusted };
	}

	template<typename T, typename U>
		requires std::derived_from<T, CEngineBase> &&
			std::derived_from<U, CEngineBase>
	UPtr<T> Cast(UPtr<U>&& obj)
	{
		if (obj && obj->IsA(T::StaticType))
		{
			// 원본 UPtr에서 소유권을 포기(release)하고 새로운 타입의 UPtr로 감쌉니다.
			return UPtr<T>(static_cast<T*>(obj.release()));
		}
		// rvalue reference 매개변수는 원본 소유권을 즉시 이전하지 않으므로 실패 시 원본은 유지된다.
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
		requires std::derived_from<Derived, CEngineBase> &&
			std::derived_from<Base, CEngineBase>
	UPtr<Derived> engine_uptr_cast(UPtr<Base>&& base)
	{
		if (!Cast<Derived>(base.get()))
		{
			return nullptr;
		}
		return UPtr<Derived>(Cast<Derived>(base.release()));
	}

	template<typename Derived, typename Base>
		requires std::derived_from<Derived, CEngineBase> &&
			std::derived_from<Base, CEngineBase>
	UPtr<Derived> static_uptr_cast(UPtr<Base>&& base)
	{
		return UPtr<Derived>(static_cast<Derived*>(base.release()));
	}

	template<typename Derived, typename Base>
		requires std::derived_from<Derived, CEngineBase> &&
			std::derived_from<Base, CEngineBase>
	UPtr<Derived> dynamic_uptr_cast(UPtr<Base>&& base)
	{
		return engine_uptr_cast<Derived>(std::move(base));
	}
}

namespace CEngineBaseExample
{
	class CTest final : public Engine::CEngineBase
	{
	public:
		DECLARE_DERIVED_TYPE(CTest, Engine::CEngineBase)

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
	class CTest2 final : public Engine::CEngineBase
	{
	public:
		DECLARE_DERIVED_TYPE(CTest2, Engine::CEngineBase)
	};

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
				Engine::SPtr<Engine::CEngineBase> sPtr2 = Engine::Cast<Engine::CEngineBase>(sPtr1);
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
