#include "sgf.hh"

#include <locale>
#include <cctype>
#include <utility>
#include <iostream>
#include <algorithm>
#include <functional>


using namespace sgf;
using namespace std;



auto ltrim( wstring &s )
{
	s.erase(
		s.begin(),
		std::find_if(
			s.begin(),
			s.end(),
			not1( ptr_fun<int, int>( std::isspace ) )
		)
	);

	return s;
}



auto rtrim( wstring &s )
{
	s.erase(
		std::find_if(
			s.rbegin(),
			s.rend(),
			not1( ptr_fun<int, int>( std::isspace ) )
		).base(),
		s.end()
	);

	return s;
}



auto trim( wstring &s )
{
	return ltrim( rtrim( s ) );
}



auto find_closing_bracket(
	const wstring& data,
	const char open_char='(',
	const char close_char=')'
)
{
	size_t current_pos   = 0;
	size_t current_level = 0;

	for( const auto& c : data )
	{
		if( c == open_char )
		{
			current_level++;
		}

		else if( c == close_char )
		{
			if( current_level == 0 )
			{
				throw runtime_error(
					"Syntax error, expected opening bracket before closing"
				);
			}
			
			current_level--;

			if( current_level == 0 )
			{
				return current_pos;
			}
		}

		current_pos++;
	}

	throw runtime_error(
		"Syntax error, unexpected end of content. Couldn't find the closing bracket"
	);
}



auto find_value_closing_bracket( const wstring& data )
{
	size_t current_pos = 0;
	bool escape = false;

	for( auto c : data )
	{
		if( c == '\\' )
		{
			escape = !escape;
		}
		else if( c == ']' && !escape )
		{
			break;
		}
		else
		{
			escape = false;
		}

		current_pos++;
	}

	return current_pos;
}



auto find_end_of_node( const wstring& data )
{
	for( auto it = data.begin(); it != data.end(); )
	{
		auto c = *it;

		if( c == ';' || c == '(' || c == ')' )
		{
			return static_cast<size_t>( it - data.begin() );
		}
		else if( c == '[' )
		{
			auto offset = static_cast<size_t>(it - data.begin());
			auto remaining_data = data.substr( offset );
			auto skip = find_value_closing_bracket( remaining_data ) + 1;
			it += skip;
			continue;
		}

		it++;
	}

	return data.length();
}



Node parse_node_data( wstring data )
{
	Node node;

	data = trim( data );

	if( data.length() && data[0] == ';' )
	{
		data = data.substr( 1 );
	}

	while( data.length() )
	{
		// Fetch property identifier
		size_t identifier_length = 0;
		for( auto c : data )
		{
			if( !std::isupper( c ) )
			{
				break;
			}

			identifier_length++;
		}
		auto identifier = data.substr( 0, identifier_length );

		data = trim( data.substr( identifier_length ) );


		// Fetch property values
		vector<PropertyValue> values;
		while( data.length() && data[0] == '[' )
		{
			// TODO: Fix this. It breaks if there are brackets in the value
			auto value_end = find_value_closing_bracket( data );
			auto value = data.substr( 1, value_end - 1 );
			values.push_back( { trim(value) } );
			data = trim( data.substr( value_end + 1 ) );
		}


		// Save the property
		node.properties[identifier] = values;
	}

	return node;
}



Node parse_data( wstring data )
{
	Node start_node;
	Node *current_node = &start_node;

	while( data.length() )
	{
		data = trim( data );

		if( !data.length() )
		{
			break;
		}

		if( data[0] == '(' )
		{
			auto closing_bracket_index = find_closing_bracket( data );
			auto content = data.substr( 1, closing_bracket_index - 1 );
			auto branch = parse_data( content );

			current_node->children.insert(
				current_node->children.end(),
				branch.children.begin(),
				branch.children.end()
			);

			// Ignore properties, there shouldn't be any
			if( branch.properties.size() != 0 )
			{
				wcout << "There's stuff in branch.properties!" << endl;
			}

			data = data.substr( closing_bracket_index + 1 );
		}

		else if( data[0] == ';' )
		{
			auto node_end = find_end_of_node( data.substr(1) ) + 1;
			if( node_end == wstring::npos )
			{
				node_end = data.size();
			}

			auto node_data = data.substr( 0, node_end );
			current_node->children.push_back( parse_node_data( node_data ) );
			current_node = &current_node->children.back();

			data = data.substr( node_end );
		}
	}

	return start_node;
}



Node sgf::read_game_tree( wstring data )
{
	auto root = parse_data( data );

	if( root.children.size() )
	{
		return root.children[0];
	}

	return root;
}



void print_node( const Node &node, size_t level=0 )
{
	wstring indent = L"";
	for( size_t i = 0; i < level; i++ )
	{
		indent += ' ';
	}

	wcout << indent << "." << endl;

	for( auto& property : node.properties )
	{
		auto identifier = property.first;
		wcout << indent << identifier << " :";

		for( auto& value : property.second )
		{
			if( !identifier.compare( L"B" ) ||
			    !identifier.compare( L"W" ) )
			{
				auto point = property_value_to<Point>( value );
				wcout << " (" << point.x << ", " << point.y << ")";
				continue;
			}

			wcout << " " << value.value;
		}

		wcout << endl;
	}

	for( auto& child : node.children )
	{
		print_node( child, level );
		level++;
	}
}



void sgf::print_game_tree( const Node &root )
{
	print_node( root, 0 );
}



template<>
Point sgf::property_value_to<Point>( const PropertyValue &val )
{
	Point point{ -1, -1 };

	if( val.value.size() != 2 )
	{
		if( val.value.size() == 0 )
		{
			// Pass, most likely
			return point;
		}

		throw std::runtime_error( "Not a valid point property value" );
	}

	wchar_t x = val.value[0];
	wchar_t y = val.value[1];

	if( x >= 'a' && x <= 'z' )
	{
		point.x = x - 'a';
	}
	else if( x >= 'A' && x <= 'Z' )
	{
		point.x = x - 'A' + 27;
	}

	if( y >= 'a' && y <= 'z' )
	{
		point.y = y - 'a';
	}
	else if( y >= 'A' && y <= 'Z' )
	{
		point.y = y - 'A' + 26;
	}

	return point;
}
