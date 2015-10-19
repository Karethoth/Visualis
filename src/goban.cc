#include "goban.hh"

#include <iostream>
#include <exception>

using namespace go;
using namespace std;



go::Stone::Stone(
	size_t x_pos,
	size_t y_pos,
	Side   player,
	size_t move_num
)
: x(x_pos),
  y(y_pos),
  side(player),
  number(move_num)
{}



go::Goban::Goban( size_t size )
: board_size( size ), current_move( 1 )
{
	clear();
}



bool stones_are_neighbors( const Stone& a, const Stone& b )
{
	if( (a.y == b.y && a.x == b.x - 1 ) ||
		(a.y == b.y && a.x == b.x + 1 ) ||
		(a.x == b.x && a.y == b.y - 1 ) ||
		(a.x == b.x && a.y == b.y + 1 ) )
	{
		return true;
	}

	return false;
}



// Takes in a vector containing the stones of one side
vector<vector<Stone>> go::Goban::form_groups( vector<Stone> stones )
{
	vector<vector<Stone>> groups;
	vector<Stone> current_group;

	while( stones.size() > 0 )
	{
		// Grab one stone 
		current_group.clear();
		current_group.push_back( stones.back() );
		stones.pop_back();

		// Find all the stones which belong to the group
		while( true )
		{
			vector<Stone> new_stones;

			// Try to find neighbors for every stone in the current_group
			for( auto& group_stone : current_group )
			{
				// Loop over all remaining stones
				for( auto it = stones.begin(); it < stones.end(); )
				{
					if( !stones_are_neighbors( group_stone, *it ) )
					{
						it++;
						continue;
					}

					// Stones are neighbors
					new_stones.push_back( *it );
					it = stones.erase( it );
				}
			}

			// Couldn't find any more stones to this group
			if( !new_stones.size() )
			{
				break;
			}

			// Found stones, append them to the current_group
			current_group.insert(
				current_group.end(),
				new_stones.begin(),
				new_stones.end()
			);
		}

		// Save the current_group and continue to the next one
		groups.push_back( current_group );
	}

	return groups;
}



vector<Stone> go::Goban::get_stone_neighbors(
	const vector<Stone>& board,
	const Stone& stone
)
{
	vector<Stone> neighbors;


	size_t index;

	if( stone.x > 0 )
	{
		index = (stone.y) * board_size + stone.x - 1;
		neighbors.push_back( board[index] );
	}

	if( stone.y > 0 )
	{
		index = (stone.y-1) * board_size + stone.x;
		neighbors.push_back( board[index] );
	}

	if( stone.x < board_size - 1 )
	{
		index = (stone.y) * board_size + stone.x + 1;
		neighbors.push_back( board[index] );
	}

	if( stone.y < board_size - 1 )
	{
		index = (stone.y+1) * board_size + stone.x;
		neighbors.push_back( board[index] );
	}

	return neighbors;
}



vector<Stone> go::Goban::find_remaining_liberties(
	const vector<Stone>& board,
	const vector<Stone>& group
)
{
	vector<Stone> liberties;

	for( auto& stone : group )
	{
		auto neighbors = get_stone_neighbors( board, stone );
		for( auto& neighbor : neighbors )
		{
			if( neighbor.side == NONE )
			{
				bool found = false;

				// Check that the liberty is unique
				for( auto& liberty : liberties )
				{
					if( liberty.x == neighbor.x &&
					    liberty.y == neighbor.y )
					{
						found = true;
					}
				}

				// Not found, go ahead and push it back
				if( !found )
				{
					liberties.push_back( neighbor );
				}
			}
		}
	}

	return liberties;
}



vector<vector<Stone>> go::Goban::capture_groups( const vector<Stone>& board, Stone stone )
{
	vector<vector<Stone>> captured_groups;

	if( stone.side == NONE )
	{
		return{};
	}

	// Collect opponent's stones in a vector
	vector<Stone> opponent_stones;
	Side opponent = stone.side == BLACK ? WHITE : BLACK;

	for( auto& stone : board )
	{
		if( stone.side == opponent )
		{
			opponent_stones.push_back( stone );
		}
	}


	// Form groups from the stones
	auto opponent_groups = form_groups( opponent_stones );

	// Find groups with only one liberty left
	for( auto& group : opponent_groups )
	{
		auto liberties = find_remaining_liberties( board, group );
		if( liberties.size() != 1 )
		{
			continue;
		}

		// Group has only one liberty left,
		// check if the stone takes it
		auto liberty = liberties[0];
		if( liberty.x == stone.x &&
			liberty.y == stone.y )
		{
			// Group is captured
			captured_groups.push_back( group );
		}
	}

	return captured_groups;
}



void go::Goban::play_stone( Stone stone )
{
	if( stone.x > board_size || stone.y > board_size ||
	    stone.x <= 0 || stone.y <= 0 )
	{
		throw runtime_error( "Tried to play stone outside of board" );
	}

	stone.x--;
	stone.y--;

	auto captured_groups = capture_groups( board, stone );
	
	// Clear every captured group
	for( auto& captured_group : captured_groups )
	{
		for( auto& captured_stone : captured_group )
		{
			auto stone_index = (captured_stone.y) * board_size
			                 + captured_stone.x;
			board[stone_index] = {
				captured_stone.x,
				captured_stone.y
			};
		}
	}

	auto index = (stone.y) * board_size + stone.x;

	board[index] = stone;
}



const std::vector<Stone>& go::Goban::get_board()
{
	return board;
}



void go::Goban::clear()
{
	board = std::vector<Stone>( (board_size * board_size), {} );
	size_t index = 0;
	for( auto &stone : board )
	{
		stone.x = index % board_size;
		stone.y = index / board_size;
		index++;
	}
}

