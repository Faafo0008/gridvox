# Cross-compilation toolchain: Linux host -> Windows x86_64 (MinGW-w64, POSIX threads).
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(MINGW_PREFIX x86_64-w64-mingw32)

set(CMAKE_C_COMPILER   ${MINGW_PREFIX}-gcc-posix)
set(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++-posix)
set(CMAKE_RC_COMPILER  ${MINGW_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_C_FLAGS_INIT   "-static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS_INIT "-static-libgcc -static-libstdc++")
