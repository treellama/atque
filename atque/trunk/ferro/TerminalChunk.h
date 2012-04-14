/* Terminal.h

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

#ifndef TERMINAL_H
#define TERMINAL_H

#include "ferro/cstypes.h"

#include <stdexcept>
#include <vector>

class AIStreamBE;
class AOStreamBE;

namespace marathon
{
	class TerminalText
	{
		friend class TerminalChunk;
	public:
		class TerminalGrouping
		{
			friend class TerminalText;
		private:
			enum { kSize = 12 };

			TerminalGrouping() : flags_(0), type_(0), permutation_(-1), start_index_(0), length_(0), maximum_line_count_(0) { }

			void Load(AIStreamBE& s);
			void Save(AOStreamBE& s) const;

			// types
			enum {
				kLogon,
				kUnfinished,
				kSuccess,
				kFailure,
				kInformation,
				kEnd,
				kInterlevelTeleport,
				kIntralevelTeleport,
				kCheckpoint,
				kSound,
				kMovie,
				kTrack,
				kPict,
				kLogoff,
				kCamera,
				kStatic,
				kTag
			};

			enum {
				kDrawObjectOnRight = 0x0001,
				kCenterObject = 0x0002
			};

			int16 flags_;
			int16 type_;
			int16 permutation_;
			int16 start_index_;
			int16 length_;
			int16 maximum_line_count_;
		};

		class FontChange
		{
			friend class TerminalText;
		private:
			enum { kSize = 6 };

			FontChange() : index_(0), face_(0), color_(0) { }
			static std::string Diff(const FontChange &p, const FontChange &n);
			void Load(AIStreamBE& s);
			void Save(AOStreamBE& s) const;

			enum // flags to indicate text styles for paragraphs
			{
				kPlain = 0x0,
				kBold = 0x1,
				kItalic = 0x2,
				kUnderline = 0x4
			};

			int16 index_;
			int16 face_;
			int16 color_;
		};

		TerminalText() { }

	private:

		enum { kTextIsEncoded = 0x0001 };

		enum { kHeaderSize = 10 };

		void Load(AIStreamBE&);
		void Save(AOStreamBE&) const;
		uint32 GetSize() const;

		void EncodeText();
		bool IsEncoded() const { return flags_ & kTextIsEncoded; }
		void DecodeText();

		bool Compile(std::istream& stream, int expected_id);
		void Decompile(std::ostream& stream) const;

		void CompileLine(FontChange* font, const std::string& line);
		void CompileGroup(std::vector<std::string>::const_iterator* it, const std::vector<std::string>::iterator& end);
		int CalculateMaximumLines(const TerminalGrouping& group);

		uint16 flags_;
		int16 lines_per_page_;

		std::vector<TerminalGrouping> groupings_;
		std::vector<FontChange> font_changes_;
		std::vector<uint8> text_;
	};

	class TerminalChunk
	{
	public:
		enum { kTag = FOUR_CHARS_TO_INT('t','e','r','m') };

		TerminalChunk() { }
		TerminalChunk(const std::vector<uint8>& data) { Load(data); }

		class ParseError : public std::runtime_error
		{
		public:
			ParseError(const std::string& what) : std::runtime_error(what) { }
		};

		void Load(const std::vector<uint8>&);
		void Decompile(const std::string& path) const;
		void Compile(const std::string& path);
		std::vector<uint8> Save() const;

	private:
		std::vector<TerminalText> terminal_texts_;
	};
};
#endif
