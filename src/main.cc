#include "sdl2.hh"
#include "common_tools.hh"
#include "window.hh"
#include "globals.hh"
#include "sgf.hh"
#include "goban.hh"

#include <mutex>
#include <memory>
#include <vector>
#include <locale>
#include <codecvt>
#include <fstream>
#include <iostream>
#include <algorithm>
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



sgf::Node read_sgf_file( const string& path )
{
	auto data = read_file_contents( path );

	// Get rid of the BOM if it's there
	if( data[0] == 0xffef ||
		data[0] == -17 )
	{
		data = data.substr( 3 );
	}

	auto root = sgf::read_game_tree( data );

	//sgf::print_game_tree( root );

	// If we got the GM property, check that the value is correct
	auto game_property = root.properties[L"GM"];
	if( game_property.size() )
	{
		auto game_type = sgf::property_value_to<int>( game_property[0] );
		if( game_type != 1 )
		{
			wcout << "Error: Wrong game type(" << game_type << ") expected 1 for go" << endl;
			throw runtime_error( "Wrong game type" );
		}
	}

	// Grab the date property
	auto date_property = root.properties[L"DT"];
	wstring date = L"Unknown";
	if( date_property.size() )
	{
		date = date_property[0].value;
	}

	wcout << "Game played at date: " << date << endl;

	return root;
}



int main( int argc, char **argv )
{
	if( argc <= 1 )
	{
		wcout << "Give directory path!" << endl;
		return 1;
	}

	srand( static_cast<unsigned>( time( 0 ) ) );

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


	wstring_convert<codecvt_utf8<wchar_t>> converter;


	/* Set up the graphics */


	// Handle SDL initialization
	if( SDL_Init( SDL_INIT_EVERYTHING ) )
	{
		wcerr << "SDL_Init() failed: " << SDL_GetError() << endl;
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


	// Create the board texture
	auto board_surface = sdl2::SurfacePtr(
		IMG_Load( "data/wood.jpg" )
	);
	if( !board_surface )
	{
		wcerr << "Couldn't load wood image" << endl;
		return 1;
	}

	auto board_texture = sdl2::TexturePtr(
		SDL_CreateTextureFromSurface(
			Globals::windows[0].renderer.get(),
			board_surface.get()
		)
	);
	if( !board_texture )
	{
		wcerr << "Couldn't create wood texture" << endl;
		return 1;
	}


	// If on windows and not in debug mode, detach the console
	#ifdef  _WIN32
	#ifndef _DEBUG
	FreeConsole();
	#endif
	#endif


	// Fetch list of the game files to go through
	vector<string> remaining_files;

	try
	{
		vector<string> remaining_directories;

		remaining_directories.push_back( string(argv[1]) );

		while( remaining_directories.size() > 0 )
		{
			vector<string> new_directories;
			for( auto& directory : remaining_directories )
			{
				auto items = tools::get_directory_listing( directory + "/*" );
				for( auto& item : items )
				{
					if( item.type == tools::DirectoryItemType::DIRECTORY )
					{
						if( !item.name.compare( "." ) || !item.name.compare( ".." ) )
						{
							continue;
						}

						new_directories.push_back( directory + "/" + item.name );
						continue;
					}

					remaining_files.push_back( directory + "/" + item.name );
				}
			}
			remaining_directories = new_directories;
		}
	}
	catch( std::runtime_error &e )
	{
		wcout << "Ran into an error: " << e.what() << endl;
	}
	catch( ... )
	{
		wcout << "Ran into an unhandled exception." << endl;
	}

	// Shuffle the order of the files
	std::random_shuffle( remaining_files.begin(), remaining_files.end() );


	// Grab the first game
	go::Goban goban;
	sgf::Node current_game_node;

	while( current_game_node.children.size() == 0 )
	{
		if( !remaining_files.size() )
		{
			wcerr << "No games found." << endl;
			return 1;
		}

		current_game_node = read_sgf_file( remaining_files.back() );
		remaining_files.pop_back();
	}

	// Grab the size property
	size_t board_size = 19;
	auto size_property = current_game_node.properties[L"SZ"];
	if( size_property.size() )
	{
		board_size = sgf::property_value_to<size_t>( size_property[0] );
	}

	goban = go::Goban{ board_size };

	auto tmp_node = current_game_node.children.front();
	current_game_node = tmp_node;



	/* Start main loop */

	SDL_Event event;

	auto next_move_time = chrono::system_clock::now();


	while( !Globals::should_quit )
	{
		while( SDL_PollEvent( &event ) )
		{
			handle_sdl_event( event );
		}

		this_thread::sleep_until( next_move_time );
		
		next_move_time = chrono::system_clock::now() + chrono::milliseconds( 500 );


		// Play the move out
		sgf::Point move{ 0, 0 };
		go::Side player;

		try
		{
			if( current_game_node.properties[L"B"].size() )
			{
				player = go::Side::BLACK;
				move = sgf::property_value_to<sgf::Point>(
					current_game_node.properties[L"B"][0]
				);
			}
			else if( current_game_node.properties[L"W"].size() )
			{
				player = go::Side::WHITE;
				move = sgf::property_value_to<sgf::Point>(
					current_game_node.properties[L"W"][0]
				);
			}
			else
			{
				wcerr << "Skip!" << endl;
				current_game_node = current_game_node.children[0];
				continue;
			}

			go::Stone new_stone = {
				move.x,
				move.y,
				player
			};

			goban.play_stone( new_stone );


			// Move to the next game if this one's played out
			if( !current_game_node.children.size() )
			{
				current_game_node = {};
				while( current_game_node.children.size() == 0 )
				{
					if( !remaining_files.size() )
					{
						wcerr << "No games left" << endl;
						return 0;
					}

					current_game_node = read_sgf_file( remaining_files.back() );

					// Grab the size property
					board_size = 19;
					auto size_property = current_game_node.properties[L"SZ"];
					if( size_property.size() )
					{
						board_size = sgf::property_value_to<size_t>( size_property[0] );
					}

					goban = go::Goban{ board_size };

					remaining_files.pop_back();
				}
				tmp_node = current_game_node.children.front();
				current_game_node = tmp_node;
			}

			// Otherwise just go to the next move
			else
			{
				tmp_node = current_game_node.children.front();
				current_game_node = tmp_node;
			}
		}
		catch( ... )
		{

		}


		// Render board

		auto& window = Globals::windows[0];
		double step_size = (window.width < window.height ?
			window.width : window.height) / (board_size + 1);

		auto stone_size = step_size - step_size / 5;

		SDL_SetRenderDrawColor( window.renderer.get(), 0, 0, 0, 255 );
		SDL_RenderClear( window.renderer.get() );
		SDL_SetRenderDrawColor( window.renderer.get(), 0, 0, 0, 255 );

		SDL_Rect board_rect =
		{
			step_size/2,
			step_size/2,
			board_size * step_size,
			board_size * step_size
		};

		SDL_RenderCopy(
			window.renderer.get(),
			board_texture.get(),
			nullptr,
			&board_rect
		);

		// Render lines

		for( size_t y = 1; y <= board_size; y++ )
		{
			SDL_RenderDrawLine(
				window.renderer.get(),
				static_cast<int>(step_size),
				static_cast<int>(y*step_size),
				static_cast<int>(board_size * step_size),
				static_cast<int>(y*step_size)
			);
		}

		for( size_t x = 1; x <= board_size; x++ )
		{
			SDL_RenderDrawLine(
				window.renderer.get(),
				static_cast<int>(x*step_size),
				static_cast<int>(step_size ),
				static_cast<int>(x*step_size),
				static_cast<int>(board_size * step_size)
			);
		}

		// Render stones

		auto& stones = goban.get_board();
		for( auto& stone : stones )
		{
			if( stone.side == go::Side::NONE )
			{
				continue;
			}

			else if( stone.side == go::Side::BLACK )
			{
				SDL_SetRenderDrawColor( window.renderer.get(), 0, 0, 0, 255 );
			}
			else
			{
				SDL_SetRenderDrawColor( window.renderer.get(), 255, 255, 255, 255 );
			}

			SDL_Rect stone_rect
			{
				static_cast<int>((stone.x+1) * step_size - stone_size / 2),
				static_cast<int>((stone.y+1) * step_size - stone_size / 2),
				static_cast<int>(stone_size),
				static_cast<int>(stone_size)
			};

			SDL_RenderFillRect( window.renderer.get(), &stone_rect );
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

