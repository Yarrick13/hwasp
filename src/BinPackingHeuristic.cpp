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
    Solver& s ) : Heuristic( s ), index( 0 ), numberOfBins( 0 ), maxBinSize( 0 ), numberOfConflicts( 0 ), coherent( true ), conflictOccured( false ), inputCorrect( true )
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

	try
	{
		name.erase(std::remove(name.begin(),name.end(),' '),name.end());

		string tmp;
		string tmp2;

		bool found;
		unsigned int i;

		if( name.compare( 0, 9, "nrofbins(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp );

			numberOfBins = atoi( tmp.c_str( ) );

			trace_msg( heuristic, 3, "Processed variable " << variable << " " << name << " ( number of bins )" );
		}
		else if( name.compare( 0, 11, "maxbinsize(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp );

			maxBinSize = atoi( tmp.c_str( ) );

			trace_msg( heuristic, 3, "Processed variable " << variable << " " << name << " ( max binsize )" );
		}
		else if( name.compare( 0, 5, "size(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			// check if entry for this vertex already exists (create otherwise)
			found = false;
			for ( i = 0; i < items.size( ) && !found; i++ )
			{
				if ( items[ i ]->name == tmp )
				{
					found = true;
					items[ i ]->size = atoi( tmp2.c_str( ) );
				}
			}
			if ( !found )
			{
				Item* item = new Item;
				item->name = tmp;
				item->size = atoi( tmp2.c_str( ) );

				items.push_back( item );
			}

			trace_msg( heuristic, 3, "Processed variable " << variable << " " << name << " ( item and size )" );
		}
		else if( name.compare( 0, 11, "vertex_bin(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			Item2Bin* i2b = new Item2Bin;
			i2b->variable = variable;
			i2b->bin = tmp2;
			i2b->item = tmp;

			item2bin.push_back( i2b );

			Bin* bin = new Bin;

			// check if entry for this bin already exists (create otherwise)
			found = false;
			for ( i = 0; i < bins.size( ) && !found; i++ )
			{
				if ( bins[ i ]->name == tmp2 )
				{
					found = true;
					bins[ i ]->usedIn.push_back( i2b );

					bin = bins.back( );
				}
			}
			if ( !found )
			{
				bin->name = tmp2;
				bin->usedIn.push_back( i2b );

				bins.push_back( bin );
			}


			Item* item = new Item;

			// check if entry for this vertex already exists (create otherwise)
			found = false;
			for ( i = 0; i < items.size( ) && !found; i++ )
			{
				if ( items[ i ]->name == tmp )
				{
					found = true;
					items[ i ]->usedIn.push_back( i2b );
					items[ i ]->correspondingBin.push_back( bin );
					items[ i ]->tried.push_back( false );
				}
			}

			if ( !found )
			{
				item->name = tmp;
				item->usedIn.push_back( i2b );
				item->correspondingBin.push_back( bin );
				item->tried.push_back( false );

				items.push_back( item );
			}

			trace_msg( heuristic, 3, "Processed variable " << variable << " " << name << " ( vertex bin )" );
		}
	}
	catch ( int e )
	{
		trace_msg( heuristic, 3, "Errors while processing " << name );
		inputCorrect = false;
	}
}

/*
 * initializes the itemsize for the heursitic
 */
void
BinPackingHeuristic::initItemsize(
	)
{
	for ( Item* item : items )
	{
		for ( unsigned int i = 0; i < item2bin.size( ); i++ )
		{
			if ( item->name == item2bin[ i ]->item )
				item2bin[ i ]->itemsize = item->size;
		}
	}
}

/*
 * TRUE if there is a solution possible for the bin/vertex combination of FALSE otherwise
 */
bool
BinPackingHeuristic::isPackingPossible(
	)
{
	unsigned int size = 0;

	for ( Item* item : items )
	{
		size += item->size;
	}

	if ( size > numberOfBins * maxBinSize )
		return false;

	return true;
}

/*
 * gets the current content size of the given bin
 *
 * @param 	bin		the bin
 */
unsigned int
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

	inputCorrect = checkInput( );

	if ( inputCorrect )
	{
		initItemsize( );

		trace_msg( heuristic, 1, "Start heuristic" );

		if ( !isPackingPossible( ) )
		{
			trace_msg( heuristic, 2, "Not enough space in all bins for all vertices!");
			coherent = false;
		}
		else
		{
			trace_msg( heuristic, 2, "Creating order" );

			quicksort( items, 0, items.size( ) );

			for ( unsigned int i = 0; i < items.size( ); i++ )
				order += items[ i ]->name + ", ";

			trace_msg( heuristic, 3, "Considering order " + order );
		}
	}
}

bool
BinPackingHeuristic::checkInput(
	)
{
	if ( inputCorrect == false )
		return false;

	if ( bins.size( ) == 0 || items.size( ) == 0 || item2bin.size( ) == 0 )
		return false;

	return true;
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

	if ( !coherent )
		return Literal::null;

	// reset index to the first assignment with truth value not TRUE in case of error
	if ( conflictOccured )
	{
		unsigned int conflictIndex = 0;

		for ( conflictIndex = 0; conflictIndex < items.size( ) && !found; conflictIndex++ )
		{
			if ( solver.getTruthValue( items[ conflictIndex ]->usedIn[ items[ conflictIndex ]->current ]->variable ) != TRUE )
			{
				trace_msg( heuristic, 3, "Reset to item " << items[ conflictIndex ]->name << " due to conflict" );
				index = conflictIndex;
				found = true;
			}
		}

		// reset the list of the tried assignments for all following items
		found = true;
		for ( ; conflictIndex < items.size( ) && found; conflictIndex++ )
		{
			found = false;
			for ( unsigned int i = 0; i < items[ conflictIndex ]->tried.size( ); i++ )
			{
				if ( items[ conflictIndex ]->tried[ i ] == true )
				{
					items[ conflictIndex ]->tried[ i ] = false;
					found = true;
				}
			}
		}

		conflictOccured = false;
		numberOfConflicts++;
	}

	do
	{
		chosenVariable = 0;

		if ( index >= items.size( ) )
		{
			trace_msg( heuristic, 4, "Binpacking heuristic found solution but wasp did not recognized it - fall back to Minisat heuristic" );
			return Literal::null;
		}

		do
		{
			current = items[ index++ ];

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
			if ( current->tried[ i ] == false &&
					( getCurrentBinContentSize( current->correspondingBin[ i ] ) + current->size <= maxBinSize ) )
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

				while ( solver.getTruthValue( items[ index ]->usedIn[ items[ index ]->current ]->variable) != UNDEFINED )
					solver.unrollOne( );

				found = true;
				for ( unsigned int conflictIndex = index + 1 ; conflictIndex < items.size( ) && found; conflictIndex++ )
				{
					found = false;
					for ( unsigned int i = 0; i < items[ conflictIndex ]->tried.size( ); i++ )
					{
						if ( items[ conflictIndex ]->tried[ i ] == true )
						{
							items[ conflictIndex ]->tried[ i ] = false;
							found = true;
						}
					}
				}
			}
			else
			{
				trace_msg( heuristic, 3, "Bin packing not possible!" );
				coherent = false;
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
	vector< Item* >& items,
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
	vector< Item* >& items,
	unsigned int p,
	unsigned int q)
{
    Item* item = items[ p ];
    unsigned int i = p;
    unsigned int j;

    for ( j = p + 1; j < q; j++ )
    {
        if ( items[ j ]->size > item->size )
        {
            i = i + 1;
            swap ( items[ i ], items[ j ] );
        }

    }

    swap ( items[ i ], items[ p ] );
    return i;
}

void
BinPackingHeuristic::onFinishedSolving(
	bool )
{
	bool found;
	unsigned int usedBins = 0;

	for ( Bin* bin : bins )
	{
		found = false;
		for ( unsigned int i = 0; i < bin->usedIn.size( ) && !found; i++ )
		{
			if ( solver.getTruthValue( bin->usedIn[ i ]->variable ) == TRUE )
				found = true;
		}

		if ( found )
			usedBins++;
	}

	cout << bins.size( ) << " bins specified; " << usedBins << " bins used in solution" << endl;
}
