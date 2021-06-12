/* CLUTResource.cpp

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
#include "CLUTResource.h"

#include "EasyBMP.h"

#include <fstream>

#include <boost/algorithm/string/predicate.hpp>

using namespace atque;
namespace algo = boost::algorithm;

void CLUTResource::Load(const std::vector<uint8>& data)
{
	if (data.size() == 6 + 256 * 6)
	{
		// M2/Win95 'clut'
		AIStreamBE stream(&data[0], data.size());
		int16 count;
		stream >> count;
		stream.ignore(4); // unknown, id

		colors_.resize(count);
		for (int i = 0; i < count; ++i)
		{
			stream >> colors_[i].red
			       >> colors_[i].green
			       >> colors_[i].blue;
		}
	}
	else
	{
		AIStreamBE stream(&data[0], data.size());
		stream.ignore(4); // seed
		int16 flags;
		stream >> flags;
		int16 size;
		stream >> size;

		colors_.resize(size + 1);
		for (int i = 0; i < colors_.size(); ++i)
		{
			int16 index;
			stream >> index;
			if (flags & 0x8000)
				index = i;
			else
				index &= 0xff;
			
			stream >> colors_[index].red
			       >> colors_[index].green
			       >> colors_[index].blue;
		}
	}
}

std::vector<uint8> CLUTResource::Save() const
{
	std::vector<uint8> result(6 + 256 * 6);
	AOStreamBE stream(&result[0], result.size());
	stream << static_cast<int16>(colors_.size());
	stream.ignore(4); // unknown, id
	
	for (int i = 0; i < colors_.size(); ++i)
	{
		stream << colors_[i].red
		       << colors_[i].green
		       << colors_[i].blue;
	}

	return result;
}

bool CLUTResource::Import(const std::string& path)
{
	if (algo::iends_with(path, ".act"))
	{
		std::ifstream infile(path.c_str(), std::ios::binary);
		infile.seekg(0, std::ios::end);
		uint32 size = infile.tellg();
		infile.seekg(0, std::ios::beg);
		if (size < 3 * 256)
			return false;
		
		std::vector<uint8> actData(size);
		infile.read(reinterpret_cast<char*>(&actData[0]), actData.size());
		AIStreamBE stream(&actData[0], actData.size());
		colors_.resize(256);
		for (int i = 0; i < colors_.size(); ++i)
		{
			uint8 red;
			uint8 green;
			uint8 blue;
			
			stream >> red >> green >> blue;
			
			colors_[i].red = red << 8;
			colors_[i].green = green << 8;
			colors_[i].blue = blue << 8;
		}
		
		if (size >= 3 * 256 + 4)
		{
			int16 colorCount;
			stream >> colorCount;
			if (colorCount < 256)
				colors_.resize(colorCount);
		}
		
		return true;
	}
	else if (algo::iends_with(path, ".bmp"))
	{
		BMP bitmap;
		if (bitmap.ReadFromFile(path.c_str()) && bitmap.TellNumberOfColors() <= 256)
		{
			colors_.resize(256);
			for (int i = 0; i < bitmap.TellNumberOfColors(); ++i)
			{
				RGBApixel color = bitmap.GetColor(i);
				colors_[i].red = color.Red << 8;
				colors_[i].green = color.Green << 8;
				colors_[i].blue = color.Blue << 8;
			}

			return true;
		}
	}

	return false;
}

void CLUTResource::Export(const std::string& path) const
{
	std::ofstream outfile(path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	std::vector<uint8> actData(3 * 256 + 4);
	AOStreamBE stream(&actData[0], actData.size());
	
	for (int i = 0; i < colors_.size(); ++i)
	{
		uint8 red = (colors_[i].red >> 8);
		uint8 green = (colors_[i].green >> 8);
		uint8 blue = (colors_[i].blue >> 8);

		stream << red << green << blue;
	}

	stream << static_cast<int16>(colors_.size());
	int16 transparent_color = 0;
	stream << transparent_color;

	outfile.write(reinterpret_cast<char *>(&actData[0]), actData.size());
	
}
