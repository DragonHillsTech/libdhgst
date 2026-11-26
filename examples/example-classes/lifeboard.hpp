/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
#ifndef DH_GST_EXAMPLES_LIFEBOARD_HPP
#define DH_GST_EXAMPLES_LIFEBOARD_HPP

#include <boost/gil.hpp>

#include <cstdint>
#include <vector>

namespace dh::gst::examples
{

/**
 * @brief Conway's Game of Life on a board of square cells over a video frame.
 * Movement in the frame seeds new cells.
 *
 * Frames must be exactly columns * cellSize pixels wide and rows * cellSize pixels high.
 * Not thread-safe: use it from one thread, e.g. the streaming thread of an appsink.
 */
class LifeBoard
{
public:
  /**
   * @param columns number of cells per row
   * @param rows number of cells per column
   * @param cellSize edge length of a cell in pixels
   * @param motionThreshold average brightness change of a cell (0-255) above which it is movement
   * @throws std::invalid_argument if a parameter is not positive
   */
  LifeBoard(int columns, int rows, int cellSize, int motionThreshold);

  /**
   * @brief Bring cells to life where the average brightness changed since the last frame.
   * @throws std::invalid_argument if the frame does not have the size of the board
   */
  void seedFromMotion(const boost::gil::rgb8c_view_t& frame);

  /**
   * @brief Advance one generation. The board wraps around at the edges.
   */
  void step();

  /**
   * @brief Draw the living cells, colored by age, over a darkened copy of the frame.
   * @throws std::invalid_argument if a frame does not have the size of the board
   */
  void draw(const boost::gil::rgb8c_view_t& frame, const boost::gil::rgb8_view_t& output) const;

  [[nodiscard]] unsigned long getGeneration() const
  {
    return generation;
  }
  [[nodiscard]] int getPopulation() const
  {
    return population;
  }

private:
  [[nodiscard]] bool isAlive(int x, int y) const;

  /// @throws std::invalid_argument if the frame does not have the size of the board
  template<typename View>
  void requireBoardSize(const View& frame) const;

  /// newborn cells are white, then they fade over yellow and magenta to blue
  [[nodiscard]] static boost::gil::rgb8_pixel_t colorOfAge(std::uint8_t age);

  const int columns;
  const int rows;
  const int cellSize;
  const int motionThreshold;
  std::vector<std::uint8_t> ages; ///< 0: dead, otherwise the number of generations the cell lived
  std::vector<std::uint8_t> nextAges;
  std::vector<int> brightness; ///< average brightness per cell of the current frame
  std::vector<int> previousBrightness; ///< -1 before the first frame
  unsigned long generation{0};
  int population{0};
};

} // namespace dh::gst::examples

#endif // DH_GST_EXAMPLES_LIFEBOARD_HPP
