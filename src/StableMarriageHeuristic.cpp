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
    Solver& s,
	float randomWalkProbability,
	unsigned int maxSteps ) : Heuristic( s ), randWalkProb( randomWalkProbability ), steps( 0 ), maxSteps( maxSteps ), size( 0 ), inputCorrect( true )
{
	srand(time(NULL));
}

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
compareMatchesDESC(
	pair< StableMarriageHeuristic::Match*, int > p1,
	pair< StableMarriageHeuristic::Match*, int > p2 )
{
	return ( p1.second < p2.second );
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
	vector< Match* > blockingPairsCurrent;
	Match* bestBlockingPath = 0;
	bool blockingPairsRemaining = true;

	createFullAssignment( );

	do
	{
		if ( ( ((float)rand()/(float)(RAND_MAX)) * 1 ) < randWalkProb )
		{
			trace_msg( heuristic, 2, "Random step..." );

			blockingPairsRemaining = getBlockingPairs( &blockingPairsCurrent );

			// ToDo: choose one random and swap
		}
		else
		{
			trace_msg( heuristic, 2, "Heuristic step..." );

			blockingPairsRemaining = getBestMatchingFromNeighbourhood( &bestBlockingPath );

			// ToDo: swap best
		}
	} while ( blockingPairsRemaining && steps < maxSteps );

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
		if ( solver.getTruthValue( matchesInput[ i ].var ) != UNDEFINED )
		{
			trace_msg( heuristic, 4, VariableNames::getName( matchesInput[ i ].var ) << " is set by solver - lock it" );

			if ( solver.getTruthValue( matchesInput[ i ].var ) == TRUE )
			{
				matchesInput[ i ].lockedBySolver = true;
				matchesInput[ i ].usedInLS = true;

				trace_msg( heuristic, 5, "Set " << matchesInput[ i ].woman->name << " as current partner for " << matchesInput[ i ].man->name );
				trace_msg( heuristic, 5, "Set " << matchesInput[ i ].man->name << " as current partner for " << matchesInput[ i ].woman->name );
				matchesInput[ i ].man->currentPartner = matchesInput[ i ].woman;
				matchesInput[ i ].woman->currentPartner = matchesInput[ i ].man;

				unmachtedMen.erase( std::remove( unmachtedMen.begin( ), unmachtedMen.end( ), matchesInput[ i ].manId ), unmachtedMen.end( ) );
				unmachtedWomen.erase( std::remove( unmachtedWomen.begin( ), unmachtedWomen.end( ), matchesInput[ i ].womanId ), unmachtedWomen.end( ) );

				matchesUsedInLS.push_back( &matchesInput[ i ] );
			}
			else
			{
				matchesInput[ i ].lockedBySolver = true;
				matchesInput[ i ].usedInLS = false;
			}
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

		trace_msg( heuristic, 5, "Set " << matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ]->woman->name << " as current partner for "
				                        << matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ]->man->name );
		trace_msg( heuristic, 5, "Set " << matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ]->man->name << " as current partner for "
				                        << matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ]->woman->name );
		matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ]->man->currentPartner = matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ]->woman;
		matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ]->woman->currentPartner = matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ]->man;

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

bool
StableMarriageHeuristic::getBlockingPairs(
	vector< Match* >* blockingPairs )
{
	trace_msg( heuristic, 3, "Looking for blocking pairs..." );
	blockingPairs->clear( );

	Person* currentPartnerM;
	Person* currentPartnerW;
	Person* possiblePartnerM;
	Person* possiblePartnerW;

	for ( unsigned int i = 0; i < matchesInput.size( ); i++ )
	{
		trace_msg( heuristic, 4, "Check pair " << VariableNames::getName( matchesInput[ i ].var ) );

		if ( !matchesInput[ i ].usedInLS && !matchesInput[ i ].lockedBySolver )
		{
			possiblePartnerM = matchesInput[ i ].man;
			possiblePartnerW = matchesInput[ i ].woman;
			currentPartnerM = possiblePartnerW->currentPartner;
			currentPartnerW = possiblePartnerM->currentPartner;

			trace_msg( heuristic, 5, possiblePartnerM->name
					                 << " preference - current partner: " << possiblePartnerM->preferncesInput.find( currentPartnerW->name )->second
									 << " preference - possible partner: " << possiblePartnerM->preferncesInput.find( possiblePartnerW->name )->second );
			trace_msg( heuristic, 5, possiblePartnerW->name
									 << " preference - current partner: " << possiblePartnerW->preferncesInput.find( currentPartnerM->name )->second
									 << " preference - possible partner: " << possiblePartnerW->preferncesInput.find( possiblePartnerM->name )->second );

			if( ( possiblePartnerM->preferncesInput.find( possiblePartnerW->name )->second >
							possiblePartnerM->preferncesInput.find( currentPartnerW->name )->second ) &&
				( possiblePartnerW->preferncesInput.find( possiblePartnerM->name )->second >
							possiblePartnerW->preferncesInput.find( currentPartnerM->name )->second ) )
			{
				if ( !matches[ possiblePartnerM->id ][ currentPartnerW->id ]->lockedBySolver &&
					 !matches[ currentPartnerM->id ][ possiblePartnerW->id ]->lockedBySolver )
				{
					trace_msg( heuristic, 5, "Blocking pair found" );
					blockingPairs->push_back( &matchesInput[ i ] );
				}
				else
				{
					trace_msg( heuristic, 5, "Blocking pair found but current matches are locked" );
				}
			}
		}
		else
		{
			trace_msg( heuristic, 5, "Blocking pair found is already used or locked" );
		}
	}

#ifdef TRACE_ON
	string out = "";
	for ( unsigned int i = 0; i < blockingPairs->size( ); i++ )
		out += VariableNames::getName( blockingPairs->at( i )->var ) + ", ";

	trace_msg( heuristic, 3, "All blocking pairs: " << out );
#endif

	if ( blockingPairs->size( ) == 0 )
		return false;
	return true;
}

bool
StableMarriageHeuristic::getBestMatchingFromNeighbourhood(
	Match** bestBlockingPath )
{
	vector< Match* > blockingPairsCurrent;
    vector< Match* > blockingPairsSimulated;
    vector< pair< Match*, int> > neighbourhood;

	trace_msg( heuristic, 2, "Get blocking pairs for current assignment..." );

	if ( !getBlockingPairs( &blockingPairsCurrent ) )
	{
		trace_msg( heuristic, 2, "No blocking pairs - stable marriage found" );
		return false;
	}

	Match* addedMatching1;
	Match* addedMatching2;
	Match* removedMatching1;
	Match* removedMatching2;

	for ( unsigned int i = 0; i < blockingPairsCurrent.size( ); i++ )
	{
		trace_msg( heuristic, 2, "Simulate removing blocking pair " << VariableNames::getName( blockingPairsCurrent[ i ]->var ) << "..." );

		addedMatching1 = blockingPairsCurrent[ i ];
		removedMatching1 = matches[ addedMatching1->man->id ][ addedMatching1->man->currentPartner->id ];
		removedMatching2 = matches[ addedMatching1->woman->currentPartner->id ][ addedMatching1->woman->id ];
		addedMatching2 = matches[ removedMatching2->man->id ][ removedMatching1->woman->id ];

		trace_msg( heuristic, 3, "Add " << VariableNames::getName( addedMatching1->var )
		                                << " and " << VariableNames::getName( addedMatching2->var )
		                                << ", remove " << VariableNames::getName( removedMatching1->var )
									    << " and " << VariableNames::getName( removedMatching2->var ) );

		addedMatching1->usedInLS = true;
		addedMatching2->usedInLS = true;
		removedMatching1->usedInLS = false;
		removedMatching2->usedInLS = false;

		trace_msg( heuristic, 4, "Set " << addedMatching1->man->name << " and " << addedMatching1->woman->name << " as partner" );
		addedMatching1->man->currentPartner = addedMatching1->woman;
		addedMatching1->woman->currentPartner = addedMatching1->man;

		trace_msg( heuristic, 4, "Set " << removedMatching2->man->name << " and " << removedMatching1->woman->name << " as partner" );
		removedMatching2->man->currentPartner = removedMatching1->woman;
		removedMatching1->woman->currentPartner = removedMatching2->man;


		getBlockingPairs( &blockingPairsSimulated );

		trace_msg( heuristic, 3, "Reset matching changes" );

		addedMatching1->usedInLS = false;
		addedMatching2->usedInLS = false;
		removedMatching1->usedInLS = true;
		removedMatching2->usedInLS = true;

		trace_msg( heuristic, 4, "Set " << addedMatching1->man->name << " and " << removedMatching1->woman->name << " as partner" );
		addedMatching1->man->currentPartner = removedMatching1->woman;
		removedMatching1->woman->currentPartner = addedMatching1->man;

		trace_msg( heuristic, 4, "Set " << removedMatching2->man->name << " and " << addedMatching1->woman->name << " as partner" );
		removedMatching2->man->currentPartner = addedMatching1->woman;
		addedMatching1->woman->currentPartner = removedMatching2->man;

		trace_msg( heuristic, 3, "Removing " << VariableNames::getName( blockingPairsCurrent[ i ]->var ) << " leads to " << blockingPairsSimulated.size( )
				                             << " blocking pairs" );

		neighbourhood.push_back( pair< Match*, int >( blockingPairsCurrent[ i ], blockingPairsSimulated.size( ) ) );
	}

	sort( neighbourhood.begin( ), neighbourhood.end( ), compareMatchesDESC );
	*bestBlockingPath = neighbourhood[ 0 ].first;
#ifdef TRACE_ON
	string out = "";
	for ( unsigned int i = 0; i < neighbourhood.size( ); i++ )
		out += VariableNames::getName( neighbourhood[ i ].first->var ) + " -> " + to_string( neighbourhood[ i ].second ) + ", ";

	trace_msg( heuristic, 2, "Removed blocking path and resulting blocking paths: " << out );
	trace_msg( heuristic, 2, "Best blocking path to remove: " << VariableNames::getName( (*bestBlockingPath)->var ) );
#endif
	return true;
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
