#include <iostream>
#include <mutex>
#include <memory>
#include <vector>
#include <exception>

#include "sdl2.hh"
#include "common_tools.hh"


#ifdef _WIN32
#pragma comment( lib, "SDL2.lib" )
#pragma comment( lib, "SDL2_ttf.lib" )
#pragma comment( lib, "SDL2_image.lib" )
#endif

#ifdef main
#undef main
#endif



struct Window
{
	sdl2::WindowPtr   window;
	sdl2::RendererPtr renderer;


	Window()
	{
		// Create the window
		auto window_ptr = SDL_CreateWindow(
			"Visualis",
			0, 0,
			460, 320,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
		);

		if( window_ptr )
		{
			window = sdl2::WindowPtr( window_ptr );
		}
		else
		{
			std::cerr << "Window::Window() - SDL_CreateWindow() failed: "
			          << SDL_GetError() << std::endl;

			return;
		}


		// Create the renderer
		auto renderer_ptr = SDL_CreateRenderer( window_ptr, 0, SDL_RENDERER_ACCELERATED );
		if( renderer_ptr )
		{
			renderer = sdl2::RendererPtr( renderer_ptr );
		}
		else
		{
			std::cerr << "Window::Window() - SDL_CreateRenderer() failed: "
			          << SDL_GetError() << std::endl;

			return;
		}
	}


	bool is_initialized()
	{
		return !!window && !!renderer;
	}


	// Delete potentially dangerous constructors and operators
	Window( Window& )             = delete;
	Window( Window&& )            = delete;
	Window& operator=( Window& )  = delete;
	Window& operator=( Window&& ) = delete;
};


using namespace std;


int main()
{
	// Handle SDL initialization
	if( SDL_Init( SDL_INIT_EVERYTHING ) )
	{
		cerr << "SDL_Init() failed: " << SDL_GetError() << endl;
		return 1;
	}

	// Call SDL_Quit at the end
	auto sdl_defer_quit = ct::make_defer( [](){
		SDL_Quit();
	} );


	// Set the GUI up
	Window window;

	if( window.is_initialized() )
	{
		return 1;
	}


	// Start the main loop
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

