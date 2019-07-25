#pragma once

#include <algorithm>
#include "geometry.h"

inline unsigned toColori(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255)
{
	return (r << 24) + (g << 16) + (b << 8) + a;
}
inline unsigned toColor(const float r, const float g, const float b, const float a = 1.0)
{
	return toColori(uint8_t(255 * std::max(0.0f, std::min(1.0f, r))),
		uint8_t(255 * std::max(0.0f, std::min(1.0f, g))),
		uint8_t(255 * std::max(0.0f, std::min(1.0f, b))));
}
inline unsigned toColor(const Vec3f& color)
{
	return toColor(color.r, color.g, color.b);
}
