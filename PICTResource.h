/* PICTResource.h
                                                               
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

#ifndef PICT_RESOURCE_H
#define PICT_RESOURCE_H

#include "ferro/cstypes.h"
#include "EasyBMP.h"

#include <vector>

class AIStreamBE;

namespace atque
{

	class PICTResource
	{
	public:
		PICTResource() { }
		PICTResource(const std::vector<uint8>& data) { Load(data); }
		void Load(const std::vector<uint8>&);
		bool LoadRaw(const std::vector<uint8>& raw_data, const std::vector<uint8>& clut);
		std::vector<uint8> Save() const;

		bool Import(const std::string& path);
		void Export(const std::string& path);

	private:
		bool LoadCopyBits(AIStreamBE& stream, bool packed, bool clipped);
		BMP bitmap_;

		int16 top_;
		int16 left_;
		int16 bottom_;
		int16 right_;

		std::vector<uint8> data_;
		
	};

}

#endif
