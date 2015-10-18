#include "common_tools.hh"

using namespace std;
using namespace tools;

#ifdef _WIN32
#include <windows.h>


vector<DirectoryItem> tools::get_directory_listing( const string& path )
{
	HANDLE                handle;
	WIN32_FIND_DATAA      data;
	vector<DirectoryItem> items;

	handle = FindFirstFileA( path.c_str(), &data );

	if( handle == INVALID_HANDLE_VALUE )
	{
		throw runtime_error( "Couldn't get directory listing for '" + path + "'" );
	}

	auto defer_close_handle = make_defer( [&]()	{
		FindClose( handle );
	} );

	do
	{
		if( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			items.push_back( {
				DirectoryItemType::DIRECTORY,
				data.cFileName,
				0,
				0
			} );
		}
		else
		{
			items.push_back( {
				DirectoryItemType::FILE,
				data.cFileName,
				data.nFileSizeHigh,
				data.nFileSizeLow
			} );
		}
	}
	while( FindNextFile( handle, &data ) );

	auto err = GetLastError();
	if( err != ERROR_NO_MORE_FILES )
	{
		throw runtime_error( "Error iterating over directory items" );
	}

	return items;
}

#else

vector<string> tools::get_directory_listing( const string& path )
{
	static_assert( false, "get_directory_listing not implemented!" );
}

#endif

