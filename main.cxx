#include "logger.hxx"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
  try {

    if (argc > 1) {
      Logger::initFromConfig(argv[1]);
      Logger::addDataFileLog("test.log");
    } else {
      Logger::init();
    }

    LOG_DEBUG("Start of loop");
    LOG_TRACE("before for loop");
    for (int i = 0; i < 10; ++i) {
      LOG_DATA_INFO("i: " << i);
    }
    LOG_TRACE("after for loop");

  } catch (std::exception& e) {
    LOG_ERROR("Exception in main: " << e.what());
    return 1;
  }

  return 0;
}