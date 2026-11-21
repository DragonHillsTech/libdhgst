/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/* Copyright (C) 2026 Sandro Stiller <sandro.stiller@dragonhills.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This file is part of Libdhgst <https://dragonhills.de/>.
 *
 * Libdhgst is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Libdhgst is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Libdhgst. If not, see <http://www.gnu.org/licenses/>.
 */

#include "lifeboard.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace dh::gst::examples
{

LifeBoard::LifeBoard(int columns, int rows, int cellSize, int motionThreshold)
: columns(columns)
, rows(rows)
, cellSize(cellSize)
, motionThreshold(motionThreshold)
{
  if(columns <= 0 || rows <= 0 || cellSize <= 0 || motionThreshold <= 0)
  {
    throw std::invalid_argument("LifeBoard: all parameters must be positive");
  }
  const auto cellCount = static_cast<std::size_t>(columns) * rows;
  ages.assign(cellCount, 0);
  nextAges.assign(cellCount, 0);
  brightness.assign(cellCount, 0);
  previousBrightness.assign(cellCount, -1);
}

void LifeBoard::seedFromMotion(const boost::gil::rgb8c_view_t& frame)
{
  requireBoardSize(frame);

  std::fill(brightness.begin(), brightness.end(), 0);
  for(int y = 0; y < frame.height(); ++y)
  {
    const auto row = frame.row_begin(y);
    int* cellRow = &brightness[(y / cellSize) * columns];
    for(int x = 0; x < frame.width(); ++x)
    {
      const auto& pixel = row[x];
      // integer approximation of the luma: (2 R + 5 G + B) / 8
      cellRow[x / cellSize] += (2 * pixel[0] + 5 * pixel[1] + pixel[2]) / 8;
    }
  }

  for(std::size_t i = 0; i < brightness.size(); ++i)
  {
    brightness[i] /= cellSize * cellSize;
    if(previousBrightness[i] >= 0 && std::abs(brightness[i] - previousBrightness[i]) > motionThreshold && !ages[i])
    {
      ages[i] = 1;
    }
  }
  std::swap(brightness, previousBrightness);
}

void LifeBoard::step()
{
  population = 0;
  for(int y = 0; y < rows; ++y)
  {
    for(int x = 0; x < columns; ++x)
    {
      int neighbours = 0;
      for(int dy = -1; dy <= 1; ++dy)
      {
        for(int dx = -1; dx <= 1; ++dx)
        {
          if((dx || dy) && isAlive(x + dx, y + dy))
          {
            ++neighbours;
          }
        }
      }

      const std::uint8_t age = ages[y * columns + x];
      std::uint8_t& nextAge = nextAges[y * columns + x];
      if(age)
      {
        // survives with 2 or 3 neighbours, gets older
        nextAge = (neighbours == 2 || neighbours == 3) ? static_cast<std::uint8_t>(std::min(age + 1, 255)) : 0;
      }
      else
      {
        // born with exactly 3 neighbours
        nextAge = (neighbours == 3) ? 1 : 0;
      }
      population += nextAge ? 1 : 0;
    }
  }
  std::swap(ages, nextAges);
  ++generation;
}

void LifeBoard::draw(const boost::gil::rgb8c_view_t& frame, const boost::gil::rgb8_view_t& output) const
{
  requireBoardSize(frame);
  requireBoardSize(output);

  for(int y = 0; y < output.height(); ++y)
  {
    const auto inRow = frame.row_begin(y);
    const auto outRow = output.row_begin(y);
    const std::uint8_t* cellRow = &ages[(y / cellSize) * columns];
    const bool gridLine = (y % cellSize) == cellSize - 1;

    for(int x = 0; x < output.width(); ++x)
    {
      const std::uint8_t age = cellRow[x / cellSize];
      if(age && !gridLine && (x % cellSize) != cellSize - 1)
      {
        outRow[x] = colorOfAge(age);
      }
      else
      {
        // the frame in the background, at a third of its brightness
        const auto& pixel = inRow[x];
        outRow[x] = boost::gil::rgb8_pixel_t(pixel[0] / 3, pixel[1] / 3, pixel[2] / 3);
      }
    }
  }
}

bool LifeBoard::isAlive(int x, int y) const
{
  x = (x + columns) % columns;
  y = (y + rows) % rows;
  return ages[y * columns + x] != 0;
}

template<typename View>
void LifeBoard::requireBoardSize(const View& frame) const
{
  if(frame.width() != columns * cellSize || frame.height() != rows * cellSize)
  {
    throw std::invalid_argument("LifeBoard: the frame does not have the size of the board");
  }
}

boost::gil::rgb8_pixel_t LifeBoard::colorOfAge(std::uint8_t age)
{
  struct Stop { int age; int r, g, b; };
  static constexpr Stop stops[] = {
    {1, 255, 255, 255},
    {4, 255, 230, 0},
    {20, 255, 0, 160},
    {60, 40, 60, 255},
  };

  if(age >= stops[std::size(stops) - 1].age)
  {
    const Stop& last = stops[std::size(stops) - 1];
    return boost::gil::rgb8_pixel_t(last.r, last.g, last.b);
  }
  std::size_t i = 0;
  while(age >= stops[i + 1].age)
  {
    ++i;
  }
  const Stop& from = stops[i];
  const Stop& to = stops[i + 1];
  const int t = (age - from.age) * 256 / (to.age - from.age);
  const auto mix = [t](int a, int b) { return static_cast<std::uint8_t>(a + (b - a) * t / 256); };
  return boost::gil::rgb8_pixel_t(mix(from.r, to.r), mix(from.g, to.g), mix(from.b, to.b));
}

} // namespace dh::gst::examples
