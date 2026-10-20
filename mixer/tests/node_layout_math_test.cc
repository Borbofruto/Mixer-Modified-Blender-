/* SPDX-FileCopyrightText: 2026 Mixer Contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "ED_node_layout.hh"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace layout = blender::ed::space_node::layout;

static void expect(const bool condition, const char *message)
{
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

static bool close(const float a, const float b)
{
  return std::abs(a - b) < 1e-4f;
}

int main()
{
  expect(layout::grid_spacing(0.0f) == 20.0f, "Legacy files must keep the 20-unit grid");
  expect(layout::grid_spacing(-1.0f) == 20.0f, "Invalid spacing must have a safe default");
  expect(layout::grid_spacing(std::numeric_limits<float>::quiet_NaN()) == 20.0f,
         "NaN spacing must not propagate into draw/snap calculations");
  expect(layout::grid_spacing(std::numeric_limits<float>::infinity()) == 20.0f,
         "Infinite spacing must not propagate");
  expect(layout::grid_spacing(1.0f) == 4.0f, "Minimum grid spacing");
  expect(layout::grid_spacing(1000.0f) == 200.0f, "Maximum grid spacing");
  expect(layout::grid_spacing(7.5f) == 7.5f, "Fractional spacing must be preserved");

  expect(layout::snap_coordinate(-39.0f, 20.0f) == -40.0f,
         "Negative coordinates snap symmetrically");
  expect(layout::snap_coordinate(-11.0f, 20.0f) == -20.0f, "Negative half-cell rounding");
  expect(layout::snap_coordinate(11.0f, 20.0f) == 20.0f, "Positive half-cell rounding");
  expect(layout::snap_coordinate(0.0f, 7.5f) == 0.0f, "The origin is on every grid");

  for (const float scale : {1.0f, 1.25f, 1.5f, 2.0f}) {
    for (const float origin : {-80.0f, 0.0f, 37.5f}) {
      for (const float spacing : {4.0f, 7.5f, 20.0f, 31.0f, 200.0f}) {
        float top = origin * scale - 23.0f * scale;
        float previous = top;
        for (int row = 0; row < 12; row++) {
          const float y = layout::socket_row_y(
              top, origin * scale, spacing * scale, 10.0f * scale);
          expect(y <= top - 10.0f * scale + 1e-4f,
                 "Padding must not overlap the previous controls");
          const float index = (y / scale - origin) / spacing;
          expect(close(index, std::round(index)),
                 "Every socket offset must be on-grid at every UI scale");
          expect(y < previous, "Successive socket rows must remain ordered");
          previous = y;
          top = std::round(y + 10.0f * scale) - 43.0f * scale;
        }
        const float width = layout::grid_width(143.0f, spacing);
        expect(width >= 143.0f - 1e-4f,
               "Padding must not shrink a node below its requested width");
        expect(close(width / spacing, std::round(width / spacing)),
               "Both socket columns must share the grid");
      }
    }
  }

  const std::array<float, 5> targets = {-100.0f, -20.0f, 0.0f, 20.0f, 100.0f};
  expect(!layout::nearest_axis_coordinate(nullptr, nullptr, 0.0f),
         "No target when the graph is empty");
  expect(*layout::nearest_axis_coordinate(
             targets.data(), targets.data() + targets.size(), -200.0f) == -100.0f,
         "Closest target below the cached range");
  expect(*layout::nearest_axis_coordinate(
             targets.data(), targets.data() + targets.size(), 200.0f) == 100.0f,
         "Closest target above the cached range");
  expect(*layout::nearest_axis_coordinate(
             targets.data(), targets.data() + targets.size(), 10.0f) == 0.0f,
         "Equal-distance targets must resolve deterministically");
  expect(*layout::nearest_axis_coordinate(
             targets.data(), targets.data() + targets.size(), 19.0f) == 20.0f,
         "Nearest target must win");
  std::cout << "MIXER_NODE_LAYOUT_MATH_OK\n";
}
