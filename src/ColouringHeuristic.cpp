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
    Solver& s ) : Heuristic( s ), index( 0 ), numberOfColours( 0 ), conflictOccured( false ), choice( 0 )
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
			vertex.current= 0;
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
			vertex.current = 0;

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
	string orderOutput = "";

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

	order.push_back( newVertexDegree( ) );
	order.back( ).degree = vertices[ 0 ].degree;
	order.back( ).vertices.push_back( &vertices[ 0 ] );

	for ( unsigned int i = 1; i < vertices.size( ); i++ )
	{
		if ( vertices[ i ].degree != order.back( ).degree )
		{
			order.push_back( newVertexDegree( ) );
			order.back( ).degree = vertices[ i ].degree;
		}

		order.back( ).vertices.push_back( &vertices[ i ] );
	}

	for ( unsigned int i = 0; i < order.size( ); i++ )
	{
		orderOutput += "with degree " + to_string( order[ i ].degree ) + ": ";

		for ( unsigned int j = 0; j < order[ i ].vertices.size( ); j++ )
			orderOutput += order[ i ].vertices[ j ]->name + ", ";
	}

	trace_msg( heuristic, 3, "Considering order " + orderOutput );
}

bool
ColouringHeuristic::addAssignment(
	Vertex *vertex,
	Var variable )
{
	vertex->current = variable;

	if ( std::find( vertex->tried.begin( ), vertex->tried.end( ), variable ) == vertex->tried.end( ) )
		vertex->tried.push_back( variable );

	return true;
}

bool
ColouringHeuristic::getTriedAssignments(
	Vertex* vertex,
	vector < Var >* tried )
{
	if ( vertex->tried.size( ) > 0 )
	{
		*tried = vertex->tried;
		return true;
	}

	return false;
}

bool
ColouringHeuristic::getVertexMRV(
	VertexDegree vd,
	unsigned int* iv )
{
	bool found = false;
	unsigned int mrv_min = numberOfColours + 1;
	unsigned int indexVertex;
	unsigned int mrv;

	for ( unsigned int i = 0; i < vd.vertices.size( ); i++ )
	{
		if ( vd.vertices[ i ]->current == 0 || solver.getTruthValue( vd.vertices[ i ]->current ) != TRUE )
		{
			mrv = 0;

			for ( ColourAssignment ca : vd.vertices[ i ]->usedIn )
			{
				if ( solver.getTruthValue( ca.variable ) == UNDEFINED )
					mrv++;

				if ( solver.getTruthValue( ca.variable ) == TRUE )
				{
					trace_msg( heuristic, 5, "Node " << vd.vertices[ i ]->name << " is already assigned with "
													 << ca.variable << " " << Literal(ca.variable, POSITIVE) );

					addAssignment( vd.vertices[ i ], ca.variable );
					mrv = numberOfColours + 1;
					break;
				}
			}

			if ( mrv < mrv_min )
			{
				mrv_min = mrv;
				indexVertex = i;
				found = true;
			}
		}
	}

	if ( found )
	{
		*iv = indexVertex;
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
	//Vertex* current;
	VertexDegree current;
	Vertex* currentVertex;
	Var chosenVariable;
	bool found = false;

	do
	{
		cout << endl << "assignments" << endl;
		for ( unsigned int i = 0; i < vertices.size( ); i++ )
		{
			if ( vertices[ i ].current != 0 )
				cout << i << ": " << VariableNames::getName( vertices[ i ].current ) << " is " <<
						solver.getTruthValue( vertices[ i ].current ) << endl;
		}
		cout << endl;


		if ( index >= order.size( ) )
			assert ( 0 && "assert index" );

		if ( conflictOccured )
		{
			bool indexReseted = false;
			unsigned int maxIndex = index;

			for ( unsigned int i = 0; i <= maxIndex; i++ )
			{
				for ( unsigned int j = 0; j < order[ i ].vertices.size( ); j++ )
				{
					if ( order[ i ].vertices[ j ]->current != 0 && solver.getTruthValue( order[ i ].vertices[ j ]->current ) != TRUE )
					{
						order[ i ].vertices[ j ]->current = 0;
						order[ i ].vertices[ j ]->tried.clear( );

						if ( !indexReseted )
						{
							index = i;
							indexReseted = true;

							//cout << "Reset index to vertex group with degree " << order[ index ].degree << " due to conflict" << endl;
							trace_msg( heuristic, 4, "Reset index to vertex group with degree " << order[ index ].degree << " due to conflict" );
						}
					}
				}
			}

			conflictOccured = false;
		}

		found = false;

		do
		{
			unsigned int iv;
			current = order[ index ];
			//cout << "Looking for uncoloured vertex with degree " << to_string( current.degree ) << endl;
			trace_msg( heuristic, 2, "Looking for uncoloured vertex with degree " << to_string( current.degree ) );

			found = getVertexMRV( current, &iv );
			if ( found )
			{
				currentVertex = order[ index ].vertices[ iv ];
				//cout << "Considering vertex " << currentVertex->name << endl;
				trace_msg( heuristic, 3, "Considering vertex " << currentVertex->name );
			}
			else
			{
				//cout << "No uncolored vertex left with degree " << to_string( current.degree ) << ". Continue with next degree." << endl;
				trace_msg( heuristic, 3, "No uncolored vertex left with degree " << to_string( current.degree ) << ". Continue with next degree." );
				index++;
			}
		}
		while ( !found );

		found = false;
		chosenVariable = 0;

		vector < Var > tried;
		choice = (choice + 1) % numberOfColours;//( index - 1 ) % numberOfColours;

		if ( !getTriedAssignments( currentVertex, &tried ) )
		{
			chosenVariable = currentVertex->usedIn[ choice ].variable;
			addAssignment( currentVertex, chosenVariable );
		}

		if ( chosenVariable == 0 )
		{
			for ( unsigned int i = 0; i < numberOfColours && !found; i++ )
			{
				unsigned int pos = ( i + choice ) % numberOfColours;

				if ( ( std::find( tried.begin(), tried.end(), currentVertex->usedIn[ pos ].variable ) == tried.end() ) )
				{
					chosenVariable = currentVertex->usedIn[ pos ].variable;
					addAssignment( currentVertex, chosenVariable );

					found = true;
				}
			}
		}

		if ( chosenVariable == 0 )
		{
			trace_msg( heuristic, 3, "Chosen variable is zero"  );

			cout << endl << "assignments" << endl;
			for ( unsigned int i = 0; i < vertices.size( ); i++ )
			{
				if ( vertices[ i ].current != 0 )
					cout << i << ": " << VariableNames::getName( vertices[ i ].current ) << " is " <<
							solver.getTruthValue( vertices[ i ].current ) << endl;
			}
			cout << endl;

			if ( index > 1 )
			{
				cout << "No more possibilities to colour this vertex -> go one step back" << endl;
				trace_msg( heuristic, 3, "No more possibilities to colour this vertex -> go one step back"  );
				//index -= 2;		// -2 because the index has already been incremented

				//while ( solver.getTruthValue( assignments[ index ].current ) != UNDEFINED )
					//solver.unrollOne( );

				//assignments.pop_back( );

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
			//cout << "Chosen variable is "<< chosenVariable << " " << Literal( chosenVariable, POSITIVE ) << endl;
			trace_msg( heuristic, 3, "Chosen variable is "<< chosenVariable << " " << Literal( chosenVariable, POSITIVE ) );

			if ( solver.getTruthValue( chosenVariable ) == FALSE )
			{
				//cout << "Chosen variable is already set to FALSE - try another assignment" << endl;
				trace_msg( heuristic, 4, "Chosen variable is already set to FALSE - try another assignment" );
			}
			if ( solver.getTruthValue( chosenVariable ) == TRUE )
				trace_msg( heuristic, 4, "Chosen variable is already set to TRUE - continue with next vertex" );
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
