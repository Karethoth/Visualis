#pragma once

#include "sdl2.hh"

namespace gui
{

struct Window
{
	sdl2::WindowPtr   window;
	sdl2::RendererPtr renderer;
	uint32_t          sdl_id; // SDL Window Id
	bool              closed;

	uint32_t          width;
	uint32_t          height;


	Window();
	~Window();

	Window( Window&& );
	Window& operator=( Window&& );

	bool is_initialized() const;
	void handle_sdl_event( const SDL_Event &e );


	// Delete potentially dangerous constructors and operators
	Window( Window& ) = delete;
	Window& operator=( Window& ) = delete;
};

} // namespace gui

