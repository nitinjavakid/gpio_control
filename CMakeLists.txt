cmake_minimum_required(VERSION 2.6.11)

project (GPIO_CONTROL)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBEVDEV REQUIRED libevdev)

add_executable (gpio_control main.cpp)

target_include_directories(gpio_control PUBLIC ${LIBEVDEV_INCLUDE_DIRS})
target_link_libraries (gpio_control wiringPi ${LIBEVDEV_LIBRARIES})
target_compile_options(gpio_control PUBLIC ${LIBEVDEV_CFLAGS_OTHER})
