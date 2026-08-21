#pragma once

#include "GameInstance.h"
#include "ResTexture2D.h"
#include <array>
#include <fstream>

NS_BEGIN(Client)

namespace UITextureResourceLoader
{
	struct TEXTURE_CANDIDATE
	{
		std::filesystem::path PNGPath{};
		std::filesystem::path DDSPath{};
	};

	inline std::string ToLowerExtension(const std::filesystem::path& path)
	{
		std::string extension = path.extension().string();
		std::ranges::transform(
			extension,
			extension.begin(),
			[](unsigned char character)
			{
				return static_cast<char>(std::tolower(character));
			});
		return extension;
	}

	inline uint32_t ReadDDSUInt32(
		const std::array<unsigned char, 148>& header,
		size_t offset)
	{
		return static_cast<uint32_t>(header[offset]) |
			(static_cast<uint32_t>(header[offset + 1u]) << 8u) |
			(static_cast<uint32_t>(header[offset + 2u]) << 16u) |
			(static_cast<uint32_t>(header[offset + 3u]) << 24u);
	}

	inline _bool IsBlockCompressedDDS(
		const std::array<unsigned char, 148>& header)
	{
		const uint32_t fourCC = ReadDDSUInt32(header, 84u);
		constexpr auto MakeFourCC = [](
			unsigned char a,
			unsigned char b,
			unsigned char c,
			unsigned char d) constexpr
		{
			return static_cast<uint32_t>(a) |
				(static_cast<uint32_t>(b) << 8u) |
				(static_cast<uint32_t>(c) << 16u) |
				(static_cast<uint32_t>(d) << 24u);
		};

		if (fourCC == MakeFourCC('D', 'X', 'T', '1') ||
			fourCC == MakeFourCC('D', 'X', 'T', '2') ||
			fourCC == MakeFourCC('D', 'X', 'T', '3') ||
			fourCC == MakeFourCC('D', 'X', 'T', '4') ||
			fourCC == MakeFourCC('D', 'X', 'T', '5') ||
			fourCC == MakeFourCC('A', 'T', 'I', '1') ||
			fourCC == MakeFourCC('A', 'T', 'I', '2') ||
			fourCC == MakeFourCC('B', 'C', '4', 'U') ||
			fourCC == MakeFourCC('B', 'C', '4', 'S') ||
			fourCC == MakeFourCC('B', 'C', '5', 'U') ||
			fourCC == MakeFourCC('B', 'C', '5', 'S'))
		{
			return true;
		}

		if (fourCC != MakeFourCC('D', 'X', '1', '0'))
			return false;

		const uint32_t dxgiFormat = ReadDDSUInt32(header, 128u);
		return (dxgiFormat >= 70u && dxgiFormat <= 84u) ||
			(dxgiFormat >= 94u && dxgiFormat <= 99u);
	}

	inline _bool IsDDSDimensionCompatible(
		const std::filesystem::path& ddsPath)
	{
		std::ifstream file(ddsPath, std::ios::binary);
		if (!file.is_open())
			return false;

		std::array<unsigned char, 148> header{};
		file.read(
			reinterpret_cast<char*>(header.data()),
			static_cast<std::streamsize>(header.size()));
		if (file.gcount() < 128 ||
			header[0] != 'D' || header[1] != 'D' ||
			header[2] != 'S' || header[3] != ' ')
		{
			return false;
		}

		if (!IsBlockCompressedDDS(header))
			return true;

		const uint32_t height = ReadDDSUInt32(header, 12u);
		const uint32_t width = ReadDDSUInt32(header, 16u);
		return width > 0u && height > 0u &&
			width % 4u == 0u && height % 4u == 0u;
	}

	inline std::string ResolvePreferredPath(
		const std::filesystem::path& sourcePath)
	{
		if (ToLowerExtension(sourcePath) == ".png")
		{
			std::filesystem::path ddsPath = sourcePath;
			ddsPath.replace_extension(".dds");
			std::error_code error{};
			if (std::filesystem::exists(ddsPath, error) &&
				std::filesystem::is_regular_file(ddsPath, error) &&
				IsDDSDimensionCompatible(ddsPath))
			{
				return ddsPath.generic_string();
			}
		}

		return sourcePath.generic_string();
	}

	inline void LoadDirectory(
		const std::string& resourceGroup,
		const std::filesystem::path& directory)
	{
		std::error_code error{};
		if (!std::filesystem::exists(directory, error) ||
			!std::filesystem::is_directory(directory, error))
		{
			return;
		}

		std::map<std::string, TEXTURE_CANDIDATE> candidates{};
		for (std::filesystem::directory_iterator iterator{
			directory,
			std::filesystem::directory_options::skip_permission_denied,
			error }, end;
			iterator != end;
			iterator.increment(error))
		{
			if (error)
			{
				error.clear();
				continue;
			}
			if (!iterator->is_regular_file(error))
				continue;

			const std::string extension = ToLowerExtension(iterator->path());
			if (extension != ".png" && extension != ".dds")
				continue;

			auto& candidate = candidates[iterator->path().stem().string()];
			if (extension == ".dds")
				candidate.DDSPath = iterator->path();
			else
				candidate.PNGPath = iterator->path();
		}

		for (const auto& [stem, candidate] : candidates)
		{
			const _bool useDDS = !candidate.DDSPath.empty() &&
				IsDDSDimensionCompatible(candidate.DDSPath);
			const std::filesystem::path& selectedPath =
				useDDS ? candidate.DDSPath : candidate.PNGPath;
			if (selectedPath.empty())
				continue;
			std::filesystem::path loadPath = selectedPath;
			if (ToLowerExtension(loadPath) == ".dds")
				loadPath.replace_extension(".dds");

			const std::string resourceTag = "TEX_" + stem;
			if (auto existingResource = E::CGameInstance::Get().
				GetResourceFirst<E::CResTexture2D>(
					resourceGroup, resourceTag))
			{
				if (!existingResource->GetSRV())
					existingResource->Load();
				continue;
			}

			if (auto resource = E::CGameInstance::Get().AddResource(
				resourceGroup,
				resourceTag,
				E::CResTexture2D::Create(loadPath.generic_string())))
			{
				resource->Load();
			}
		}
	}
}

NS_END
