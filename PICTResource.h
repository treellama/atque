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
class AOStreamBE;

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

		struct Rect
		{
			int16 top;
			int16 left;
			int16 bottom;
			int16 right;

			void Load(AIStreamBE&);
			void Save(AOStreamBE&) const;

			int16 height() const { return bottom - top; }
			int16 width() const { return right - left; }

			Rect() { }
			Rect(int width, int height) : top(0), left(0), bottom(height), right(width) { }
		};

	private:
		bool LoadCopyBits(AIStreamBE& stream, bool packed, bool clipped);
		bool LoadJPEG(AIStreamBE& stream);
		std::vector<uint8> SaveJPEG() const;
		std::vector<uint8> SaveBMP() const;
		BMP bitmap_;

		struct HeaderOp
		{
			enum {
				kTag = 0x0c00,
				kVersion = 0xfffe,
				kSize = 26
			};

			int16 headerOp;
			int16 headerVersion;
			int16 reserved1;
			int32 hRes;
			int32 vRes;
			Rect srcRect;
			int32 reserved2;

			HeaderOp() : headerOp(kTag), headerVersion(kVersion), reserved1(0), hRes(72 << 16), vRes(72 << 16), reserved2(0) { }
			void Load(AIStreamBE&);
			void Save(AOStreamBE&) const;
		};

		struct PixMap
		{
			enum {
				kSize = 46,
				kRGBDirect = 0x10
			};

			int16 rowBytes;
			Rect bounds;
			int16 pmVersion;
			int16 packType;
			uint32 packSize;
			uint32 hRes;
			uint32 vRes;
			int16 pixelType;
			int16 pixelSize;
			int16 cmpCount;
			int16 cmpSize;
			uint32 planeBytes;
			uint32 pmTable;
			uint32 pmReserved;

			PixMap() { };
			PixMap(int depth, int rowBytes);

			void Load(AIStreamBE&);
			void Save(AOStreamBE&) const;
		};

		std::vector<uint8> data_;
		std::vector<uint8> jpeg_;
	};

}

#endif
