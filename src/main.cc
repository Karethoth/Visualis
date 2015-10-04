#include <iostream>
#include <mutex>
#include <memory>
#include <vector>
#include <exception>
#include <type_traits>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>


#ifdef _WIN32
#pragma comment( lib, "SDL2.lib" )
#pragma comment( lib, "SDL2_ttf.lib" )
#pragma comment( lib, "SDL2_image.lib" )
#endif

#ifdef main
#undef main
#endif

using namespace std;



// owner<T> pointer helper
template<typename T>
using owner = T;



// can_call
struct can_call_test
{
	template<typename F>
	static decltype(std::declval<F>()(), std::true_type())
	f( int );

	template<typename F, typename... A>
	static std::false_type
	f( ... );
};

template<typename F, typename...A>
using can_call = decltype(can_call_test::f<F>( 0 ));



// Defer
template<typename F>
struct Defer
{
	Defer( F func ) : call_when_destroyed( func )
	{
	}


	// Allow returning an instance of Defer by a move.
	Defer( Defer&& def ) : call_when_destroyed(def.call_when_destroyed)
	{
		// If trying to move in other situations, this will always fail
		static_assert(false, "Defer<F> - Can't be constructed from an universal reference!");
	}


	~Defer() noexcept
	{
		static_assert(can_call<F>{}, "Defer<F> - F has to be a callable type!");

		try
		{
			call_when_destroyed();
		}
		catch( ... )
		{
			std::cout << "Defer<F> - Ran into an exception when called the deferred func!" << std::endl;
		}
	}

	// Delete dangerous constructors and operators
	Defer( Defer& )             = delete;
	Defer& operator=( Defer& )  = delete;
	Defer& operator=( Defer&& ) = delete;


private:
	const F call_when_destroyed;
};



// Helper for making defers
template <typename F>
constexpr auto make_defer( F func )
{
	// This is what we need the Defer move constructor to exist for
	return Defer<decltype(func)>{ func };
}



// Wrapping SDL2 stuff
namespace sdl2
{
	struct Deleter
	{
		void operator()( SDL_Surface  *ptr ) { if( ptr ) SDL_FreeSurface( ptr ); }
		void operator()( SDL_Window   *ptr ) { if( ptr ) SDL_DestroyWindow( ptr ); }
		void operator()( SDL_Texture  *ptr ) { if( ptr ) SDL_DestroyTexture( ptr ); }
		void operator()( SDL_Renderer *ptr ) { if( ptr ) SDL_DestroyRenderer( ptr ); }
	};

	using SurfacePtr  = std::unique_ptr<SDL_Surface,  Deleter>;
	using WindowPtr   = std::unique_ptr<SDL_Window,   Deleter>;
	using TexturePtr  = std::unique_ptr<SDL_Texture,  Deleter>;
	using RendererPtr = std::unique_ptr<SDL_Renderer, Deleter>;
}



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
			std::cout << "Window created\n";
			window = sdl2::WindowPtr( window_ptr );
		}
		else
		{
			throw std::runtime_error(
				"Window::Window() - SDL_CreateWindow() failed: " +
				std::string(SDL_GetError())
			);
		}


		// Create the renderer
		auto renderer_ptr = SDL_CreateRenderer( window_ptr, 0, SDL_RENDERER_ACCELERATED );
		if( renderer_ptr )
		{
			std::cout << "Renderer created\n";
			renderer = sdl2::RendererPtr( renderer_ptr );
		}
		else
		{
			throw std::runtime_error(
				"Window::Window() - SDL_CreateRenderer() failed: " +
				std::string( SDL_GetError() )
			);
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



int main()
{
	// Handle SDL initialization
	if( SDL_Init( SDL_INIT_EVERYTHING ) )
	{
		cerr << "SDL_Init() failed: " << SDL_GetError() << endl;
		return 1;
	}

	// Call SDL_Quit at the end
	auto sdl_defer_quit = make_defer( [](){ SDL_Quit(); } );


	// Set the GUI up
	Window window;

	std::cout << "Window.is_initialized(): " << window.is_initialized() << std::endl;

	bool should_quit = false;


	// Start the main loop
	SDL_Event event;

	while( !should_quit )
	{
		SDL_PollEvent( &event );

		if( event.type == SDL_KEYDOWN )
		{
			if( event.key.keysym.sym == SDLK_ESCAPE )
			{
				break;
			}
		}
	}


	return 0;
}

