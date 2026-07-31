#pragma once

namespace Engine
{
	inline void LogMemoryUsageImpl(const char* file, int line, const char* func, const char* tag = "")
	{
		PROCESS_MEMORY_COUNTERS_EX pmc{};
		if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
		{
			SYSTEMTIME st;
			GetLocalTime(&st);

			const char* fileName = strrchr(file, '\\');
			if (!fileName) fileName = strrchr(file, '/');
			fileName = fileName ? fileName + 1 : file;

			const double MB = 1024.0 * 1024.0;
			char buf[1024];
			sprintf_s(buf,
				"[%02d:%02d:%02d.%03d] [TID:%04lu] [MEM%s%s]\n"
				"  -> Loc : %s(%d) - %s()\n"
				"  -> Mem : WS: %8.2f MB (Peak: %8.2f MB) | Private: %8.2f MB | PF: %8.2f MB\n",
				st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
				GetCurrentThreadId(),
				tag[0] ? ":" : "", tag,
				fileName, line, func,
				pmc.WorkingSetSize / MB,
				pmc.PeakWorkingSetSize / MB,
				pmc.PrivateUsage / MB,
				pmc.PagefileUsage / MB
			);
			OutputDebugStringA(buf);
		}
	}

	template<class T>
	constexpr std::string_view MagicEnumToStringView(T&& t)
	{
		if constexpr (std::is_enum_v<std::remove_cvref_t<T>>)
			return magic_enum::enum_name(t);
		else
			return std::string_view(t);
	}

	template<class T>
	constexpr std::string_view MagicEnumToStringView(const T&& t)
	{
		if constexpr (std::is_enum_v<std::remove_cvref_t<T>>)
			return magic_enum::enum_name(t);
		else
			return std::string_view(t);
	}

	inline _float4 ColorIntToFloat4(int c) {
		_float4 color;
		color.x = ((c >> 0) & 0xFF) / 255.0f; // R
		color.y = ((c >> 8) & 0xFF) / 255.0f; // G
		color.z = ((c >> 16) & 0xFF) / 255.0f; // B
		color.w = ((c >> 24) & 0xFF) / 255.0f; // A
		return color;
	}

	// LSY 변경: Look 방향과 Up이 평행하거나 입력 벡터가 유효하지 않아
	// View 행렬의 Right 축을 만들 수 없는 경우를 공통으로 방어한다.
	inline _matrix MakeSafeLookToLH(
		DirectX::FXMVECTOR vEye,
		DirectX::FXMVECTOR vDirection,
		DirectX::FXMVECTOR vPreferredUp)
	{
		constexpr _float MIN_VECTOR_LENGTH_SQ = 0.000001f;
		constexpr _float PARALLEL_DOT_THRESHOLD = 0.999f;

		DirectX::XMVECTOR vSafeDirection = vDirection;
		if (DirectX::XMVector3IsNaN(vSafeDirection) ||
			DirectX::XMVector3IsInfinite(vSafeDirection) ||
			DirectX::XMVectorGetX(
				DirectX::XMVector3LengthSq(vSafeDirection)) <=
			MIN_VECTOR_LENGTH_SQ)
		{
			vSafeDirection =
				DirectX::XMVectorSet(0.f, -1.f, 0.f, 0.f);
		}

		vSafeDirection =
			DirectX::XMVector3Normalize(vSafeDirection);

		DirectX::XMVECTOR vSafeUp = vPreferredUp;
		if (DirectX::XMVector3IsNaN(vSafeUp) ||
			DirectX::XMVector3IsInfinite(vSafeUp) ||
			DirectX::XMVectorGetX(
				DirectX::XMVector3LengthSq(vSafeUp)) <=
			MIN_VECTOR_LENGTH_SQ)
		{
			vSafeUp =
				DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
		}

		vSafeUp = DirectX::XMVector3Normalize(vSafeUp);

		const _float fDirectionUpDot = fabsf(
			DirectX::XMVectorGetX(
				DirectX::XMVector3Dot(
					vSafeDirection,
					vSafeUp)));

		if (fDirectionUpDot >= PARALLEL_DOT_THRESHOLD)
		{
			const DirectX::XMVECTOR vWorldUp =
				DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
			const _float fDirectionWorldUpDot = fabsf(
				DirectX::XMVectorGetX(
					DirectX::XMVector3Dot(
						vSafeDirection,
						vWorldUp)));

			vSafeUp =
				fDirectionWorldUpDot < PARALLEL_DOT_THRESHOLD
				? vWorldUp
				: DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f);
		}

		return DirectX::XMMatrixLookToLH(
			vEye,
			vSafeDirection,
			vSafeUp);
	}

	inline _matrix MakeSafeLookAtLH(
		DirectX::FXMVECTOR vEye,
		DirectX::FXMVECTOR vTarget,
		DirectX::FXMVECTOR vPreferredUp)
	{
		return MakeSafeLookToLH(
			vEye,
			DirectX::XMVectorSubtract(vTarget, vEye),
			vPreferredUp);
	}

	inline float Randf(float min, float max)
	{
		if (min > max) std::swap(min, max);

		thread_local std::random_device rd;
		thread_local std::mt19937 gen(rd());

		std::uniform_real_distribution<float> dist(min, max);
		return dist(gen);
	}

	inline int RandInt(int min, int max)
	{
		if (min > max) std::swap(min, max);

		thread_local std::random_device rd;
		thread_local std::mt19937 gen(rd());

		std::uniform_int_distribution<int> dist(min, max);
		return dist(gen);
	}

	inline float Hash01(uint32_t x)
	{
		x = (x ^ 61u) ^ (x >> 16u);
		x *= 9u;
		x = x ^ (x >> 4u);
		x *= 0x27d4eb2du;
		x = x ^ (x >> 15u);

		// x는 uint32_t라 (float)x * (1/2^32) 결과가 이미 [0, 1) 범위이므로
		// HLSL의 frac() 호출이 CPU에서는 필요 없음
		return (float)x * 2.3283064365386963e-10f; // 1 / 2^32
	}
	template<typename T>
	constexpr int32_t ETOI(T e)
	{
		return static_cast<int32_t>(e);
	}

	template<typename T>
	constexpr uint32_t ETOUI(T e)
	{
		return static_cast<uint32_t>(e);
	}

	template<typename T>
	void	Safe_Delete(T& Pointer)
	{
		if (nullptr != Pointer)
		{
			delete Pointer;
			Pointer = nullptr;
		}
	}

	template<typename T>
	void	Safe_Delete_Array(T& Pointer)
	{
		if (nullptr != Pointer)
		{
			delete[] Pointer;
			Pointer = nullptr;
		}
	}

	template<typename T>
	unsigned int Safe_AddRef(T& pInstance)
	{
		unsigned int	iRefCnt = 0;

		if (nullptr != pInstance)
			iRefCnt = pInstance->AddRef();
		return iRefCnt;
	}

	template<typename T>
	unsigned int Safe_Release(T& pInstance)
	{
		unsigned int	iRefCnt = 0;

		if (nullptr != pInstance)
		{
			iRefCnt = pInstance->Release();

			if (0 == iRefCnt)
				pInstance = nullptr;
		}

		return iRefCnt;
	}

	inline std::string WStringToString(const std::wstring& wstr) {
		if (wstr.empty()) return "";
		int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(sizeNeeded, 0);
		WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
		return strTo;
	}

	inline std::wstring StringToWString(const std::string& str)
	{
		if (str.empty()) return L"";
		int sizeNeeded = MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), nullptr, 0);
		std::wstring wstrTo(sizeNeeded, 0);
		MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), &wstrTo[0], sizeNeeded);
		return wstrTo;
	}

	inline std::string WStringToUTF8(const std::wstring& wstr)
	{
		if (wstr.empty()) return std::string();

		// WideChar -> UTF-8 변환
		int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
		std::string strTo(sizeNeeded, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &strTo[0], sizeNeeded, nullptr, nullptr);

		return strTo;
	}

	inline std::wstring StringToWUTF8(const std::string& str)
	{
		if (str.empty()) return L"";

		// CP_ACP 대신 CP_UTF8로 변경해야 한글 UTF-8 데이터를 올바르게 UTF-16으로 변환합니다.
		int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
		std::wstring wstrTo(sizeNeeded, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstrTo[0], sizeNeeded);

		return wstrTo;
	}
}

