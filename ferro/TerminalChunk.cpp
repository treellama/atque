/* TerminalChunk.cpp

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

#include "AStream.h"
#include "ferro/macroman.h"
#include "ferro/TerminalChunk.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/assign.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace marathon;
namespace algo = boost::algorithm;

void TerminalText::TerminalGrouping::Load(AIStreamBE& stream)
{
	stream >> flags_;
	stream >> type_;
	stream >> permutation_;
	stream >> start_index_;
	stream >> length_;
	stream >> maximum_line_count_;
}

void TerminalText::TerminalGrouping::Save(AOStreamBE& stream) const
{
	stream << flags_;
	stream << type_;
	stream << permutation_;
	stream << start_index_;
	stream << length_;
	stream << maximum_line_count_;
}

std::string TerminalText::FontChange::Diff(const FontChange& p, const FontChange& n)
{
	std::ostringstream diff;
	if (p.color_ != n.color_)
	{
		diff << "$C" << n.color_;
	}

	int16 face_change = p.face_ ^ n.face_;
	if (face_change & kBold)
	{
		diff << ((n.face_ & kBold) ? "$B" : "$b");
	}

	if (face_change & kItalic)
	{
		diff << ((n.face_ & kItalic) ? "$I" : "$i");
	}

	if (face_change & kUnderline)
	{
		diff << ((n.face_ & kUnderline) ? "$U" : "$u");
	}

	return diff.str();
}

void TerminalText::FontChange::Load(AIStreamBE& stream)
{
	stream >> index_;
	stream >> face_;
	stream >> color_;
}

void TerminalText::FontChange::Save(AOStreamBE& stream) const
{
	stream << index_;
	stream << face_;
	stream << color_;
}

void TerminalText::Load(AIStreamBE& stream)
{
	uint32 start = stream.tellg();

	groupings_.clear();
	font_changes_.clear();
	text_.clear();

	// read header
	uint16 total_length, grouping_count, font_changes_count;
	stream >> total_length;
	stream >> flags_;
	stream >> lines_per_page_;
	stream >> grouping_count;
	stream >> font_changes_count;

	// read groupings
	for (int i = 0; i < grouping_count; ++i)
	{
		TerminalGrouping grouping;
		grouping.Load(stream);
		groupings_.push_back(grouping);
	}

	for (int i = 0; i < font_changes_count; ++i)
	{
		FontChange font_change;
		font_change.Load(stream);
		font_changes_.push_back(font_change);
	}

	// anything remaining is the text
	text_.resize(total_length - (stream.tellg() - start));
	stream.read(&text_[0], text_.size());

	DecodeText();
}

void TerminalText::Save(AOStreamBE& stream) const
{
	stream << static_cast<uint16>(GetSize());
	stream << flags_;
	stream << lines_per_page_;
	stream << static_cast<uint16>(groupings_.size());
	stream << static_cast<uint16>(font_changes_.size());

	for (std::vector<TerminalGrouping>::const_iterator it = groupings_.begin(); it != groupings_.end(); ++it)
	{
		it->Save(stream);
	}

	for (std::vector<FontChange>::const_iterator it = font_changes_.begin(); it != font_changes_.end(); ++it)
	{
		it->Save(stream);
	}

	stream.write(&text_[0], text_.size());
}

uint32 TerminalText::GetSize() const
{
	return kHeaderSize + (groupings_.size() * TerminalGrouping::kSize) + (font_changes_.size() * FontChange::kSize) + text_.size();
}

void TerminalText::EncodeText()
{
	uint8 *p = &text_[0];

	for (int i = 0; i < text_.size() / 4; ++i)
	{
		p += 2;
		*p++ ^= 0xfe;
		*p++ ^= 0xed;
	}

	for (int i = 0; i < text_.size() % 4; ++i)
	{
		*p++ ^= 0xfe;
	}

	flags_ |= kTextIsEncoded;
}

void TerminalText::DecodeText()
{
	if (flags_ & kTextIsEncoded)
	{
		EncodeText();
		flags_ &= ~kTextIsEncoded;
	}
}

struct group_header_data {
	std::string name;
	bool has_permutation;

	group_header_data(const char *name_, bool has_permutation_) : name(name_), has_permutation(has_permutation_) { }
};

static std::vector<group_header_data> group_data = boost::assign::list_of<group_header_data>
	("LOGON", true ) // permutation is the logo id to draw...
	("UNFINISHED", false )
	("FINISHED", false )
	("FAILED", false )
	("INFORMATION", false )
	("END", false )
	("INTERLEVEL TELEPORT", true )
	("INTRALEVEL TELEPORT", true )
	("CHECKPOINT", true )
	("SOUND", true )
	("MOVIE", true )
	("TRACK", true )
	("PICT", true)
	("LOGOFF", true ) // permutation is the logo id to draw...
	("CAMERA", true ) // permutation is the object index
	("STATIC", true ) // permutation is the duration of static.
	("TAG", true ) // permutation is the tag to activate
	;

static std::string get_line(std::istream& stream)
{
	std::string line;
	char c;
	stream.get(c);
	while (!stream.eof())
	{
		if (c == '\r')
		{
			if (stream.peek() != '\n')
			{
				break;
			}
		}
		else if (c == '\n')
		{
			break;
		}
		else if (c != '\0')
		{
			line += c;
		}

		stream.get(c);
	}

	return utf8_to_mac_roman(line);
}

static int find_group(const std::string& line)
{
	if (!line.size()) return -1;

	for (int i = 0; i < group_data.size(); ++i)
	{
		if (algo::starts_with(line, group_data[i].name))
		{
			return i;
		}
	}

	return -1;
}

void TerminalText::CompileLine(FontChange *font, const std::string& line)
{
	for (std::string::const_iterator it = line.begin(); it != line.end(); ++it)
	{
		if (*it == '$' && (it + 1 != line.end()))
		{
			font->index_ = text_.size();
			char c = *(it + 1);
			if (c == 'B')
			{
				font->face_ |= FontChange::kBold;
				font_changes_.push_back(*font);
				++it;
			}
			else if (c == 'b')
			{
				font->face_ &= ~FontChange::kBold;
				font_changes_.push_back(*font);
				++it;
			}
			else if (c == 'I')
			{
				font->face_ |= FontChange::kItalic;
				font_changes_.push_back(*font);
				++it;
			}
			else if (c == 'i')
			{
				font->face_ &= ~FontChange::kItalic;
				font_changes_.push_back(*font);
				++it;
			}
			else if (c == 'U')
			{
				font->face_ |= FontChange::kUnderline;
				font_changes_.push_back(*font);
				++it;
			}
			else if (c == 'u')
			{
				font->face_ &= ~FontChange::kUnderline;
				font_changes_.push_back(*font);
				++it;
			}
			else if (c == 'C' && (it + 2 != line.end()))
			{
				char cc = *(it + 2);
				if (cc >= '0' && cc <= '7')
				{
					font->color_ = cc - '0';
					font_changes_.push_back(*font);
					it += 2;
				}
				else
				{
					text_.push_back(*it);
				}
			}
			else
			{
				text_.push_back(*it);
			}
		}
		else
		{
			text_.push_back(*it);
		}
	}

	text_.push_back('\r');
}

// this is very terrible

static bool calculate_line(char *base_text, short width, short start_index, short text_end_index, short *end_index)
{
	bool done = false;

	if (base_text[start_index]) {
		int index = start_index, running_width = 0;
		
		while (running_width < width && base_text[index] && base_text[index] != '\r') {
			if (base_text[index] == '\t' || base_text[index] >= ' ')
				running_width += 7;
			index++;
		}
		
		// Now go backwards, looking for whitespace to split on
		if (base_text[index] == '\r')
			index++;
		else if (base_text[index]) {
			int break_point = index;

			while (break_point>start_index) {
				if (base_text[break_point] == ' ')
					break; 	// Non printing
				break_point--;	// this needs to be in front of the test
			}
			
			if (break_point != start_index)
				index = break_point+1;	// Space at the end of the line
		}
		
		*end_index= index;
	} else
		done = true;
	
	return done;
}

static short count_total_lines(
	char *base_text,
	short width,
	short start_index,
	short end_index)
{
	short total_line_count= 0;
	short text_end_index= end_index;

	while(!calculate_line(base_text, width, start_index, text_end_index, &end_index))
	{
		total_line_count++;
		start_index= end_index;
	}

	return total_line_count;
}

int TerminalText::CalculateMaximumLines(const TerminalGrouping& group)
{
	const int BORDER_INSET = 9;
	switch (group.type_)
	{
	case TerminalGrouping::kLogon:
	case TerminalGrouping::kLogoff:
	case TerminalGrouping::kInterlevelTeleport:
	case TerminalGrouping::kIntralevelTeleport:
	case TerminalGrouping::kSound:
	case TerminalGrouping::kTag:
	case TerminalGrouping::kMovie:
	case TerminalGrouping::kTrack:
	case TerminalGrouping::kCamera:
	case TerminalGrouping::kStatic:
	case TerminalGrouping::kEnd:
		return 1;
		break;
	case TerminalGrouping::kUnfinished:
	case TerminalGrouping::kSuccess:
	case TerminalGrouping::kFailure:
		return 0;
		break;
	case TerminalGrouping::kCheckpoint:
	case TerminalGrouping::kPict:
	{
		if (group.flags_ & TerminalGrouping::kCenterObject)
			return 1;

		int width = (640 - 2 * (BORDER_INSET)) / 2 - BORDER_INSET / 2;

		return count_total_lines(reinterpret_cast<char *>(&text_[0]), width, group.start_index_, group.start_index_ + group.length_);
	}
	break;
	case TerminalGrouping::kInformation:
	{
		int width = 640 - 2 * (72 - BORDER_INSET);
		return count_total_lines(reinterpret_cast<char *>(&text_[0]), width, group.start_index_, group.start_index_ + group.length_);
	}
	break;
	default:
		return 0;
		break;
	}
}

void TerminalText::CompileGroup(std::vector<std::string>::const_iterator* it, const std::vector<std::string>::iterator& end)
{
	TerminalGrouping group;
	FontChange font;
	group.start_index_ = text_.size();
	bool group_start = false;
	// read until start of group
	while (*it != end && !group_start)
	{
		std::string line = **it;
		if (line[0] == '#')
		{
			group.type_ = find_group(line.substr(1));
			if (group.type_ >= 0)
			{
				if (group_data[group.type_].has_permutation)
				{
					std::istringstream stream(line.substr(1 + group_data[group.type_].name.size()));
					stream >> group.permutation_;
					if (group.type_ == TerminalGrouping::kPict)
					{
						std::string position;
						stream >> position;
						
						if (position == "RIGHT")
							group.flags_ = TerminalGrouping::kDrawObjectOnRight;
						else if (position == "CENTER")
							group.flags_ = TerminalGrouping::kCenterObject;
					}
				}
				group_start = true;
			}
		}

		++(*it);
	}

	// compile lines until end of group
	bool group_end = false;
	if (group.type_ == TerminalGrouping::kPict && (group.flags_ & TerminalGrouping::kCenterObject))
	{
		// text here will cause Aleph One to go into an infinite loop
		group_end = true;
	}
	while(*it != end && !group_end)
	{
		std::string line = **it;
		if (line[0] == '#')
		{
			if (find_group(line.substr(1)) >= 0)
			{
				group_end = true;
			}
			else
			{
				CompileLine(&font, line);
			}
		}
		else if (line[0] != ';')
		{
			CompileLine(&font, line);
		}
		if (!group_end) ++(*it);
	}

	if (group_start)
	{
		text_.push_back('\0');
		group.length_ = text_.size() - group.start_index_;
		group.maximum_line_count_ = CalculateMaximumLines(group);
		groupings_.push_back(group);
	}
}

bool TerminalText::Compile(std::istream& stream, int expected_id)
{
	text_.clear();
	groupings_.clear();
	font_changes_.clear();
	
	int terminal_id = 0;

	// read until start of text
	bool start = false;
	while (!stream.eof() && !start)
	{
		std::string line = get_line(stream);
		if (algo::starts_with(line, "#TERMINAL"))
		{
			std::istringstream s(line);
			std::string temp;
			s >> temp >> terminal_id;
			if (s.fail())
				throw TerminalChunk::ParseError("#TERMINAL without number");
			else if (terminal_id != expected_id)
				throw TerminalChunk::ParseError("misnumbered #TERMINAL");
			start = true;
		}
		else if (line[0] != '\0' && line[0] != ';')
		{
			throw TerminalChunk::ParseError("expected #TERMINAL");
		}
	}

	if (!start)
		return false;

	std::vector<std::string> lines;
	// read/append lines until end of text
	bool end = false;
	while (!stream.eof() && !end)
	{
		std::string line = get_line(stream);
		if (algo::starts_with(line, "#ENDTERMINAL"))
			end = true;
		else
			lines.push_back(line);
	}

	if (end)
	{
		std::vector<std::string>::const_iterator it = lines.begin();
			while (it != lines.end())
			{
				CompileGroup(&it, lines.end());
			}
	}
	else
	{
		throw TerminalChunk::ParseError("expected #ENDTERMINAL");
	}

	EncodeText();
	
	const int DEFAULT_WORLD_HEIGHT = 320;
	const int BORDER_HEIGHT = 18;
	const int FONT_LINE_HEIGHT = 12;
	const int FUDGE_FACTOR = 1;
	lines_per_page_ = (DEFAULT_WORLD_HEIGHT - 2 * BORDER_HEIGHT) / FONT_LINE_HEIGHT - FUDGE_FACTOR;
	
	return true;
}

void TerminalText::Decompile(std::ostream& stream) const
{
	std::vector<FontChange>::const_iterator font_iterator = font_changes_.begin();
	for (std::vector<TerminalGrouping>::const_iterator it = groupings_.begin(); it != groupings_.end(); ++it)
	{
		FontChange currentFont;

		stream << "#" << group_data[it->type_].name;
		if (group_data[it->type_].has_permutation)
		{
			stream << " " << it->permutation_;
			if (it->type_ == TerminalGrouping::kPict)
			{
				if (it->flags_ & TerminalGrouping::kDrawObjectOnRight)
				{
					stream << " RIGHT";
				}
				else if (it->flags_ & TerminalGrouping::kCenterObject)
				{
					stream << " CENTER";
				}
			}
		}
		stream << std::endl;
		int end = it->start_index_ + it->length_;
		int index = it->start_index_;
		while (index < end)
		{
			if (font_iterator != font_changes_.end() && index == font_iterator->index_)
			{
				stream << FontChange::Diff(currentFont, *font_iterator);
				currentFont = *font_iterator;
				++font_iterator;		
			}
			else if (text_[index] == '\r')
			{
				++index;
				stream << std::endl;
			}
			else if (text_[index] == '\0')
			{
				++index;
			}
			else
			{
				std::string temp;
				temp += text_[index];
				stream << mac_roman_to_utf8(temp);
				++index;
			}
		}
	}
}

void TerminalChunk::Load(const std::vector<uint8>& data)
{
	terminal_texts_.clear();
	AIStreamBE stream(&data[0], data.size());

	while (stream.tellg() < stream.maxg())
	{
		TerminalText terminal_text;
		terminal_text.Load(stream);
		terminal_texts_.push_back(terminal_text);
	}

}

std::vector<uint8> TerminalChunk::Save() const
{
	// calculate the size
	uint32 size = 0;
	for (std::vector<TerminalText>::const_iterator it = terminal_texts_.begin(); it != terminal_texts_.end(); ++it)
	{
		size += it->GetSize();
	}

	std::vector<uint8> v(size);
	AOStreamBE stream(&v[0], v.size());
	
	for (std::vector<TerminalText>::const_iterator it = terminal_texts_.begin(); it != terminal_texts_.end(); ++it)
	{
		it->Save(stream);
	}

	return v;
}

void TerminalChunk::Compile(const std::string& path)
{
	std::ifstream stream(path.c_str());

	terminal_texts_.clear();
	while (!stream.eof())
	{
		TerminalText tt;
		if (tt.Compile(stream, terminal_texts_.size()))
			terminal_texts_.push_back(tt);
	}
}

void TerminalChunk::Decompile(const std::string& path) const
{
	std::ofstream stream(path.c_str(), std::ios::out | std::ios::trunc);
	
	for (int index = 0; index < terminal_texts_.size(); ++index)
	{
		stream << ";" << std::endl;
		stream << "#TERMINAL " << index << std::endl;
		terminal_texts_[index].Decompile(stream);
		stream << "#ENDTERMINAL " << index << std::endl;
	}
}
