/* -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file pipelineFromDesc.cpp
 * @brief This program works similar to gst-launch-1.0. (less features) You can run a pipeline like this:
 * ./pipelineFromDesc videotestsrc ! videoconvert ! queue ! autovideosink
 * quit with [ctl]+[C]
 * @author Sandro Stiller
 * @date 2024-09-15
 */

#include "helpers.hpp"
#include "pipeline.hpp"
#include "messageparser.hpp"


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

  std::cout << desc << std::endl;

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
      std::cerr << "Error from '" << source << "' | Message: " << errorMessage << " | Debug info: " << debugMessage << std::endl;
      std::cerr << "Quitting" << std::endl;
      g_main_loop_quit(mainLoop);
    }
  );
  messageParser->infoSignal.connect(
    [](const std::string& source, const std::string& infoMessage, const std::string& debugMessage)
    {
      std::cout << "Info from '" << source << "' | Message: " << infoMessage << " | Debug info: " << debugMessage << std::endl;
    }
  );

  messageParser->warningSignal.connect(
    [](const std::string& source, const std::string& warningMessage, const std::string& debugMessage)
    {
      std::cout << "Warning from '" << source << "' | Message: " << warningMessage << " | Debug info: " << debugMessage << std::endl;
    }
  );
  messageParser->stateChangedSignal.connect(
    [](const std::string& source, GstState oldState, GstState newState, GstState pendingState)
    {
      std::cout << "State change '" << source << "': "
                << gst_element_state_get_name(oldState) << " -> "
                << gst_element_state_get_name(newState) << " ("
                << gst_element_state_get_name(pendingState) << ")" << std::endl;
    }
  );
  messageParser->endOfStreamSignal.connect(
    [](const std::string& sourceName)
    {
       std::cout << "EOS from '" << sourceName << "'" << std::endl;
    }
  );
  messageParser->streamStatusSignal.connect(
    [](const std::string& sourceName, GstStreamStatusType statusType, const std::string& ownerName)
    {
      std::cout << "Stream status from '" << sourceName << "' | Status Type: "
                << dh::gst::helpers::gstStreamStatusTypeToString(statusType) << " | Owner: "
                << ownerName << std::endl;
    }
  );
  messageParser->streamStartSignal.connect(
    [](const std::string& sourceName)
    {
      std::cout << "Stream start from '" << sourceName << "'" << std::endl;
    }
  );
  messageParser->asyncDoneSignal.connect(
    [](const std::string& sourceName, GstClockTime runningTime)
    {
      std::cout << "Stream status from '" << sourceName << "' | Running time: "
                << runningTime << "ns" << std::endl;
    }
  );

  messageParser->elementMessageSignal.connect(
    [](const std::string& sourceName, const GstStructure*)
    {
      std::cout << "Element specific message from '" << sourceName << "'" << std::endl;
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
