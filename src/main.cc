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



// owner<T> pointer alias
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
	Defer( F func )
	: is_broken(false),
	  call_when_destroyed(func)
	{
	}


	Defer( Defer&& old )
	: is_broken(false),
	  call_when_destroyed(old.call_when_destroyed)
	{
		old.is_broken = true;
	}


	~Defer() noexcept
	{
		static_assert( can_call<F>{}, "Defer<F> - F has to be a callable type!" );

		if( is_broken )
		{
			return;
		}

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
	bool is_broken;
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

	auto window_initialized = window.is_initialized();
	std::cout << "Window.is_initialized(): " << window_initialized << std::endl;

	if( !window_initialized )
	{
		return 1;
	}


	// Start the main loop
	SDL_Event event;


	bool should_quit = false;
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

