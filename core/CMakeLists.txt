cmake_minimum_required(VERSION 2.8.5)
cmake_policy(SET CMP0015 NEW)

# product options
OPTION(BUILD_WITH_COVERAGE "Build with coverage analysis support" OFF)

set(LANG_MODULES XlaterENGLISH.cpp)

add_definitions( -std=gnu++11 -Wno-deprecated -Wno-write-strings -fpermissive -g -Wall -O0 -fprofile-arcs -ftest-coverage)
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")

set(PROTEUS_SRCS Proteus.cpp convs.cpp InfonIO.cpp Functions.cpp wordLibrary.cpp ${LANG_MODULES})
set(UNICODE_LIBS icuio icui18n icutu icuuc icudata icule iculx icutest dl)
set(PROTEUS_LIBS readline gmpxx gmp sqlite3 ${UNICODE_LIBS})

include_directories(# ../builds/libs/GMP_BigInts/gmp-5.0.4/include/
                    ../builds/libs/unicode/release-51-1/include/
                    ../../dev/boost_1_51_0/
                    ../builds/libs/sqlite3/sqlite-autoconf-3071602/include/
                    #../builds/libs/Readline/readline-6.2/include/

                    )
link_directories(#../builds/libs/GMP_BigInts/gmp-5.0.4/lib/
                 ../builds/libs/unicode/release-51-1/lib/
                # ../builds/libs/Readline/readline-6.2/lib/
                 ../builds/libs/sqlite3/sqlite-autoconf-3071602/lib/
                 )

#add_library(Proteus SHARED ${PROTEUS_SRCS})
add_executable(clip clip.cpp ${PROTEUS_SRCS})
target_link_libraries(clip ${PROTEUS_LIBS})
#set_target_properties(clip PROPERTIES COMPILE_FLAGS "-m38" LINK_FLAGS "-m39")

add_executable(testCore testCore.cpp ${PROTEUS_SRCS})
target_link_libraries(testCore ${PROTEUS_LIBS})


##############################################
## INCLUDE TESTS, COVERAGE, PACKAGING
IF(UNIX)
  SET(MEMORYCHECK_COMMAND "/usr/bin/valgrind")
  SET(COVERAGE_COMMAND "/usr/bin/gcov")
ENDIF()

INCLUDE(CTest)
IF(BUILD_WITH_COVERAGE)
    # enable coverage analysis using GCC and GCOV
    IF (CMAKE_COMPILER_IS_GNUCC)
        SET(CMAKE_C_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
        SET(CMAKE_CXX_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")

    ENDIF()
ENDIF(BUILD_WITH_COVERAGE)

#        # add lcov target
#        include(${CMAKE_SOURCE_DIR}/lcov.cmake)
#        ADD_DEPENDENCIES(lcov clip)

## lcov target
ADD_CUSTOM_TARGET(lcov
    COMMAND mkdir -p coverage
    COMMAND lcov --directory . --zerocounters
    COMMAND ctest --timeout 4
    COMMAND lcov --directory . --capture --output-file ./coverage/stap.info
    COMMAND genhtml -o ./coverage/stap ./coverage/stap.info.cleaned
    COMMAND echo "Open ${CMAKE_BINARY_DIR}/coverage/index.html to view the coverage analysis results."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
ADD_DEPENDENCIES(lcov clip)

INCLUDE(CMakeTests.txt)
