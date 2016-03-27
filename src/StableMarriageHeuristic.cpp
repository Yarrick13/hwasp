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

#include "StableMarriageHeuristic.h"

#include <string>
#include <vector>
#include <map>

#include "Solver.h"
#include "util/Assert.h"
#include "util/Constants.h"
#include "util/VariableNames.h"
#include "util/HeuristicUtil.h"

StableMarriageHeuristic::StableMarriageHeuristic(
    Solver& s ) : Heuristic( s ), size( 0 ), inputCorrect( true )
{ }

/*
 * Processes all variables related to the CPP *
 * @param v	the variable to process
 */
void
StableMarriageHeuristic::processVariable (
    Var v )
{
	string name = VariableNames::getName( v );

	try
	{
		name.erase(std::remove(name.begin(),name.end(),' '),name.end());

		string tmp;
		string tmp2;
		string tmp3;
		bool found = false;

		if( name.compare( 0, 16, "manAssignsScore(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2, &tmp3 );

			for ( unsigned int i = 0; i < men.size( ) && !found; i++ )
			{
				if ( men[ i ].name.compare( tmp ) == 0 )
				{
					men[ i ].preferncesInput.insert( std::pair< string, int >( tmp2, strtoul( tmp3.c_str(), NULL, 0 ) ) );
					found = true;
				}
			}

			if ( !found )
			{
				Person p;
				p.var = v;
				p.name = tmp;
				p.id = strtoul( tmp.substr(2).c_str(), NULL, 0 ) - 1;
				p.preferncesInput.insert( std::pair< string, int >( tmp2, strtoul( tmp3.c_str(), NULL, 0 ) ) );

				men.push_back( p );
			}
		}
		else if( name.compare( 0, 18, "womanAssignsScore(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2, &tmp3 );

			for ( unsigned int i = 0; i < women.size( ) && !found; i++ )
			{
				if ( women[ i ].name.compare( tmp ) == 0 )
				{
					women[ i ].preferncesInput.insert( std::pair< string, int >( tmp2, strtoul( tmp3.c_str(), NULL, 0 ) ) );
					found = true;
				}
			}

			if ( !found )
			{
				Person p;
				p.var = v;
				p.name = tmp;
				p.id = strtoul( tmp.substr(2).c_str(), NULL, 0 ) - 1;
				p.preferncesInput.insert( std::pair< string, int >( tmp2, strtoul( tmp3.c_str(), NULL, 0 ) ) );

				women.push_back( p );
			}
		}
		else if( name.compare( 0, 6, "match(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			Match m;
			m.var = v;
			m.manId = strtoul( tmp.substr(2).c_str(), NULL, 0 ) - 1;;
			m.womanId = strtoul( tmp2.substr(2).c_str(), NULL, 0 ) - 1;;

			matchesInput.push_back( m );
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
StableMarriageHeuristic::onFinishedParsing (
	)
{
	trace_msg( heuristic, 1, "Initializing stable marriage heuristic" );
	trace_msg( heuristic, 1, "Start processing variables" );

	for ( Var variable : variables )
	{
		if ( !VariableNames::isHidden( variable ) )
			processVariable( variable );
	}

	for ( unsigned int i = 0; i < men.size( ); i++ )
	{
		cout << i << " : " << men[ i ].var << ", " << men[ i ].name << ", " << men[ i ].id << endl;

		for ( auto it = men[ i ].preferncesInput.begin( ); it != men[ i ].preferncesInput.end( ); ++it )
			cout << "\t" << it->first << " -> " << it->second << endl;
	}

	for ( unsigned int i = 0; i < women.size( ); i++ )
	{
		cout << i << " : " << women[ i ].var << ", " << women[ i ].name << ", " << women[ i ].id << endl;

		for ( auto it = women[ i ].preferncesInput.begin( ); it != women[ i ].preferncesInput.end( ); ++it )
			cout << "\t" << it->first << " -> " << it->second << endl;
	}

	for ( unsigned int i = 0; i < matchesInput.size( ); i++ )
		cout << "Match " << i << ": " << matchesInput[ i ].manId << " to " << matchesInput[ i ].womanId << endl;

	initData( );

	if ( inputCorrect && checkInput( ) )
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

bool
compareValuesASC(
	pair< StableMarriageHeuristic::Person*, int > p1,
	pair< StableMarriageHeuristic::Person*, int > p2 )
{
	return ( p1.second < p2.second );
};

bool
compareValuesDESC(
	pair< StableMarriageHeuristic::Person*, int > p1,
	pair< StableMarriageHeuristic::Person*, int > p2 )
{
	return ( p1.second > p2.second );
};

bool
compareMatchesASC(
	StableMarriageHeuristic::Match m1,
	StableMarriageHeuristic::Match m2 )
{
	if ( m1.manId < m2.manId && m1.womanId < m2.womanId )
		return true;
	return false;
};

void
StableMarriageHeuristic::initData(
	)
{
	bool found = false;

	size = men.size( );

	//-------------------------------------------------------

	trace_msg( heuristic, 2, "Initialize preference lists (ID/Pref) and matching references..." );
	for ( unsigned int i = 0; i < size; i++ )
	{
		for ( unsigned int j = 0; j < size; j++ )
		{
			men[ i ].prefsByID.push_back( pair< Person*, int > ( &women[ j ], women[ j ].id ) );
			men[ i ].prefsByPref.push_back( pair< Person*, int > ( &women[ j ], men[ i ].preferncesInput.find( women[ j ].name )->second ) );

			women[ i ].prefsByID.push_back( pair< Person*, int > ( &men[ j ], women[ i ].id ) );
			women[ i ].prefsByPref.push_back( pair< Person*, int > ( &men[ j ], women[ i ].preferncesInput.find( men[ j ].name )->second ) );

			found = false;
			for ( unsigned int k = 0; k < matchesInput.size( ) && !found; k++ )
			{
				if ( matchesInput[ k ].manId == men[ i ].id && matchesInput[ k ].womanId == women[ j ].id )
				{
					matchesInput[ k ].man = &men[ i ];
					matchesInput[ k ].woman = &women[ j ];
					found = true;
				}
			}
		}

		sort( men[ i ].prefsByID.begin( ), men[ i ].prefsByID.end( ), compareValuesASC );
		sort( men[ i ].prefsByPref.begin( ), men[ i ].prefsByPref.end( ), compareValuesDESC );
		sort( women[ i ].prefsByID.begin( ), women[ i ].prefsByID.end( ), compareValuesASC );
		sort( women[ i ].prefsByPref.begin( ), women[ i ].prefsByPref.end( ), compareValuesDESC );
	}

	sort( matchesInput.begin( ), matchesInput.end( ), compareMatchesASC );

	//-------------------------------------------------------

	trace_msg( heuristic, 2, "Initialize matching vectors..." );
	for ( unsigned int i = 0; i < size; i++ )
	{
		vector< Match* > m;

		for ( unsigned int j = 0; j < size; j++ )
			m.push_back( &matchesInput[ i * 4 + j ] );

		matches.push_back( m );
	}

	//-------------------------------------------------------
}

/*
 * make a choice for the solver
 */
Literal
StableMarriageHeuristic::makeAChoiceProtected(
	)
{
	createFullAssignment( );
//	// A1 first, A2 afterwards - start
//	if ( useA1A2 )
//	{
//		if ( !fallback )
//		{
//			Literal l = greedyMatching( );
//			if ( l != Literal::null )
//				return l;
//
//			fallback = true;
//			resetHeuristic( );
//			return greedyCBPC( );
//		}
//
//		return greedyCBPC( );
//	}
//	// A1 first, A2 afterwards - end
//	else
//	// A2 only - start
//	{
//		return greedyCBPC( );
//	}
//	// A2 only - end
	exit( 0 );
	return Literal::null;
}

void
StableMarriageHeuristic::createFullAssignment(
	)
{
	trace_msg( heuristic, 2, "Creating full assignment..." );
	matchesUsedInLS.clear( );

	trace_msg( heuristic, 3, "Analyze partial assignment..." );
	vector< int > unmachtedMen;
	vector< int > unmachtedWomen;

	for ( unsigned int i = 0; i < size; i++ )
	{
		unmachtedMen.push_back( i );
		unmachtedWomen.push_back( i );
	}

	for ( unsigned int i = 0; i < matchesInput.size( ); i++ )
	{
		if ( solver.getTruthValue( matchesInput[ i ].var ) == TRUE )
		{
			trace_msg( heuristic, 4, VariableNames::getName( matchesInput[ i ].var ) << " is set by solver - lock it" );
			matchesInput[ i ].lockedBySolver = true;
			matchesInput[ i ].usedInLS = true;

			unmachtedMen.erase( std::remove( unmachtedMen.begin( ), unmachtedMen.end( ), matchesInput[ i ].manId ), unmachtedMen.end( ) );
			unmachtedWomen.erase( std::remove( unmachtedWomen.begin( ), unmachtedWomen.end( ), matchesInput[ i ].womanId ), unmachtedWomen.end( ) );

			matchesUsedInLS.push_back( &matchesInput[ i ] );
		}
		else
		{
			trace_msg( heuristic, 4, VariableNames::getName( matchesInput[ i ].var ) << " is not set by solver" );
			matchesInput[ i ].lockedBySolver = false;
			matchesInput[ i ].usedInLS = false;
		}
	}

#ifdef TRACE_ON
	string outM = "";
	string outW = "";

	for ( unsigned int i = 0; i < unmachtedMen.size( ); i++ )
		outM += to_string( unmachtedMen[ i ] ) + ", ";

	for ( unsigned int i = 0; i < unmachtedWomen.size( ); i++ )
		outW += to_string( unmachtedWomen[ i ] ) + ", ";

	trace_msg( heuristic, 3, "unmachted men (id): " + outM );
	trace_msg( heuristic, 3, "unmachted women (id): " + outW );
#endif

	trace_msg( heuristic, 3, "Extending partial assignment..." );
	int rm, rw;
	while ( unmachtedMen.size( ) > 0 )
	{
		rm = rand() % unmachtedMen.size( );
		rw = rand() % unmachtedWomen.size( );

		matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ]->usedInLS = true;
		matchesUsedInLS.push_back( matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ] );

		trace_msg( heuristic, 4, "Match man (id) " + to_string( unmachtedMen[ rm ] ) + " and women (id) " + to_string( unmachtedWomen[ rw ] ) );

		unmachtedMen.erase( unmachtedMen.begin( ) + rm );
		unmachtedWomen.erase( unmachtedWomen.begin( ) + rw );
	}

#ifdef TRACE_ON
	string out = "";
	for ( unsigned int i = 0; i < matchesUsedInLS.size( ); i++ )
		out += VariableNames::getName( matchesUsedInLS[ i ]->var ) + ", ";

	trace_msg( heuristic, 3, "Created assignment: " + out );
#endif
}

///*
// * Greedy algorithm for colouring, bin packing and connectedness
// */
//Literal
//CCPHeuristic::greedyCBPC(
//	)
//{
//	Var chosenVariable;
//
//	bool binChosen = false;
//	bool binSearch = false;
//	unsigned int currentVertexColour;
//	unsigned int currentVertexBin;
//
//	do
//	{
//		if ( !orderingValid )
//			return Literal::null;
//
//		chosenVariable = 0;
//		currentV = 0;
//		currentVertexColour = 0;
//		currentVertexBin = 0;
//
//		if ( index < order.size( ) )
//		{
//			// if queue empty, get the next vertex of continue with the queue otherwise
//			if ( queue.size( ) == 0 )
//			{
//				trace_msg( heuristic, 2, "[CBPC] Queue empty - get next vertex" );
//
//				if ( index > 0 )
//				{
//					currentColour++;
//					trace_msg( heuristic, 3, "[CBPC] Increase colour value to " << ( currentColour + 1 ) );
//
//					if ( currentColour >= nrOfColors )
//					{
//						trace_msg( heuristic, 3, "[CBPC] No more colours available - fallback to minisat" );
//						print( );
//						onFinishedSolving( false );
//						return Literal::null;
//					}
//				}
//
//				//get next vertex
//				do
//				{
//					trace_msg( heuristic, 3, "[CBPC] Consider vertex " << order[ index ]->name <<
//							( ( order[ index ]->considered == true ) ? " (already considered)" : "" ) );
//
//					if ( !order[ index++ ]->considered )
//						currentV = order[ index - 1 ];
//
//				} while ( currentV == 0 && index < order.size( ) );
//
//				if ( currentV == 0 )
//				{
//					trace_msg( heuristic, 3, "[CBPC] No more vertices - fallback to minisat" );
//					print( );
//					onFinishedSolving( false );
//					return Literal::null;
//				}
//
//				queuePushBack( currentV );
//			}
//			else
//			{
//				currentV = queueGetFirst( );
//
//				trace_msg( heuristic, 2, "[CBPC] Get next element in queue (" << "Consider vertex " << currentV->name <<
//						( ( currentV->considered == true ) ? " (already considered)" : "" ) << ")" );
//			}
//
//			// check if the vertex is already coloured or colour it otherwise
//			currentVertexColour = getVertexColour( currentV );
//			if ( currentVertexColour == 0 )
//			{
//				trace_msg( heuristic, 3, "[CBPC] Vertex not coloured - use colour " << ( currentColour + 1 ) <<
//						" (chosen variable is " << currentV->allColours[ currentColour ]->var << " " <<
//						VariableNames::getName( currentV->allColours[ currentColour ]->var ) << ")" );
//				chosenVariable = currentV->allColours[ currentColour ]->var;
//			}
//			else
//			{
//				if ( ( currentVertexColour - 1 ) == currentColour )
//				{
//					trace_msg( heuristic, 3, "[CBPC] Vertex already coloured with colour " << ( currentVertexColour ) << " - find bin" );
//					currentVertexBin = getVertexBin( currentV );
//				}
//			}
//
//			// find a bin for the vertex if no bin is assigned to it yet
//			if ( chosenVariable == 0 && ( currentVertexColour - 1 ) == currentColour && currentVertexBin == 0 )
//			{
//				binSearch = true;
//
//				for ( unsigned int i = 0; i < nrOfBins && chosenVariable == 0; i++ )
//				{
//					if ( getUsedBinSize( i, currentColour ) + currentV->size <= maxBinSize )
//					{
//						trace_msg( heuristic, 3, "[CBPC] Place vertex in bin " << ( i + 1 ) << " (chosen variable is " << currentV->allBins[ i ]->var <<
//								" " << VariableNames::getName( currentV->allBins[ i ]->var ) << ")" );
//						chosenVariable = currentV->allBins[ i ]->var;
//						binChosen = true;
//					}
//				}
//			}
//
//			// check chosen variable
//			if ( chosenVariable == 0 )
//			{
//				if ( ( currentVertexColour - 1 ) != currentColour )
//				{
//					trace_msg( heuristic, 3, "[CBPC] Vertex " << currentV->name << " is not coloured in the current colour - ignore it" );
//					queueEraseFirst( );
//				}
//				else if ( binSearch )
//				{
//					trace_msg( heuristic, 3, "[CBPC] No bin found for vertex " << currentV->name << " - ignore it" );
//					queueEraseFirst( );
//					currentV->considered = true;
//				}
//				else if ( currentVertexBin != 0 )
//				{
//					trace_msg( heuristic, 3, "[CBPC] Vertex already placed in bin " << ( currentVertexBin ) );
//					queueEraseFirst( );
//					queueAddNeighbours( currentV );
//					currentV->considered = true;
//				}
//			}
//			else if ( solver.getTruthValue( chosenVariable ) == TRUE )
//			{
//				trace_msg( heuristic, 3, "[CBPC] Chosen variable already set to true - continue" );
//				if ( binChosen )
//				{
//					queueEraseFirst( );
//					currentV->considered = true;
//				}
//			}
//			else if ( solver.getTruthValue( chosenVariable ) == FALSE )
//			{
//				trace_msg( heuristic, 3, "[CBPC] Chosen variable already set to false - continue" );
//				queueEraseFirst( );
//			}
//			else if ( solver.getTruthValue( chosenVariable ) == UNDEFINED )
//			{
//				trace_msg( heuristic, 3, "[CBPC] Chosen variable is undefined - return" );
//			}
//			else
//			{
//				if ( binChosen )
//				{
//					queueEraseFirst( );
//					queueAddNeighbours( currentV );
//					currentV->considered = true;
//				}
//			}
//		}
//
//		nIterations++;
//	}
//	while ( chosenVariable == 0 || solver.getTruthValue( chosenVariable ) != UNDEFINED );
//
//	nChoices++;
//	return Literal( chosenVariable, POSITIVE );
//}

/*
 * solver found conflict
 */
void
StableMarriageHeuristic::conflictOccurred(
	)
{
//	trace_msg( heuristic, 1, "Conflict occured" );
//	nConflicts++;
//	resetHeuristic( );
}

/*
 * @return	true if the input is correct or false otherwise
 */
bool
StableMarriageHeuristic::checkInput(
	)
{
	if ( men.size( ) > 0 && women.size( ) > 0 )
		return true;
	return false;
}

/*
 * reset the heuristic
 */
void
StableMarriageHeuristic::resetHeuristic(
	 )
{
//	trace_msg( heuristic, 1, "Reset heuristic" );
//
//	index = 0;
//
//	for ( unsigned int i = 0; i < vertices.size( ); i++ )
//		vertices[ i ].considered = false;
//
//	currentColour = 0;
//
//	queue.clear( );
}
