/*
 *
 *  Copyright 2013 Mario Alviano, Carmine Dodaro, and Francesco Ricca.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include "BinPackingHeuristic.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include "Solver.h"
#include "util/Assert.h"
#include "util/Constants.h"
#include "util/VariableNames.h"
#include "util/HeuristicUtil.h"

BinPackingHeuristic::BinPackingHeuristic(
    Solver& s ) : Heuristic( s ), index( 0 ), conflictOccured( false )
{
}

/*
 * processes the input variables
 */
void
BinPackingHeuristic::processVariable (
    Var variable )
{
	string name = VariableNames::getName( variable );

	name.erase(std::remove(name.begin(),name.end(),' '),name.end());

	string tmp;
	string tmp2;

	if( name.compare( 0, 8, "binsize(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		Bin bin;
		bin.variable = variable;
		bin.name = atoi( tmp.c_str( ) );
		bin.size = atoi( tmp2.c_str( ) );

		bins.push_back( bin );

		trace_msg( heuristic, 3, "Processed variable " << variable << " " << name << " ( bin )" );
	}
	else if( name.compare( 0, 9, "itemsize(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		Item item;
		item.variable = variable;
		item.name = atoi( tmp.c_str( ) );
		item.size = atoi( tmp2.c_str( ) );

		items.push_back( item );

		trace_msg( heuristic, 3, "Processed variable " << variable << " " << name << " ( item )" );
	}
	else if( name.compare( 0, 9, "item2bin(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		Item2Bin i2b;
		i2b.variable = variable;
		i2b.bin = atoi( tmp2.c_str( ) );
		i2b.item = atoi( tmp.c_str( ) );

		item2bin.push_back( i2b );

		trace_msg( heuristic, 3, "Processed variable " << variable << " " << name << " ( item2bin )" );
	}
}

/*
 * initializes the connections between bins and items for the herusitic
 */
void
BinPackingHeuristic::initUsedIn(
	)
{
	bool found;

	for ( unsigned int i = 0; i < bins.size( ); i++ )
	{
		for ( unsigned int j = 0; j < item2bin.size( ); j++ )
		{
			if ( bins[ i ].name == item2bin[ j ].bin )
				bins[ i ].usedIn.push_back( &item2bin[ j ] );
		}
	}

	for ( unsigned int i = 0; i < items.size( ); i++ )
	{
		for ( unsigned int j = 0; j < item2bin.size( ); j++ )
		{
			if ( items[ i ].name == item2bin[ j ].item )
			{
				item2bin[ j ].itemsize = items[ i ].size;

				items[ i ].usedIn.push_back( &item2bin[ j ] );
				items[ i ].tried.push_back( false );

				found = false;
				for ( unsigned int k = 0; k < bins.size( ) && !found; k++ )
				{
					if ( bins[ k ].name == item2bin[ j ].bin )
					{
						items[ i ].correspondingBin.push_back( &bins[ k ] );
						found = true;
					}
				}
			}
		}
	}
}

/*
 * gets the current content size of the given bin
 *
 * @param 	bin		the bin
 */
int
BinPackingHeuristic::getCurrentBinContentSize(
	Bin* bin )
{
	int current = 0;

	for ( Item2Bin* i2b : bin->usedIn )
	{
		if ( solver.getTruthValue( i2b->variable ) == TRUE )
			current += i2b->itemsize;
	}

	return current;
}

/*
 * initialize heuristic after input parsing
 */
void
BinPackingHeuristic::onFinishedParsing (
	)
{
	string order = "";

	trace_msg( heuristic, 1, "Initializing bin packing heuristic" );
	trace_msg( heuristic, 2, "Start processing variables" );

	for ( Var variable : variables )
	{
		if ( !VariableNames::isHidden( variable ) )
			processVariable( variable );
	}

	trace_msg( heuristic, 2, "Initializing bins and items" );

	initUsedIn( );

	trace_msg( heuristic, 1, "Start heuristic" );
	trace_msg( heuristic, 2, "Creating order" );

	quicksort( items, 0, items.size( ) );

	for ( unsigned int i = 0; i < items.size( ); i++ )
		order += to_string( items[ i ].name ) + ", ";

	trace_msg( heuristic, 3, "Considering order " + order );
}

/*
 * make choice for solver
 */
Literal
BinPackingHeuristic::makeAChoiceProtected(
	)
{
	Var chosenVariable = 0;
	Item* current;
	bool found = false;

	// reset index to the first assignment with truth value not TRUE in case of error
	if ( conflictOccured )
	{
		unsigned int conflictIndex = 0;

		for ( conflictIndex = 0; conflictIndex < items.size( ) && !found; conflictIndex++ )
		{
			if ( solver.getTruthValue( items[ conflictIndex ].usedIn[ items[ conflictIndex ].current ]->variable ) != TRUE )
			{
				trace_msg( heuristic, 3, "Reset to item " << items[ conflictIndex ].name << " due to conflict" );
				index = conflictIndex;
				found = true;
			}
		}

		// reset the list of the tried assignments for all following items
		found = true;
		for ( ; conflictIndex < items.size( ) && found; conflictIndex++ )
		{
			found = false;
			for ( unsigned int i = 0; i < items[ conflictIndex ].tried.size( ); i++ )
			{
				if ( items[ conflictIndex ].tried[ i ] == true )
				{
					items[ conflictIndex ].tried[ i ] = false;
					found = true;
				}
			}
		}

		conflictOccured = false;
	}

	do
	{
		chosenVariable = 0;

		do
		{
			current = &items[ index++ ];

			// check if item has already been assigned
			found = false;
			for ( unsigned int i = 0; i < current->usedIn.size( ) && !found; i++ )
			{
				if ( solver.getTruthValue( current->usedIn[ i ]->variable ) == TRUE )
				{
					found = true;
					current->tried[ i ] = true;
					current->current = i;

					trace_msg( heuristic, 3, "Item " << current->name << " is already assigned to bin "
													 << current->usedIn[ i ]->variable << " " << Literal(current->usedIn[ i ]->variable, POSITIVE)
													 << " -> continue with next item");
				}
			}
		}
		while( found );

		// find possible assignment
		found = false;
		for ( unsigned int i = 0; i < current->usedIn.size() && !found; i++ )
		{
			if ( current->tried[ i ] == false && ( getCurrentBinContentSize( current->correspondingBin[ i ] ) + current->size <= current->correspondingBin[ i ]->size ) )
			{
				current->tried[ i ] = true;
				current->current = i;

				found = true;
				chosenVariable = current->usedIn[ i ]->variable;
			}
		}

		if ( chosenVariable == 0 )
		{
			trace_msg( heuristic, 3, "Chosen variable is zero"  );

			if ( index > 1 )
			{
				trace_msg( heuristic, 3, "No more possibilities to place this item -> go one step back"  );
				index -= 2;		// -2 because the index has already been incremented

				while ( solver.getTruthValue( items[ index ].usedIn[ items[ index ].current ]->variable) != UNDEFINED )
					solver.unrollOne( );

				found = true;
				for ( unsigned int conflictIndex = index + 1 ; conflictIndex < items.size( ) && found; conflictIndex++ )
				{
					found = false;
					for ( unsigned int i = 0; i < items[ conflictIndex ].tried.size( ); i++ )
					{
						if ( items[ conflictIndex ].tried[ i ] == true )
						{
							items[ conflictIndex ].tried[ i ] = false;
							found = true;
						}
					}
				}
			}
			else
			{
				trace_msg( heuristic, 3, "Bin packing not possible!" );
				return Literal::null;
			}
		}
		else
		{
			trace_msg( heuristic, 3, "Chosen variable is "<< chosenVariable << " " << Literal( chosenVariable, POSITIVE ) );

			if ( solver.getTruthValue( chosenVariable ) == FALSE )
			{
				trace_msg( heuristic, 4, "Chosen variable is already set to FALSE - try another assignment" );
				index--;
			}
			if ( solver.getTruthValue( chosenVariable ) == TRUE )
			{
				trace_msg( heuristic, 4, "Chosen variable is already set to TRUE - continue with next node" );
			}
		}
	}
	while( chosenVariable == 0 || !solver.isUndefined( chosenVariable ) );

	return Literal( chosenVariable, POSITIVE );
}

/*
 * Quicksort (DESC) for items according to their size
 *
 * @param 	items	the items
 * @param	p		startindex( 0 )
 * @param 	q		endindex( items.size( ) )
 */
void
BinPackingHeuristic::quicksort(
	vector< Item >& items,
	unsigned int p,
	unsigned int q)
{
	unsigned int r;
    if ( p < q )
    {
        r = partition ( items, p, q );
        quicksort ( items, p, r );
        quicksort ( items, r + 1, q );
    }
}

/*
 * partitioning method for quicksort
 *
 * @param 	items	the items
 * @param	p		startindex
 * @param 	q		endindex
 */
int
BinPackingHeuristic::partition(
	vector< Item >& items,
	unsigned int p,
	unsigned int q)
{
    Item item = items[ p ];
    unsigned int i = p;
    unsigned int j;

    for ( j = p + 1; j < q; j++ )
    {
        if ( items[ j ].size > item.size )
        {
            i = i + 1;
            swap ( items[ i ], items[ j ] );
        }

    }

    swap ( items[ i ], items[ p ] );
    return i;
}
