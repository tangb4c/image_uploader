project( image_uploader_backend )

include( FetchContent )

cmake_minimum_required( VERSION 3.10 )
set(CMAKE_CXX_STANDARD 20)

FetchContent_Declare( fmt
	GIT_REPOSITORY
		https://github.com/fmtlib/fmt.git
	GIT_TAG
		9.1.0
)

FetchContent_Declare( json
	GIT_REPOSITORY
		https://github.com/nlohmann/json.git
	GIT_TAG
		v3.11.2
)

FetchContent_Declare( spdlog
	GIT_REPOSITORY
		https://github.com/gabime/spdlog.git
	GIT_TAG
		v1.12.0
)

FetchContent_MakeAvailable( fmt json spdlog)

set( CMAKE_EXECUTABLE_SUFFIX ".cgi" )

add_executable( image_uploader_backend
	image_uploader_backend.cpp
)

target_compile_features( image_uploader_backend
	PRIVATE
		cxx_std_20
)

target_link_libraries( image_uploader_backend
	PRIVATE
		fmt
		nlohmann_json
		spdlog
)

if( NOT PASSWORD )
	message( FATAL_ERROR "please define PASSWORD" )
endif()

if( NOT SITE_DOMAIN )
	message( FATAL_ERROR "please define SITE_DOMAIN" )
endif()

target_compile_definitions( image_uploader_backend
	PRIVATE
		-DPASSWORD="${PASSWORD}"
		-DSITE_DOMAIN="${SITE_DOMAIN}"
)
