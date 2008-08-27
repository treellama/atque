/* SndResource.h
                                                               
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

#ifndef SND_RESOURCE_H
#define SND_RESOURCE_H

#include "ferro/cstypes.h"

#include <string>
#include <vector>

class AIStreamBE;

namespace atque
{
	class SndResource
	{
	public:
		SndResource() { }
		SndResource(const std::vector<uint8>& data) { Load(data); }
		bool Load(const std::vector<uint8>&);
		std::vector<uint8> Save() const;

		void Export(const std::string& path) const;
		bool Import(const std::string& path);

	private:
		bool UnpackStandardSystem7Header(AIStreamBE&);
		bool UnpackExtendedSystem7Header(AIStreamBE&);
		bool sixteen_bit_;
		bool stereo_;
		bool signed_8bit_;
		int bytes_per_frame_;
		uint32 rate_;

		std::vector<uint8> data_;
	};
}

#endif

		
