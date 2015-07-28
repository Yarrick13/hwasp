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

#include "ColouringHeuristic.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <time.h>

#include "Solver.h"
#include "util/Assert.h"
#include "util/Constants.h"
#include "util/VariableNames.h"
#include "util/HeuristicUtil.h"

ColouringHeuristic::ColouringHeuristic(
    Solver& s ) : Heuristic( s ), index( 0 ), firstChoiceIndex( 0 ), numberOfColours( 0 ), conflictOccured( false )
{
}

/*
 * processes the input variables
 */
void
ColouringHeuristic::processVariable (
    Var v )
{
	string name = VariableNames::getName( v );

	name.erase(std::remove(name.begin(),name.end(),' '),name.end());

	string tmp;
	string tmp2;

	bool found;
	unsigned int i;

	if( name.compare( 0, 13, "vertex_color(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		ColourAssignment ca;
		ca.variable = v;
		ca.node = tmp;
		ca.colour = tmp2;

		colourAssignments.push_back( ca );

		found = false;
		for ( i = 0; i < vertices.size( ) && !found; i++ )
		{
			if ( vertices[ i ].name == tmp )
			{
				found = true;
				vertices[ i ].usedIn.push_back( ca );
				vertices[ i ].tried.push_back( false );
			}
		}

		if ( !found )
		{
			Vertex vertex;
			vertex.name = tmp;
			vertex.degree = 0;
			vertex.usedIn.push_back( ca );
			vertex.tried.push_back( false );

			vertices.push_back( vertex );
		}

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( colour assignment and vertex )" );
	}
	else if( name.compare( 0, 7, "degree(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		found = false;
		for ( i = 0; i < vertices.size( ) && !found; i++ )
		{
			if ( vertices[ i ].name == tmp )
			{
				found = true;
				vertices[ i ].degree = atoi( tmp2.c_str( ) );
			}
		}

		if ( !found )
		{
			Vertex vertex;
			vertex.name = tmp;
			vertex.degree = 0;
			vertex.degree = atoi( tmp2.c_str( ) );

			vertices.push_back( vertex );
		}

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( degree and vertex )" );
	}
	else if( name.compare( 0, 11, "nrofcolors(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp );

		numberOfColours = atoi( tmp.c_str( ) );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( number of colours )" );
	}
}

/*
 * initialize heuristic after input parsing
 */
void
ColouringHeuristic::onFinishedParsing (
	)
{
	string order = "";

	trace_msg( heuristic, 1, "Initializing colouring heuristic" );
	trace_msg( heuristic, 2, "Start processing variables" );

	for ( Var variable : variables )
	{
		if ( !VariableNames::isHidden( variable ) )
			processVariable( variable );
	}

	trace_msg( heuristic, 1, "Start heuristic" );
	trace_msg( heuristic, 2, "Creating order" );

	quicksort( vertices, 0, vertices.size( ) );

	for ( unsigned int i = 0; i < vertices.size( ); i++ )
		order += vertices[ i ].name + ", ";

	trace_msg( heuristic, 3, "Considering order " + order );
}

/*
 * make choice for solver
 */
Literal
ColouringHeuristic::makeAChoiceProtected( )
{
	Var chosenVariable = 0;
	Vertex* current;
	bool found = false;

	// handle conflict case
	if ( conflictOccured )
	{
		unsigned int conflictIndex = 0;

		// reset to first assignment with truth value not TRUE
		for ( conflictIndex = 0; conflictIndex < vertices.size( ) && !found; conflictIndex++ )
		{
			if ( solver.getTruthValue( vertices[ conflictIndex ].usedIn[ vertices[ conflictIndex ].current ].variable ) != TRUE )
			{
				trace_msg( heuristic, 3, "Reset to Vertex " << vertices[ conflictIndex ].name << " due to conflict" );
				index = conflictIndex;
				found = true;
			}
		}

		// clear tried assignment for all following
		found = true;
		for ( ; conflictIndex < vertices.size( ) && found; conflictIndex++ )
		{
			found = false;
			for ( unsigned int i = 0; i < vertices[ conflictIndex ].tried.size( ); i++ )
			{
				if ( vertices[ conflictIndex ].tried[ i ] == true )
				{
					vertices[ conflictIndex ].tried[ i ] = false;
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
			current = &vertices[ index++ ];

			// check if item has already been assigned
			found = false;
			for ( unsigned int i = 0; i < current->usedIn.size( ) && !found; i++ )
			{
				if ( solver.getTruthValue( current->usedIn[ i ].variable ) == TRUE )
				{
					found = true;
					current->tried[ i ] = true;
					current->current = i;

					trace_msg( heuristic, 3, "Vertex " << current->name << " is already assigned to with "
													 << current->usedIn[ i ].variable << " " << Literal(current->usedIn[ i ].variable, POSITIVE)
													 << " -> continue with next vertex");
				}
			}
		}
		while( found );

		// loop over all colours for the first colour assignment of each node
		unsigned int choice = 0;
		if ( index == ( firstChoiceIndex + 1 ) )
		{
			choice = firstChoiceIndex % numberOfColours;
			firstChoiceIndex++;
		}

		// find possible assingment
		found = false;
		for ( unsigned int i = choice; i < current->usedIn.size() && !found; i++ )
		{
			if ( current->tried[ i ] == false )
			{
				current->tried[ i ] = true;
				current->current = i;

				found = true;
				chosenVariable = current->usedIn[ i ].variable;
			}
		}

		if ( chosenVariable == 0 )
		{
			trace_msg( heuristic, 3, "Chosen variable is zero"  );

			if ( index > 1 )
			{
				trace_msg( heuristic, 3, "No more possibilities to place this node -> go one step back"  );
				index -= 2;		// -2 because the index has already been incremented

				while ( solver.getTruthValue( vertices[ index ].usedIn[ vertices[ index ].current ].variable) != UNDEFINED )
					solver.unrollOne( );

				found = true;
				for ( unsigned int conflictIndex = index + 1 ; conflictIndex < vertices.size( ) && found; conflictIndex++ )
				{
					found = false;
					for ( unsigned int i = 0; i < vertices[ conflictIndex ].tried.size( ); i++ )
					{
						if ( vertices[ conflictIndex ].tried[ i ] == true )
						{
							vertices[ conflictIndex ].tried[ i ] = false;
							found = true;
						}
					}
				}
			}
			else
			{
				trace_msg( heuristic, 3, "Graph can not be coloured" );
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
ColouringHeuristic::quicksort(
	vector< Vertex >& vertices,
	unsigned int p,
	unsigned int q)
{
	unsigned int r;
    if ( p < q )
    {
        r = partition ( vertices, p, q );
        quicksort ( vertices, p, r );
        quicksort ( vertices, r + 1, q );
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
ColouringHeuristic::partition(
	vector< Vertex >& vertices,
	unsigned int p,
	unsigned int q)
{
    Vertex vertex = vertices[ p ];
    unsigned int i = p;
    unsigned int j;

    for ( j = p + 1; j < q; j++ )
    {
        if ( vertices[ j ].degree > vertex.degree )
        {
            i = i + 1;
            swap ( vertices[ i ], vertices[ j ] );
        }

    }

    swap ( vertices[ i ], vertices[ p ] );
    return i;
}
