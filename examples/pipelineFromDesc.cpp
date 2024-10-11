/* -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file pipelineFromDesc.cpp
 * @author Sandro Stiller
 * @date 2024-09-15
 */

#include "helpers.hpp"
#include "pipeline.hpp"
#include "messageparser.hpp"

#include <spdlog/spdlog.h>

// std
#include <iostream>
#include <sstream>

void printHelp(const std::string& appName);
std::string concatArgs(int argc, const char* argv[]);

int main(const int argc, const char **argv)
{
  if(argc < 2)
  {
    printHelp(argv[0]);
    return 1;
  }

  GMainLoop* mainLoop = g_main_loop_new(nullptr, FALSE);

  const std::string desc = concatArgs(argc, argv);

  spdlog::info(desc);

  gst_init(const_cast<int*>(&argc), const_cast<char***>(&argv));
  auto ppl = dh::gst::Pipeline::fromDescription(desc);

  auto messageParser = dh::gst::MessageParser::create(
    [](auto task)
    {
      // Use g_main_context_invoke to post the task to the default main context.
      g_main_context_invoke(nullptr, [](gpointer userData) -> gboolean
      {
        auto* taskPtr = static_cast<std::function<void()>*>(userData);
        (*taskPtr)(); // Call the task
        delete taskPtr; // Clean up
        return G_SOURCE_REMOVE; // Remove the source after it's executed
      },
      new std::function<void()>(std::move(task)));
    }
  );

  messageParser->errorSignal.connect(
    [mainLoop](const std::string& source, const std::string& errorMessage, const std::string& debugMessage)
    {
      spdlog::error("Error from '{}' | Message: {} | Debug info: {}", source, errorMessage, debugMessage);
      spdlog::error("Quitting");
      g_main_loop_quit(mainLoop);
    }
  );
  messageParser->infoSignal.connect(
    [](const std::string& source, const std::string& infoMessage, const std::string& debugMessage)
    {
      spdlog::info("Info from '{}' | Message: {} | Debug info: {}", source, infoMessage, debugMessage);
    }
  );

  messageParser->warningSignal.connect(
    [](const std::string& source, const std::string& warningMessage, const std::string& debugMessage)
    {
      spdlog::warn("Warning from '{}' | Message: {} | Debug info: {}", source, warningMessage, debugMessage);
    }
  );
  messageParser->stateChangedSignal.connect(
    [](const std::string& source, GstState oldState, GstState newState, GstState pendingState)
    {
      spdlog::info("State change '{}': {} -> {} ({})",
                   source,
                   gst_element_state_get_name(oldState),
                   gst_element_state_get_name(newState),
                   gst_element_state_get_name(pendingState));
    }
  );
  messageParser->endOfStreamSignal.connect(
    [](const std::string& sourceName)
    {
       spdlog::info("EOS from '{}'", sourceName);
    }
  );
  messageParser->streamStatusSignal.connect(
    [](const std::string& sourceName, GstStreamStatusType statusType, const std::string& ownerName)
    {
      spdlog::info("Stream status from '{}' | Status Type: {} | Owner: {}",
             sourceName,
             dh::gst::helpers::gstStreamStatusTypeToString(statusType),
             ownerName);
    }
  );
  messageParser->streamStartSignal.connect(
    [](const std::string& sourceName)
    {
      spdlog::info("Stream start from '{}'", sourceName);
    }
  );
  messageParser->asyncDoneSignal.connect(
    [](const std::string& sourceName, GstClockTime runningTime)
    {
      spdlog::info("Stream status from '{}' | Running time: {}ns",
       sourceName,
       runningTime);
    }
  );

  messageParser->elementMessageSignal.connect(
    [](const std::string& sourceName, const GstStructure*)
    {
      spdlog::info("Element specific message from '{}'", sourceName);
    }
  );

  auto bus = ppl.getBus();
  bus->newSyncMessageSignal().connect(
    [&messageParser](dh::gst::GstMessageSPtr message)
    {
      messageParser->parse(*message);
    }
  );

  ppl.setState(GST_STATE_PLAYING);
  g_main_loop_run(mainLoop);

  // Clean up after running.
  ppl.setState(GST_STATE_NULL);
  gst_deinit();
  g_main_loop_unref(mainLoop);
  return 0;
}

void printHelp(const std::string& appName)
{
  std::cout << "Usage: " << appName << " <pipelineDesc>\n";
}

std::string concatArgs(int argc, const char* argv[]) {
  std::ostringstream oss;

  for (int i = 1; i < argc; ++i) {
    if (i > 1) {
      oss << ' ';  // Add a space before every argument except the first
    }
    oss << argv[i];
  }

  return oss.str();
}
