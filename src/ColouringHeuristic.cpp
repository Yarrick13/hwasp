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
    Solver& s ) : Heuristic( s ), index( 0 ), conflictIndex( 0 ), firstChoiceIndex( 0 ), numberOfColours( 0 ), conflictOccured( false ),
	conflictHandled( true ), redoAfterConflict( false ), assignedSinceConflict( 0 )
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

	if( name.compare( 0, 13, "chosenColour(" ) == 0 )
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
			}
		}

		if ( !found )
		{
			Vertex vertex;
			vertex.name = tmp;
			vertex.degree = 0;
			vertex.usedIn.push_back( ca );

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

bool
ColouringHeuristic::searchAndAddAssignment(
	Var variable )
{
	unsigned int current = index - 1;			// because index gets incremented each time a new node is acquired from the order

	if ( current < assignments.size( ) )
	{
		assignments[ current ].current = variable;

		if ( std::find( assignments[ current ].tried.begin( ), assignments[ current ].tried.end( ), variable ) == assignments[ current ].tried.end( ) )
			assignments[ current ].tried.push_back( variable );

		return true;
	}
	else
	{
		Assignment a;

		a.current = variable;
		a.tried.push_back( variable );

		assignments.push_back( a );

		return false;
	}
}

bool
ColouringHeuristic::getTriedAssignments(
	Vertex* node,
	vector < Var >* tried )
{
	unsigned int current = index - 1;			// because index gets incremented each time a new node is acquired from the order

	if ( current < assignments.size( ) )
	{
		*tried = assignments[ current ].tried;
		return true;
	}

	return false;
}

/*
 * make choice for solver
 */
Literal
ColouringHeuristic::makeAChoiceProtected( )
{
	Vertex* current;
	Var chosenVariable;
	Assignment a;
	bool found;

//	cout << endl << "assignments" << endl;
//	for ( unsigned int i = 0; i < assignments.size( ); i++ )
//	{
//		cout << i << ": " << VariableNames::getName( assignments[ i ].current ) << " is " <<
//				solver.getTruthValue( assignments[ i ].current ) << endl;
//	}
//	cout << endl;

	do
	{
		chosenVariable = 0;

		do
		{
			if ( index >= vertices.size( ) )
			{
				assert( 0 && "assert index");
			}

			if ( conflictOccured )
			{
				found = false;
				unsigned int pos = 0;

				while ( pos < assignments.size( ) && !found )
				{
					if ( solver.getTruthValue( assignments[ pos ].current ) != TRUE )
					{
						found = true;
						index = pos;
						trace_msg( heuristic, 4, "Reset index to vertex " << vertices[ pos ].name << " ( index " << pos << " ) due to conflict" );
					}
					else
						pos++;
				}

				while ( index < assignments.size( ) )
					assignments.pop_back( );

				conflictOccured = false;
			}

			found = false;

			current = &vertices[ index++ ];

			for ( unsigned int i = 0; i < current->usedIn.size( ) && !found; i++ )
			{
				ColourAssignment za = current->usedIn[ i ];
				if ( solver.getTruthValue( za.variable ) == TRUE )
				{
					found = true;

					trace_msg( heuristic, 3, "Node " << current->name << " is already assigned with "
													 << za.variable << " " << Literal(za.variable, POSITIVE)
													 << " -> continue with next zone/sensor");

					searchAndAddAssignment( za.variable );
				}
			}
		}
		while( found );

		vector < Var > tried;
		unsigned int choice = ( index - 1 ) % numberOfColours;	// do not start with the same colour at each node

		// if no colours have been tried, get the first in the list ( according to choice )
		if ( !getTriedAssignments( current, &tried ) )
		{
			chosenVariable = current->usedIn[ choice ].variable;
			searchAndAddAssignment( chosenVariable );
		}

		// if there are some tried colours, get the next
		if ( chosenVariable == 0 )
		{
			for ( unsigned int i = 0; i < numberOfColours && !found; i++ )
			{
				unsigned int pos = ( i + choice ) % numberOfColours;

				if ( ( std::find( tried.begin(), tried.end(), current->usedIn[ pos ].variable ) == tried.end() ) )
				{
					chosenVariable = current->usedIn[ pos ].variable;
					searchAndAddAssignment( chosenVariable );

					found = true;
				}
			}
		}

		if ( chosenVariable == 0 )
		{
			trace_msg( heuristic, 3, "Chosen variable is zero"  );

			if ( index > 1 )
			{
				trace_msg( heuristic, 3, "No more possibilities to colour this vertex -> go one step back"  );
				index -= 2;		// -2 because the index has already been incremented

				while ( solver.getTruthValue( assignments[ index ].current ) != UNDEFINED )
					solver.unrollOne( );

				assignments.pop_back( );

				conflictOccured = true;
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
				trace_msg( heuristic, 4, "Chosen variable is already set to TRUE - continue with next zone/sensor" );
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
