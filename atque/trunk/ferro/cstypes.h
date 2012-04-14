/* cstypes.h

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

#ifndef CSTYPES_H
#define CSTYPES_H

#include <boost/cstdint.hpp>

enum {
	NONE = -1,
	UNONE = 65536
};

typedef boost::uint8_t uint8;
typedef boost::int8_t int8;
typedef boost::uint16_t uint16;
typedef boost::int16_t int16;
typedef boost::uint32_t uint32;
typedef boost::int32_t int32;

#define FOUR_CHARS_TO_INT(a,b,c,d) (((uint32)(a) << 24) | ((uint32)(b) << 16) | ((uint32)(c) << 8) | (uint32)(d))

#endif

