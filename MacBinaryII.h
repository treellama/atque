/* MacBinaryII.h

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

#ifndef MACBINARYII_H
#define MACBINARYII_H

#include <boost/endian.hpp>

struct MacBinaryII
{
	static constexpr auto max_filename_length = 63;
	
	boost::endian::big_uint8_t old_version_number;
	boost::endian::big_uint8_t filename_length;
	char filename[max_filename_length];
	boost::endian::big_uint32_t file_type;
	boost::endian::big_uint32_t file_creator;
	boost::endian::big_uint8_t original_finder_flags;
	boost::endian::big_uint8_t zero_fill1;
	boost::endian::big_int16_t vertical_position;
	boost::endian::big_int16_t horizontal_position;
	boost::endian::big_int16_t window_or_folder_id;
	boost::endian::big_uint8_t protected_flag;
	boost::endian::big_uint8_t zero_fill2;
	boost::endian::big_uint32_t data_fork_length;
	boost::endian::big_int32_t resource_fork_length;
	boost::endian::big_int32_t creation_date;
	boost::endian::big_int32_t last_modified_date;
	boost::endian::big_int16_t get_info_comment_length;
	boost::endian::big_uint8_t finder_flags;
	char unused[14];
	boost::endian::big_int32_t packed_file_length;
	boost::endian::big_int16_t secondary_header_length;
	boost::endian::big_uint8_t macbinary_version;
	boost::endian::big_uint8_t min_macbinary_version;
	boost::endian::big_uint16_t crc;
	char padding[2];

	uint32_t GetDataOffset() const { return sizeof(MacBinaryII); }
	uint32_t GetResourceForkOffset() const {
		// pad to 128 bytes
		return GetDataOffset() + (data_fork_length + 0x7f) & ~0x7f;
	}

	uint16_t CalculateCRC() const;
	bool IsValid() const;
};

static_assert(sizeof(MacBinaryII) == 128);

#endif
