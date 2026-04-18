cmake_minimum_required(VERSION 3.12)
project(update C CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(update
  # Amldbglog.dll
  Amldbglog.cpp
  # AmlLibusb.dll
  AmlLibusb.cpp
  # AmlUsbScanX3.dll
  AmlUsbScanX3.cpp
  # UsbRomDrv.dll
  UsbRomDrv.cpp
  # update.exe
  scan.cpp
  update.cpp
)
target_compile_definitions(update PRIVATE BUGFIX)
target_include_directories(update PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(update ${LIBUSB_LIBRARIES})
target_include_directories(update PRIVATE ${LIBUSB_INCLUDE_DIRS})
target_compile_options(update PRIVATE ${LIBUSB_CFLAGS_OTHER})
