#ifndef PROTOCOLE_H
#define PROTOCOLE_H

#include <iostream>

enum class PACKET_TYPE
{
	START_STREAM,
	FRAME
};

struct PacketHeader
{
	PACKET_TYPE type;
	std::uint32_t size;
};

struct StreamInfo
{
	std::uint32_t bitrate;
	std::uint32_t framerate;
	std::uint32_t width;
	std::uint32_t height;
};

#endif
