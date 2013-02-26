cmake_minimum_required(VERSION 2.8.5)
cmake_policy(SET CMP0015 NEW)

set(LANG_MODULES XlaterENGLISH.cpp)

add_definitions( -std=gnu++11 -Wno-deprecated -Wno-write-strings -fpermissive -g -Wall)

set(PROTEUS_SRCS Proteus.cpp convs.cpp InfonIO.cpp Functions.cpp wordLibrary.cpp ${LANG_MODULES})
set(UNICODE_LIBS icuio icui18n icutu icuuc icudata icule iculx icutest dl)
set(PROTEUS_LIBS readline gmpxx gmp sqlite3 ${UNICODE_LIBS})

include_directories( #../builds/libs/GMP_BigInts/gmp-5.0.4/include/
                    ../builds/libs/unicode/release-49-1-1/include/
                    ../../dev/boost_1_51_0/
                    #../builds/libs/Readline/readline-6.2/include/
                    )
link_directories(#../builds/libs/GMP_BigInts/gmp-5.0.4/lib/
                 ../builds/libs/unicode/release-49-1-1/lib/
                # ../builds/libs/Readline/readline-6.2/lib/
                 )

#add_library(Proteus SHARED ${PROTEUS_SRCS})
add_executable(clip clip.cpp ${PROTEUS_SRCS})
target_link_libraries(clip ${PROTEUS_LIBS})
#set_target_properties(clip PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")

add_executable(testCore testCore.cpp ${PROTEUS_SRCS})
target_link_libraries(testCore ${PROTEUS_LIBS})


##############################################3
## Tests
ENABLE_TESTING()
#define a macro to simplify adding tests, then use it
#macro (do_test arg result)
#  add_test (TutorialComp${arg} test ${arg})
#  set_tests_properties (TutorialComp${arg} PROPERTIES PASS_REGULAR_EXPRESSION ${result})
#endmacro (do_test)
