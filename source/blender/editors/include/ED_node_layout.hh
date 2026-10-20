/* SPDX-FileCopyrightText: 2026 Mixer Contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

namespace blender::ed::space_node::layout {

constexpr float default_grid_spacing = 20.0f;
constexpr float min_grid_spacing = 4.0f;
constexpr float max_grid_spacing = 200.0f;

/** Zero is the value read from Blender files written before this setting existed. */
inline float grid_spacing(const float stored)
{
  return std::isfinite(stored) && stored > 0.0f ?
             std::clamp(stored, min_grid_spacing, max_grid_spacing) :
             default_grid_spacing;
}

inline float snap_coordinate(const float coordinate, const float step)
{
  return std::round(coordinate / step) * step;
}

/** Pad downwards rather than moving a socket into the controls above it. */
inline float socket_row_y(const float row_top,
                          const float origin_y,
                          const float step,
                          const float half_row_height)
{
  return origin_y + std::floor((row_top - half_row_height - origin_y) / step + 1e-5f) * step;
}

inline float grid_width(const float width, const float step)
{
  return std::ceil(width / step - 1e-5f) * step;
}

/** Prefer the lower coordinate on ties, independently of insertion/draw order. */
inline std::optional<float> nearest_axis_coordinate(const float *begin,
                                                    const float *end,
                                                    const float coordinate)
{
  if (begin == end) {
    return std::nullopt;
  }
  const float *next = std::lower_bound(begin, end, coordinate);
  if (next == begin) {
    return *next;
  }
  if (next == end || coordinate - *(next - 1) <= *next - coordinate) {
    return *(next - 1);
  }
  return *next;
}

}  // namespace blender::ed::space_node::layout
