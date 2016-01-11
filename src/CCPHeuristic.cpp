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
    Solver& s ) : Heuristic( s ), nrOfColors( 0 ), nrOfBins( 0 ), maxBinSize( 0 ), inputCorrect( true ), index( 0 ), currentColour( 0 ),
	nConflicts( 0 ), nChoices( 0 ), currentV( 0 ), currentBe( 0 )
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

	string vertexOutput = "";
	for ( unsigned int i = 0; i < vertices.size( ); i++ )
		vertexOutput += vertices[ i ].name + ", ";

	trace_msg( heuristic, 2, "Consider vertices: " << vertexOutput );
#endif
}

/*
 * make a choice for the solver
 */
Literal
CCPHeuristic::makeAChoiceProtected(
	)
{
	Literal l = greedyMatching( );
	if ( l != Literal::null )
		return l;
	return greedyCBPC( );
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
			trace_msg( heuristic, 2, "Consider " << currentBe->element );

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
					trace_msg( heuristic, 3, "Chosen variable is 0 - continue with next vertex" );
				}
				else
				{
					trace_msg( heuristic, 3, "Chosen variable is 0 and no more vertics left - fallback to minisat" );
					return Literal::null;
				}
			}
			else
			{
				trace_msg( heuristic, 3, "Chosen variable is " << VariableNames::getName( chosenVariable) << " (" << chosenVariable << ")" );

				if ( solver.getTruthValue( chosenVariable ) == TRUE )
				{
					trace_msg( heuristic, 3, "Chosen variable already set to true - continue" );

				}
				else if ( solver.getTruthValue( chosenVariable ) == FALSE )
				{
					trace_msg( heuristic, 3, "Chosen variable already set to false - continue" );
				}
			}
		}
		else
		{
			trace_msg( heuristic, 3, "No more vertices left - fallback to minisat" );
			return Literal::null;
		}
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
		chosenVariable = 0;
		currentV = 0;
		currentVertexColour = 0;
		currentVertexBin = 0;

		if ( index < vertices.size( ) )
		{
			// if queue empty, get the next vertex of continue with the queue otherwise
			if ( queue.size( ) == 0 )
			{
				trace_msg( heuristic, 2, "Queue empty - get next vertex" );

				if ( index > 0 )
				{
					currentColour++;
					trace_msg( heuristic, 3, "Increase colour value to " << ( currentColour + 1 ) );

					if ( currentColour >= nrOfColors )
					{
						trace_msg( heuristic, 3, "No more colours availabel - fallback to minisat" );
						print( );
						return Literal::null;
					}
				}

				//get next vertex
				do
				{
					trace_msg( heuristic, 3, "Consider vertex " << vertices[ index ].name <<
							( ( vertices[ index ].considered == true ) ? " (already considered)" : "" ) );
					if ( !vertices[ index++ ].considered )
						currentV = &vertices[ index - 1 ];
				} while ( currentV == 0 && index < vertices.size( ) );

				if ( currentV == 0 )
				{
					trace_msg( heuristic, 3, "No more vertices - fallback to minisat" );
					print( );
					return Literal::null;
				}

				queuePushBack( currentV );
			}
			else
			{
				trace_msg( heuristic, 2, "Get next element in queue" );

				currentV = queueGetFirst( );
			}

			// check if the vertex is already coloured or colour it otherwise
			currentVertexColour = getVertexColour( currentV );
			if ( currentVertexColour == 0 )
			{
				trace_msg( heuristic, 3, "Vertex not coloured - use colour " << ( currentColour + 1 ) <<
						" (chosen variable is " << currentV->allColours[ currentColour ]->var << " " <<
						VariableNames::getName( currentV->allColours[ currentColour ]->var ) << ")" );
				chosenVariable = currentV->allColours[ currentColour ]->var;
			}
			else
			{
				if ( ( currentVertexColour - 1 ) == currentColour )
				{
					trace_msg( heuristic, 3, "Vertex already coloured with colour " << ( currentVertexColour ) << " - find bin" );
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
						trace_msg( heuristic, 3, "Place vertex in bin " << ( i + 1 ) << " (chosen variable is " << currentV->allBins[ i ]->var <<
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
					trace_msg( heuristic, 3, "Vertex " << currentV->name << " is not coloured in the current colour - ignore it" );
					queueEraseFirst( );
				}
				else if ( binSearch )
				{
					trace_msg( heuristic, 3, "No bin found for vertex " << currentV->name << " - ignore it" );
					queueEraseFirst( );
					currentV->considered = true;
				}
				else if ( currentVertexBin != 0 )
				{
					trace_msg( heuristic, 3, "Vertex already placed in bin " << ( currentVertexBin ) );
					queueAddNeighbours( currentV );
					queueEraseFirst( );
					currentV->considered = true;
				}
			}
			else if ( solver.getTruthValue( chosenVariable ) == TRUE )
			{
				trace_msg( heuristic, 3, "Chosen variable already set to true - continue" );
				if ( binChosen )
				{
					queueEraseFirst( );
					currentV->considered = true;
				}
			}
			else if ( solver.getTruthValue( chosenVariable ) == FALSE )
			{
				trace_msg( heuristic, 3, "Chosen variable already set to false - continue" );
				queueEraseFirst( );
			}
			else
			{
				if ( binChosen )
				{
					queueAddNeighbours( currentV );
					queueEraseFirst( );
					currentV->considered = true;
				}
			}
		}
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
			trace_msg( heuristic, 1, "[DEBUG]" << VariableNames::getName( ba.var ) << " is " << solver.getTruthValue( ba.var ) );
	}

	for ( BinAssignment ba : binAssignments )
	{
		if ( solver.getTruthValue( ba.var ) == FALSE )
			trace_msg( heuristic, 1, "[DEBUG]" << VariableNames::getName( ba.var ) << " is " << solver.getTruthValue( ba.var ) );
	}

	for ( BinAssignment ba : binAssignments )
	{
		if ( solver.getTruthValue( ba.var ) == UNDEFINED )
			trace_msg( heuristic, 1, "[DEBUG]" << VariableNames::getName( ba.var ) << " is " << solver.getTruthValue( ba.var ) );
	}
}

/*
 * solver found conflict
 */
void
CCPHeuristic::conflictOccurred(
	)
{
	nConflicts++;
	resetHeuristic( );
}

/*
 * add the given vertex to the end of the queue
 * @param vertex	the vertex to add
 */
void
CCPHeuristic::queuePushBack(
	Vertex* vertex )
{
	trace_msg( heuristic, 4, "[queue] push_back: " << vertex->name );
	queue.push_back( vertex );
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
	trace_msg( heuristic, 4, "[queue] get_front: " << front->name );
	return front;
}

/*
 * removes the first element in the queue
 */
void
CCPHeuristic::queueEraseFirst(
	)
{
	trace_msg( heuristic, 4, "[queue] erase_front: " << queue.front( )->name );
	queue.erase( queue.begin( ), queue.begin( ) + 1 );
}

/*
 * adds all the neighbours from the given vertex to the queue
 * @param vertex	the vertex
 */
void
CCPHeuristic::queueAddNeighbours(
	Vertex* vertex )
{
	for ( unsigned int i = 0; i < vertex->predecessor.size( ); i++ )
	{
		if ( !vertex->predecessor[ i ]->considered )
			queuePushBack( vertex->predecessor[ i ] );
	}
	for ( unsigned int i = 0; i < vertex->successors.size( ); i++ )
	{
		if ( !vertex->successors[ i ]->considered )
			queuePushBack( vertex->successors[ i ] );
	}
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
			maxBinSize != 0 && nrOfBins != 0 && nrOfColors != 0 )
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
 * compares two colours based on the colour number
 */
bool
compareColours (
	CCPHeuristic::VertexColour* c1,
	CCPHeuristic::VertexColour* c2 )
{
	return c1->colour < c2->colour;
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

		from->successors.push_back( to );
		to->predecessor.push_back( from );
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

	trace_msg( heuristic, 2, "Get matching related edges" );

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

	resetHeuristic( );

	return true;
}

/*
 * reset the heuristic
 */
void
CCPHeuristic::resetHeuristic(
	)
{
	trace_msg( heuristic, 1, "Conflict occured - reset heuristic" );

	for ( unsigned int i = 0; i < vertices.size( ); i++ )
		vertices[ i ].considered = false;

	index = 0;
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
