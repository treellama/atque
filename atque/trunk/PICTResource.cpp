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

#include <boost/algorithm/string/predicate.hpp>

#include <string.h>

using namespace atque;
namespace algo = boost::algorithm;

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
	jpeg_.clear();
	AIStreamBE stream(&data[0], data.size());

	int16 size;
	stream >> size;

	Rect rect;
	rect.Load(stream);

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

		case 0x0c00: {	// HeaderOp
			HeaderOp headerOp;
			headerOp.Load(stream);
			break;
		}

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
			else if (jpeg_.size())
			{
				bitmap_.SetSize(1, 1);
			}
			else if (bitmap_.TellWidth() != rect.width() && bitmap_.TellWidth() == 614)
					
			{
				std::cerr << "Damnit, Cinemascope" << std::endl;
				bitmap_.SetSize(1, 1);
				done = true;
			}
			break;
		}

		case 0x8200: {	// Compressed QuickTime image (we only handle JPEG compression)
			if (jpeg_.size())
			{
				std::cerr << "Banded JPEG PICT" << std::endl;
				jpeg_.clear();
				done = true;
			}
			else if (!LoadJPEG(stream))
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
				std::cerr << "Unimplemented opcode" << std::hex << opcode << std::dec << " at " << stream.tellg() << std::endl;
				done = true;
			}
			break;
		}
	}

	if (bitmap_.TellHeight() == 1 && bitmap_.TellWidth() == 1 && !jpeg_.size())
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
	Rect rect;
	rect.Load(stream);

	uint16 width = rect.width();
	uint16 height = rect.height();
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

	return true;
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
	if (offset_x != 0 || offset_y != 0)
	{
		std::cerr << "Banded JPEG PICT" << std::endl;
		return false;
	}

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
	{
		std::cerr << "QuickTime compression only supports JPEG" << std::endl;
		return false;
	}

	stream.ignore(36); // resvd1/resvd2/dataRefIndex/version/revisionLevel/vendor/temporalQuality/spatialQuality/width/height/hRes/vRes
	uint32 data_size;
	stream >> data_size;
	stream.ignore(38); // frameCount/name/depth/clutID

	jpeg_.resize(data_size);
	stream.read(&jpeg_[0], jpeg_.size());
	
	stream.ignore(opcode_start + opcode_size - stream.tellg());
	return true;
}

template <class T>
static std::vector<uint8> PackRow(const std::vector<T>& scan_line)
{
	std::vector<uint8> result(scan_line.size() * sizeof(T) * 2);
	AOStreamBE stream(&result[0], result.size());

	typename std::vector<T>::const_iterator run = scan_line.begin();
	typename std::vector<T>::const_iterator start = scan_line.begin();
	typename std::vector<T>::const_iterator end = scan_line.begin() + 1;

	while (end != scan_line.end())
	{
		if (*end != *(end - 1))
		{
			run = end;
		}
		
		end++;
		if (end - run == 3)
		{
			if (run > start)
			{
				uint8 block_length = run - start - 1;
				stream << block_length;
				while (start < run)
				{
					stream << *start++;
				}
			}
			while (end != scan_line.end() && *end == *(end - 1) && end - run < 128)
			{
				++end;
			}
			uint8 run_length = 1 - (end - run);
			stream << run_length;
			stream << *run;
			run = end;
			start = end;
		}
		else if (end - start == 128)
		{
			uint8 block_length = end - start - 1;
			stream << block_length;
			while (start < end)
			{
				stream << *start++;
			}
			run = end;
		}
	}
	if (end > start)
	{
		uint8 block_length = end - start - 1;
		stream << block_length;
		while (start < end)
		{
			stream << *start++;
		}
	}

	result.resize(stream.tellp());
	return result;
}

bool PICTResource::LoadRaw(const std::vector<uint8>& data, const std::vector<uint8>& clut)
{
	AIStreamBE stream(&data[0], data.size());
	Rect rect;
	rect.Load(stream);

	int height = rect.height();
	int width = rect.width();
	
	int16 depth;
	stream >> depth;
	

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

std::vector<uint8> PICTResource::SaveBMP() const
{
	std::vector<uint8> result;
	
	int depth = const_cast<BMP&>(bitmap_).TellBitDepth();
	int width = const_cast<BMP&>(bitmap_).TellWidth();
	int height = const_cast<BMP&>(bitmap_).TellHeight();
	if (depth < 8) 
		depth = 8;
	else if (depth == 24)
		depth = 32;
	
	// size(2), rect(8), versionOp(2), version(2), headerOp(26), clip(12)
	int output_length = 10 + 2 + 2 + HeaderOp::kSize + 12;
	int row_bytes;
	if (depth == 8)
	{
		row_bytes = width;
		// opcode(2), pixmap(46), colorTable(8+256*8), srcRect/dstRect/mode(18)
		output_length += 2 + PixMap::kSize + 8 + 256 * 8 + 18;
		
	}
	else
	{
		row_bytes = width * (depth == 16 ? 2 : 4);
		// opcode(2), pmBaseAddr(4), pixmap(46), srcRect/dstRect/mode(18)
		output_length += 2 + 4 + PixMap::kSize + 18;
	}

	// data is variable--allocate twice what we need
	output_length += height * row_bytes * 2;
	result.resize(output_length);
	AOStreamBE ostream(&result[0], result.size());

	int16 size = 0;
	Rect clipRect(width, height);

	ostream << size;
	clipRect.Save(ostream);

	int16 versionOp = 0x0011;
	int16 version = 0x02ff;
	
	ostream << versionOp
		<< version;

	HeaderOp headerOp;
	headerOp.srcRect = clipRect;
	headerOp.Save(ostream);

	int16 clip = 0x0001;
	int16 clipSize = 10;
	ostream << clip
	       << clipSize;

	clipRect.Save(ostream);

	if (depth == 8)
	{
		int16 PackBitsRectOp = 0x0098;
		ostream << PackBitsRectOp;
	}
	else
	{
		int16 DirectBitsRectOp = 0x009a;
		uint32 pmBaseAddr = 0x000000ff;
		ostream << DirectBitsRectOp
			<< pmBaseAddr;
	}

	PixMap pixMap(depth, row_bytes);
	pixMap.bounds = clipRect;
	pixMap.Save(ostream);

	// color table
	if (depth == 8)
	{
		int32 seed = 0;
		int16 flags = 0;
		int16 size = 255;
		
		ostream << seed
			<< flags
			<< size;

		for (int16 index = 0; index < 256; ++index)
		{
			RGBApixel pixel = const_cast<BMP&>(bitmap_).GetColor(index);
			uint16 red = pixel.Red << 8;
			uint16 green = pixel.Green << 8;
			uint16 blue = pixel.Blue << 8;

			ostream << index
				<< red
				<< green
				<< blue;
		}
	}

	// source
	clipRect.Save(ostream);
	// destination
	clipRect.Save(ostream);

	int16 transfer_mode = 0;
	ostream << transfer_mode;

	std::map<RGBApixel, int> color_map; // for faster saving of 8-bit images
	if (depth == 8)
	{
		for (int i = 255; i >= 0; --i)
		{
			color_map[const_cast<BMP&>(bitmap_).GetColor(i)] = i;
		}
	}

	for (int y = 0; y < height; ++y)
	{
		std::vector<uint8> scan_line;
		if (depth == 8)
		{
			std::vector<uint8> pixels(width);
			for (int x = 0; x < width; ++x)
			{
				pixels[x] = color_map[bitmap_.GetPixel(x, y)];
			}

			scan_line = PackRow(pixels);
		}
		else if (depth == 16)
		{
			std::vector<uint16> pixels(width);
			for (int x = 0; x < width; ++x)
			{
				uint16 red = bitmap_.GetPixel(x, y).Red >> 3;
				uint16 green = bitmap_.GetPixel(x, y).Green >> 3;
				uint16 blue = bitmap_.GetPixel(x, y).Blue >> 3;
				pixels[x] = (red << 10) | (green << 5) | blue;
			}

			scan_line = PackRow(pixels);
		}
		else
		{
			std::vector<uint8> pixels(width * 3);
			for (int x = 0; x < width; ++x)
			{
				pixels[x] = bitmap_.GetPixel(x, y).Red;
				pixels[x + width] = bitmap_.GetPixel(x, y).Green;
				pixels[x + width * 2] = bitmap_.GetPixel(x, y).Blue;
			}

			scan_line = PackRow(pixels);
		}

		if (row_bytes > 250)
			ostream << static_cast<uint16>(scan_line.size());
		else
			ostream << static_cast<uint8>(scan_line.size());
	
		ostream.write(&scan_line[0], scan_line.size());
	}

	if (ostream.tellp() & 1)
		ostream.ignore(1);

	int16 endPic = 0x00ff;
	ostream << endPic;
	result.resize(ostream.tellp());

	return result;
}

static bool ParseJPEGDimensions(const std::vector<uint8>& data, int16& width, int16& height)
{
	try
	{

		AIStreamBE stream(&data[0], data.size());
		uint16 magic;
		stream >> magic;
		if (magic != 0xffd8)
			return false;
		
		while (stream.tellg() < stream.maxg())
		{
			// eat until we find 0xff
			uint8 c;
			do 
			{
				stream >> c;
			} 
			while (c != 0xff);

			// eat 0xffs until we find marker code
			do
			{
				stream >> c;
			}
			while (c == 0xff);

			switch (c) 
			{
			case 0xd9: // end of image
			case 0xda: // start of scan
				return false;
				break;
			case 0xc0:
			case 0xc1:
			case 0xc2:
			case 0xc3:
			case 0xc5:
			case 0xc6:
			case 0xc7:
			case 0xc8:
			case 0xc9:
			case 0xca:
			case 0xcb:
			case 0xcd:
			case 0xce:
			case 0xcf: {
				// start of frame
				uint16 length;
				uint8 precision;
				stream >> length
				       >> precision
				       >> height
				       >> width;
				
				return true;
				break;
			}
			default: 
			{
				uint16 length;
				stream >> length;
				if (length < 2)
					return false;
				else
					stream.ignore(length - 2);
				break;
			}
			}
		}

		return false;
	}
	catch (const AStream::failure&)
	{
		return false;
	}
}

std::vector<uint8> PICTResource::SaveJPEG() const
{
	std::vector<uint8> result;

	int16 width, height;
	if (!ParseJPEGDimensions(jpeg_, width, height)) 
		return result;

	// size(2), rect(8), versionOp(2), version(2), headerOp(26), clip(12)
	int output_length = 10 + 2 + 2 + HeaderOp::kSize + 12;

	// PICT opcode
	output_length += 76;
	
	// image description
	output_length += 86;

	output_length += jpeg_.size();

	// end opcode
	output_length += 2;

	if (output_length & 1)
		output_length++;

	result.resize(output_length);
	AOStreamBE ostream(&result[0], result.size());

	int16 size = 0;
	Rect clipRect(width, height);

	ostream << size;
	clipRect.Save(ostream);

	int16 versionOp = 0x0011;
	int16 version = 0x02ff;

	ostream << versionOp
		<< version;

	HeaderOp headerOp;
	headerOp.srcRect = clipRect;
	headerOp.Save(ostream);

	int16 clip = 0x0001;
	int16 clipSize = 10;
	ostream << clip
		<< clipSize;
	clipRect.Save(ostream);

	uint16 opcode = 0x8200;
	uint32 opcode_size = 154 + jpeg_.size();
	ostream << opcode
		<< opcode_size;

	ostream.ignore(2); // version
	std::vector<int16> matrix(18);
	matrix[0] = 1;
	matrix[8] = 1;
	matrix[16] = 0x4000;

	for (int i = 0; i < matrix.size(); ++i)
	{
		ostream << matrix[i];
	}
	
	ostream.ignore(4); // matte size
	ostream.ignore(8); // matte rect
	
	uint16 transfer_mode = 0x0040;
	ostream << transfer_mode;
	clipRect.Save(ostream);
	uint32 accuracy = 768;
	ostream << accuracy;
	ostream.ignore(4); // mask size
	
	uint32 id_size = 86;
	uint32 codec_type = FOUR_CHARS_TO_INT('j','p','e','g');
	ostream << id_size
		<< codec_type;
	ostream.ignore(8); // rsrvd1, rsrvd2, dataRefIndex
	ostream.ignore(4); // revision, revisionLevel
	ostream.ignore(4); // vendor
	ostream.ignore(4); // temporalQuality
	uint32 res = 72 << 16;
	ostream << accuracy // spatialQuality
		<< width
		<< height
		<< res // hRes
		<< res; // vRes

	uint32 data_size = jpeg_.size();
	uint16 frame_count = 1;
	ostream << data_size
		<< frame_count;
	ostream.ignore(32); // name
	int16 depth = 32;
	int16 clut_id = -1;
	ostream << depth
		<< clut_id;
	
	ostream.write(&jpeg_[0], jpeg_.size());

	if (ostream.tellp() & 1)
		ostream.ignore(1);

	int16 endPic = 0x00ff;
	ostream << endPic;
	
	return result;
	
}

std::vector<uint8> PICTResource::Save() const
{
	if (const_cast<BMP&>(bitmap_).TellHeight() != 1 || const_cast<BMP&>(bitmap_).TellWidth() != 1)
	{
		return SaveBMP();
	}
	else if (jpeg_.size())
	{
		return SaveJPEG();
	}
	else
	{
		return data_;
	}
}

bool PICTResource::Import(const std::string& path)
{
	data_.clear();
	jpeg_.clear();
	if (algo::ends_with(path, ".bmp"))
	{
		bitmap_.ReadFromFile(path.c_str());
	}
	else if (algo::ends_with(path, ".jpg"))
	{
		std::ifstream infile(path.c_str(), std::ios::binary);
		infile.seekg(0, std::ios::end);
		std::streamsize length = infile.tellg();
		
		infile.seekg(0);
		jpeg_.resize(length);
		infile.read(reinterpret_cast<char*>(&jpeg_[0]), jpeg_.size());
	}
	else
	{
		std::ifstream infile(path.c_str(), std::ios::binary);
		infile.seekg(0, std::ios::end);
		std::streamsize length = infile.tellg();
		
		if (length < 528) return false;
		infile.seekg(512);
		
		std::vector<uint8> pict_data(length - 512);
		infile.read(reinterpret_cast<char*>(&pict_data[0]), pict_data.size());
		Load(pict_data);
	}
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

void PICTResource::Rect::Load(AIStreamBE& stream)
{
	stream >> top;
	stream >> left;
	stream >> bottom;
	stream >> right;
}

void PICTResource::Rect::Save(AOStreamBE& stream) const
{
	stream << top;
	stream << left;
	stream << bottom;
	stream << right;
}

void PICTResource::HeaderOp::Load(AIStreamBE& stream)
{
	stream >> headerVersion;
	stream >> reserved1;
	stream >> hRes;
	stream >> vRes;
	srcRect.Load(stream);
	stream >> reserved2;
}

void PICTResource::HeaderOp::Save(AOStreamBE& stream) const
{
	stream << headerOp;
	stream << headerVersion;
	stream << reserved1;
	stream << hRes;
	stream << vRes;
	srcRect.Save(stream);
	stream << reserved2;
}

PICTResource::PixMap::PixMap(int depth, int rowBytes_) : rowBytes(rowBytes_ | 0x8000), pmVersion(0), packType(0), packSize(0), hRes(72 << 16), vRes(72 << 16), pixelSize(depth), planeBytes(0), pmTable(0), pmReserved(0)
{
	if (depth == 8)
	{
		pixelType = 0;
		cmpSize = 8;
		cmpCount = 1;
	}
	else if (depth == 16)
	{
		pixelType = kRGBDirect;
		cmpSize = 5;
		cmpCount = 3;
	}
	else 
	{
		pixelType = kRGBDirect;
		cmpSize = 8;
		cmpCount = 3;
	}
}

void PICTResource::PixMap::Save(AOStreamBE& stream) const
{
	stream << rowBytes;
	bounds.Save(stream);
	stream << pmVersion
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
}

void PICTResource::PixMap::Load(AIStreamBE& stream)
{
	stream >> rowBytes;
	bounds.Load(stream);
	stream >> pmVersion
	       >> packType
	       >> packSize
	       >> hRes
	       >> vRes
	       >> pixelType
	       >> pixelSize
	       >> cmpCount
	       >> cmpSize
	       >> planeBytes
	       >> pmTable
	       >> pmReserved;
}
