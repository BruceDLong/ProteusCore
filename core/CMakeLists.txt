cmake_minimum_required(VERSION 2.8.5)
cmake_policy(SET CMP0015 NEW)

# product options
OPTION(BUILD_WITH_COVERAGE "Build with coverage analysis support" OFF)

set(LANG_MODULES XlaterENGLISH.cpp)

add_definitions( -std=gnu++11 -Wno-deprecated -Wno-write-strings -fpermissive -g -Wall -O0 -fPIC)
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ")

set(PROTEUS_SRCS Proteus.cpp convs.cpp InfonIO.cpp Functions.cpp wordLibrary.cpp ${LANG_MODULES} repoMngr.cpp)
set(UNICODE_LIBS icuio icui18n icutu icuuc icudata icule iculx icutest dl)
set(PROTEUS_LIBS git2 gmpxx gmp dl pthread sqlite3 ${UNICODE_LIBS})
set(CLIP_LIBS ncurses readline)

set(HomeDir ${CMAKE_CURRENT_BINARY_DIR})


include_directories( ${HomeDir}/../builds/libs/GMP_BigInts/${GMP_Version}/include/
                     ${HomeDir}/../builds/libs/unicode/release-51-1/include/
                     ${Slipstream_SOURCE_DIR}/deps/archive/
                     ${HomeDir}/../builds/libs/sqlite3/sqlite-autoconf-3071602/include/
                     ${HomeDir}/../builds/libs/Readline/readline-6.2/include/
                     ${HomeDir}/../builds/libs/GIT_LIB/libgit2-0.18.0/include/
                    )
                    
link_directories(${HomeDir}/../builds/libs/GMP_BigInts/${GMP_Version}/lib/
                 ${HomeDir}/../builds/libs/unicode/release-51-1/lib/
                 ${HomeDir}/../builds/libs/Readline/readline-6.2/lib/
                 ${HomeDir}/../builds/libs/sqlite3/sqlite-autoconf-3071602/lib/
                 ${HomeDir}/../builds/libs/GIT_LIB/libgit2-0.18.0/lib/
                 )

add_library(Proteus SHARED ${PROTEUS_SRCS})
target_link_libraries(Proteus Proteus)

if(NOT(${PLAT_BUILD} STREQUAL "xANDROID_ARM"))
	add_executable(clip clip.cpp)
	target_link_libraries(clip Proteus  ${PROTEUS_LIBS} ${CLIP_LIBS})
endif()

if(${PLAT_BUILD} STREQUAL "HOST")
	##############################################
	## INCLUDE TESTS, COVERAGE, PACKAGING
	IF(UNIX)
	 # SET(MEMORYCHECK_COMMAND "/usr/bin/valgrind")
	 # SET(COVERAGE_COMMAND "/usr/bin/gcov")
	ENDIF()

	INCLUDE(CTest)
	IF(BUILD_WITH_COVERAGE)
		# enable coverage analysis using GCC and GCOV
		IF (CMAKE_COMPILER_IS_GNUCC)
			SET(CMAKE_C_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
			SET(CMAKE_CXX_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
	#        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")

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

	# Create the testCore program
	INCLUDE(${Slipstream_SOURCE_DIR}/core/CMakeTests.txt)
	add_executable(testCore testCore.cpp)
	add_custom_command(TARGET testCore POST_BUILD COMMAND cp ${Slipstream_SOURCE_DIR}/core/M ${HomeDir})
	target_link_libraries(testCore Proteus ${PROTEUS_LIBS})
endif()
