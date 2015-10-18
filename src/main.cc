#include "sdl2.hh"
#include "common_tools.hh"
#include "window.hh"
#include "globals.hh"
#include "sgf.hh"

#include <mutex>
#include <memory>
#include <vector>
#include <fstream>
#include <iostream>
#include <exception>


#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>

#pragma comment( lib, "SDL2.lib" )
#pragma comment( lib, "SDL2_ttf.lib" )
#pragma comment( lib, "SDL2_image.lib" )
#endif


#ifdef main
#undef main
#endif


using namespace std;


// Set up the Globals
bool Globals::should_quit = false;

vector<gui::Window> Globals::windows{};
mutex Globals::windows_mutex = {};




void handle_sdl_event( const SDL_Event &e )
{
	if( e.type == SDL_QUIT )
	{
		Globals::should_quit = true;
	}

	else if( e.type == SDL_KEYDOWN )
	{
		if( e.key.keysym.sym == SDLK_ESCAPE )
		{
			Globals::should_quit = true;
		}
	}

	else if( e.type == SDL_WINDOWEVENT )
	{
		auto window_id = e.window.windowID;

		lock_guard<mutex> windows_lock{ Globals::windows_mutex };
		for( auto& window : Globals::windows )
		{
			if( window.sdl_id == window_id )
			{
				window.handle_sdl_event( e );
				return;
			}
		}

		cerr << "Unhandled SDL_WINDOWEVENT, target window "
		     << window_id << " not found" << endl;
	}
}



auto read_file_contents( const string& filename )
{
	ifstream in( filename, ios_base::in | ios_base::binary );
	if( !in.is_open() )
	{
		throw runtime_error( "Couldn't open the file!" );
	}

	wstring str{ istreambuf_iterator<char>( in ),
	             istreambuf_iterator<char>() };

	return str;
}



int main()
{
	// Wait for user input at the end when in debug mode
	#ifdef  _DEBUG
	auto defer_enter_to_quit = tools::make_defer( []()
	{
		wcout << endl << "Press enter to quit... ";
		cin.ignore();
	} );
	#endif

	#ifdef _WIN32
	int const newMode = _setmode( _fileno( stdout ), _O_U8TEXT );
	#endif


	try
	{
		auto data = read_file_contents( "in.sgf" );

		// Get rid of the BOM if it's there
		if( data[0] == 0xffef || data[0] == -17 )
		{
			data = data.substr( 3 );
		}

		auto root = sgf::read_game_tree( data );

		sgf::print_game_tree( root );

		auto comment = root.properties[L"C"].front();
	}
	catch( std::runtime_error &e )
	{
		wcout << "Ran into an error: " << e.what() << endl;
	}
        catch( ... )
        {
		wcout << "Ran into an unhandled exception." << endl;
        }

	return 0;


	/* Set up the graphics */


	// Handle SDL initialization
	if( SDL_Init( SDL_INIT_EVERYTHING ) )
	{
		cerr << "SDL_Init() failed: " << SDL_GetError() << endl;
		return 1;
	}

	// Call SDL_Quit at the end
	auto defer_sdl_quit = tools::make_defer( [](){
		SDL_Quit();
	} );


	// Create a window
	Globals::windows.emplace_back();
	if( Globals::windows.size() <= 0 ||
	   !Globals::windows[0].is_initialized() )
	{
		return 1;
	}

	// Clear windows automatically at the end,
	// while the SDL context is still okay
	auto defer_close_windows = tools::make_defer( []()
	{
		lock_guard<mutex> windows_lock{ Globals::windows_mutex };
		Globals::windows.clear();
	} );


	// If on windows and not in debug mode, detach the console
	#ifdef  _WIN32
	#ifndef _DEBUG
	FreeConsole();
	#endif
	#endif


	/* Start the main loop */


	SDL_Event event;

	while( !Globals::should_quit )
	{
		while( SDL_PollEvent( &event ) )
		{
			handle_sdl_event( event );
		}

		// Remove closed windows and render all windows
		lock_guard<mutex> windows_lock{ Globals::windows_mutex };
		for( auto it = Globals::windows.begin();
		     it != Globals::windows.end(); )
		{
			if( (*it).closed )
			{
				it = Globals::windows.erase( it );
				continue;
			}

			SDL_RenderPresent( (*it).renderer.get() );
			++it;
		}
	}


	return 0;
}

