/* CLUTResource.h
                                                               
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

#ifndef CLUT_RESOURCE_H
#define CLUT_RESOURCE_H

#include "ferro/cstypes.h"

#include <string>
#include <vector>

namespace atque
{
	class CLUTResource
	{
	public:
		CLUTResource() { }
		CLUTResource(const std::vector<uint8>& data) { Load(data); }
		void Load(const std::vector<uint8>&);

		void Export(const std::string& path) const;

	private:
		struct Color
		{
			int16 red;
			int16 green;
			int16 blue;

			Color() : red(0), green(0), blue(0) { }
		};

		std::vector<Color> colors_;
	};
}

#endif

		

