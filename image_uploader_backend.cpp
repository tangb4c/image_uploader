#include <span>
#include <cstdlib>
#include <string>
#include <charconv>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <fstream>
#include <functional>
#include <filesystem>
#include "fmt/format.h"
#include "nlohmann/json.hpp"

std::string_view env( const char *str )
{
	if ( auto result = getenv( str ) )
	{
		return result;
	}
	throw std::runtime_error( "missing environment variable: " + std::string( str ) );
}

template < typename T >
T to_number( std::string_view str )
{
	T result;
	auto [ ptr, ec ] = std::from_chars( str.begin(), str.end(), result );
	if ( ec == std::errc() )
	{
		return result;
	}
	throw std::system_error( std::make_error_code( ec ) );
}

std::string_view remove_quotes( std::string_view str )
{
	if ( str.size() >= 2 )
	{
		switch ( str.front() )
		{
			case '"':
			case '\'':
				if ( str.back() == str.front() )
				{
					return str.substr( 1, str.size() - 2 );
				}
		}
	}
	return str;
}

std::vector< std::string_view > split( std::string_view str, std::string_view separator )
{
	std::vector< std::string_view > result;
	auto pos = str.find( separator );
	while ( pos != str.npos )
	{
		result.push_back( str.substr( 0, pos ) );
		str = str.substr( pos + separator.size() );
		pos = str.find( separator );
	}
	result.push_back( str );
	return result;
}

template < typename T, typename F >
constexpr auto for_each( T t, F f )
{
	for ( auto &i : t ) i = f( i );
	return t;
}

constexpr std::string_view trim( std::string_view str )
{
	while ( !str.empty() && std::isspace( str.front() ) )
	{
		str = str.substr( 1 );
	}
	while ( !str.empty() && std::isspace( str.back() ) )
	{
		str = str.substr( 0, str.size() - 1 );
	}
	return str;
}

auto get_post_from(std::istream &stream, std::string_view ignore) {
  std::unordered_map<std::string, std::string> result;
  std::string line;
  std::string body, value;
  constexpr std::string_view content = "Content-Disposition: form-data;";
  const std::string boundary = std::string("--").append(ignore).append(1, '\r');
  const std::string boundary_end = std::string("--").append(ignore).append("--\r");

  bool parse_header = true;

  while (std::getline(stream, line)) {
//		const auto trimmed_line = trim( line );

    if (boundary == line || boundary_end == line) {
      if (value.size() && body.size()) {
        body.pop_back();    // 移除最后一个\r
        result[value]= body;
      }
      value.clear();
      body.clear();
      parse_header = true;
      if(boundary_end == line){
        break;
      }
      continue;
    }
    if (parse_header) {
      if (line == "\r") {
        parse_header = false;
        continue;
      }
      if (line.starts_with(content)) {
        const auto attr_list = for_each(split(line, ";"), trim);
        for (auto &&attr: attr_list) {
          const auto attr_value = for_each(split(attr, "="), trim);
          if (attr_value[0] == "name") {
            value = remove_quotes(attr_value.at(1));
          } else if (attr_value[0] == "filename") {
            result["filename"] = remove_quotes(attr_value.at(1));
            result["binary"] = "from curl";  // 来自curl之类的
          }
        }

      }
    } else {
      if (!body.empty()) {
        body.append(1, '\n'); // 补上被getline移除的\n
      }
      body.append(line);
    }
  }
  return result;
}

std::string base64_decode( std::string_view in )
{
	std::string out;
	out.reserve( in.size() * 64 / 255 );

    constexpr std::string_view lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t val = 0;
	int valb = -8;
    for ( auto c : in )
	{
		const auto value = lookup.find( c );
		if ( value != std::string_view::npos )
		{
			val = ( val << 6 ) + value;
			valb += 6;
			if ( valb >= 0 )
			{
				out.push_back( static_cast<std::string::value_type>( ( val >> valb ) & 0xFF ) );
				valb -= 8;
			}
		}
    }
    return out;
}

template < typename T >
void store( std::filesystem::path path, std::span< T > data )
{
	std::ofstream( path.c_str(), std::ios::binary ).write( reinterpret_cast< const char* >( data.data() ), data.size() );
}

std::string_view extension( std::string_view str )
{
	const auto filename_start = std::max( str.find_last_of( '/' ), 0ul );
	const auto extension_start = str.find( '.', filename_start );
	return str.substr( extension_start );
}

std::string tolower( std::string_view str )
{
	std::string result;
	std::transform(
		str.begin(),
		str.end(),
		std::back_insert_iterator( result ),
		[]( auto c ) { return std::tolower( c ); }
	);
	return result;
}
#if defined(__clang__)
const std::string CONVERT = "/usr/local/bin/convert";
#else
const std::string CONVERT = "/usr/bin/convert";
#endif

std::filesystem::path resize( std::filesystem::path source, const std::string& signature, int width )
{
	auto ext = tolower( extension( source.string() ) );
	if ( ext == ".heic" ) ext = ".jpg";
	std::filesystem::path result = signature + "-" + std::to_string( width ) + ext;
	system(
		( CONVERT + ' ' + source.string() + " -thumbnail " + std::to_string( width ) + "x " + source.parent_path().string() + '/' + result.string() ).c_str()
	);
	return result;
}

#ifndef SITE_DOMAIN
#error "Please define SITE_DOMAIN for link"
#endif

std::string handle_file( const nlohmann::json &js )
{
  std::string data;
  if(js.contains("binary")){
      data = js.at("file").get<std::string>();
  }else{
    data = base64_decode( js.at( "file" ).get< std::string >() );
  }
	const auto signature = std::to_string(
		std::hash< std::string_view >{}( std::string_view{ reinterpret_cast< const char* >( data.data() ), data.size() } )
	);
	const auto dirname = std::to_string(std::hash<std::string_view>{}(std::string_view{ data.data(), data.size()/3})).substr(0, 3);

	const auto filename = std::filesystem::path( js.at( "filename" ).get< std::string >() );
	const auto extension = filename.extension();
	const auto prefix = std::filesystem::path( "storage" ) / dirname;
	const auto metadata_filename = signature + "-metadata.json";
  const auto original_filename = signature + "-original" + extension.string();
  const auto original_file = prefix/original_filename;
	if ( !std::filesystem::is_regular_file(original_file))
	{
		nlohmann::json metadata;
		metadata[ "metadata" ] = metadata_filename;
		std::filesystem::create_directories( prefix );
		metadata[ "prefix" ] = prefix.string();
		metadata[ "original" ] = original_filename;
		metadata[ "original_name" ] = filename.string();
		const auto source = metadata[ "original" ];
		store( original_file, std::span{ data } );
		metadata[ "thumbnail" ] = resize( original_file, signature,  120 ).string();
		metadata[ "forum" ] = resize( original_file, signature, 600 ).string();
		metadata[ "link"] = SITE_DOMAIN + std::string("/") + std::string( original_file);
		auto dump_str = metadata.dump();
		#if defined(__clang__)
		store( prefix / metadata_filename, std::span(dump_str) );
		#else
		store( prefix / metadata[ "metadata" ], std::span(dump_str.begin(), dump_str.end()) );
		#endif
		return dump_str;
	}
	std::ifstream file( ( prefix / metadata_filename ).c_str() );
	return std::string( std::istream_iterator< char >{ file }, std::istream_iterator< char >{} );
}

#ifndef PASSWORD
#error "Please define PASSWORD for password checking"
#endif


int main( int argc, char *argv[] )
{
	nlohmann::json js;
	fmt::print( "Content-type: application/json\r\n\r\n" );
	// const auto args = std::span{ argv, static_cast< size_t >( argc ) };
	try
	{
		// const auto content_length = to_number< size_t >( env( "CONTENT_LENGTH" ) );
		const auto content_type = for_each( split( env( "CONTENT_TYPE" ), ";" ), trim );
		constexpr std::string_view boundary = "boundary=";
		std::string multi_part_boundary;
		for ( auto i : content_type )
		{
			if ( i.starts_with( boundary ) )
			{
				auto part = i.substr( boundary.size() );
                                multi_part_boundary = part;
//				multi_part_boundary = part.substr( part.find_first_not_of( "-" ) );
			}
		}
		for ( auto &[k,v] : get_post_from( std::cin, multi_part_boundary ) )
		{
			js[ k ] = v;
		}

		if ( !js.contains( "password" ) )
		{
			throw std::runtime_error( "no password specified" );
		}
		if ( PASSWORD != js[ "password" ].get< std::string >() )
		{
			throw std::runtime_error( "invalid password" );
		}

		fmt::print( "{}", handle_file( js ) );
	}
	catch( const std::exception &err )
	{
		js[ "error" ] = err.what();
		fmt::print( "{}", js.dump() );
	}

	return 0;
}
