#pragma once

namespace Engine
{
	inline float Randf(float min, float max)
	{
		return min +
			(max - min) *
			(rand() / (float)RAND_MAX);
	}

	inline int RandInt(int min, int max)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());

		std::uniform_int_distribution<int> dist(min, max);
		return dist(gen);
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

	
}

