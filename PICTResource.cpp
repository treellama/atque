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

#include <bitset>
#include <fstream>

#include <string.h>

using namespace atque;

/*
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
}*/

void PICTResource::Load(const std::vector<uint8>& data)
{
	bitmap_.SetSize(1, 1);
	data_.clear();
	AIStreamBE stream(&data[0], data.size());

	int16 size, top, left, bottom, right;
	stream >> size 
	       >> top 
	       >> left 
	       >> bottom 
	       >> right;

	bool done = false;
	while (!done)
	{
		uint16 opcode;
		stream >> opcode;
		
		switch (opcode) {
			
		case 0x0000:	// NOP
		case 0x0011:	// VersionOp
		case 0x001c:	// HiliteMode
		case 0x001e:	// DefHilite
		case 0x0038:	// FrameSameRect
		case 0x0039:	// PaintSameRect
		case 0x003a:	// EraseSameRect
		case 0x003b:	// InvertSameRect
		case 0x003c:	// FillSameRect
		case 0x02ff:	// Version
			break;

		case 0x00ff:	// OpEndPic
			done = true;
			break;

		case 0x0001: {	// Clipping region
			uint16 size;
			stream >> size;
			if (size & 1)
				size++;
			stream.ignore(size - 2);
			break;
		}

		case 0x0003:	// TxFont
		case 0x0004:	// TxFace
		case 0x0005:	// TxMode
		case 0x0008:	// PnMode
		case 0x000d:	// TxSize
		case 0x0015:	// PnLocHFrac
		case 0x0016:	// ChExtra
		case 0x0023:	// ShortLineFrom
		case 0x00a0:	// ShortComment
			stream.ignore(2);
			break;

		case 0x0006:	// SpExtra
		case 0x0007:	// PnSize
		case 0x000b:	// OvSize
		case 0x000c:	// Origin
		case 0x000e:	// FgColor
		case 0x000f:	// BgColor
		case 0x0021:	// LineFrom
			stream.ignore(4);
			break;

		case 0x001a:	// RGBFgCol
		case 0x001b:	// RGBBkCol
		case 0x001d:	// HiliteColor
		case 0x001f:	// OpColor
		case 0x0022:	// ShortLine
			stream.ignore(6);
			break;

		case 0x0002:	// BkPat
		case 0x0009:	// PnPat
		case 0x000a:	// FillPat
		case 0x0010:	// TxRatio
		case 0x0020:	// Line
		case 0x0030:	// FrameRect
		case 0x0031:	// PaintRect
		case 0x0032:	// EraseRect
		case 0x0033:	// InvertRect
		case 0x0034:	// FillRect
			stream.ignore(8);
			break;

		case 0x0c00:	// HeaderOp
			stream.ignore(24);
			break;

		case 0x00a1: {	// LongComment
			stream.ignore(2);
			int16 size;
			stream >> size;
			if (size & 1)
				size++;
			stream.ignore(size);
			break;
		}

		case 0x0098:	// Packed CopyBits
		case 0x0099:	// Packed CopyBits with clipping region
		case 0x009a:	// Direct CopyBits
		case 0x009b: {	// Direct CopyBits with clipping region
			bool packed = (opcode == 0x0098 || opcode == 0x0099);
			bool clipped = (opcode == 0x0099 || opcode == 0x009b);
			if (!LoadCopyBits(stream, packed, clipped))
			{
				bitmap_.SetSize(1, 1);
				done = true;
			}
			else
			{
				if (bitmap_.TellHeight() > (bottom - top) || bitmap_.TellWidth() > (right - left))
				{
					std::cerr << "Damnit, Cinemascope" << std::endl;
					bitmap_.SetSize(1, 1);
					done = true;
				}
			}
			break;
		}

		case 0x8200: {	// Compressed QuickTime image (we only handle JPEG compression)
			if (!LoadJPEG(stream))
			{
				bitmap_.SetSize(1, 1);
				jpeg_.clear();
				done = true;
			}
			break;
		}
		
		default:
			if (opcode >= 0x0300 && opcode < 0x8000)
				stream.ignore((opcode >> 8) * 2);
			else if (opcode >= 0x8000 && opcode < 0x8100)
				break;
			else {
				std::cerr << "Unimplemented opcode" << std::hex << opcode << std::dec << std::endl;
				done = true;
			}
			break;
		}
	}

	if (bitmap_.TellHeight() == 1 && bitmap_.TellWidth() == 1)
	{
		data_ = data;
	}
}

template <class T>
static std::vector<T> UnpackRow(AIStreamBE& stream, int row_bytes)
{
	std::vector<T> result;
	int row_length;
	if (row_bytes > 250)
	{
		uint16 length;
		stream >> length;
		row_length = length;
	}
	else
	{
		uint8 length;
		stream >> length;
		row_length = length;
	}

	uint32 end = stream.tellg() + row_length;
	while (stream.tellg() < end)
	{
		int8 c;
		stream >> c;

		if (c < 0)
		{
			int size = -c + 1;
			T data;
			stream >> data;
			for (int i = 0; i < size; ++i)
			{
				result.push_back(data);
			}
		}
		else if (c != -128)
		{
			int size = c + 1;
			T data;
			for (int i = 0; i < size; ++i)
			{
				stream >> data;
				result.push_back(data);
			}
		}
	}
	
	return result;
}

static std::vector<uint8> ExpandPixels(const std::vector<uint8>& scan_line, int depth)
{
	std::vector<uint8> result;
	for (std::vector<uint8>::const_iterator it = scan_line.begin(); it != scan_line.end(); ++it)
	{
		if (depth == 4)
		{
			result.push_back((*it) >> 4);
			result.push_back((*it) & 0xf);
		}
		else if (depth == 2)
		{
			result.push_back((*it) >> 6);
			result.push_back(((*it) >> 4) & 0x3);
			result.push_back(((*it) >> 2) & 0x3);
			result.push_back((*it) & 0x3);
		}
		else if (depth == 1)
		{
			std::bitset<8> bits(*it);
			for (int i = 0; i < 8; ++i)
			{
				result.push_back(bits[i] ? 1 : 0);
			}
		}
	}

	return result;
}

bool PICTResource::LoadCopyBits(AIStreamBE& stream, bool packed, bool clipped)
{
	if (!packed)
		stream.ignore(4); // pmBaseAddr

	uint16 row_bytes;
	stream >> row_bytes;
	bool is_pixmap = (row_bytes & 0x8000);
	row_bytes &= 0x3fff;
	uint16 top, left, bottom, right;
	stream >> top
	       >> left
	       >> bottom
	       >> right;

	uint16 width = right - left;
	uint16 height = bottom - top;
	uint16 pack_type, pixel_size;
	if (is_pixmap)
	{
		stream.ignore(2); // pmVersion
		stream >> pack_type;
		stream.ignore(14); // packSize/hRes/vRes/pixelType
		stream >> pixel_size;
		stream.ignore(16); // cmpCount/cmpSize/planeBytes/pmTable/pmReserved
	} 
	else
	{
		pack_type = 0;
		pixel_size = 1;
	}

	bitmap_.SetSize(width, height);
	if (pixel_size <= 8)
		bitmap_.SetBitDepth(8);
	else
		bitmap_.SetBitDepth(pixel_size);
	
	// read the color table
	if (is_pixmap && packed)
	{
		stream.ignore(4); // ctSeed
		uint16 flags, num_colors;
		stream >> flags
		       >> num_colors;
		num_colors++;
		for (int i = 0; i < num_colors; ++i)
		{
			uint16 index, red, green, blue;
			stream >> index
			       >> red
			       >> green
			       >> blue;

			if (flags & 0x8000)
				index = i;
			else
				index &= 0xff;

			RGBApixel pixel = { blue >> 8, green >> 8, red >> 8, 0xff };
			bitmap_.SetColor(index, pixel);
		}
	}

	// src/dst/transfer mode
	stream.ignore(18);
	
	// clipping region
	if (clipped)
	{
		uint16 size;
		stream >> size;
		stream.ignore(size - 2);
	}

	// the picture itself
	if (pixel_size <= 8)
	{
		for (int y = 0; y < height; ++y)
		{
			std::vector<uint8> scan_line;
			if (row_bytes < 8)
			{
				scan_line.resize(row_bytes);
				stream.read(&scan_line[0], scan_line.size());
			}
			else
			{
				scan_line = UnpackRow<uint8>(stream, row_bytes);
			}

			if (pixel_size == 8)
			{
				for (int x = 0; x < width; ++x)
				{
					bitmap_.SetPixel(x, y, bitmap_.GetColor(scan_line[x]));
				}
			}
			else
			{
				std::vector<uint8> pixels = ExpandPixels(scan_line, pixel_size);
				
				for (int x = 0; x < width; ++x)
				{
					bitmap_.SetPixel(x, y, bitmap_.GetColor(pixels[x]));
				}
			}
		}		
	}
	else if (pixel_size == 16)
	{
		for (int y = 0; y < height; ++y)
		{
			std::vector<uint16> scan_line;
			if (row_bytes < 8 || pack_type == 1)
			{
				for (int x = 0; x < width; ++x)
				{
					uint16 pixel;
					stream >> pixel;
					scan_line.push_back(pixel);
				}
			}
			else if (pack_type == 0 || pack_type == 3)
			{
				scan_line = UnpackRow<uint16>(stream, row_bytes);
			}

			for (int x = 0; x < width; ++x)
			{
				RGBApixel pixel;
				pixel.Red = (scan_line[x] >> 10) & 0x1f;
				pixel.Green = (scan_line[x] >> 5) & 0x1f;
				pixel.Blue = scan_line[x] & 0x1f;
				pixel.Red = (pixel.Red * 255 + 16) / 31;
				pixel.Green = (pixel.Green * 255 + 16) / 31;
				pixel.Blue = (pixel.Blue * 255 + 16) / 31;
				pixel.Alpha = 0xff;
				
				bitmap_.SetPixel(x, y, pixel);
			}
		}
	}
	else if (pixel_size == 32)
	{
		for (int y = 0; y < height; ++y)
		{
			std::vector<uint8> scan_line;
			if (row_bytes < 8 || pack_type == 1)
			{
				scan_line.resize(width * 3);
				for (int x = 0; x < width; ++x)
				{
					uint32 pixel;
					stream >> pixel;
					scan_line[x] = pixel >> 16;
					scan_line[x + width] = pixel >> 8;
					scan_line[x + width * 2] = pixel;
				}
			}
			else if (pack_type == 0 || pack_type == 4)
			{
				scan_line = UnpackRow<uint8>(stream, row_bytes);
			}

			for (int x = 0; x < width; ++x)
			{
				RGBApixel pixel;
				pixel.Red = scan_line[x];
				pixel.Green = scan_line[x + width];
				pixel.Blue = scan_line[x + width * 2];
				pixel.Alpha = 0xff;
				bitmap_.SetPixel(x, y, pixel);
			}
		}
	}
					
	if (stream.tellg() & 1)
		stream.ignore(1);

	return (pixel_size == 8 || pixel_size == 16 || pixel_size == 32);
}

bool PICTResource::LoadJPEG(AIStreamBE& stream)
{
	uint32 opcode_size;
	stream >> opcode_size;
	if (opcode_size & 1) 
		opcode_size++;

	uint32 opcode_start = stream.tellg();
	
	stream.ignore(26); // version/matrix (hom. part)
	int16 offset_x, offset_y;
	stream >> offset_x;
	stream.ignore(2);
	stream >> offset_y;
	stream.ignore(2);
	stream.ignore(4); // rest of matrix

	uint32 matte_size;
	stream >> matte_size;
	stream.ignore(22); // matte rect/srcRect/accuracy

	uint32 mask_size;
	stream >> mask_size;

	if (matte_size)
	{
		uint32 matte_id_size;
		stream >> matte_id_size;
		stream.ignore(matte_id_size - 4);
	}

	stream.ignore(matte_size);
	stream.ignore(mask_size);

	uint32 id_size, codec_type;
	stream >> id_size
	       >> codec_type;

	if (codec_type != FOUR_CHARS_TO_INT('j','p','e','g'))
		return false;

	stream.ignore(36); // resvd1/resvd2/dataRefIndex/version/revisionLevel/vendor/temporalQuality/spatialQuality/width/height/hRes/vRes
	uint32 data_size;
	stream >> data_size;
	stream.ignore(38); // frameCount/name/depth/clutID

	jpeg_.resize(data_size);
	stream.read(&jpeg_[0], jpeg_.size());
	
	stream.ignore(opcode_start + opcode_size - stream.tellg());
	return true;
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
/*
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
*/

bool PICTResource::LoadRaw(const std::vector<uint8>& data, const std::vector<uint8>& clut)
{
	AIStreamBE stream(&data[0], data.size());
	int16 top, left, bottom, right, depth;

	stream >> top
	       >> left
	       >> bottom
	       >> right
	       >> depth;
	
	int height = bottom - top;
	int width = right - left;

	if (depth != 8 && depth != 16)
		return false;

	bitmap_.SetBitDepth(depth);	
	bitmap_.SetSize(width, height);

	if (depth == 8)
	{
		if (clut.size() != 6 + 256 * 6)
			return false;

		AIStreamBE clut_stream(&clut[0], clut.size());
		clut_stream.ignore(6);
		for (int i = 0; i < 256; ++i)
		{
			uint16 red, green, blue;
			clut_stream >> red 
				    >> green
				    >> blue;

			RGBApixel color = { blue >> 8, green >> 8, red >> 8, 0xff };
			bitmap_.SetColor(i, color);
		}
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				uint8 pixel;
				stream >> pixel;
				bitmap_.SetPixel(x, y, bitmap_.GetColor(pixel));
			}
		}
	}
	else
	{
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				uint16 color;
				stream >> color;
				RGBApixel pixel;
				pixel.Red = (color >> 10) & 0x1f;
				pixel.Green = (color >> 5) & 0x1f;
				pixel.Blue = color & 0x1f;
				pixel.Red = (pixel.Red * 255 + 16) / 31;
				pixel.Green = (pixel.Green * 255 + 16) / 31;
				pixel.Blue = (pixel.Blue * 255 + 16) / 31;
				pixel.Alpha = 0xff;
				
				bitmap_.SetPixel(x, y, pixel);
			}
		}
	}

	return true;
}

std::vector<uint8> PICTResource::Save() const
{
	std::vector<uint8> result;
/*	result.resize(data_.size() + 10);
	AOStreamBE stream(&result[0], result.size());

	stream << static_cast<uint16>(0);

	stream << top_;
	stream << left_;
	stream << bottom_;
	stream << right_;

	stream.write(&data_[0], data_.size());
*/
	result = data_;
	return result;

}

bool PICTResource::Import(const std::string& path)
{
	std::ifstream infile(path.c_str(), std::ios::binary);
	infile.seekg(0, std::ios::end);
	std::streamsize length = infile.tellg();

	if (length < 528) return false;
	infile.seekg(512);
	
	std::vector<uint8> pict_data(length - 512);
	infile.read(reinterpret_cast<char*>(&pict_data[0]), pict_data.size());
	Load(pict_data);
	return true;
}

void PICTResource::Export(const std::string& path)
{
	if (bitmap_.TellHeight() != 1 || bitmap_.TellWidth() != 1)
	{
		std::string bmp_path = path + ".bmp";
		bitmap_.WriteToFile(bmp_path.c_str());
	}
	else if (jpeg_.size())
	{
		std::string jpeg_path = path + ".jpg";
		std::ofstream outfile(jpeg_path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
		outfile.write(reinterpret_cast<const char *>(&jpeg_[0]), jpeg_.size());
	}
	else
	{
		std::string pict_path = path + ".pct";
		std::ofstream outfile(pict_path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
		for (int i = 0; i < 512; ++i)
		{
			outfile.put('\0');
		}
		

		std::vector<uint8> pict = Save();
		outfile.write(reinterpret_cast<const char*>(&pict[0]), pict.size());
	}
}
