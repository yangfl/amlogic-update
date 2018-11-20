cmake_minimum_required(VERSION 3.12)
project(update)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb)

add_executable(update
  Amldbglog/Amldbglog.cpp AmlLibusb/AmlLibusb.cpp AmlUsbScan/AmlUsbScan.cpp
  AmlUsbScanX3/AmlUsbScanX3.cpp UsbRomDrv/UsbRomDll.cpp UsbRomDrv/UsbRomDrv.cpp
  AmlTime.c simg_probe.cpp update.cpp)
target_link_libraries(update ${LIBUSB_LIBRARIES})
target_include_directories(update PUBLIC ${LIBUSB_INCLUDE_DIRS})
target_compile_options(update PUBLIC ${LIBUSB_CFLAGS_OTHER})
