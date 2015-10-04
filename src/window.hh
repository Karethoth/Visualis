#pragma once

#include "sdl2.hh"

namespace gui
{

struct Window
{
	sdl2::WindowPtr   window;
	sdl2::RendererPtr renderer;


	Window();

	bool is_initialized() const;


	// Delete potentially dangerous constructors and operators
	Window( Window& ) = delete;
	Window( Window&& ) = delete;
	Window& operator=( Window& ) = delete;
	Window& operator=( Window&& ) = delete;
};

} // namespace gui

