cmake_minimum_required(VERSION 3.12)
project(update C CXX)

option(WITH_LIBUSB1 "Use libusb-1.0" ON)

find_package(PkgConfig REQUIRED)
if(WITH_LIBUSB1)
  pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
else()
  pkg_check_modules(LIBUSB REQUIRED libusb)
endif()

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
  usbcompat.c
)
target_compile_definitions(update PRIVATE BUGFIX)
if(WITH_LIBUSB1)
  target_compile_definitions(update PRIVATE WITH_LIBUSB1)
endif()
target_include_directories(update PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(update ${LIBUSB_LIBRARIES})
target_include_directories(update PRIVATE ${LIBUSB_INCLUDE_DIRS})
target_compile_options(update PRIVATE ${LIBUSB_CFLAGS_OTHER})
