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

#include <fstream>

using namespace atque;

void CLUTResource::Load(const std::vector<uint8>& data)
{
	if (data.size() == 6 + 256 * 6)
	{
		// M2/Win95 'clut'
		AIStreamBE stream(&data[0], data.size());
		stream.ignore(6);

		colors_.resize(256);
		for (int i = 0; i < 256; ++i)
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

void CLUTResource::Export(const std::string& path) const
{
	std::ofstream outfile(path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	std::vector<uint8> actData(3 * 256);
	AOStreamBE stream(&actData[0], actData.size());
	
	for (int i = 0; i < colors_.size(); ++i)
	{
		uint8 red = (colors_[i].red >> 8);
		uint8 green = (colors_[i].green >> 8);
		uint8 blue = (colors_[i].blue >> 8);

		stream << red << green << blue;
	}

	outfile.write(reinterpret_cast<char *>(&actData[0]), actData.size());
	
}
