#pragma once

namespace Engine
{
	//namespace TypeDef
	//{
		typedef	signed char	_byte;
		typedef	unsigned char _ubyte;
		typedef bool _bool;
		typedef char _char;
		typedef wchar_t _tchar;
		typedef float _float;
		typedef double _double;
		typedef std::string _string;
		typedef std::string_view _string_view;
		typedef uint64_t _string_id;
		typedef std::wstring _wstring;
		typedef std::wstring_view _wstring_view;
		typedef uint64_t _wstring_id;

		/* 저장용 데이터 */
		typedef XMFLOAT2 _float2;
		typedef XMFLOAT3 _float3;
		typedef XMFLOAT4 _float4;
		typedef XMFLOAT4X4 _float4x4;

		typedef XMINT3 _int3;

		/* 연산용 데이터 */
		typedef XMVECTOR _vector;
		typedef FXMVECTOR _fvector;
		typedef GXMVECTOR _gvector;
		typedef HXMVECTOR _hvector;
		typedef CXMVECTOR _cvector;

		typedef XMMATRIX _matrix;
		typedef FXMMATRIX _fmatrix;
		typedef CXMMATRIX _cmatrix;

		typedef std::function<void()> _Func;
		




		constexpr _string_id STRID(const char* str)
		{
			_string_id hash = 14695981039346656037ull; // offset basis (64bit)
			while (*str != '\0') {
				hash ^= static_cast<uint8_t>(*str++);
				hash *= 1099511628211ull; // prime (64bit)
			}
			return hash;
		}

		constexpr _string_id STRID(const char* str, size_t len)
		{
			_string_id hash = 14695981039346656037ull;
			for (size_t i = 0; i < len; ++i) {
				hash ^= static_cast<uint8_t>(str[i]);
				hash *= 1099511628211ull;
			}
			return hash;
		}

		constexpr _string_id WSTRID(const wchar_t* str)
		{
			_string_id hash = 14695981039346656037ull;

			while (*str != L'\0')
			{
				wchar_t c = *str++;

				// wchar_t를 byte 단위로 분해 (constexpr-safe)
				for (size_t i = 0; i < sizeof(wchar_t); ++i)
				{
					uint8_t byte = (c >> (i * 8)) & 0xFF;
					hash ^= byte;
					hash *= 1099511628211ull;
				}
			}

			return hash;
		}

		constexpr _string_id WSTRID(const wchar_t* str, size_t len)
		{
			_string_id hash = 14695981039346656037ull;

			for (size_t j = 0; j < len; ++j)
			{
				wchar_t c = str[j];

				for (size_t i = 0; i < sizeof(wchar_t); ++i)
				{
					uint8_t byte = (c >> (i * 8)) & 0xFF;
					hash ^= byte;
					hash *= 1099511628211ull;
				}
			}

			return hash;
		}

//#define STRID_NODBG


		struct StringID
		{
			_string_id hash{};

#ifndef STRID_NODBG
			const _char* str;
		private:
			// 동적 문자열 소유용 스마트 포인터
			std::shared_ptr<_char[]> owned_str;
#endif // !STRID_NODBG


		public:
			StringID() = default;
			const _char* GetDbgStr() const
			{
#ifndef STRID_NODBG
				return str;
#else
				static thread_local char buffer[32];
				snprintf(buffer, sizeof(buffer), "%llu", hash);
				return buffer;
#endif
			}

			// 리터럴 문자열 생성자
			constexpr StringID(const _char* s)
				: hash(STRID(s))
#ifndef STRID_NODBG		
				, str(s), owned_str(nullptr) {}
#else
			{}
#endif

			// 동적 문자열 생성자 (std::wstring 또는 동적 wchar_t*)
			StringID(const _string& s)
				: hash(STRID(s.c_str(), s.size()))
			{
#ifndef STRID_NODBG
				size_t len = s.size() + 1;
				owned_str = std::make_shared<_char[]>(len);
				std::copy(s.c_str(), s.c_str() + len, owned_str.get());
				str = owned_str.get();
#endif
			}

			// 임시 C 스타일 문자열로부터 생성
			StringID(const _char* s, size_t len)
				: hash(STRID(s, len))
			{
#ifndef STRID_NODBG
				owned_str = std::make_shared<_char[]>(len + 1);
				std::copy(s, s + len, owned_str.get());
				owned_str[len] = L'\0';
				str = owned_str.get();
#endif
			}

			StringID(std::basic_string_view<_char> sv)
				: hash(STRID(sv.data(), sv.size()))
			{
#ifndef STRID_NODBG
				owned_str = std::make_shared<_char[]>(sv.size() + 1);
				std::copy(sv.begin(), sv.end(), owned_str.get());
				owned_str[sv.size()] = 0;
				str = owned_str.get();
#endif
			}

			template<typename E>
				requires std::is_enum_v<E>
			StringID(E e)
				: StringID(magic_enum::enum_name(e))
			{
			}

			bool operator==(const StringID& other) const
			{
				return hash == other.hash;
			}

			bool operator<(const StringID& other) const
			{
				return hash < other.hash;
			}
		};

		struct WStringID
		{
			_wstring_id hash;

#ifndef STRID_NODBG
			const wchar_t* str;
		private:
			std::shared_ptr<wchar_t[]> owned_str;
#endif

		public:
			const wchar_t* GetDbgStr() const
			{
#ifndef STRID_NODBG
				return str;
#else
				 static thread_local wchar_t buffer[32];
				 swprintf(buffer, 32, L"%llu", hash);
				 return buffer;
#endif
			}

			
			constexpr WStringID(const wchar_t* s)
				: hash(WSTRID(s))
#ifndef STRID_NODBG
				, str(s), owned_str(nullptr) {}
#else
			{}
#endif

			
			WStringID(const std::wstring& s)
				: hash(WSTRID(s.c_str(), s.size()))
			{
#ifndef STRID_NODBG
				size_t len = s.size() + 1;
				owned_str = std::make_shared<wchar_t[]>(len);
				std::copy(s.c_str(), s.c_str() + len, owned_str.get());
				str = owned_str.get();
#endif
			}

			
			WStringID(const wchar_t* s, size_t len)
				: hash(WSTRID(s, len))
			{
#ifndef STRID_NODBG
				owned_str = std::make_shared<wchar_t[]>(len + 1);
				std::copy(s, s + len, owned_str.get());
				owned_str[len] = L'\0';
				str = owned_str.get();
#endif
			}

			bool operator==(const WStringID& other) const
			{
				return hash == other.hash;
			}

			bool operator<(const WStringID& other) const
			{
				return hash < other.hash;
			}
		};


}

namespace std
{
	template<>
	struct hash<Engine::StringID>
	{
		size_t operator()(const Engine::StringID& id) const noexcept {
			return static_cast<size_t>(id.hash);
		}
	};

	template<>
	struct hash<Engine::WStringID>
	{
		size_t operator()(const Engine::WStringID& id) const noexcept {
			return static_cast<size_t>(id.hash);
		}
	};
}


namespace std {
	template<>
	struct hash<XMINT3> {
		size_t operator()(const XMINT3& v) const {
			size_t h = 0;
			h ^= hash<int>{}(v.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= hash<int>{}(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= hash<int>{}(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};
}

namespace DirectX {
	inline bool operator==(const XMINT3& a, const XMINT3& b) {
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}
}
