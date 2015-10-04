#include "sdl2.hh"
#include "common_tools.hh"
#include "window.hh"

#include <iostream>
#include <mutex>
#include <memory>
#include <vector>
#include <exception>


#ifdef _WIN32
#pragma comment( lib, "SDL2.lib" )
#pragma comment( lib, "SDL2_ttf.lib" )
#pragma comment( lib, "SDL2_image.lib" )

#include <windows.h>

#endif


#ifdef main
#undef main
#endif


using namespace std;


int main()
{
	// Wait for user input at the end when in debug mode
	#ifdef  _DEBUG
	auto defer_enter_to_quit = ct::make_defer( []()
	{
		cout << endl << "Press enter to quit... ";
		cin.ignore();
	} );
	#endif


	/* Set up the graphics */


	// Handle SDL initialization
	if( SDL_Init( SDL_INIT_EVERYTHING ) )
	{
		cerr << "SDL_Init() failed: " << SDL_GetError() << endl;
		return 1;
	}

	// Call SDL_Quit at the end
	auto defer_sdl_quit = ct::make_defer( [](){
		SDL_Quit();
	} );


	// Create the window
	gui::Window window;

	if( !window.is_initialized() )
	{
		return 1;
	}


	// If on windows and not in debug mode, detach the console
	#ifdef  _WIN32
	#ifndef _DEBUG
	FreeConsole();
	#endif
	#endif


	/* Start the main loop */


	bool should_quit = false;
	SDL_Event event;

	while( !should_quit )
	{
		SDL_PollEvent( &event );

		if( event.type == SDL_KEYDOWN )
		{
			if( event.key.keysym.sym == SDLK_ESCAPE )
			{
				should_quit = true;
			}
		}

		SDL_RenderPresent( window.renderer.get() );
	}


	return 0;
}

