// Copyright (C) 2003-2008 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#include "Common.h"
#include "FileUtil.h"
#include "Blob.h"
#include "CompressedBlob.h"
#include "FileBlob.h"
#include "DriveBlob.h"
#include "MappedFile.h"

namespace DiscIO
{

// Provides caching and split-operation-to-block-operations facilities.
// Used for compressed blob reading and direct drive reading.

void SectorReader::SetSectorSize(int blocksize)
{
	for (int i = 0; i < CACHE_SIZE; i++)
	{
		cache[i] = new u8[blocksize];
		cache_tags[i] = (u64)(s64) - 1;
	}
	m_blocksize = blocksize;
}

SectorReader::~SectorReader() {
	for (int i = 0; i < CACHE_SIZE; i++)
		delete[] cache[i];
}

const u8 *SectorReader::GetBlockData(u64 block_num)
{
	if (cache_tags[0] == block_num)
	{
		return cache[0];
	}
	else 
	{
		GetBlock(block_num, cache[0]);
		cache_tags[0] = block_num;
		return cache[0];
	}
}

bool SectorReader::Read(u64 offset, u64 size, u8* out_ptr)
{
	u64 startingBlock = offset / m_blocksize;
	u64 remain = size;

	int positionInBlock = (int)(offset % m_blocksize);
	u64 block = startingBlock;

	while (remain > 0)
	{
		const u8* data = GetBlockData(block);
		if (!data)
			return false;

		u32 toCopy = m_blocksize - positionInBlock;
		if (toCopy >= remain)
		{
			// yay, we are done!
			memcpy(out_ptr, data + positionInBlock, (size_t)remain);
			return true;
		}
		else
		{
			memcpy(out_ptr, data + positionInBlock, toCopy);
			out_ptr += toCopy;
			remain -= toCopy;
			positionInBlock = 0;
			block++;
		}
	}
	return true;
}


IBlobReader* CreateBlobReader(const char* filename)
{
	//if (strlen(filename) < 4 && filename[1] == ':')  // Drive, for sure.
	//	return DriveReader::Create(filename);

	if (!File::Exists(filename))
		return 0;

	if (IsCompressedBlob(filename))
		return CompressedBlobReader::Create(filename);

	// Still here? Assume plain file.
	return PlainFileReader::Create(filename);
}

}  // namespace
