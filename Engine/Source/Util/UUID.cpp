#include "epch.h"
#include "UUID.h"

#include <random>

namespace Cosmos
{
	static std::random_device sDevice;
	static std::mt19937_64 sEngine(sDevice());
	static std::uniform_int_distribution<uint32_t> sDistribution;

	UUID::UUID()
		: mUUID(sDistribution(sEngine))
	{
	}

	UUID::UUID(uint32_t id)
		: mUUID(id)
	{
	}

	UUID::UUID(std::string id)
	{
		mUUID = std::stoi(id);
	}
}