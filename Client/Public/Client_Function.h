#pragma once

#include <string>
#include <filesystem>

namespace Client
{
	inline std::string MakeStaticModelResourceTag(const std::filesystem::path& rootPath, const std::filesystem::path& binPath)
	{
		std::filesystem::path relativePath = binPath.lexically_relative(rootPath);
		if (relativePath.empty())
		{
			relativePath = binPath.filename();
		}

		relativePath.replace_extension();

		std::string resourceTag = relativePath.string();
		for (char& ch : resourceTag)
		{
			const unsigned char value = static_cast<unsigned char>(ch);
			if (!std::isalnum(value))
			{
				ch = '_';
			}
		}

		return resourceTag;
	}

	inline void MultiByteCharToWstringClient(const std::string& inStr, std::wstring& outWStr)
	{
		// 1. 입력 문자열이 비어있으면 바로 빈 문자열 처리
		if (inStr.empty())
		{
			outWStr = L"";
			return;
		}

		// 2. 변환에 필요한 길이(버퍼 크기) 계산
		// 시스템 기본 인코딩(MBCS)인 경우 CP_ACP 사용 (만약 inStr이 UTF-8이면 CP_UTF8로 변경)
		int sizeNeeded = MultiByteToWideChar(CP_ACP, 0, &inStr[0], (int)inStr.size(), NULL, 0);

		// 3. 계산된 길이만큼 wstring 크기 할당
		outWStr.assign(sizeNeeded, 0);

		// 4. 실제 문자열 변환 진행
		MultiByteToWideChar(CP_ACP, 0, &inStr[0], (int)inStr.size(), &outWStr[0], sizeNeeded);
	}

}
