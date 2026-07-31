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

