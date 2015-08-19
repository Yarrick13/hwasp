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
	else if( name.compare( 0, 5, "link(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		Link l;
		l.vertex1 = tmp;
		l.vertex2 = tmp2;

		links.push_back( l );
	}
}

void
ColouringHeuristic::initEdges(
	)
{
	Vertex *v1;
	Vertex *v2;
	bool found;

	trace_msg( heuristic, 2, "Creating vertex connections" );

	for ( Link l : links)
	{
		found = false;
		for ( unsigned int i = 0; i < vertices.size() && !found; i++ )
		{
			if ( l.vertex1 == vertices[ i ].name )
			{
				v1 = &vertices[ i ];
				found = true;
			}
		}

		found = false;
		for ( unsigned int i = 0; i < vertices.size() && !found; i++ )
		{
			if ( l.vertex2 == vertices[ i ].name )
			{
				v2 = &vertices[ i ];
				found = true;
			}
		}

		if ( std::find( v1->neighbours.begin( ), v1->neighbours.end( ), v2 ) == v1->neighbours.end( ) )
			v1->neighbours.push_back( v2 );

		if ( std::find( v2->neighbours.begin( ), v2->neighbours.end( ), v1 ) == v2->neighbours.end( ) )
			v2->neighbours.push_back( v1 );
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

	// for lcv
	//-------
	trace_msg( heuristic, 2, "Initialize edges" );
	initEdges( );
	//-------

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
	unsigned int* mrv )
{
	bool found = false;
	unsigned int mrv_min = numberOfColours + 1;
	unsigned int mrv_current;

	for ( unsigned int i = 0; i < vd.vertices.size( ); i++ )
	{
		if ( vd.vertices[ i ]->current == 0 || solver.getTruthValue( vd.vertices[ i ]->current ) != TRUE )
		{
			mrv_current = 0;

			for ( ColourAssignment ca : vd.vertices[ i ]->usedIn )
			{
				if ( solver.getTruthValue( ca.variable ) == UNDEFINED )
					mrv_current++;

				if ( solver.getTruthValue( ca.variable ) == TRUE )
				{
					trace_msg( heuristic, 5, "Node " << vd.vertices[ i ]->name << " is already assigned with "
													 << ca.variable << " " << Literal(ca.variable, POSITIVE) );

					addAssignment( vd.vertices[ i ], ca.variable );
					mrv_current = numberOfColours + 1;
					break;
				}
			}

			if ( mrv_current < mrv_min )
			{
				mrv_min = mrv_current;
				*mrv = i;
				found = true;
			}
		}
	}

	if ( found )
		return true;

	return false;
}

// assuming the order of the colours is the same for each vertex in the usedIn vector
bool
ColouringHeuristic::getVertexLCV(
	VertexDegree vd,
	unsigned int mrv,
	unsigned int* lcv )
{
	bool found = false;

	Vertex* v = vd.vertices[ mrv ];
	unsigned int i;

	vector< unsigned int > colourCnt;
	unsigned int min;

	for ( i = 0; i < numberOfColours; i++ )
		colourCnt.push_back( 0 );

	for ( Vertex* n : v->neighbours )
	{
		for ( i = 0; i < numberOfColours; i++ )
		{
			if ( solver.getTruthValue( n->usedIn[ i ].variable ) == UNDEFINED )
				colourCnt[ i ]++;
		}
	}

	min = v->neighbours.size( ) + 1;

	for ( i = 0; i < numberOfColours; i++ )
	{
		if ( colourCnt[ i ] < min && solver.getTruthValue( v->usedIn[ i ].variable ) != FALSE &&
				std::find( v->tried.begin( ), v->tried.end( ), v->usedIn[ i ].variable ) == v->tried.end( ) )
		{
			found = true;
			min = colourCnt[ i ];
			*lcv = i;
		}
	}

	if ( found )
		return true;

	return false;
}

/*
 * make choice for solver
 */
Literal
ColouringHeuristic::makeAChoiceProtected( )
{
	VertexDegree current;
	Vertex* currentVertex;
	Var chosenVariable;
	bool found = false;
	unsigned int mrv;

	do
	{
//		cout << endl << "assignments" << endl;
//		for ( unsigned int i = 0; i < vertices.size( ); i++ )
//		{
//			if ( vertices[ i ].current != 0 )
//				cout << i << ": " << VariableNames::getName( vertices[ i ].current ) << " is " <<
//						solver.getTruthValue( vertices[ i ].current ) << endl;
//		}
//		cout << endl;

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
							trace_msg( heuristic, 4, "Reset index to vertex group with degree " << order[ index ].degree << " due to conflict" );
							index = i;
							indexReseted = true;
						}
					}
				}
			}

			conflictOccured = false;
		}

		found = false;

		do
		{
			current = order[ index ];
			trace_msg( heuristic, 2, "Looking for uncoloured vertex with degree " << to_string( current.degree ) );

			found = getVertexMRV( current, &mrv );
			if ( found )
			{
				trace_msg( heuristic, 3, "Considering vertex " << currentVertex->name );
				currentVertex = order[ index ].vertices[ mrv ];
			}
			else
			{
				trace_msg( heuristic, 3, "No uncolored vertex left with degree " << to_string( current.degree ) << ". Continue with next degree." );
				index++;
			}
		}
		while ( !found );

		found = false;
		chosenVariable = 0;

		// for lcv
		//-------
		if ( getVertexLCV( current, mrv, &choice ) )
		{
			chosenVariable = currentVertex->usedIn[ choice ].variable;
			addAssignment( currentVertex, chosenVariable );
		}
		//-------

		// without lcv
		//-------
//		vector < Var > tried;
//
//		choice = (choice + 1) % numberOfColours;
//		if ( !getTriedAssignments( currentVertex, &tried ) )
//		{
//			chosenVariable = currentVertex->usedIn[ choice ].variable;
//			addAssignment( currentVertex, chosenVariable );
//		}
//
//		if ( chosenVariable == 0 )
//		{
//			for ( unsigned int i = 0; i < numberOfColours && !found; i++ )
//			{
//				unsigned int pos = ( i + choice ) % numberOfColours;
//
//				if ( ( std::find( tried.begin(), tried.end(), currentVertex->usedIn[ pos ].variable ) == tried.end() ) )
//				{
//					chosenVariable = currentVertex->usedIn[ pos ].variable;
//					addAssignment( currentVertex, chosenVariable );
//
//					found = true;
//				}
//			}
//		}
		//-------

		if ( chosenVariable == 0 )
		{
			trace_msg( heuristic, 3, "Chosen variable is zero"  );

			if ( index > 1 )
			{
				trace_msg( heuristic, 3, "No more possibilities to colour this vertex -> go one step back"  );
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
				trace_msg( heuristic, 4, "Chosen variable is already set to FALSE - try another assignment" );
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
