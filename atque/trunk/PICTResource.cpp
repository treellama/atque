/* PICTResource.cpp

   Copyright (C) 2008 by Gregory Smith
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   This license is contained in the file "COPYING", which is included
   with this source code; it is available online at
   http://www.gnu.org/licenses/gpl.html
   
*/

#include "ferro/AStream.h"
#include "PICTResource.h"

#include <fstream>

using namespace atque;

void PICTResource::Load(const std::vector<uint8>& data)
{

	data_.clear();
	AIStreamBE stream(&data[0], data.size());

	int16 size;
	stream >> size;
	
	stream >> top_;
	stream >> left_;
	stream >> bottom_;
	stream >> right_;
	
	data_.resize(data.size() - 10);
	stream.read(&data_[0], data_.size());
}

static std::vector<uint8> PackBits(std::vector<uint8>::const_iterator start, std::vector<uint8>::const_iterator end)
{
	std::vector<uint8> result;
	std::vector<uint8> block;
	std::vector<uint8>::const_iterator p = start;


	int run_length = 1;
	while (p != end)
	{
		if (block.size() && *p == block.back())
		{
			run_length++;
		}
		else
		{
			run_length = 1;
		}
		
		block.push_back(*p);
		++p;

		if (run_length == 3)
		{
			if (block.size() > 3)
			{
				result.push_back(block.size() - 3 - 1);
				result.insert(result.end(), block.begin(), block.end() - 3);
				block.erase(block.begin(), block.end() - 3);
			}
			while (p != end && *p == block.back() && run_length < 127)
			{
				++p;
				++run_length;
			}
			result.push_back(1 - run_length);
			result.push_back(block.back());
			block.clear();
			run_length = 1;
		}
		else if (block.size() == 128)
		{
			result.push_back(block.size() - 1);
			result.insert(result.end(), block.begin(), block.end());
			block.clear();
			run_length = 1;
		}
	}
	if (block.size())
	{
		result.push_back(block.size() - 1);
		result.insert(result.end(), block.begin(), block.end());
	}

	return result;

}

bool PICTResource::LoadRaw(const std::vector<uint8>& data, const std::vector<uint8>& clut)
{
	data_.clear();
	AIStreamBE stream(&data[0], data.size());
	
	stream >> top_;
	stream >> left_;
	stream >> bottom_;
	stream >> right_;

	int16 depth;
	stream >> depth;

	int height = bottom_ - top_;

	if (depth != 8 && depth != 16) 
		return false;

	if (depth == 8 && clut.size() != 6 + 256 * 6)
	{
		return false;
	}

	// versionOp(2), version(2), headerOp(26), clip(12)
	int output_length = 2 + 2 + 26 + 12;
	int16 row_bytes;
	if (depth == 8) {
		// opcode(2), pixMap(46), colorTable(8+256*8), srcRect/dstRect/mode(18), data(variable)
		row_bytes = right_ - left_;
		output_length += 2 + 46 + 8+256*8 + 18;
	} else {
		// opcode(2), pixMap(50), srcRect/dstRect/mode(18), data(variable)
		row_bytes = (right_ - left_) * 2;
		output_length += 2 + 50 + 18;
	}
	// data(variable), opEndPic(2)
	if (depth == 8)
	{
		// packBits could increase size, in the worst case
		// give ourselves some extra buffer to pack into
		output_length += row_bytes * height * 2 + 2;
	}
	else
	{
		output_length += row_bytes * height + 2;
	}

	data_.resize(output_length);
	AOStreamBE ostream(&data_[0], data_.size());

	// versionOp
	int16 versionOp = 0x0011;
	int16 version = 0x02ff;
	
	ostream << versionOp
		<< version;
	
	// headerOp
	int16 headerOp = 0x0c00;
	int16 headerVersion = 0xfffe;
	int16 reserved = 0;
	int32 hRes = 72 << 16;
	int32 vRes = 72 << 16;
	
	ostream << headerOp
		<< headerVersion
		<< reserved
		<< hRes
		<< vRes
		<< top_
		<< left_
		<< bottom_
		<< right_
		<< reserved
		<< reserved;

	// clip
	int16 clip = 0x0001;
	int16 clipSize = 10;
	ostream << clip
		<< clipSize
		<< top_
		<< left_
		<< bottom_
		<< right_;

	// opcode
	if (depth == 8)
	{
		int16 PackBitsRect = 0x0098;
		ostream << PackBitsRect;
	}
	else
	{
		int16 DirectBitsRect = 0x009a;
		uint32 pmBaseAddr = 0x000000ff;
		ostream << DirectBitsRect
			<< pmBaseAddr;
	}

	// PixMap
	int16 pmVersion = 0;
	int16 packType = (depth == 8 ? 0 : 1); // default or unpacked
	int32 packSize = 0;
	int16 pixelType = (depth == 8 ? 0 : 0x10);
	int16 pixelSize = depth;
	int16 cmpCount = (depth == 8 ? 1 : 3);
	int16 cmpSize = (depth == 8 ? 8 : 5);
	int32 planeBytes = 0;
	uint32 pmTable = 0;
	int32 pmReserved = 0;
	ostream << static_cast<int16>(row_bytes | 0x8000)
		<< top_
		<< left_
		<< bottom_
		<< right_
		<< pmVersion
		<< packType
		<< packSize
		<< hRes
		<< vRes
		<< pixelType
		<< pixelSize
		<< cmpCount
		<< cmpSize
		<< planeBytes
		<< pmTable
		<< pmReserved;

	// color table
	if (depth == 8)
	{
		int32 seed = 0;
		int16 flags = 0;
		int16 size = 255;
		
		ostream << seed
			<< flags
			<< size;

		AIStreamBE clut_stream(&clut[0], clut.size());
		clut_stream.ignore(6);
		for (int16 index = 0; index < 256; ++index)
		{
			uint16 red;
			uint16 green;
			uint16 blue;
			
			clut_stream >> red
				    >> green
				    >> blue;

			ostream << index
				<< red
				<< green
				<< blue;
		}
	}

	// source/destination rect and transfer mode
	ostream << top_
		<< left_
		<< bottom_
		<< right_;

	ostream << top_
		<< left_
		<< bottom_
		<< right_;

	int16 transfer_mode = 0;
	ostream << transfer_mode;

	// graphics data
	if (depth == 8)
	{
		std::vector<uint8> scan_line(row_bytes);
		for (int i = 0; i < height; ++i)
		{
			stream.read(&scan_line[0], scan_line.size());
			std::vector<uint8> compressed_scan_line = PackBits(scan_line.begin(), scan_line.end());
			if (row_bytes > 250)
			{
				ostream << static_cast<int16>(compressed_scan_line.size());
			} 
			else
			{
				ostream << static_cast<int8>(compressed_scan_line.size());
			}
			
			ostream.write(&compressed_scan_line[0], compressed_scan_line.size());
		}
		
	}
	else
	{
		std::vector<uint8> scan_line(row_bytes);
		for (int i = 0; i < height; ++i)
		{
			stream.read(&scan_line[0], scan_line.size());
			ostream.write(&scan_line[0], scan_line.size());
		}
	}
	
	if (ostream.tellp() % 2)
	{
			ostream.ignore(1);
	}

	int16 endPic = 0x00ff;
	ostream << endPic;

	if (depth == 8)
	{
		data_.resize(ostream.tellp());
	}
	else
	{
		if (ostream.tellp() != ostream.maxp())
		{
			std::cerr << ostream.maxp() - ostream.tellp() << " bytes short!" << std::endl;
		}
	}

	return true;
}

std::vector<uint8> PICTResource::Save() const
{
	std::vector<uint8> result;
	result.resize(data_.size() + 10);
	AOStreamBE stream(&result[0], result.size());

	stream << static_cast<uint16>(0);

	stream << top_;
	stream << left_;
	stream << bottom_;
	stream << right_;

	stream.write(&data_[0], data_.size());
	return result;
}

void PICTResource::Export(const std::string& path) const
{
	std::ofstream outfile(path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	for (int i = 0; i < 512; ++i)
	{
		outfile.put('\0');
	}

	std::vector<uint8> pict = Save();
	outfile.write(reinterpret_cast<const char*>(&pict[0]), pict.size());
}
