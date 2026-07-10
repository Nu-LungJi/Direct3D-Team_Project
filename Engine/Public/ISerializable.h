#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class ISerializer;
class IDeserializer;

class ENGINE_DLL ISerializable {
public:
	virtual ~ISerializable() = default;

	virtual void Serialize(ISerializer& serializer) const = 0;
	virtual void Deserialize(IDeserializer& deserializer) = 0;
};
NS_END
