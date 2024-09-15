/* -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file pipelineFromDesc.cpp
 * @author Sandro Stiller
 * @date 2024-09-15
 */

#include "pipeline.hpp"

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

  const std::string desc = concatArgs(argc, argv);

  spdlog::info(desc);

  gst_init(const_cast<int*>(&argc), const_cast<char***>(&argv));
  auto ppl = dh::gst::Pipeline::fromDescription(desc);

  ppl.setState(GST_STATE_PLAYING);

  gst_deinit(); // hangs :-) That's good because we currently have no main loop
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