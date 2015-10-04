#pragma once

#include "window.hh"
#include <iostream>


using namespace std;


gui::Window::Window()
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
		cerr << "Window::Window() - SDL_CreateWindow() failed: "
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
		cerr << "Window::Window() - SDL_CreateRenderer() failed: "
		     << SDL_GetError() << endl;

		return;
	}
}


bool gui::Window::is_initialized() const
{
	return !!window && !!renderer;
}