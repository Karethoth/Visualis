#pragma once

#include <vector>
#include <iostream>


namespace go
{
	enum Side
	{
		NONE,
		BLACK,
		WHITE
	};



	struct Stone
	{
		size_t x = 0;
		size_t y = 0;

		Side side = NONE;
		size_t number = 0;

		Stone(
			size_t x_pos = 0,
			size_t y_pos = 0,
			Side   player = Side::NONE,
			size_t move_num = 0
		);
	};



	class Goban
	{
		size_t             board_size;
		size_t             current_move;
		std::vector<Stone> board;


	  public:
		Goban( size_t size = 19 );

		void play_stone( Stone stone );

		const std::vector<Stone>& get_board();

		void clear();



	  protected:
		std::vector<std::vector<Stone>>
		form_groups( std::vector<Stone> stones );


		std::vector<Stone> get_stone_neighbors(
			const std::vector<Stone>& board,
			const Stone& stone
		);

		std::vector<Stone> find_remaining_liberties(
			const std::vector<Stone>& board,
			const std::vector<Stone>& group
		);

		std::vector<std::vector<Stone>> capture_groups(
			const std::vector<Stone>& board,
			Stone stone
		);
	};
}

