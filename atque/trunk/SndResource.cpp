/* SndResource.cpp

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

#include <sndfile.h>

#include "ferro/AStream.h"
#include "SndResource.h"

#include <stdio.h> // SEEK_SET, SEEK_END, SEEK_CUR

using namespace atque;

static const int kBufferSize = 8192;

bool SndResource::Load(const std::vector<uint8>& data)
{
	AIStreamBE stream(&data[0], data.size());
	uint16 format;
	stream >> format;
	if (format != 1 && format != 2)
		return false;

	if (format == 1) {
		uint16 num_data_formats;
		stream >> num_data_formats;
		stream.ignore(num_data_formats * 6);
	} 
	else if (format == 2)
	{
		stream.ignore(2);
	}

	uint16 num_commands;
	stream >> num_commands;
	for (int i = 0; i < num_commands; i++)
	{
		uint16 cmd;
		uint16 param1;
		uint32 param2;
		stream >> cmd;
		stream >> param1;
		stream >> param2;

		if (cmd == 0x8051)
		{
			AIStreamBE sample(&data[param2], data.size() - param2);
			if (data[param2 + 20] == 0x00)
			{
				return UnpackStandardSystem7Header(sample);
			}
			else if (data[param2 + 20] == 0xff || data[param2 + 20] == 0xfe)
			{
				return UnpackExtendedSystem7Header(sample);
			}
		}
	}

	return false;
}

std::vector<uint8> SndResource::Save() const
{
	uint32 length = 10 + 10 + data_.size() + 64;
	std::vector<uint8> result(length);
	AOStreamBE stream(&result[0], result.size());

	// resource header
	int16 format = 1;
	int16 data_formats = 1;
	int16 data_type = 5;
	uint32 initialization_options = (stereo_ ? 0x00c0 : 0x0080);
	
	stream << format
	       << data_formats
	       << data_type
	       << initialization_options;

	// command
	int16 num_commands = 1;
	int16 command = 0x8051;
	uint16 param1 = 0;
	uint32 param2 = 20;

	stream << num_commands
	       << command
	       << param1
	       << param2;

	// extended sound header
	uint32 ptr = 0;
	int32 num_channels = (stereo_ ? 2 : 1);
	uint32 loop_start = 0;
	uint32 loop_end = 0;
	uint8 header_type = 0xff;
	uint8 baseFrequency = 0;
	int32 num_frames = data_.size() / bytes_per_frame_;
	int16 sample_size = (sixteen_bit_ ? 16 : 8);

	stream << ptr
	       << num_channels
	       << rate_
	       << loop_start
	       << loop_end
	       << header_type
	       << baseFrequency
	       << num_frames;
	stream.ignore(22);
	stream << sample_size;

	stream.write(&data_[0], data_.size());

	return result;

}

// all this just to read a file from memory!?
struct sf_adapter
{
public:
	sf_adapter(std::vector<uint8>& data) : data_(data), p_(data.begin()) { }

	static sf_count_t get_filelen(void *pv) {
		return ((sf_adapter*) pv)->_get_filelen();
	}

	static sf_count_t seek(sf_count_t offset, int whence, void *pv) {
		return ((sf_adapter*) pv)->_seek(offset, whence);
	}

	static sf_count_t read(void *ptr, sf_count_t count, void *pv) {
		return ((sf_adapter*) pv)->_read(ptr, count);
	}

	static sf_count_t write(const void *ptr, sf_count_t count, void *pv) {
		return ((sf_adapter*) pv)->_write(ptr, count);
	}

	static sf_count_t tell(void *pv) {
		return ((sf_adapter*) pv)->_tell();
	}

private:
	std::vector<uint8>& data_;
	std::vector<uint8>::iterator p_;

	sf_count_t _get_filelen() {
		return data_.size();
	}

	sf_count_t _seek(sf_count_t offset, int whence) {
		if (whence == SEEK_SET)
			p_ = data_.begin() + offset;
		else if (whence == SEEK_END)
			p_ = data_.end() - offset;
		else if (whence == SEEK_CUR)
			p_ += offset;

		return ((p_ >= data_.begin() && p_ <= data_.end()) ? 0 : -1);
	}

	sf_count_t _read(void *ptr, sf_count_t count) {
		if (p_ >= data_.end()) return -1;
		char *dst = reinterpret_cast<char *>(ptr);
		int i = 0;
		for (; i < count && p_ < data_.end(); ++i)
		{
			*(dst++) = *(p_++);
		}

		return i;
	}

	sf_count_t _write(const void *ptr, sf_count_t count) {
		if (p_ >= data_.end()) return -1;

		const char *src = reinterpret_cast<const char *>(ptr);
		int i = 0;
		for (; i < count && p_ < data_.end(); ++i)
		{
			*(p_++) = *(src++);
		}

		return i;
	}

	sf_count_t _tell() {
		return p_ - data_.begin();
	}

};

bool SndResource::Import(const std::string& path)
{
	SF_INFO inputInfo;
	SNDFILE *infile = sf_open(path.c_str(), SFM_READ, &inputInfo);
	if (!infile) return false;

	sixteen_bit_ = !(inputInfo.format & (SF_FORMAT_PCM_S8 | SF_FORMAT_PCM_U8));
	rate_ = static_cast<uint32>(inputInfo.samplerate) << 16;
	stereo_ = (inputInfo.channels == 2);
	signed_8bit_ = false;
	bytes_per_frame_ = (sixteen_bit_ ? 2 : 1) * (stereo_ ? 2 : 1);
	
	SF_INFO outputInfo;
	outputInfo.samplerate = inputInfo.samplerate;
	outputInfo.channels = stereo_ ? 2 : 1;
	if (sixteen_bit_)
	{
		outputInfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW | SF_ENDIAN_BIG;
	}
	else
	{
		outputInfo.format = SF_FORMAT_PCM_U8 | SF_FORMAT_RAW | SF_ENDIAN_BIG;
	}

	SF_VIRTUAL_IO virtual_io = {
		&sf_adapter::get_filelen,
		&sf_adapter::seek,
		&sf_adapter::read,
		&sf_adapter::write,
		&sf_adapter::tell };

	data_.resize(inputInfo.frames * bytes_per_frame_);
	sf_adapter adapter(data_);

	SNDFILE *outfile = sf_open_virtual(&virtual_io, SFM_WRITE, &outputInfo, &adapter);
	if (!outfile) 
		return false;

	int frames_remaining = inputInfo.frames;
	while (frames_remaining)
	{
		int buf[kBufferSize * 2];
		int frames = std::min(kBufferSize, frames_remaining);
		
		if (sf_readf_int(infile, buf, frames) != frames)
		{
			std::cerr << "Read error" << std::endl;
			return false;
		}
		if (sf_writef_int(outfile, buf, frames) != frames)
		{
			std::cerr << "Write error" << std::endl;
			return false;
		}

		frames_remaining -= frames;
	}

	sf_close(infile);
	sf_close(outfile);

	return true;
}

void SndResource::Export(const std::string& path) const
{
	// open an output file

	int inputFormat;
	int outputFormat;
	if (sixteen_bit_)
	{
		inputFormat = outputFormat = SF_FORMAT_PCM_16;
	}
	else if (signed_8bit_)
	{
		inputFormat = SF_FORMAT_PCM_S8;
		outputFormat = SF_FORMAT_PCM_U8;
	}
	else 
		inputFormat = outputFormat = SF_FORMAT_PCM_U8;

	SF_INFO outputInfo;
	outputInfo.samplerate = static_cast<int32>(rate_ >> 16);
	outputInfo.channels = stereo_ ? 2 : 1;
	outputInfo.format = SF_FORMAT_WAV | outputFormat;
	
	SNDFILE *outfile = sf_open(path.c_str(), SFM_WRITE, &outputInfo);


	SF_VIRTUAL_IO virtual_io = {
		&sf_adapter::get_filelen,
		&sf_adapter::seek,
		&sf_adapter::read,
		&sf_adapter::write,
		&sf_adapter::tell };

	sf_adapter adapter(const_cast<std::vector<uint8>& >(data_));

	SF_INFO inputInfo;
	inputInfo.samplerate = static_cast<int32>(rate_ >> 16);
	inputInfo.channels = stereo_ ? 2 : 1;
	inputInfo.format = SF_FORMAT_RAW | inputFormat | SF_ENDIAN_BIG;

	SNDFILE *infile = sf_open_virtual(&virtual_io, SFM_READ, &inputInfo, &adapter);

	int frames_remaining = data_.size() / bytes_per_frame_;
	while (frames_remaining)
	{
		int buf[kBufferSize * 2];
		int frames = std::min(kBufferSize, frames_remaining);
		if (sf_readf_int(infile, buf, frames) != frames)
		{
			std::cerr << "Read error" << std::endl;
			return;
		}

		if (sf_writef_int(outfile, buf, frames) != frames)
		{
			std::cerr << "Write error (" << sf_strerror(outfile) << ")" << std::endl;
			return;
		}

		frames_remaining -= frames;
	}

	sf_close(infile);
	sf_close(outfile);
}

bool SndResource::UnpackStandardSystem7Header(AIStreamBE& stream)
{
	bytes_per_frame_ = 1;
	signed_8bit_ = false;
	sixteen_bit_ = false;
	stereo_ = false;
	stream.ignore(4); // sample pointer

	int32 length;
	stream >> length;
	
	stream >> rate_;
	stream.ignore(8); // loop_start, loop_end
	stream.ignore(2);

	data_.resize(length);
	stream.read(&data_[0], data_.size());

	return true;
}

bool SndResource::UnpackExtendedSystem7Header(AIStreamBE& stream)
{
	signed_8bit_ = false;
	stream.ignore(4); // sample pointer
	int32 num_channels;
	stream >> num_channels;
	stereo_ = (num_channels == 2);
	stream >> rate_;
	stream.ignore(8); // loop_start, loop_end;
	uint8 header_type;
	stream >> header_type;
	stream.ignore(1); // baseFrequency
	int32 num_frames;
	stream >> num_frames;

	if (header_type == 0xfe)
	{
		stream.ignore(10); // AIFF rate
		stream.ignore(4); // marker chunk
		uint32 format;
		stream >> format;
		stream.ignore(4 * 3); // future use, ptr, ptr
		int16 comp_id;
		stream >> comp_id;
		if (format != FOUR_CHARS_TO_INT('t','w','o','s') || comp_id != -1)
		{
			std::cerr << "Unsupported compression" << std::endl;
			return false;
		}
		signed_8bit_ = true;
		stream.ignore(4);
	}
	else
	{
		stream.ignore(22);
	}

	int16 sample_size;
	stream >> sample_size;

	if (header_type != 0xfe)
		stream.ignore(14);

	sixteen_bit_ = (sample_size == 16);
	bytes_per_frame_ = (sixteen_bit_ ? 2 : 1) * (stereo_ ? 2 : 1);
	
	data_.resize(num_frames * bytes_per_frame_);
	stream.read(&data_[0], data_.size());
	
	return true;
}
