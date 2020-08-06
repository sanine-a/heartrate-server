cmake_minimum_required(VERSION 3.10)

project(heartrate-server C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

find_package(Threads REQUIRED)

add_executable(heartrate-server
  src/mg/mongoose.c
  src/smmServer.cpp
  src/ArduinoSerial.cpp
  src/SignalProcessor.cpp
  src/SignalProcessor-Auxillary.cpp
  src/ConnectionManager.cpp
  src/main.cpp)

target_link_libraries(heartrate-server ssl crypto Threads::Threads serialport sfml-audio)

if (WIN32)
  target_link_libraries(heartrate-server ws2_32)
endif()
