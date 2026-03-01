#pragma once

#include <cstdint>
#include <Arduino.h>

namespace esphome {
namespace lifx_emulation {

inline byte nibble(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

inline void hexCharacterStringToBytes(byte *byteArray, const char *hexString)
{
	bool oddLength = strlen(hexString) & 1;

	byte currentByte = 0;
	byte byteIndex = 0;
	byte removed = 0;

	for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++)
	{
		bool oddCharIndex = (charIndex - removed) & 1;
		if (hexString[charIndex] == '-')
		{
			removed++;
			continue;
		}

		if (oddLength)
		{
			if (oddCharIndex)
			{
				currentByte = nibble(hexString[charIndex]) << 4;
			}
			else
			{
				currentByte |= nibble(hexString[charIndex]);
				byteArray[byteIndex++] = currentByte;
				currentByte = 0;
			}
		}
		else
		{
			if (!oddCharIndex)
			{
				currentByte = nibble(hexString[charIndex]) << 4;
			}
			else
			{
				currentByte |= nibble(hexString[charIndex]);
				byteArray[byteIndex++] = currentByte;
				currentByte = 0;
			}
		}
	}
}

/******************************************************************************
 * HSB to RGB conversion.
 * hue (index): 0-767, sat and bright: 0-255, color[]: output RGB bytes
 *****************************************************************************/
inline void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3])
{
	uint16_t r_temp, g_temp, b_temp;
	uint8_t index_mod;
	uint8_t inverse_sat = (sat ^ 255);

	index = index % 768;
	index_mod = index % 256;

	if (index < 256)
	{
		r_temp = index_mod ^ 255;
		g_temp = index_mod;
		b_temp = 0;
	}
	else if (index < 512)
	{
		r_temp = 0;
		g_temp = index_mod ^ 255;
		b_temp = index_mod;
	}
	else if (index < 768)
	{
		r_temp = index_mod;
		g_temp = 0;
		b_temp = index_mod ^ 255;
	}
	else
	{
		r_temp = 0;
		g_temp = 0;
		b_temp = 0;
	}

	r_temp = ((r_temp * sat) / 255) + inverse_sat;
	g_temp = ((g_temp * sat) / 255) + inverse_sat;
	b_temp = ((b_temp * sat) / 255) + inverse_sat;

	r_temp = (r_temp * bright) / 255;
	g_temp = (g_temp * bright) / 255;
	b_temp = (b_temp * bright) / 255;

	color[0] = (uint8_t)r_temp;
	color[1] = (uint8_t)g_temp;
	color[2] = (uint8_t)b_temp;
}

} // namespace lifx_emulation
} // namespace esphome
