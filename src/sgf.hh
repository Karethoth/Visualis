#pragma once
#include <map>
#include <vector>
#include <sstream>

/*
	http://www.red-bean.com/sgf/

	"..." : terminal symbols
	[...] : option: occurs at most once
	{...} : repetition: any number of times, including zero
	(...) : grouping
	  |   : exculsive or

	Collection = GameTree { GameTree }
	GameTree   = "(" Sequence { GameTree } ")"
	Sequence   = Node { Node }
	Node       = ";" { Property }
	Property   = PropIdent PropValue { PropValue }
	PropIdent  = UcLetter { UcLetter }
	PropValue  = "[" CValueType "]"
	CValueType = (ValueType | Compose)
	ValueType  = (None | Number | Real | Double | Color | SimpleText | Text | Point | Move | Stone)


	Two types of property lists: "list of" and "elist of":
	"list of":  PropValue { PropValue }
	"elist of": ((PropValue { PropValue }) | None)


	Properties

	GM: Game type [1] for go
		full list: http://www.red-bean.com/sgf/properties.html#GM

	FF: File format version [4] to allow usage of UcLetters as coordinates
	CA: Character-set [UTF-8]
	AP: Name and Version number of application used to create game tree

	ST: How variations should be shown
		[0] - show variations of successor node
		[1] - show variations of current node
		[2] - no board markup

	RU: Ruleset used
	    ["AGA" | "GOE" | "Japanese" | "Chinese" | "NZ"]

	SZ: Board size [19]
	KM: Komi [7.50]
	TM: Main time in seconds [1800]
	OT: Over time [5x60 byo-yomi]
	GN: Game name
	PW: Player white
	PB: Player black
	WR: White rank
	BR: Black rank
	DT: Date [YYYY-MM-DD]
	EV: Event [2nd MLily Cup]
	RO: Round  [Quarter Finals]
	PC: Place [Guangzhou, China]
	SO: Source [http-link]
	RE: Result [W+Resign] | [B+R] | [W+15.50]
	LB: Write given text to board [qd:1]
	C:  Comment
 */


namespace sgf
{
	using namespace std;

	struct Point
	{
		int x;
		int y;
	};

	struct PropertyValue
	{
		wstring value;
	};

	struct Node
	{
		vector<Node> children;
		map<wstring, vector<PropertyValue>> properties;
	};

	Node read_game_tree( wstring data );
	void print_game_tree( const Node &root );


	template<typename T>
	T property_value_to( const PropertyValue &val )
	{
		std::stringstream ss( val.value );
		T ret;
		ss >> ret;
		return ret;
	}

	template<>
	Point property_value_to<Point>( const PropertyValue &val );
}

