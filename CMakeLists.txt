cmake_minimum_required(VERSION 3.0)
project(lotrbfme-launchers)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

enable_language(C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

if(MSVC)
	set(CompilerFlags
		CMAKE_CXX_FLAGS
		CMAKE_CXX_FLAGS_DEBUG
		CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_RELWITHDEBINFO
		CMAKE_CXX_FLAGS_MINSIZEREL
		CMAKE_C_FLAGS
		CMAKE_C_FLAGS_DEBUG
		CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_RELWITHDEBINFO
		CMAKE_C_FLAGS_MINSIZEREL
	)

	foreach(CompilerFlag ${CompilerFlags})
		string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
endif()

set(LAUNCHER_FILES launcher.c launcher.h)

add_executable(lotrbfme WIN32
	${LAUNCHER_FILES}
	./lotrbfme.rc
	./lotrbfme.ico
)
target_compile_definitions(lotrbfme PUBLIC LOTRBFME_GAME=1)

add_executable(lotrbfme2 WIN32
	${LAUNCHER_FILES}
	./lotrbfme2.rc
	./lotrbfme2.ico
)
target_compile_definitions(lotrbfme2 PUBLIC LOTRBFME_GAME=2)

add_executable(lotrbfme2ep1 WIN32
	${LAUNCHER_FILES}
	./lotrbfme2ep1.rc
	./lotrbfme2ep1.ico
)
target_compile_definitions(lotrbfme2ep1 PUBLIC LOTRBFME_GAME=3)