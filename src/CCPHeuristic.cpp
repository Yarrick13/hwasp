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

#include "CCPHeuristic.h"

#include <string>
#include <vector>

#include "Solver.h"
#include "util/Assert.h"
#include "util/Constants.h"
#include "util/VariableNames.h"
#include "util/HeuristicUtil.h"

CCPHeuristic::CCPHeuristic(
    Solver& s,
	bool useO,
	bool useA1 ) : Heuristic( s ), nrOfColors( 0 ), nrOfBins( 0 ), maxBinSize( 0 ), inputCorrect( true ), index( 0 ), currentColour( 0 ),
	nConflicts( 0 ), nChoices( 0 ), nIterations( 0 ), fallback( false ), orderingValid( false ), useOrdering( useO ), useA1A2( useA1 ), currentV( 0 ), currentBe( 0 )
{
}

/*
 * Processes all variables related to the CPP *
 * @param v	the variable to process
 */
void
CCPHeuristic::processVariable (
    Var v )
{
	string name = VariableNames::getName( v );

	try
	{
		name.erase(std::remove(name.begin(),name.end(),' '),name.end());

		string tmp;
		string tmp2;
		string tmp3;

		if( name.compare( 0, 7, "vertex(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp );

			Vertex vertex;
			vertex.name = tmp;
			vertex.var = v;
			vertex.inPath = 0;
			vertex.orderingValue = 0;
			vertex.orderingValueMinor = 0;
			vertex.consideredForOrderingValue = false;
			vertex.nPredecessors = 0;
			vertex.nSuccessors = 0;

			vertices.push_back( vertex );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( Vertex )" );
		}
		else if ( name.compare( 0, 5, "edge(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			Edge edge;
			edge.from = tmp;
			edge.to = tmp2;
			edge.var = v;

			edges.push_back( edge );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( Edge )" );
		}
		else if ( name.compare( 0, 5, "size(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			VertexSize size;
			size.vertex = tmp;
			size.size = strtoul( tmp2.c_str(), NULL, 0 );
			size.var = v;

			vertexSizes.push_back( size );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( Size )" );
		}
		else if ( name.compare( 0, 13, "vertex_color(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			VertexColour colour;
			colour.vertex = tmp;
			colour.colour = strtoul( tmp2.c_str(), NULL, 0 );
			colour.var = v;

			vertexColours.push_back( colour );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( Vertex_Color )" );
		}
		else if ( name.compare( 0, 11, "vertex_bin(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			VertexBin bin;
			bin.vertex = tmp;
			bin.bin = strtoul( tmp2.c_str(), NULL, 0 );
			bin.var = v;

			vertexBins.push_back( bin );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( Vertex_Bin )" );
		}
		else if ( name.compare( 0, 23, "edge_matching_selected(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			EdgeMatching em;
			em.area = tmp;
			em.borderelement = tmp2;
			em.var = v;

			edgeMatchings.push_back( em );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( area )" );
		}
		else if ( name.compare( 0, 5, "area(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp );

			Area a;
			a.area = tmp;
			a.var = v;
			a.counter = 0;
			a.total = 0;

			areas.push_back( a );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( borderelement )" );
		}
		else if ( name.compare( 0, 14, "borderelement(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp );

			Borderelement be;
			be.element = tmp;
			be.var = v;

			borderelements.push_back( be );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( borderelement )" );
		}
		else if ( name.compare( 0, 6, "path1(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp );

			path1.push_back( tmp );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( path1 )" );
		}
		else if ( name.compare( 0, 6, "path2(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp );

			path2.push_back( tmp );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( path2 )" );
		}
		else if ( name.compare( 0, 11, "maxbinsize(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp );

			maxBinSize = strtoul( tmp.c_str(), NULL, 0 );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( maxBinSize )" );
		}
		else if ( name.compare( 0, 9, "nrofbins(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp );

			nrOfBins = strtoul( tmp.c_str(), NULL, 0 );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( nrOfBins )" );
		}
		else if ( name.compare( 0, 11, "nrofcolors(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp );

			nrOfColors = strtoul( tmp.c_str(), NULL, 0 );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( nrOfColors )" );
		}
		else if ( name.compare( 0, 4, "bin(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2, &tmp3 );

			BinAssignment ba;
			ba.colour = strtoul( tmp.c_str(), NULL, 0 );
			ba.bin = strtoul( tmp2.c_str(), NULL, 0 );
			ba.vertex = tmp3;
			ba.var = v;

			binAssignments.push_back( ba );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( bin )" );
		}
	}
	catch ( int e )
	{
		trace_msg( heuristic, 3, "Error while parsing " << name );
		inputCorrect = false;
	}
}

/*
 * initializes the heuristic after reading the input
 */
void
CCPHeuristic::onFinishedParsing (
	)
{
	trace_msg( heuristic, 1, "Initializing CCP heuristic" );
	trace_msg( heuristic, 2, "Start processing variables" );

	for ( Var variable : variables )
	{
		if ( !VariableNames::isHidden( variable ) )
			processVariable( variable );
	}

	if ( inputCorrect && checkInput( ) && initData( ) )
		inputCorrect = true;
	else
		inputCorrect = false;

#ifdef TRACE_ON
	if ( inputCorrect )
	{
		trace_msg( heuristic, 1, "Start heuristic" );
	}
	else
	{
		trace_msg( heuristic, 1, "Input not correct!" );
	}
#endif
}

/*
 * make a choice for the solver
 */
Literal
CCPHeuristic::makeAChoiceProtected(
	)
{
	// A1 first, A2 afterwards - start
	if ( useA1A2 )
	{
		if ( !fallback )
		{
			Literal l = greedyMatching( );
			if ( l != Literal::null )
				return l;

			fallback = true;
			resetHeuristic( );
			return greedyCBPC( );
		}

		return greedyCBPC( );
	}
	// A1 first, A2 afterwards - end
	else
	// A2 only - start
	{
		return greedyCBPC( );
	}
	// A2 only - end
}

Literal
CCPHeuristic::greedyMatching(
	)
{
	Var chosenVariable;
	unsigned int min = 0;

	do
	{
		chosenVariable = 0;
		currentBe = 0;

		for ( unsigned int i = 0; i < areas.size( ); i++ )
		{
			areas[ i ].counter = 0;
			areas[ i ].selected = false;
		}

		if ( index < borderelements.size( ) )
		{
			currentBe = &borderelements[ index++ ];
			trace_msg( heuristic, 2, "[M] Consider " << currentBe->element );

			for ( unsigned int i = 0; i < edgeMatchings.size( ); i++ )
			{
				if ( solver.getTruthValue( edgeMatchings[ i ].var ) == TRUE )
					edgeMatchings[ i ].realtedArea->counter++;
			}

			min = areas[ 0 ].counter + 1;

			for ( unsigned int i = 0; i < currentBe->usedIn.size( ); i++ )
			{
				if ( currentBe->usedIn[ i ]->realtedArea->counter < min )
					chosenVariable = currentBe->usedIn[ i ]->var;
			}

			// check chosen variable
			if ( chosenVariable == 0 )
			{
				if ( index < borderelements.size( ) )
				{
					trace_msg( heuristic, 3, "[M] Chosen variable is 0 - continue with next vertex" );
				}
				else
				{
					trace_msg( heuristic, 3, "[M] Chosen variable is 0 and no more vertics left - fallback to minisat" );
					onFinishedSolving( false );
					return Literal::null;
				}
			}
			else
			{
				trace_msg( heuristic, 3, "[M] Chosen variable is " << VariableNames::getName( chosenVariable) << " (" << chosenVariable << ")" );

				if ( solver.getTruthValue( chosenVariable ) == TRUE )
				{
					trace_msg( heuristic, 3, "[M] Chosen variable already set to true - continue" );
				}
				else if ( solver.getTruthValue( chosenVariable ) == FALSE )
				{
					trace_msg( heuristic, 3, "[M] Chosen variable already set to false - continue" );
				}
			}
		}
		else
		{
			trace_msg( heuristic, 3, "[M] No more vertices left - fallback to minisat" );
			onFinishedSolving( false );
			return Literal::null;
		}

		nIterations++;
	}
	while ( chosenVariable == 0 || solver.getTruthValue( chosenVariable ) != UNDEFINED );

	nChoices++;
	return Literal( chosenVariable, POSITIVE );
}

/*
 * Greedy algorithm for colouring, bin packing and connectedness
 */
Literal
CCPHeuristic::greedyCBPC(
	)
{
	Var chosenVariable;

	bool binChosen = false;
	bool binSearch = false;
	unsigned int currentVertexColour;
	unsigned int currentVertexBin;

	do
	{
		if ( !orderingValid )
			return Literal::null;

		chosenVariable = 0;
		currentV = 0;
		currentVertexColour = 0;
		currentVertexBin = 0;

		if ( index < order.size( ) )
		{
			// if queue empty, get the next vertex of continue with the queue otherwise
			if ( queue.size( ) == 0 )
			{
				trace_msg( heuristic, 2, "[CBPC] Queue empty - get next vertex" );

				if ( index > 0 )
				{
					currentColour++;
					trace_msg( heuristic, 3, "[CBPC] Increase colour value to " << ( currentColour + 1 ) );

					if ( currentColour >= nrOfColors )
					{
						trace_msg( heuristic, 3, "[CBPC] No more colours available - fallback to minisat" );
						print( );
						onFinishedSolving( false );
						return Literal::null;
					}
				}

				//get next vertex
				do
				{
					trace_msg( heuristic, 3, "[CBPC] Consider vertex " << order[ index ]->name <<
							( ( order[ index ]->considered == true ) ? " (already considered)" : "" ) );

					if ( !order[ index++ ]->considered )
						currentV = order[ index - 1 ];

				} while ( currentV == 0 && index < order.size( ) );

				if ( currentV == 0 )
				{
					trace_msg( heuristic, 3, "[CBPC] No more vertices - fallback to minisat" );
					print( );
					onFinishedSolving( false );
					return Literal::null;
				}

				queuePushBack( currentV );
			}
			else
			{
				currentV = queueGetFirst( );

				trace_msg( heuristic, 2, "[CBPC] Get next element in queue (" << "Consider vertex " << currentV->name <<
						( ( currentV->considered == true ) ? " (already considered)" : "" ) << ")" );
			}

			// check if the vertex is already coloured or colour it otherwise
			currentVertexColour = getVertexColour( currentV );
			if ( currentVertexColour == 0 )
			{
				trace_msg( heuristic, 3, "[CBPC] Vertex not coloured - use colour " << ( currentColour + 1 ) <<
						" (chosen variable is " << currentV->allColours[ currentColour ]->var << " " <<
						VariableNames::getName( currentV->allColours[ currentColour ]->var ) << ")" );
				chosenVariable = currentV->allColours[ currentColour ]->var;
			}
			else
			{
				if ( ( currentVertexColour - 1 ) == currentColour )
				{
					trace_msg( heuristic, 3, "[CBPC] Vertex already coloured with colour " << ( currentVertexColour ) << " - find bin" );
					currentVertexBin = getVertexBin( currentV );
				}
			}

			// find a bin for the vertex if no bin is assigned to it yet
			if ( chosenVariable == 0 && ( currentVertexColour - 1 ) == currentColour && currentVertexBin == 0 )
			{
				binSearch = true;

				for ( unsigned int i = 0; i < nrOfBins && chosenVariable == 0; i++ )
				{
					if ( getUsedBinSize( i, currentColour ) + currentV->size <= maxBinSize )
					{
						trace_msg( heuristic, 3, "[CBPC] Place vertex in bin " << ( i + 1 ) << " (chosen variable is " << currentV->allBins[ i ]->var <<
								" " << VariableNames::getName( currentV->allBins[ i ]->var ) << ")" );
						chosenVariable = currentV->allBins[ i ]->var;
						binChosen = true;
					}
				}
			}

			// check chosen variable
			if ( chosenVariable == 0 )
			{
				if ( ( currentVertexColour - 1 ) != currentColour )
				{
					trace_msg( heuristic, 3, "[CBPC] Vertex " << currentV->name << " is not coloured in the current colour - ignore it" );
					queueEraseFirst( );
				}
				else if ( binSearch )
				{
					trace_msg( heuristic, 3, "[CBPC] No bin found for vertex " << currentV->name << " - ignore it" );
					queueEraseFirst( );
					currentV->considered = true;
				}
				else if ( currentVertexBin != 0 )
				{
					trace_msg( heuristic, 3, "[CBPC] Vertex already placed in bin " << ( currentVertexBin ) );
					queueEraseFirst( );
					queueAddNeighbours( currentV );
					currentV->considered = true;
				}
			}
			else if ( solver.getTruthValue( chosenVariable ) == TRUE )
			{
				trace_msg( heuristic, 3, "[CBPC] Chosen variable already set to true - continue" );
				if ( binChosen )
				{
					queueEraseFirst( );
					currentV->considered = true;
				}
			}
			else if ( solver.getTruthValue( chosenVariable ) == FALSE )
			{
				trace_msg( heuristic, 3, "[CBPC] Chosen variable already set to false - continue" );
				queueEraseFirst( );
			}
			else if ( solver.getTruthValue( chosenVariable ) == UNDEFINED )
			{
				trace_msg( heuristic, 3, "[CBPC] Chosen variable is undefined - return" );
			}
			else
			{
				if ( binChosen )
				{
					queueEraseFirst( );
					queueAddNeighbours( currentV );
					currentV->considered = true;
				}
			}
		}

		nIterations++;
	}
	while ( chosenVariable == 0 || solver.getTruthValue( chosenVariable ) != UNDEFINED );

	nChoices++;
	return Literal( chosenVariable, POSITIVE );
}

/*
 * debug print
 */
void
CCPHeuristic::print(
	)
{
	trace_msg( heuristic, 1, "[DEBUG] Debug output" );

	for ( BinAssignment ba : binAssignments )
	{
		if ( solver.getTruthValue( ba.var ) == TRUE )
		{
			trace_msg( heuristic, 1, "[DEBUG]" << VariableNames::getName( ba.var ) << " is " << solver.getTruthValue( ba.var ) );
		}
	}

	for ( BinAssignment ba : binAssignments )
	{
		if ( solver.getTruthValue( ba.var ) == FALSE )
		{
			trace_msg( heuristic, 1, "[DEBUG]" << VariableNames::getName( ba.var ) << " is " << solver.getTruthValue( ba.var ) );
		}
	}

	for ( BinAssignment ba : binAssignments )
	{
		if ( solver.getTruthValue( ba.var ) == UNDEFINED )
		{
			trace_msg( heuristic, 1, "[DEBUG]" << VariableNames::getName( ba.var ) << " is " << solver.getTruthValue( ba.var ) );
		}
	}
}

/*
 * solver found conflict
 */
void
CCPHeuristic::conflictOccurred(
	)
{
	trace_msg( heuristic, 1, "Conflict occured" );
	nConflicts++;
	resetHeuristic( );
}

bool
compareColours (
	CCPHeuristic::VertexColour* c1,
	CCPHeuristic::VertexColour* c2 )
{
	return c1->colour < c2->colour;
}

bool compareVertexOrderValueDESC (
	CCPHeuristic::Vertex* v1,
	CCPHeuristic::Vertex* v2 )
{
	return v1->orderingValue > v2->orderingValue;
};

bool compareVertexOrderValueMinorDESC (
	CCPHeuristic::Vertex* v1,
	CCPHeuristic::Vertex* v2 )
{
	return v1->orderingValueMinor > v2->orderingValueMinor;
};

bool compareVertexInPathASC (
	CCPHeuristic::Vertex* v1,
	CCPHeuristic::Vertex* v2 )
{
	return v1->inPath < v2->inPath;
};

bool compareVertexInPathDESC (
	CCPHeuristic::Vertex* v1,
	CCPHeuristic::Vertex* v2 )
{
	return v1->inPath > v2->inPath;
};

/*
 * add the given vertex to the end of the queue
 * @param vertex	the vertex to add
 */
void
CCPHeuristic::queuePushBack(
	Vertex* vertex )
{
	queue.push_back( vertex );

#ifdef TRACE_ON
	string vertexOutput = "";
	for ( unsigned int i = 0; i < queue.size( ); i++ )
		vertexOutput += queue[ i ]->name + ", ";

	trace_msg( heuristic, 4, "[CBPC] [queue] push_back: " << vertex->name
			<< "; current queue is: " << vertexOutput );
#endif
}

/*
 * returns the first vertex in the queue
 * @return	the first vertex in the queue
 */
CCPHeuristic::Vertex*
CCPHeuristic::queueGetFirst(
	)
{
	Vertex* front = queue.front( );

#ifdef TRACE_ON
	string vertexOutput = "";
	for ( unsigned int i = 0; i < queue.size( ); i++ )
		vertexOutput += queue[ i ]->name + ", ";

	trace_msg( heuristic, 4, "[CBPC] [queue] get_front: " << front->name
			<< "; current queue is: " << vertexOutput );
#endif

	return front;
}

/*
 * removes the first element in the queue
 */
void
CCPHeuristic::queueEraseFirst(
	)
{
#ifdef TRACE_ON
	string vertexOutput = "";
	for ( unsigned int i = 0; i < queue.size( ); i++ )
		vertexOutput += queue[ i ]->name + ", ";

	trace_msg( heuristic, 4, "[CBPC] [queue] erase_front: " << queue.front( )->name
			<< "; current queue is: " << vertexOutput );
#endif

	queue.erase( queue.begin( ) );
}

/*
 * adds all the neighbours from the given vertex to the queue
 * @param vertex	the vertex
 */
void
CCPHeuristic::queueAddNeighbours(
	Vertex* vertex )
{
	for ( unsigned int i = 0; i < vertex->neighbours.size( ); i++ )
	{
		if ( !vertex->neighbours[ i ]->considered &&
				( vertex->inPath == 0 || vertex->neighbours[ i ]->inPath == 0 || vertex->inPath == vertex->neighbours[ i ]->inPath ) )
			queuePushBack( vertex->neighbours[ i ] );
	}

	if ( vertex->inPath > 0 )
	{
		std::sort ( queue.begin( ), queue.end(), compareVertexInPathDESC );
	}
	else
	{
		std::sort ( queue.begin( ), queue.end(), compareVertexInPathASC );
	}

#ifdef TRACE_ON
	string vertexOutput = "";
	for ( unsigned int i = 0; i < queue.size( ); i++ )
		vertexOutput += queue[ i ]->name + ", ";

	trace_msg( heuristic, 4, "[CBPC] [queue] current vertex is "
			<< ( vertex->inPath > 0 ? "in a path - prioritization of neighbours in path  " : "not in a path - prioritization of neighbours in path" )
			<< "; current queue is: " << vertexOutput );
#endif
}

/*
 * gets the currently used bin size for the given bin with the given colour
 * @param bin		the bin number
 * @param colour	the colour number
 */
unsigned int
CCPHeuristic::getUsedBinSize(
	unsigned int bin,
	unsigned int colour )
{
	unsigned int used = 0;

	for ( BinAssignment* ba : (binsByColour[ colour ])[ bin ].allVertices )
	{
		if ( solver.getTruthValue( ba->var ) == TRUE )
			used += ba->vertexSize;
	}

	return used;
}

/*
 * @return	true if the input is correct or false otherwise
 */
bool
CCPHeuristic::checkInput(
	)
{
	if ( vertices.size( ) > 0 && edges.size( ) > 0 && vertexSizes.size( ) > 0 && vertexColours.size( ) > 0 && vertexBins.size( ) > 0 &&
			maxBinSize != 0 && nrOfBins != 0 && nrOfColors != 0 && areas.size( ) > 0 && borderelements.size( ) > 0 && edgeMatchings.size( ) > 0 )
		return true;
	return false;
}

/*
 * compares two bins based on the bin number
 */
bool
compareBins (
	CCPHeuristic::VertexBin* b1,
	CCPHeuristic::VertexBin* b2 )
{
	return b1->bin < b2->bin;
}

/*
 * initializes the data for the heuristic
 */
bool
CCPHeuristic::initData(
	)
{
	Vertex *from;
	Vertex *to;
	unsigned int nFound;
	bool found;

	trace_msg( heuristic, 2, "Creating graph..." );

	for ( Edge e : edges )
	{
		nFound = 0;
		for ( unsigned int i = 0; i < vertices.size( ) && nFound < 2; i++ )
		{
			if ( e.from.compare( vertices[ i ].name ) == 0 )
			{
				from = &vertices[ i ];
				nFound++;
			}

			if ( e.to.compare( vertices[ i ].name ) == 0 )
			{
				to = &vertices[ i ];
				nFound++;
			}
		}

		if ( nFound != 2 )
			return false;

		from->neighbours.push_back( to );
		to->neighbours.push_back( from );

		from->nSuccessors++;
		to->nPredecessors++;
	}

	//--------------------------------------------------------------------

	trace_msg( heuristic, 2, "Determine vertex sizes..." );

	for ( VertexSize vs : vertexSizes )
	{
		found = false;
		for ( unsigned int i = 0; i < vertices.size( ) && !found; i++ )
		{
			if ( vs.vertex.compare( vertices[ i ].name ) == 0 )
			{
				from = &vertices[ i ];
				from->size = vs.size;
				found = true;
			}
		}

		if ( !found )
			return false;

		found = false;
		for ( unsigned int i = 0; i < binAssignments.size( ); i++ )
		{
			if ( vs.vertex.compare( binAssignments[ i ].vertex ) == 0 )
			{
				binAssignments[ i ].vertexSize = vs.size;
				found = true;
			}
		}

		if ( !found )
			return false;
	}

	//--------------------------------------------------------------------

	trace_msg( heuristic, 2, "Determine vertex colours..." );

	for ( unsigned int i = 0; i < vertexColours.size( ); i++ )
	{
		found = false;
		for ( unsigned int j = 0; j < vertices.size( ) && !found; j++ )
		{
			if ( vertexColours[ i ].vertex.compare( vertices[ j ].name ) == 0 )
			{
				from = &vertices[ j ];
				from->allColours.push_back( &vertexColours[ i ] );
				found = true;
			}
		}

		if ( !found )
			return false;
	}

	//-------------------------------------------------------------------

	trace_msg( heuristic, 2, "Determine vertex bins..." );

	for ( unsigned int i = 0; i < vertexBins.size( ); i++ )
	{
		found = false;
		for ( unsigned int j = 0; j < vertices.size( ) && !found; j++ )
		{
			if ( vertexBins[ i ].vertex.compare( vertices[ j ].name ) == 0 )
			{
				from = &vertices[ j ];
				from->allBins.push_back( &vertexBins[ i ] );
				found = true;
			}
		}

		if ( !found )
			return false;
	}

	//-------------------------------------------------------------------

	trace_msg( heuristic, 2, "Initialize bins..." );

	for ( unsigned int iCol = 0; iCol < nrOfColors; iCol++ )
	{
		vector< Bin > bins;

		for ( unsigned int i = 0; i < nrOfBins; i++ )
		{
			Bin bin;
			bin.name = i + 1;

			bins.push_back( bin );
		}

		binsByColour.push_back( bins );
	}

	for ( unsigned int i = 0; i < binAssignments.size( ); i++ )
	{
		(binsByColour[ binAssignments[ i ].colour - 1 ])[ binAssignments[ i ].bin - 1 ].allVertices.push_back( &binAssignments[ i ] );
	}

	//-------------------------------------------------------------------

	trace_msg( heuristic, 2, "Sort bins and colours..." );

	for ( unsigned int i = 0; i < vertices.size( ); i++ )
	{
		std::sort ( vertices[ i ].allBins.begin( ), vertices[ i ].allBins.end(), compareBins );
		std::sort ( vertices[ i ].allColours.begin( ), vertices[ i ].allColours.end(), compareColours );
	}

	//-------------------------------------------------------------------

	trace_msg( heuristic, 2, "Get matching related edges..." );

	for ( unsigned int i = 0; i < borderelements.size( ); i++ )
	{
		for ( unsigned int j = 0; j < edgeMatchings.size( ); j++ )
		{
			if ( borderelements[ i ].element.compare( edgeMatchings[ j ].borderelement ) == 0 )
			{
				found = true;
				borderelements[ i ].usedIn.push_back( &edgeMatchings[ j ] );
			}
		}
	}

	for ( unsigned int i = 0; i < areas.size( ); i++ )
	{
		for ( unsigned int j = 0; j < edgeMatchings.size( ); j++ )
		{
			if ( areas[ i ].area.compare( edgeMatchings[ j ].area ) == 0 )
			{
				areas[ i ].total++;
				edgeMatchings[ j ].realtedArea = &areas[ i ];
			}
		}
	}

	//-------------------------------------------------------------------

	// with order - start

	if ( useOrdering )
	{
		trace_msg( heuristic, 2, "Initialize vertex value for ordering..." );

//		for ( unsigned int i = 0; i < path1.size( ); i++ )
//		{
//			found = false;
//			for ( unsigned int j = 0; j < vertices.size( ) && ! found; j++ )
//			{
//				if ( path1[ i ].compare( vertices[ j ].name ) == 0 )
//				{
//					found = true;
//					vertices[ j ].inPath = 1;
//					vertices[ j ].orderingValue++;
//				}
//			}
//		}
//
//		for ( unsigned int i = 0; i < path2.size( ); i++ )
//		{
//			found = false;
//			for ( unsigned int j = 0; j < vertices.size( ) && ! found; j++ )
//			{
//				if ( path2[ i ].compare( vertices[ j ].name ) == 0 )
//				{
//					found = true;
//					vertices[ j ].inPath = 2;
//					vertices[ j ].orderingValue++;
//				}
//			}
//		}

		for ( unsigned int i = 0; i < vertices.size( ); i++ )
		{
			if ( ( vertices[ i ].nPredecessors == 0 ) || ( vertices[ i ].nSuccessors == 0 ) )
			{
				vertices[ i ].orderingValue+=1;
			}

			if ( vertices[ i ].orderingValue > 0 )
				startingVertices.push_back( &vertices[ i ] );
		}

//		for ( unsigned int i = 0; i < vertices.size( ); i++ )
//		{
//			addDegreeNeighbourValue( &vertices[ i ] );
//			resetConsideredForOrderingValue( &vertices[ i ], 4 );
//		}

		//std::random_shuffle ( startingVertices.begin(), startingVertices.end() );
		std::sort ( startingVertices.begin( ), startingVertices.end(), compareVertexOrderValueDESC );

		//-------------------------------------------------------------------

		order.clear( );
		for ( unsigned int i = 0; i < vertices.size( ); i++ )
			order.push_back( &vertices[ i ] );

		orderingValid = createOrder( );
	}
	// with ordering - end
	else
	//without ordering - start
	{
		for ( unsigned int i = 0; i < vertices.size( ); i++ )
			order.push_back( &vertices[ i ] );

		orderingValid = true;

#ifdef TRACE_ON
		string vertexOutput = "";
		for ( unsigned int i = 0; i < order.size( ); i++ )
			vertexOutput += order[ i ]->name + ", ";

		trace_msg( heuristic, 2, "Consider order: " << vertexOutput );
#endif
	}
	// withoug ordering - end

	resetHeuristic( );

	return true;
}

void
CCPHeuristic::addDegreeNeighbourValue(
	Vertex* vertex )
{
	vertex->orderingValueMinor += vertex->neighbours.size( );
	vertex->consideredForOrderingValue = true;

	for ( unsigned int i = 0; i < vertex->neighbours.size( ); i++ )
	{
		addDegreeNeighbourValue( vertex, vertex->neighbours[ i ], 1 );
	}
}

void
CCPHeuristic::addDegreeNeighbourValue(
	Vertex* addTo,
	Vertex* current,
	float value )
{
	value = value * 0.1;

	if ( value >= 0.0001 )
	{
		current->consideredForOrderingValue = true;

		for ( unsigned int i = 0; i < current->neighbours.size( ); i++ )
		{
			if ( current->neighbours[ i ]->consideredForOrderingValue == false )
			{
				addTo->orderingValueMinor += value;
				addDegreeNeighbourValue( addTo, current->neighbours[ i ], value );
			}
		}
	}
}

void
CCPHeuristic::resetConsideredForOrderingValue(
	Vertex* vertex,
	int steps )
{
	vertex->consideredForOrderingValue = false;
	steps--;
	if ( steps >= 0 )
	{
		for ( unsigned int i = 0; i < vertex->neighbours.size( ); i++ )
			resetConsideredForOrderingValue( vertex->neighbours[ i ], steps );
	}
}

/*
 * creates a new ordering
 */
bool
CCPHeuristic::createOrder(
	)
{
	if ( startingVertices.size( ) == 0 )
		return false;

	Vertex* start = startingVertices.front( );
	startingVertices.erase( startingVertices.begin( ) );

	start->orderingValue+=10;
	//std::random_shuffle ( order.begin(), order.end() );
	//std::stable_sort ( order.begin( ), order.end(), compareVertexOrderValueMinorDESC );
	std::sort ( order.begin( ), order.end(), compareVertexOrderValueDESC );
	start->orderingValue-=10;

#ifdef TRACE_ON
	string vertexOutput = "";

	std::ostringstream oss;
	for ( unsigned int i = 0; i < order.size( ); i++ )
	{
		oss.str("");
		oss.clear( );
		oss << order[ i ]->orderingValue;
		vertexOutput += order[ i ]->name + "(" + oss.str() + "), ";
	}

	trace_msg( heuristic, 2, "Consider order: " << vertexOutput );
#endif

	start->orderingValue = 0;

	return true;
}

/*
 * reset the heuristic
 */
void
CCPHeuristic::resetHeuristic(
	 )
{
	trace_msg( heuristic, 1, "Reset heuristic" );

	index = 0;

	for ( unsigned int i = 0; i < vertices.size( ); i++ )
		vertices[ i ].considered = false;

	currentColour = 0;

	queue.clear( );
}

/*
 * returns the colour of the given vertex
 * @vertex 	the vertex
 * @return	a number greater or equal one for the colour of the vertex, or zero if the vertex is not coloured yet
 */
unsigned int
CCPHeuristic::getVertexColour(
	Vertex* vertex )
{
	for ( unsigned int i = 0; i < vertex->allColours.size( ); i++ )
	{
		if ( solver.getTruthValue( vertex->allColours[ i ]->var ) == TRUE )
			return vertex->allColours[ i ]->colour;
	}

	return 0;
}

/*
 * returns the bin of the given vertex
 * @vertex 	the vertex
 * @return	a number greater or equal one for the bin of the vertex, or zero if the vertex is not placed in a bin yet
 */
unsigned int
CCPHeuristic::getVertexBin(
	Vertex* vertex )
{
	for ( unsigned int i = 0; i < vertex->allBins.size( ); i++ )
	{
		if ( solver.getTruthValue( vertex->allBins[ i ]->var ) == TRUE )
			return vertex->allBins[ i ]->bin;
	}

	return 0;
}
