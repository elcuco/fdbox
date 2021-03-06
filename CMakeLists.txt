# set the project name
project(fdbox)

# add the executable
cmake_minimum_required(VERSION 3.10)

project(fdbox VERSION 0.1)

include(cmake/CPM.cmake)

cpmaddpackage(
  NAME
  Format.cmake
  VERSION
  1.7.0
  GITHUB_REPOSITORY
  TheLartians/Format.cmake
  OPTIONS # set to yes skip cmake formatting
  "FORMAT_SKIP_CMAKE NO"
  # path to exclude (optional, supports regular expressions)
  "CMAKE_FORMAT_EXCLUDE cmake/CPM.cmake")

add_executable(
  tests
  tests.c
  lib/applet.c
  lib/args.c
  lib/strextra.c
  lib/environ.c
  )
target_include_directories(tests PUBLIC "${PROJECT_BINARY_DIR}"
                                        "${PROJECT_SOURCE_DIR}")


add_executable(
  fdbox
  readme.md
  main.c
  applets.c
  help.c
  lib/applet.c
  lib/args.c
  lib/strextra.c
  lib/environ.c
  dos/beep.c
  dos/call.c
  dos/cd.c
  dos/cls.c
  dos/command.c
  dos/copymove.c
  dos/datetime.c
  dos/del.c
  dos/dir.c
  dos/echo.c
  dos/exit.c
  dos/for.c
  dos/goto.c
  dos/if.c
  dos/md.c
  dos/path.c
  dos/prompt.c
  dos/reboot.c
  dos/rem.c
  dos/set.c
  dos/shift.c
  dos/type.c
  dos/ver.c
  unix/cal.c
  unix/head.c
  unix/hexdump.c
  unix/tail.c)

if(WIN32)
  target_sources(fdbox PRIVATE
      lib/win32/win32-glob.c
      lib/win32/win32-glob.h
      lib/tc202/stdextra.c
    )
endif()

target_include_directories(fdbox PUBLIC "${PROJECT_BINARY_DIR}"
                                        "${PROJECT_SOURCE_DIR}")
