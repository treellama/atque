/* MacBinaryII.cpp

   Copyright (C) 2026 by Gregory Smith
   
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

#include "MacBinaryII.h"

#include <boost/crc.hpp>

uint16_t MacBinaryII::CalculateCRC() const
{
	boost::crc_optimal<16, 0x1021, 0, 0, false, false> crc_algorithm;
	crc_algorithm.process_bytes(this, 124);
	return crc_algorithm.checksum();
}

bool MacBinaryII::IsValid() const
{
	if (old_version_number || filename_length > max_filename_length ||
		zero_fill1 || min_macbinary_version > 129)
	{
		return false;
	}

	return crc == CalculateCRC();
}
