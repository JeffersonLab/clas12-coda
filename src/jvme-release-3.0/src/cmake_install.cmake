# Install script for directory: /usr/clas12/release/2.0.0/coda/src/jvme-release-3.0/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/clas12/release/2.0.0/coda/src/jvme-release-3.0/linuxvme")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_display" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_display")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_display"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin" TYPE EXECUTABLE FILES "/usr/clas12/release/2.0.0/coda/src/jvme-release-3.0/src/vme_display")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_display" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_display")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_display"
         OLD_RPATH "/usr/clas12/release/2.0.0/coda/src/jvme-release-3.0:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_display")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_modmem" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_modmem")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_modmem"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin" TYPE EXECUTABLE FILES "/usr/clas12/release/2.0.0/coda/src/jvme-release-3.0/src/vme_modmem")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_modmem" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_modmem")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_modmem"
         OLD_RPATH "/usr/clas12/release/2.0.0/coda/src/jvme-release-3.0:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_modmem")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_sysreset" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_sysreset")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_sysreset"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin" TYPE EXECUTABLE FILES "/usr/clas12/release/2.0.0/coda/src/jvme-release-3.0/src/vme_sysreset")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_sysreset" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_sysreset")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_sysreset"
         OLD_RPATH "/usr/clas12/release/2.0.0/coda/src/jvme-release-3.0:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Linux_x86_64_vme/bin/vme_sysreset")
    endif()
  endif()
endif()

