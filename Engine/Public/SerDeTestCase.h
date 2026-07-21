#pragma once

#include "Engine_Defines.h"
#include "BinSerializeFormat.h"
#include "ISerializable.h"
#include "SerializeManager.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

NS_BEGIN(Engine)
NS_BEGIN(SerializeTest)

struct REGRESSION_REPORT
{
	size_t iPassed{};
	size_t iTotal{};
	std::string sText{};

	bool Passed() const noexcept
	{
		return iPassed == iTotal;
	}
};

namespace Detail
{
	enum class TEST_STATE : int
	{
		IDLE,
		ACTIVE,
		FINISHED
	};

	enum class SMALL_ENUM_STATE : uint8_t
	{
		ZERO,
		MAX_VALUE = std::numeric_limits<uint8_t>::max()
	};

	struct NESTED_DATA final : public ISerializable
	{
		int iID{};
		std::string sName{};

		NESTED_DATA() = default;
		NESTED_DATA(int id, std::string name)
			: iID{ id }, sName{ std::move(name) }
		{
		}

		void Serialize(ISerializer& serializer) const override
		{
			serializer.Write("ID", iID);
			serializer.Write("Name", sName);
		}

		void Deserialize(IDeserializer& deserializer) override
		{
			deserializer.Read("ID", iID);
			deserializer.Read("Name", sName);
		}

		bool operator==(const NESTED_DATA& rhs) const
		{
			return iID == rhs.iID && sName == rhs.sName;
		}
	};

	// 저장 당시의 V1 구조체다. 구버전 파일을 만드는 회귀 테스트에만 사용한다.
	struct MIGRATION_DATA_V1 final : public ISerializable
	{
		static constexpr uint32_t VERSION = 1;

		std::string sName{};
		int iHealth{};

		void Serialize(ISerializer& serializer) const override
		{
			serializer.Write("Version", VERSION);
			serializer.Write("Name", sName);
			serializer.Write("Health", iHealth);
		}

		void Deserialize(IDeserializer& deserializer) override
		{
			uint32_t version{};
			deserializer.Read("Version", version);
			if (version != VERSION)
				throw std::runtime_error("MIGRATION_DATA_V1 version mismatch");

			deserializer.Read("Name", sName);
			deserializer.Read("Health", iHealth);
		}
	};

	// 현재 사용 중인 V2 구조체다. Deserialize가 저장 데이터 버전을 해석하고
	// 구버전 필드를 현재 필드로 변환한다.
	struct MIGRATION_DATA_CURRENT final : public ISerializable
	{
		static constexpr uint32_t CURRENT_VERSION = 2;

		std::string sName{};
		int iCurrentHealth{};
		int iMaxHealth{ 100 };
		_float3 vSpawnPosition{ 1.f, 2.f, 3.f };

		void Serialize(ISerializer& serializer) const override
		{
			serializer.Write("Version", CURRENT_VERSION);
			serializer.Write("Name", sName);
			serializer.Write("CurrentHealth", iCurrentHealth);
			serializer.Write("MaxHealth", iMaxHealth);
			serializer.Write("SpawnPosition", vSpawnPosition);
		}

		void Deserialize(IDeserializer& deserializer) override
		{
			uint32_t version{};
			deserializer.Read("Version", version);

			switch (version)
			{
			case 1:
				DeserializeV1(deserializer);
				break;

			case CURRENT_VERSION:
				DeserializeV2(deserializer);
				break;

			default:
				throw std::runtime_error("Unsupported MIGRATION_DATA_CURRENT version");
			}
		}

		bool operator==(const MIGRATION_DATA_CURRENT& rhs) const
		{
			return sName == rhs.sName &&
				iCurrentHealth == rhs.iCurrentHealth &&
				iMaxHealth == rhs.iMaxHealth &&
				vSpawnPosition.x == rhs.vSpawnPosition.x &&
				vSpawnPosition.y == rhs.vSpawnPosition.y &&
				vSpawnPosition.z == rhs.vSpawnPosition.z;
		}

	private:
		void DeserializeV1(IDeserializer& deserializer)
		{
			int legacyHealth{};
			deserializer.Read("Name", sName);
			deserializer.Read("Health", legacyHealth);

			iCurrentHealth = legacyHealth;
			iMaxHealth = 100;
			vSpawnPosition = { 1.f, 2.f, 3.f };
		}

		void DeserializeV2(IDeserializer& deserializer)
		{
			deserializer.Read("Name", sName);
			deserializer.Read("CurrentHealth", iCurrentHealth);
			deserializer.Read("MaxHealth", iMaxHealth);
			deserializer.Read("SpawnPosition", vSpawnPosition);
		}
	};

	struct REGRESSION_DATA final : public ISerializable
	{
		bool bEnabled{ true };
		int8_t iSigned8{};
		uint8_t iUnsigned8{};
		int16_t iSigned16{};
		uint16_t iUnsigned16{};
		int iCount{ 7 };
		uint32_t iUnsigned{ 11 };
		uint64_t iLarge{ 9'000'000'001ull };
		int64_t iSignedLarge{};
		float fRatio{ 1.25f };
		double dPreciseRatio{ 0.125 };
		std::string sName{ "DefaultName" };
		StringID sTag{ "DefaultTag" };
		_float3 vPosition{ 1.f, 2.f, 3.f };
		TEST_STATE eState{ TEST_STATE::IDLE };
		SMALL_ENUM_STATE eSmallState{ SMALL_ENUM_STATE::ZERO };
		NESTED_DATA tNested{ 1, "DefaultNested" };
		std::vector<int> vecValues{ 1, 2, 3 };
		std::vector<NESTED_DATA> vecNested{};
		std::vector<std::vector<int>> vecNestedArrays{};
		std::map<std::string, int> mapValues{};
		std::unordered_map<int, float> unorderedValues{};
		std::pair<StringID, StringID> pairTags{ "Left", "Right" };
		int iFixed[3]{ 4, 5, 6 };
		std::array<int16_t, 4> arrFixed{ 7, 8, 9, 10 };
		std::optional<int16_t> optNumber{ 42 };
		std::optional<NESTED_DATA> optNested{};

		void Serialize(ISerializer& serializer) const override
		{
			serializer.Write("Enabled", bEnabled);
			serializer.Write("Signed8", iSigned8);
			serializer.Write("Unsigned8", iUnsigned8);
			serializer.Write("Signed16", iSigned16);
			serializer.Write("Unsigned16", iUnsigned16);
			serializer.Write("Count", iCount);
			serializer.Write("Unsigned", iUnsigned);
			serializer.Write("Large", iLarge);
			serializer.Write("SignedLarge", iSignedLarge);
			serializer.Write("Ratio", fRatio);
			serializer.Write("PreciseRatio", dPreciseRatio);
			serializer.Write("Name", sName);
			serializer.Write("Tag", sTag);
			serializer.Write("Position", vPosition);
			serializer.Write("State", eState);
			serializer.Write("SmallState", eSmallState);
			serializer.Write("Nested", tNested);
			serializer.Write("Values", vecValues);
			serializer.Write("NestedValues", vecNested);
			serializer.Write("NestedArrays", vecNestedArrays);
			serializer.Write("MapValues", mapValues);
			serializer.Write("UnorderedValues", unorderedValues);
			serializer.Write("PairTags", pairTags);
			serializer.Write("Fixed", iFixed);
			serializer.Write("StdArray", arrFixed);
			serializer.Write("OptionalNumber", optNumber);
			serializer.Write("OptionalNested", optNested);
		}

		void Deserialize(IDeserializer& deserializer) override
		{
			deserializer.Read("Enabled", bEnabled);
			deserializer.Read("Signed8", iSigned8);
			deserializer.Read("Unsigned8", iUnsigned8);
			deserializer.Read("Signed16", iSigned16);
			deserializer.Read("Unsigned16", iUnsigned16);
			deserializer.Read("Count", iCount);
			deserializer.Read("Unsigned", iUnsigned);
			deserializer.Read("Large", iLarge);
			deserializer.Read("SignedLarge", iSignedLarge);
			deserializer.Read("Ratio", fRatio);
			deserializer.Read("PreciseRatio", dPreciseRatio);
			deserializer.Read("Name", sName);
			deserializer.Read("Tag", sTag);
			deserializer.Read("Position", vPosition);
			deserializer.Read("State", eState);
			deserializer.Read("SmallState", eSmallState);
			deserializer.Read("Nested", tNested);
			deserializer.Read("Values", vecValues);
			deserializer.Read("NestedValues", vecNested);
			deserializer.Read("NestedArrays", vecNestedArrays);
			deserializer.Read("MapValues", mapValues);
			deserializer.Read("UnorderedValues", unorderedValues);
			deserializer.Read("PairTags", pairTags);
			deserializer.Read("Fixed", iFixed);
			deserializer.Read("StdArray", arrFixed);
			deserializer.Read("OptionalNumber", optNumber);
			deserializer.Read("OptionalNested", optNested);
		}

		bool operator==(const REGRESSION_DATA& rhs) const
		{
			return bEnabled == rhs.bEnabled &&
				iSigned8 == rhs.iSigned8 &&
				iUnsigned8 == rhs.iUnsigned8 &&
				iSigned16 == rhs.iSigned16 &&
				iUnsigned16 == rhs.iUnsigned16 &&
				iCount == rhs.iCount &&
				iUnsigned == rhs.iUnsigned &&
				iLarge == rhs.iLarge &&
				iSignedLarge == rhs.iSignedLarge &&
				fRatio == rhs.fRatio &&
				dPreciseRatio == rhs.dPreciseRatio &&
				sName == rhs.sName &&
				sTag == rhs.sTag &&
				vPosition.x == rhs.vPosition.x &&
				vPosition.y == rhs.vPosition.y &&
				vPosition.z == rhs.vPosition.z &&
				eState == rhs.eState &&
				eSmallState == rhs.eSmallState &&
				tNested == rhs.tNested &&
				vecValues == rhs.vecValues &&
				vecNested == rhs.vecNested &&
				vecNestedArrays == rhs.vecNestedArrays &&
				mapValues == rhs.mapValues &&
				unorderedValues == rhs.unorderedValues &&
				pairTags == rhs.pairTags &&
				std::equal(std::begin(iFixed), std::end(iFixed), std::begin(rhs.iFixed)) &&
				arrFixed == rhs.arrFixed &&
				optNumber == rhs.optNumber &&
				optNested == rhs.optNested;
		}
	};

	inline REGRESSION_DATA MakePopulatedData()
	{
		REGRESSION_DATA data{};
		data.bEnabled = false;
		data.iSigned8 = std::numeric_limits<int8_t>::min();
		data.iUnsigned8 = std::numeric_limits<uint8_t>::max();
		data.iSigned16 = std::numeric_limits<int16_t>::min();
		data.iUnsigned16 = std::numeric_limits<uint16_t>::max();
		data.iCount = -42;
		data.iUnsigned = 4'000'000'000u;
		data.iLarge = 18'446'000'000'000'000'000ull;
		data.iSignedLarge = std::numeric_limits<int64_t>::min();
		data.fRatio = 7.5f;
		data.dPreciseRatio = 0.125;
		data.sName = "RegressionData";
		data.sTag = "RegressionTag";
		data.vPosition = { -3.f, 10.f, 0.5f };
		data.eState = TEST_STATE::FINISHED;
		data.eSmallState = SMALL_ENUM_STATE::MAX_VALUE;
		data.tNested = { 77, "NestedRoot" };
		data.vecValues = { -10, 0, 10, 20 };
		data.vecNested = { { 1, "One" }, { 2, "Two" } };
		data.vecNestedArrays = { { 1, 2 }, {}, { 3, 4, 5 } };
		data.mapValues = { { "HP", 100 }, { "MP", 55 } };
		data.unorderedValues = { { 10, 1.5f }, { 20, 2.5f } };
		data.pairTags = { "FirstTag", "SecondTag" };
		data.iFixed[0] = 91;
		data.iFixed[1] = 92;
		data.iFixed[2] = 93;
		data.arrFixed = {
			std::numeric_limits<int16_t>::min(), -1, 0,
			std::numeric_limits<int16_t>::max()
		};
		data.optNumber = std::numeric_limits<int16_t>::max();
		data.optNested.reset();
		return data;
	}

	inline REGRESSION_DATA MakeEmptyData()
	{
		REGRESSION_DATA data{};
		data.sName.clear();
		data.vecValues.clear();
		data.vecNested.clear();
		data.vecNestedArrays.clear();
		data.mapValues.clear();
		data.unorderedValues.clear();
		data.optNumber.reset();
		data.optNested = NESTED_DATA{ 808, "OptionalNested" };
		return data;
	}

	inline bool WriteTextFile(const std::filesystem::path& path, const std::string& text)
	{
		std::ofstream output{ path, std::ios::binary | std::ios::trunc };
		if (!output.is_open()) return false;
		output.write(text.data(), static_cast<std::streamsize>(text.size()));
		output.flush();
		return output.good();
	}

	inline bool ReadBinaryFile(
		const std::filesystem::path& path,
		std::vector<uint8_t>& outBytes)
	{
		std::ifstream input{ path, std::ios::binary | std::ios::ate };
		if (!input.is_open()) return false;

		const std::streampos endPosition = input.tellg();
		if (endPosition < std::streampos{}) return false;
		outBytes.resize(static_cast<size_t>(endPosition));
		input.seekg(0, std::ios::beg);
		if (!outBytes.empty())
			input.read(
				reinterpret_cast<char*>(outBytes.data()),
				static_cast<std::streamsize>(outBytes.size()));
		return input.good();
	}

	inline bool WriteBinaryFile(
		const std::filesystem::path& path,
		const std::vector<uint8_t>& bytes)
	{
		std::ofstream output{ path, std::ios::binary | std::ios::trunc };
		if (!output.is_open()) return false;
		if (!bytes.empty())
			output.write(
				reinterpret_cast<const char*>(bytes.data()),
				static_cast<std::streamsize>(bytes.size()));
		output.flush();
		return output.good();
	}

	inline bool UpdatePayloadSize(std::vector<uint8_t>& bytes)
	{
		if (bytes.size() < sizeof(BinSerializeFormat::HEADER)) return false;

		BinSerializeFormat::HEADER header{};
		std::memcpy(&header, bytes.data(), sizeof(header));
		header.iPayloadSize = bytes.size() - sizeof(header);
		std::memcpy(bytes.data(), &header, sizeof(header));
		return true;
	}

	inline bool HasTemporaryArtifact(const std::filesystem::path& targetPath)
	{
		const std::filesystem::path parentPath = targetPath.parent_path();
		const std::string prefix = targetPath.filename().string() + ".tmp.";

		std::error_code ec;
		for (std::filesystem::directory_iterator it{ parentPath, ec }, end;
			!ec && it != end; it.increment(ec))
		{
			if (it->path().filename().string().starts_with(prefix)) return true;
		}
		return false;
	}

	class CTemporaryDirectory final
	{
	public:
		CTemporaryDirectory()
		{
			std::error_code ec;
			const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
			m_Path = std::filesystem::temp_directory_path(ec) /
				("JusinSerializerRegression_" +
					std::to_string(GetCurrentProcessId()) + "_" +
					std::to_string(timestamp));
			if (!ec) std::filesystem::create_directories(m_Path, ec);
			m_bValid = !ec;
		}

		~CTemporaryDirectory()
		{
			std::error_code ec;
			std::filesystem::remove_all(m_Path, ec);
		}

		const std::filesystem::path& GetPath() const noexcept { return m_Path; }
		bool IsValid() const noexcept { return m_bValid; }

	private:
		std::filesystem::path m_Path{};
		bool m_bValid{};
	};

	class CRegressionRunner final
	{
	public:
		template<typename Test>
		void Run(const std::string& name, Test&& test)
		{
			bool bPassed = false;
			std::string detail{};
			try
			{
				bPassed = std::forward<Test>(test)();
			}
			catch (const std::exception& e)
			{
				detail = e.what();
			}
			catch (...)
			{
				detail = "unknown exception";
			}

			m_Results.push_back({ name, bPassed, std::move(detail) });
		}

		REGRESSION_REPORT MakeReport() const
		{
			REGRESSION_REPORT report{};
			report.iTotal = m_Results.size();
			report.sText = "Serialization Regression Tests\n\n";

			for (const auto& result : m_Results)
			{
				if (result.bPassed) ++report.iPassed;
				report.sText += result.bPassed ? "[PASS] " : "[FAIL] ";
				report.sText += result.sName;
				if (!result.sDetail.empty())
					report.sText += " - " + result.sDetail;
				report.sText += '\n';
			}

			report.sText += "\nResult: " + std::to_string(report.iPassed) + "/" +
				std::to_string(report.iTotal) + " passed";
			return report;
		}

	private:
		struct RESULT
		{
			std::string sName{};
			bool bPassed{};
			std::string sDetail{};
		};

		std::vector<RESULT> m_Results{};
	};
}

inline REGRESSION_REPORT RunSerializationRegressionTests(CSerializeManager& manager)
{
	using namespace Detail;
	constexpr const char* ROOT_NAME = "Root";

	CRegressionRunner runner{};
	CTemporaryDirectory tempDirectory{};
	if (!tempDirectory.IsValid())
	{
		REGRESSION_REPORT report{};
		report.iTotal = 1;
		report.sText = "[FAIL] Unable to create the serializer regression directory";
		return report;
	}

	const std::filesystem::path root = tempDirectory.GetPath();
	const REGRESSION_DATA populated = MakePopulatedData();

	runner.Run("JSON round trip with nested containers", [&]
	{
		const auto path = root / "round_trip.json";
		REGRESSION_DATA restored{};
		return SUCCEEDED(manager.JsonSerialize(path.string(), populated, ROOT_NAME, false)) &&
			SUCCEEDED(manager.JsonDeSerialize(path.string(), restored, ROOT_NAME, false)) &&
			restored == populated;
	});

	runner.Run("Binary round trip with nested containers", [&]
	{
		const auto path = root / "round_trip.bin";
		REGRESSION_DATA restored{};
		return SUCCEEDED(manager.BinSerialize(path.string(), populated, ROOT_NAME, false)) &&
			SUCCEEDED(manager.BinDeSerialize(path.string(), restored, ROOT_NAME, false)) &&
			restored == populated;
	});

	runner.Run("JSON migrates V1 data into the current schema", [&]
	{
		const auto path = root / "migration_v1.json";
		MIGRATION_DATA_V1 legacy{};
		legacy.sName = "LegacyPlayer";
		legacy.iHealth = 73;
		MIGRATION_DATA_CURRENT restored{};
		MIGRATION_DATA_CURRENT expected{};
		expected.sName = "LegacyPlayer";
		expected.iCurrentHealth = 73;

		return SUCCEEDED(manager.JsonSerialize(path.string(), legacy, ROOT_NAME, false)) &&
			SUCCEEDED(manager.JsonDeSerialize(path.string(), restored, ROOT_NAME, false)) &&
			restored == expected;
	});

	runner.Run("Binary migrates V1 data into the current schema", [&]
	{
		const auto path = root / "migration_v1.bin";
		MIGRATION_DATA_V1 legacy{};
		legacy.sName = "LegacyPlayer";
		legacy.iHealth = 73;
		MIGRATION_DATA_CURRENT restored{};
		MIGRATION_DATA_CURRENT expected{};
		expected.sName = "LegacyPlayer";
		expected.iCurrentHealth = 73;

		return SUCCEEDED(manager.BinSerialize(path.string(), legacy, ROOT_NAME, false)) &&
			SUCCEEDED(manager.BinDeSerialize(path.string(), restored, ROOT_NAME, false)) &&
			restored == expected;
	});

	runner.Run("JSON and Binary empty values", [&]
	{
		const REGRESSION_DATA empty = MakeEmptyData();
		const auto jsonPath = root / "empty.json";
		const auto binPath = root / "empty.bin";
		REGRESSION_DATA jsonRestored = populated;
		REGRESSION_DATA binRestored = populated;
		return SUCCEEDED(manager.JsonSerialize(jsonPath.string(), empty, ROOT_NAME, false)) &&
			SUCCEEDED(manager.JsonDeSerialize(jsonPath.string(), jsonRestored, ROOT_NAME, false)) &&
			jsonRestored == empty &&
			SUCCEEDED(manager.BinSerialize(binPath.string(), empty, ROOT_NAME, false)) &&
			SUCCEEDED(manager.BinDeSerialize(binPath.string(), binRestored, ROOT_NAME, false)) &&
			binRestored == empty;
	});

	runner.Run("JSON missing fields preserve existing values", [&]
	{
		const auto path = root / "missing_fields.json";
		if (!WriteTextFile(path, R"({"Root":{"Count":321}})")) return false;

		REGRESSION_DATA restored = populated;
		REGRESSION_DATA expected = populated;
		expected.iCount = 321;
		return SUCCEEDED(manager.JsonDeSerialize(path.string(), restored, ROOT_NAME, false)) &&
			restored == expected;
	});

	runner.Run("JSON invalid type preserves the destination", [&]
	{
		const auto path = root / "invalid_type.json";
		if (!WriteTextFile(
			path,
			R"({"Root":{"Enabled":true,"Count":"invalid"}})"))
		{
			return false;
		}

		REGRESSION_DATA destination = populated;
		const REGRESSION_DATA before = destination;
		const SERIALIZE_RESULT result =
			manager.JsonDeSerializeDetailed(path.string(), destination, ROOT_NAME);
		return result.eError == SERIALIZE_ERROR::DATA_DESERIALIZATION_FAILED &&
			destination == before;
	});

	runner.Run("JSON rejects an out-of-range fixed-width integer", [&]
	{
		const auto path = root / "integer_overflow.json";
		if (!WriteTextFile(path, R"({"Root":{"Unsigned8":256}})")) return false;

		REGRESSION_DATA destination = populated;
		const REGRESSION_DATA before = destination;
		const SERIALIZE_RESULT result =
			manager.JsonDeSerializeDetailed(path.string(), destination, ROOT_NAME);
		return result.eError == SERIALIZE_ERROR::DATA_DESERIALIZATION_FAILED &&
			destination == before;
	});

	runner.Run("JSON rejects an inconsistent optional value state", [&]
	{
		const auto path = root / "invalid_optional.json";
		if (!WriteTextFile(
			path,
			R"({"Root":{"OptionalNumber":{"HasValue":true}}})"))
		{
			return false;
		}

		REGRESSION_DATA destination = populated;
		const REGRESSION_DATA before = destination;
		const SERIALIZE_RESULT result =
			manager.JsonDeSerializeDetailed(path.string(), destination, ROOT_NAME);
		return result.eError == SERIALIZE_ERROR::DATA_DESERIALIZATION_FAILED &&
			destination == before;
	});

	runner.Run("Malformed JSON preserves the destination", [&]
	{
		const auto path = root / "malformed.json";
		if (!WriteTextFile(path, R"({"Root":{"Count":10})")) return false;

		REGRESSION_DATA destination = populated;
		const REGRESSION_DATA before = destination;
		const SERIALIZE_RESULT result =
			manager.JsonDeSerializeDetailed(path.string(), destination, ROOT_NAME);
		return result.eError == SERIALIZE_ERROR::SOURCE_FILE_INVALID &&
			destination == before;
	});

	const auto validBinaryPath = root / "valid_source.bin";
	std::vector<uint8_t> validBinary{};
	const bool bValidBinaryReady =
		SUCCEEDED(manager.BinSerialize(validBinaryPath.string(), populated, ROOT_NAME, false)) &&
		ReadBinaryFile(validBinaryPath, validBinary);

	runner.Run("Truncated Binary preserves the destination", [&]
	{
		if (!bValidBinaryReady || validBinary.size() <= sizeof(BinSerializeFormat::HEADER) + 3)
			return false;

		auto bytes = validBinary;
		bytes.resize(bytes.size() - 3);
		if (!UpdatePayloadSize(bytes)) return false;
		const auto path = root / "truncated.bin";
		if (!WriteBinaryFile(path, bytes)) return false;

		REGRESSION_DATA destination = MakeEmptyData();
		const REGRESSION_DATA before = destination;
		const SERIALIZE_RESULT result =
			manager.BinDeSerializeDetailed(path.string(), destination, ROOT_NAME);
		return result.eError == SERIALIZE_ERROR::DATA_DESERIALIZATION_FAILED &&
			destination == before;
	});

	runner.Run("Binary rejects an invalid JSBN magic", [&]
	{
		if (!bValidBinaryReady) return false;
		auto bytes = validBinary;
		bytes[0] ^= 0xffu;
		const auto path = root / "invalid_magic.bin";
		if (!WriteBinaryFile(path, bytes)) return false;

		REGRESSION_DATA destination = MakeEmptyData();
		const REGRESSION_DATA before = destination;
		const SERIALIZE_RESULT result =
			manager.BinDeSerializeDetailed(path.string(), destination, ROOT_NAME);
		return result.eError == SERIALIZE_ERROR::SOURCE_FILE_INVALID &&
			destination == before;
	});

	runner.Run("Binary rejects an unsupported format version", [&]
	{
		if (!bValidBinaryReady) return false;
		auto bytes = validBinary;
		BinSerializeFormat::HEADER header{};
		std::memcpy(&header, bytes.data(), sizeof(header));
		header.iVersion = static_cast<uint16_t>(BinSerializeFormat::VERSION + 1);
		std::memcpy(bytes.data(), &header, sizeof(header));
		const auto path = root / "invalid_version.bin";
		if (!WriteBinaryFile(path, bytes)) return false;

		REGRESSION_DATA destination = MakeEmptyData();
		const REGRESSION_DATA before = destination;
		const SERIALIZE_RESULT result =
			manager.BinDeSerializeDetailed(path.string(), destination, ROOT_NAME);
		return result.eError == SERIALIZE_ERROR::SOURCE_FILE_INVALID &&
			destination == before;
	});

	runner.Run("Binary rejects trailing data", [&]
	{
		if (!bValidBinaryReady) return false;
		auto bytes = validBinary;
		bytes.push_back(0xcd);
		if (!UpdatePayloadSize(bytes)) return false;
		const auto path = root / "trailing_data.bin";
		if (!WriteBinaryFile(path, bytes)) return false;

		REGRESSION_DATA destination = MakeEmptyData();
		const REGRESSION_DATA before = destination;
		const SERIALIZE_RESULT result =
			manager.BinDeSerializeDetailed(path.string(), destination, ROOT_NAME);
		return result.eError == SERIALIZE_ERROR::TRAILING_DATA &&
			destination == before;
	});

	runner.Run("Binary rejects a non-canonical bool", [&]
	{
		if (!bValidBinaryReady ||
			validBinary.size() <= sizeof(BinSerializeFormat::HEADER))
		{
			return false;
		}

		auto bytes = validBinary;
		bytes[sizeof(BinSerializeFormat::HEADER)] = 2u;
		const auto path = root / "invalid_bool.bin";
		if (!WriteBinaryFile(path, bytes)) return false;

		REGRESSION_DATA destination = MakeEmptyData();
		const REGRESSION_DATA before = destination;
		const SERIALIZE_RESULT result =
			manager.BinDeSerializeDetailed(path.string(), destination, ROOT_NAME);
		return result.eError == SERIALIZE_ERROR::DATA_DESERIALIZATION_FAILED &&
			destination == before;
	});

	runner.Run("Repeated saves safely replace JSON and Binary", [&]
	{
		REGRESSION_DATA modified = populated;
		modified.iCount = 999;
		modified.sName = "Replaced";
		const auto jsonPath = root / "replace.json";
		const auto binPath = root / "replace.bin";
		REGRESSION_DATA jsonRestored{};
		REGRESSION_DATA binRestored{};

		return SUCCEEDED(manager.JsonSerialize(jsonPath.string(), populated, ROOT_NAME, false)) &&
			SUCCEEDED(manager.JsonSerialize(jsonPath.string(), modified, ROOT_NAME, false)) &&
			SUCCEEDED(manager.JsonDeSerialize(jsonPath.string(), jsonRestored, ROOT_NAME, false)) &&
			jsonRestored == modified &&
			SUCCEEDED(manager.BinSerialize(binPath.string(), populated, ROOT_NAME, false)) &&
			SUCCEEDED(manager.BinSerialize(binPath.string(), modified, ROOT_NAME, false)) &&
			SUCCEEDED(manager.BinDeSerialize(binPath.string(), binRestored, ROOT_NAME, false)) &&
			binRestored == modified;
	});

	runner.Run("Failed replacement preserves the original and removes temp files", [&]
	{
		const auto path = root / "locked_target.json";
		if (FAILED(manager.JsonSerialize(path.string(), populated, ROOT_NAME, false))) return false;

		const HANDLE hFile = CreateFileW(
			path.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);
		if (hFile == INVALID_HANDLE_VALUE) return false;

		REGRESSION_DATA modified = populated;
		modified.iCount = 123456;
		const SERIALIZE_RESULT result =
			manager.JsonSerializeDetailed(path.string(), modified, ROOT_NAME);
		CloseHandle(hFile);

		REGRESSION_DATA restored{};
		return result.eError == SERIALIZE_ERROR::FILE_COMMIT_FAILED &&
			!HasTemporaryArtifact(path) &&
			SUCCEEDED(manager.JsonDeSerialize(path.string(), restored, ROOT_NAME, false)) &&
			restored == populated;
	});

	runner.Run("Save creates missing parent directories", [&]
	{
		const auto path = root / "new" / "nested" / "directory" / "data.json";
		REGRESSION_DATA restored{};
		return SUCCEEDED(manager.JsonSerialize(path.string(), populated, ROOT_NAME, false)) &&
			std::filesystem::exists(path) &&
			SUCCEEDED(manager.JsonDeSerialize(path.string(), restored, ROOT_NAME, false)) &&
			restored == populated;
	});

	return runner.MakeReport();
}

NS_END
NS_END
