
#include "pe_reader.hpp"

#include "int_pack.hpp"

#include <cstring>
#include <vector>

std::uint16_t pe_reader::read_u16_le()
{
	char buf[2];

	if (file.read(buf, 2) == 2)
	{
		return int_pack_16(buf[0], buf[1]);
	}

	return 0;
}

std::uint32_t pe_reader::read_u32_le()
{
	char buf[4];

	if (file.read(buf, 4) == 4)
	{
		return int_pack_32(buf[0], buf[1], buf[2], buf[3]);
	}

	return 0;
}

pe_reader::ResourceDirectory pe_reader::read_ResourceDirectory()
{
	return pe_reader::ResourceDirectory{
		read_u32_le(),
		read_u32_le(),
		read_u16_le(),
		read_u16_le(),
		read_u16_le(),
		read_u16_le()
	};
}

pe_reader::ResourceDirectoryEntry pe_reader::read_ResourceDirectoryEntry()
{
	return pe_reader::ResourceDirectoryEntry{
		pe_reader::ResourceType(read_u32_le()),
		read_u32_le()
	};
}

pe_reader::ResourceDataEntry pe_reader::read_ResourceDataEntry()
{
	return pe_reader::ResourceDataEntry{
		read_u32_le(),
		read_u32_le(),
		read_u32_le(),
		read_u32_le()
	};
}

bool pe_reader::read_header()
{
	char buf[8];

	file.seek(0x3C);
	std::uint16_t pe_header_address = read_u16_le();

	file.skip(pe_header_address - 0x3C - 0x02);
	file.read(buf, 4);

	if (std::memcmp(buf, "PE\0", 4) != 0)
		return false;

	file.skip(0x02);
	std::uint16_t sections = read_u16_le();

	file.skip(0x78 - 0x04 + 0x0C);
	virtual_address = read_u32_le();

	file.skip(0x6C + 0x08 + 0x04);

	for (unsigned int i = 0; i < sections; ++i)
	{
		std::uint32_t check_virtual_address = read_u32_le();

		if (check_virtual_address == virtual_address)
		{
			file.skip(0x04);

			root_address = read_u32_le();
			break;
		}

		file.skip(0x24);
	}

	if (!root_address)
		return false;

	file.seek(root_address);

	ResourceDirectory root_directory = read_ResourceDirectory();

	unsigned int directory_entries = root_directory.NumberOfNamedEntries + root_directory.NumberOfIdEntries;

	ResourceDirectoryEntry entry;

	for (unsigned int i = 0; i < directory_entries; ++i)
	{
		entry = read_ResourceDirectoryEntry();

		if (std::uint32_t(entry.ResourceType_) < 0x80000000 && entry.ResourceType_ == ResourceType::Bitmap)
		{
			if (entry.SubDirectoryOffset < 0x80000000)
				return false;

			entry.SubDirectoryOffset -= 0x80000000;

			bitmap_directory_entry = entry;

			file.skip(8 * (directory_entries - i - 1));

			break;
		}
	}

	return true;
}

std::map<int, std::pair<std::size_t, std::size_t>> pe_reader::read_bitmap_table()
{
	char buf[16];

	std::map<int, std::pair<std::size_t, std::size_t>> bitmap_pointers;

	if (bitmap_directory_entry.ResourceType_ != ResourceType::Bitmap)
		return bitmap_pointers;

	file.seek(root_address + bitmap_directory_entry.SubDirectoryOffset);

	ResourceDirectory bitmap_directory;
	file.read(buf, 16);
	bitmap_directory.NumberOfNamedEntries = std::uint8_t(buf[12]) + std::uint8_t(buf[13]) * 0x100U;
	bitmap_directory.NumberOfIdEntries = std::uint8_t(buf[14]) + std::uint8_t(buf[15]) * 0x100U;

	unsigned int directory_entries = bitmap_directory.NumberOfNamedEntries + bitmap_directory.NumberOfIdEntries;

	std::vector<ResourceDirectoryEntry> bitmap_entries;
	bitmap_entries.reserve(directory_entries);

	ResourceDirectoryEntry entry;

	for (unsigned int i = 0; i < directory_entries; ++i)
	{
		file.read(buf, 8);
		entry.ResourceType_ = ResourceType(std::uint8_t(buf[0]) + std::uint8_t(buf[1]) * 0x100U + std::uint8_t(buf[2]) * 0x10000U + std::uint8_t(buf[3]) * 0x1000000U);
		entry.SubDirectoryOffset = std::uint8_t(buf[4]) + std::uint8_t(buf[5]) * 0x100U + std::uint8_t(buf[6]) * 0x10000U + std::uint8_t(buf[7]) * 0x1000000U;

		if (entry.SubDirectoryOffset > 0x80000000)
		{
			entry.SubDirectoryOffset -= 0x80000000;
			bitmap_entries.push_back(entry);
		}
	}

	ResourceDataEntry data_entry;

	for (auto it = bitmap_entries.begin(); it != bitmap_entries.end(); ++it)
	{
		file.seek(root_address + it->SubDirectoryOffset + 16);

		file.read(buf, 8);
		entry.SubDirectoryOffset = std::uint8_t(buf[4]) + std::uint8_t(buf[5]) * 0x100U + std::uint8_t(buf[6]) * 0x10000U + std::uint8_t(buf[7]) * 0x1000000U;

		file.seek(root_address + entry.SubDirectoryOffset);

		file.read(buf, 16);
		data_entry.OffsetToData = std::uint8_t(buf[0]) + std::uint8_t(buf[1]) * 0x100U + std::uint8_t(buf[2]) * 0x10000U + std::uint8_t(buf[3]) * 0x1000000U;
		data_entry.Size = std::uint8_t(buf[4]) + std::uint8_t(buf[5]) * 0x100U + std::uint8_t(buf[6]) * 0x10000U + std::uint8_t(buf[7]) * 0x1000000U;

		auto entry = std::make_pair(data_entry.OffsetToData - virtual_address + root_address, data_entry.Size);
		bitmap_pointers.insert({std::uint32_t(it->ResourceType_), entry});
	}

	return bitmap_pointers;
}

bool pe_reader::read_resource(char* buf, std::pair<std::size_t, std::size_t> offsets)
{
	if (!file.seek(offsets.first))
		return false;

	if (file.read(buf, offsets.second) != offsets.second)
		return false;

	return true;
}
