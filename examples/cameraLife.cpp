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

/*
 * Conway's Game of Life, seeded by the movement in front of the camera.
 *
 * Two pipelines with the application in the middle:
 *
 *   <source> ! videoconvert ! videoscale ! RGB 640x480 ! appsink     (camera pipeline)
 *                                                         |
 *                                   motion -> new cells, one Life generation, draw
 *                                                         |
 *   appsrc ! videoconvert ! autovideosink                            (display pipeline)
 *
 * Wave your hand: wherever the image changes, cells are born. Then Life takes over.
 *
 * Usage: cameraLife [source description]
 *   cameraLife                                          # the default camera (autovideosrc)
 *   cameraLife videotestsrc is-live=true pattern=ball    # no camera
 *   cameraLife "filesrc location=movie.mp4 ! decodebin"  # a video file (quote the "!" for the shell)
 */

// example classes
#include "lifeboard.hpp"

// libdhgst
#include "appsink.hpp"
#include "appsrc.hpp"
#include "gilview.hpp"
#include "helpers.hpp"
#include "messageparser.hpp"
#include "pipeline.hpp"

// gstreamer
#include <glib-unix.h>
#include <gst/gst.h>
#include <gst/video/video.h>

// std
#include <csignal>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

constexpr int width = 640;
constexpr int height = 480;
constexpr int cellSize = 4; // pixels per cell edge

/// minimum change of the average brightness of a cell (0-255) that counts as movement
constexpr int motionThreshold = 14;

using dh::gst::examples::LifeBoard;

/// the caps of both pipelines: the camera frames are converted to it, the display gets the same
const std::string videoCaps =
  "video/x-raw,format=RGB,width=" + std::to_string(width) + ",height=" + std::to_string(height)
  + ",framerate=30/1,pixel-aspect-ratio=1/1";

/**
 * @brief Maps a video buffer as GstVideoFrame for its lifetime.
 */
class MappedFrame
{
public:
  MappedFrame(GstVideoInfo& info, GstBuffer& buffer, GstMapFlags flags)
  {
    if(!gst_video_frame_map(&frame, &info, &buffer, flags))
    {
      throw std::runtime_error("Failed to map video frame");
    }
  }

  ~MappedFrame()
  {
    gst_video_frame_unmap(&frame);
  }

  MappedFrame(const MappedFrame&) = delete;
  MappedFrame& operator=(const MappedFrame&) = delete;

  GstVideoFrame& get() { return frame; }

private:
  GstVideoFrame frame{};
};

/**
 * @brief Run one Life generation on a camera frame and draw it into a new buffer.
 */
dh::gst::GstBufferSPtr renderLifeFrame(const dh::gst::GstSampleSPtr& sample, LifeBoard& board)
{
  GstVideoInfo info = dh::gst::helpers::createVideoInfo(*gst_sample_get_caps(sample.get()));
  if(GST_VIDEO_INFO_WIDTH(&info) != width || GST_VIDEO_INFO_HEIGHT(&info) != height)
  {
    throw std::runtime_error("Unexpected frame size");
  }

  // a new buffer with the default layout of the caps, the display pipeline has the same caps
  dh::gst::GstBufferSPtr outputBuffer = dh::gst::makeGstSharedPtr(
    gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr),
    dh::gst::TransferType::full
  );

  MappedFrame input(info, *gst_sample_get_buffer(sample.get()), GST_MAP_READ);
  MappedFrame output(info, *outputBuffer, GST_MAP_WRITE);
  const auto inputView = dh::gst::createGilView<boost::gil::rgb8c_view_t>(input.get());
  const auto outputView = dh::gst::createGilView<boost::gil::rgb8_view_t>(output.get());

  board.seedFromMotion(inputView);
  board.step();
  board.draw(inputView, outputView);
  return outputBuffer;
}

void pushFrame(dh::gst::AppSrc& appSrc, const dh::gst::GstBufferSPtr& buffer)
{
  // GST_FLOW_FLUSHING only means the display is stopping
  const GstFlowReturn result = appSrc.pushBuffer(buffer);
  if(result != GST_FLOW_OK && result != GST_FLOW_FLUSHING)
  {
    throw std::runtime_error(std::string("Failed to push frame: ") + gst_flow_get_name(result));
  }
}

/// about once per second
void printStatistics(const LifeBoard& board)
{
  if(board.getGeneration() % 30 == 0)
  {
    std::cout << "\rgeneration " << board.getGeneration() << ", population " << board.getPopulation() << "      "
              << std::flush;
  }
}

/**
 * @brief Owns a GMainLoop that also quits on Ctrl+C.
 */
class MainLoop
{
public:
  MainLoop()
  : loop(g_main_loop_new(nullptr, FALSE))
  , interruptSourceId(g_unix_signal_add(SIGINT, &MainLoop::onInterrupt, loop))
  {
  }

  ~MainLoop()
  {
    g_source_remove(interruptSourceId);
    g_main_loop_unref(loop);
  }

  MainLoop(const MainLoop&) = delete;
  MainLoop& operator=(const MainLoop&) = delete;

  void run() { g_main_loop_run(loop); }
  void quit() { g_main_loop_quit(loop); }

private:
  static gboolean onInterrupt(gpointer loop)
  {
    g_main_loop_quit(static_cast<GMainLoop*>(loop));
    return G_SOURCE_CONTINUE; // removed by the destructor
  }

  GMainLoop* const loop;
  const guint interruptSourceId;
};

/**
 * @brief The two pipelines and the Life board between them.
 * The destructor stops the pipelines: their owner must set them to NULL before they are destroyed.
 */
class CameraLife
{
public:
  explicit CameraLife(const std::string& source)
  : camera(createCameraPipeline(source))
  , display(createDisplayPipeline())
  , appSink(dh::gst::AppSink::fromElement(*camera->getElementByName("sink")))
  , appSrc(dh::gst::AppSrc::fromElement(*display->getElementByName("src")))
  {
    appSrc->setCaps(
      dh::gst::makeGstSharedPtr(
        gst_caps_from_string(videoCaps.c_str()),
        dh::gst::TransferType::full
      )
    );

    sampleConnection = appSink->connectNewSample(
      [this](dh::gst::GstSampleSPtr sample)
      {
        pushFrame(*appSrc, renderLifeFrame(sample, board));
        printStatistics(board);
      }
    );
    // the camera ended (e.g. a file source): let the display finish, too
    endOfStreamConnection = appSink->connectEndOfStream(
      [this]()
      {
        (void)appSrc->endOfStream();
      }
    );
  }

  ~CameraLife()
  {
    // the camera first: no more frames are pushed into the stopped display then
    camera->setState(GST_STATE_NULL);
    display->setState(GST_STATE_NULL);
  }

  CameraLife(const CameraLife&) = delete;
  CameraLife& operator=(const CameraLife&) = delete;

  /**
   * @throws std::runtime_error if a pipeline can not be started
   */
  void start()
  {
    if(display->setState(GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE
       || camera->setState(GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
      throw std::runtime_error("Failed to start the pipelines (no camera? try \"videotestsrc is-live=true\")");
    }
  }

  [[nodiscard]] std::shared_ptr<dh::gst::Pipeline> getCamera() const { return camera; }
  [[nodiscard]] std::shared_ptr<dh::gst::Pipeline> getDisplay() const { return display; }

private:
  static std::shared_ptr<dh::gst::Pipeline> createCameraPipeline(const std::string& source)
  {
    // drop=true, max-buffers=1: process only the newest frame if Life is too slow
    auto pipeline = dh::gst::Pipeline::fromDescription(
      source + " ! videoconvert ! videoscale ! videorate ! " + videoCaps
      + " ! appsink name=sink drop=true max-buffers=1"
    );
    pipeline->setName("camera");
    return pipeline;
  }

  static std::shared_ptr<dh::gst::Pipeline> createDisplayPipeline()
  {
    // the frames are timestamped when they are pushed, with the clock of the display pipeline
    auto pipeline = dh::gst::Pipeline::fromDescription(
      "appsrc name=src is-live=true format=time do-timestamp=true ! videoconvert ! autovideosink"
    );
    pipeline->setName("display");
    return pipeline;
  }

  const std::shared_ptr<dh::gst::Pipeline> camera;
  const std::shared_ptr<dh::gst::Pipeline> display;
  const std::shared_ptr<dh::gst::AppSink> appSink;
  const std::shared_ptr<dh::gst::AppSrc> appSrc;
  LifeBoard board{width / cellSize, height / cellSize, cellSize, motionThreshold}; ///< only used by the streaming thread of the appsink
  dh::gst::SignalConnection sampleConnection;
  dh::gst::SignalConnection endOfStreamConnection;
};

/**
 * @brief Run tasks of the MessageParser in the GLib main loop, like examples/pipelineFromDesc.cpp.
 */
void invokeInMainContext(std::function<void()> task)
{
  g_main_context_invoke(
    nullptr,
    [](gpointer userData) -> gboolean
    {
      auto* taskPtr = static_cast<std::function<void()>*>(userData);
      (*taskPtr)();
      delete taskPtr;
      return G_SOURCE_REMOVE;
    },
    new std::function<void()>(std::move(task))
  );
}

/**
 * @brief Quits the main loop on an error in any pipeline, or when the last pipeline reached end-of-stream.
 */
class QuitOnFinish
{
public:
  QuitOnFinish(
    MainLoop& mainLoop,
    const std::vector<std::shared_ptr<dh::gst::Pipeline>>& pipelines,
    const std::string& lastPipelineName
  )
  {
    connections.push_back(messageParser->connectSignalError(
      [&mainLoop](const std::string& sourceName, const std::string& message, const std::string& debugInfo)
      {
        // also reached when the window is closed
        std::cerr << "\nError from '" << sourceName << "': " << message << "\n" << debugInfo << std::endl;
        mainLoop.quit();
      }
    ));
    connections.push_back(messageParser->connectSignalEndOfStream(
      [&mainLoop, lastPipelineName](const std::string& sourceName)
      {
        if(sourceName == lastPipelineName)
        {
          mainLoop.quit();
        }
      }
    ));
    for(const auto& pipeline : pipelines)
    {
      connections.push_back(pipeline->getBus()->connectSignalNewSyncMessage(
        [parser = messageParser](dh::gst::GstMessageSPtr message)
        {
          parser->parse(*message);
        }
      ));
    }
  }

private:
  const std::shared_ptr<dh::gst::MessageParser> messageParser{dh::gst::MessageParser::create(invokeInMainContext)};
  std::vector<dh::gst::SignalConnection> connections;
};

/**
 * @brief All arguments together are the source description, quoted or not.
 */
std::string sourceFromArguments(int argc, char** argv)
{
  std::string source;
  for(int i = 1; i < argc; ++i)
  {
    source += (i > 1 ? " " : "") + std::string(argv[i]);
  }
  return source.empty() ? "autovideosrc" : source;
}

void printUsage(const char* appName)
{
  std::cout << "Usage: " << appName << " [source description]\n"
            << "  e.g. " << appName << " videotestsrc is-live=true pattern=ball\n";
}

void run(const std::string& source)
{
  MainLoop mainLoop;
  CameraLife cameraLife(source);
  QuitOnFinish quitOnFinish(mainLoop, {cameraLife.getCamera(), cameraLife.getDisplay()}, "display");

  cameraLife.start();
  std::cout << "Wave at the camera! Ctrl+C or close the window to quit." << std::endl;
  mainLoop.run();
  std::cout << std::endl;
}

} // namespace

int main(int argc, char** argv)
{
  gst_init(&argc, &argv);

  const std::string source = sourceFromArguments(argc, argv);
  if(source == "-h" || source == "--help")
  {
    printUsage(argv[0]);
    return 0;
  }

  try
  {
    run(source);
  }
  catch(const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
