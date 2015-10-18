#include "window.hh"

#include <iostream>


using namespace std;
using namespace gui;


Window::Window()
: closed(false), sdl_id(0), width(460), height(320)
{
	// Create the window
	auto window_ptr = SDL_CreateWindow(
		"Visualis",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
	);

	if( window_ptr )
	{
		sdl_id = SDL_GetWindowID( window_ptr );
		std::wcout << "Created window with id " << sdl_id << std::endl;
		window = sdl2::WindowPtr( window_ptr );
	}
	else
	{
		wcerr << "Window::Window() - SDL_CreateWindow() failed: "
		     << SDL_GetError() << endl;

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
		wcerr << "Window::Window() - SDL_CreateRenderer() failed: "
		     << SDL_GetError() << endl;

		return;
	}
}



Window::Window( Window&& other )
{
	using std::swap;
	swap( window,   other.window );
	swap( renderer, other.renderer );
	swap( sdl_id,   other.sdl_id );
	swap( closed,   other.closed );

	wcout << "Window move constructed" << endl;
}



Window& Window::operator=( Window&& other )
{
	using std::swap;
	swap( window,   other.window );
	swap( renderer, other.renderer );
	swap( sdl_id,   other.sdl_id );
	swap( closed,   other.closed );

	wcout << "Window moved" << endl;
	return *this;
}



Window::~Window()
{
	wcout << "Closed window" << endl;
}



bool Window::is_initialized() const
{
	return !closed && !!window && !!renderer;
}



void Window::handle_sdl_event( const SDL_Event &e )
{
	switch( e.window.event )
	{
		case SDL_WINDOWEVENT_SHOWN:
		case SDL_WINDOWEVENT_HIDDEN:
		case SDL_WINDOWEVENT_EXPOSED:
		case SDL_WINDOWEVENT_MOVED:
		case SDL_WINDOWEVENT_MINIMIZED:
		case SDL_WINDOWEVENT_MAXIMIZED:
		case SDL_WINDOWEVENT_RESTORED:
		case SDL_WINDOWEVENT_ENTER:
		case SDL_WINDOWEVENT_LEAVE:
		case SDL_WINDOWEVENT_FOCUS_GAINED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			break;

		case SDL_WINDOWEVENT_RESIZED:
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			width = e.window.data1;
			height = e.window.data2;
			break;

		case SDL_WINDOWEVENT_CLOSE:
			closed = true;
			break;
	}
}

