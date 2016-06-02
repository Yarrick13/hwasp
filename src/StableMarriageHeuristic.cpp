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
#include <list>
#include <limits.h>
#include <math.h>

#include "Solver.h"
#include "util/Assert.h"
#include "util/Constants.h"
#include "util/VariableNames.h"
#include "util/HeuristicUtil.h"

StableMarriageHeuristic::StableMarriageHeuristic(
    Solver& s,
	float randomWalkProbability,
	unsigned int maxSteps,
	unsigned int timeoutDefault,
	unsigned int samplingTimeoutDefault,
	bool useSimulatedAnnealing ) : Heuristic( s ), augmentedPathFound( false ), randWalkProb( randomWalkProbability ), steps( 0 ), maxSteps( maxSteps ), stepCount( 0 ), heuCount( 0 ),
							 timeout( timeoutDefault ), samplingTimeout( samplingTimeoutDefault ),
	                         size( 0 ), inputCorrect( true ), noMoveCount( 0 ), index( 0 ), runLocalSearch( true ), sendToSolver( false ), marriageFound( false ), startingGenderMale( true ),
							 simAnnealing( useSimulatedAnnealing ), fallbackCount( 0 ), callMinisatCount( 0 ), nmCount( 0 ), gs_finished( false )
{
	minisat = new MinisatHeuristic( s );
	srand(time(NULL));
	start = std::chrono::system_clock::now();
	heuristic_time = start-start;

	temperature = 100;
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
				p.id = strtoul( tmp.substr(1).c_str(), NULL, 0 ) - 1;
				p.matched = false;
				p.lastConsidered = -1;
				p.male = true;
				p.considered = false;
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
				p.id = strtoul( tmp.substr(1).c_str(), NULL, 0 ) - 1;
				p.matched = false;
				p.lastConsidered = -1;
				p.male = false;
				p.considered = false;
				p.preferncesInput.insert( std::pair< string, int >( tmp2, strtoul( tmp3.c_str(), NULL, 0 ) ) );

				women.push_back( p );
			}
		}
		else if( name.compare( 0, 6, "match(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			Match m;
			m.var = v;
			m.manId = strtoul( tmp.substr(1).c_str(), NULL, 0 ) - 1;
			m.womanId = strtoul( tmp2.substr(1).c_str(), NULL, 0 ) - 1;

			m.inEC = false;
			m.inEPrime = true;
			m.inMatching = false;
			m.level = -1;

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
	minisat->onFinishedParsing( );

	trace_msg( heuristic, 1, "Initializing stable marriage heuristic" );
	trace_msg( heuristic, 1, "Start processing variables" );

	for ( Var variable : variables )
	{
		if ( !VariableNames::isHidden( variable ) )
			processVariable( variable );
	}

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

/*
 * compares two matches based in their IDs for man an woman
 */
bool
compareMatchesASC(
	StableMarriageHeuristic::Match m1,
	StableMarriageHeuristic::Match m2 )
{
	if ( m1.manId < m2.manId )
		return true;
	if ( m1.manId == m2.manId && m1.womanId < m2.womanId )
		return true;
	return false;
};

bool
comparePairDESC(
	pair< StableMarriageHeuristic::Person*, int> p1,
	pair< StableMarriageHeuristic::Person*, int> p2 )
{
	return p1.second > p2.second;
}

bool
comparePairASC(
	pair< StableMarriageHeuristic::Person*, int> p1,
	pair< StableMarriageHeuristic::Person*, int> p2 )
{
	return p1.second < p2.second;
}

void
StableMarriageHeuristic::initData(
	)
{
	bool found = false;
	start_init = std::chrono::system_clock::now();

	size = men.size( );

	//-------------------------------------------------------

	trace_msg( heuristic, 2, "Initialize matching references..." );
	for ( unsigned int i = 0; i < size; i++ )
	{
		for ( unsigned int j = 0; j < size; j++ )
		{
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
	}

	sort( matchesInput.begin( ), matchesInput.end( ), compareMatchesASC );

	//-------------------------------------------------------

	trace_msg( heuristic, 2, "Initialize matching vectors..." );
	for ( unsigned int i = 0; i < size; i++ )
	{
		vector< Match* > m;

		for ( unsigned int j = 0; j < size; j++ )
		{
			m.push_back( &matchesInput[ i * size + j ] );
			matchesPosition.push_back( i * size + j );
		}

		matches.push_back( m );
	}

	//-------------------------------------------------------
	// gale-shapley

	if ( maxSteps == 0 )
	{
		for ( unsigned int i = 0; i < size; i++ )
		{
			for ( unsigned int j = 0; j < size; j++ )
			{
				men[ i ].gs_preference.push_back( pair< Person*, int >( &women[ j ], men[ i ].preferncesInput.find( women[ j ].name )->second ) );
				women[ i ].gs_preference.push_back( pair< Person*, int >( &men[ j ], women[ i ].preferncesInput.find( men[ j ].name )->second ) );
			}

			sort( men[ i ].gs_preference.begin( ), men[ i ].gs_preference.end( ), comparePairDESC );
			sort( women[ i ].gs_preference.begin( ), women[ i ].gs_preference.end( ), comparePairDESC );
		}
	}

	//-------------------------------------------------------
	// strong stable marriage

	for ( unsigned int i = 0; i < size; i++ )
	{
		for ( unsigned int j = 0; j < size; j++ )
		{
			men[ i ].strong_preferences.push_back( pair< Person*, int >( &women[ j ], men[ i ].preferncesInput.find( women[ j ].name )->second ) );
			women[ i ].strong_preferences.push_back( pair< Person*, int >( &men[ j ], women[ i ].preferncesInput.find( men[ j ].name )->second ) );
		}

		sort( men[ i ].strong_preferences.begin( ), men[ i ].strong_preferences.end( ), comparePairASC );
		sort( women[ i ].strong_preferences.begin( ), women[ i ].strong_preferences.end( ), comparePairDESC );
	}

	end_init = std::chrono::system_clock::now();
}

/*
 * make a choice for the solver
 */
Literal
StableMarriageHeuristic::makeAChoiceProtected(
	)
{
	start_heuristic = std::chrono::system_clock::now();

	if ( maxSteps != 0 )
	{
		if ( !runLocalSearch && !sendToSolver )
		{
			end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end-start;

			if ( elapsed_seconds.count( ) > timeout )
			{
				trace_msg( heuristic, 1, "Run local search..." );
				runLocalSearch = true;
				steps = 0;
			}
		}

		if ( runLocalSearch )
		{
			marriageFound = false;

			Match* chosenBlockingPath = 0;
			bool blockingPathsRemaining = true;

			createFullAssignment( );

			do
			{
				trace_msg( heuristic, 2, "Starting step " << steps << "..." );

				if ( !simAnnealing )
				{
					if ( ( ((float)rand()/(float)(RAND_MAX)) * 1 ) < randWalkProb )
					{
						trace_msg( heuristic, 2, "Random step..." );

						blockingPathsRemaining = getRandomBlockingPath( &chosenBlockingPath );
					}
					else
					{
						trace_msg( heuristic, 2, "Heuristic step..." );

						blockingPathsRemaining = getBestPathFromNeighbourhood( &chosenBlockingPath );
					}

					if ( blockingPathsRemaining )
					{
						removeBlockingPath( chosenBlockingPath );
					}
					else
					{
						marriageFound = true;
						trace_msg( heuristic, 2, "No more blocking paths found..." );
					}
				}
				else
				{
					trace_msg( heuristic, 2, "Heuristic step (simulated annealing; temp: " << temperature << ")..." );

					if ( simulatedAnnealingStep( &chosenBlockingPath, true, true ) )
						removeBlockingPath( chosenBlockingPath );

					temperature -= 1;
					if ( temperature <= 1 || noMoveCount > 5 )
						steps = maxSteps + 1;

					if ( noMoveCount > 5 )
						nmCount++;
				}

				steps++;
				stepCount++;

	#ifdef TRACE_ON
				string out = "";
				for ( unsigned int i = 0; i < matchesInput.size( ); i++ )
				{
					if ( matchesInput[ i ].usedInLS )
						out += VariableNames::getName( matchesInput[ i ].var ) + ", ";
				}
				trace_msg( heuristic, 2, "Current assignment: " << out );

				trace_msg( heuristic, 5, "Partner (men)" );
				for ( unsigned int i = 0; i < men.size( ); i++ )
					trace_msg( heuristic, 5, men[ i ].name << " is partner of " << men[ i ].currentPartner->name );

				trace_msg( heuristic, 5, "Partner (women)" );
				for ( unsigned int i = 0; i < women.size( ); i++ )
					trace_msg( heuristic, 5, women[ i ].name << " is partner of " << women[ i ].currentPartner->name );
	#endif

			} while ( blockingPathsRemaining && steps <= maxSteps );

			heuCount++;

			trace_msg( heuristic, 2, "Prepare assignment to send to solver..." );
			matchesInMarriage.clear( );
			for ( unsigned int i = 0; i < matchesInput.size( ); i++ )
			{
				if ( matchesInput[ i ].usedInLS )
					matchesInMarriage.push_back( &matchesInput[ i ] );
			}

			index = 0;
			runLocalSearch = false;
			sendToSolver = true;
			fallbackCount++;
			start = std::chrono::system_clock::now();
			trace_msg( heuristic, 1, "Fallback..." );
		}
	}
	else
	{
		if ( !sendToSolver & !gs_finished )
		{
			std::chrono::time_point<std::chrono::system_clock> gs_start, gs_end;
			gs_start = std::chrono::system_clock::now();

			//galeShapley( );

			//---------------------------
			strongStableMarriage( );
			matchesInMarriage.clear( );
			for ( unsigned int i = 0; i < matchesInput.size( ); i++ )
			{
				if ( matchesInput[ i ].inMatching )
					matchesInMarriage.push_back( &matchesInput[ i ] );
			}
			//---------------------------

			heuCount++;
			sendToSolver = true;
			gs_finished = true;
			index = 0;

			gs_end = std::chrono::system_clock::now();
			std::chrono::duration<double> gs_time = gs_end-gs_start;
			trace_msg( heuristic, 3, "[GS] Time: " << gs_time.count( ) << " seconds" );
		}
	}

	if ( sendToSolver )
	{
		//for ( unsigned int i = 0; i < matchesInMarriage.size( ); i++ )
		while ( index < matchesInMarriage.size( ) )
		{
			if ( solver.getTruthValue( matchesInMarriage[ index ]->var ) == UNDEFINED )
			{
				trace_msg( heuristic, 3, "Send " << VariableNames::getName( matchesInMarriage[ index ]->var ) << " as " << (index+1) << "th" );
				return Literal( matchesInMarriage[ index++ ]->var, POSITIVE );
			}
			index++;
		}
		sendToSolver = false;
	}

	end_heuristic = std::chrono::system_clock::now();
	heuristic_time += end_heuristic-start_heuristic;

	callMinisatCount++;

	exit(0);
	return minisat->makeAChoice( );
}

void
StableMarriageHeuristic::createFullAssignment(
	)
{
	trace_msg( heuristic, 2, "Creating full assignment..." );

#ifdef TRACE_ON
    vector< Match* > matchesUsedInLS;
#endif

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

#ifdef TRACE_ON
				matchesUsedInLS.push_back( &matchesInput[ i ] );
#endif
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

//	fixed matching for sample
//#ifdef TRACE_ON
//	matches[ 2 ][ 2 ]->usedInLS = true;
//	matchesUsedInLS.push_back( matches[ 2 ][ 2 ] );
//	matches[ 2 ][ 2 ]->man->currentPartner = matches[ 2 ][ 2 ]->woman;
//	matches[ 2 ][ 2 ]->woman->currentPartner = matches[ 2 ][ 2 ]->man;
//
//	matches[ 0 ][ 1 ]->usedInLS = true;
//	matchesUsedInLS.push_back( matches[ 0 ][ 1 ] );
//	matches[ 0 ][ 1 ]->man->currentPartner = matches[ 0 ][ 1 ]->woman;
//	matches[ 0 ][ 1 ]->woman->currentPartner = matches[ 0 ][ 1 ]->man;
//
//	matches[ 1 ][ 3 ]->usedInLS = true;
//	matchesUsedInLS.push_back( matches[ 1 ][ 3 ] );
//	matches[ 1 ][ 3 ]->man->currentPartner = matches[ 1 ][ 3 ]->woman;
//	matches[ 1 ][ 3 ]->woman->currentPartner = matches[ 1 ][ 3 ]->man;
//
//	matches[ 3 ][ 0 ]->usedInLS = true;
//	matchesUsedInLS.push_back( matches[ 3 ][ 0 ] );
//	matches[ 3 ][ 0 ]->man->currentPartner = matches[ 3 ][ 0 ]->woman;
//	matches[ 3 ][ 0 ]->woman->currentPartner = matches[ 3 ][ 0 ]->man;
//#endif

	int rm, rw;
	while ( unmachtedMen.size( ) > 0 )
	{
		rm = rand() % unmachtedMen.size( );
		rw = rand() % unmachtedWomen.size( );

		matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ]->usedInLS = true;
#ifdef TRACE_ON
		matchesUsedInLS.push_back( matches[ unmachtedMen[ rm ] ][ unmachtedWomen[ rw ] ] );
#endif

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

unsigned int
StableMarriageHeuristic::getBlockingPathsSampling(
	vector< Match* > bpToCheck,
	vector< Match* >* blockingPaths )
{
	trace_msg( heuristic, 3, "==========sim start==============" );
	vector< Match* > blockingPathsUndominated;

	Person* currentPartnerM;
	Person* currentPartnerW;
	Person* possiblePartnerM;
	Person* possiblePartnerW;

	unsigned int currentManId;
	unsigned int currentWomanId;

	for ( unsigned int i = 0; i < bpToCheck.size( ); i++ )
	{
		trace_msg( heuristic, 3, "Sample for man " << bpToCheck[ i ]->man->name << " from " << VariableNames::getName( bpToCheck[ i ]->var ) );
		currentManId = bpToCheck[ i ]->man->id;

		for ( unsigned int i = 0; i < size; i++ )
		{
			trace_msg( heuristic, 4, "Check path " << VariableNames::getName( matches[ currentManId ][ i ]->var ) );

			if ( !matches[ currentManId ][ i ]->usedInLS && !matches[ currentManId ][ i ]->lockedBySolver )
			{
				possiblePartnerM = matches[ currentManId ][ i ]->man;
				possiblePartnerW = matches[ currentManId ][ i ]->woman;
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
						trace_msg( heuristic, 5, "Blocking path found" );
						blockingPaths->push_back( matches[ currentManId ][ i ] );
					}
					else
					{
						trace_msg( heuristic, 5, "Blocking path found but locked" );
					}
				}
			}
			else
			{
				trace_msg( heuristic, 5, "Blocking path found is already used or locked" );
			}
		}

		trace_msg( heuristic, 3, "Sample for woman " << bpToCheck[ i ]->woman->name << " from " << VariableNames::getName( bpToCheck[ i ]->var ) );
		currentWomanId = bpToCheck[ i ]->woman->id;

		for ( unsigned int i = 0; i < size; i++ )
		{
			trace_msg( heuristic, 4, "Check path " << VariableNames::getName( matches[ i ][ currentWomanId ]->var ) );

			if ( !matches[ i ][ currentWomanId ]->usedInLS && !matches[ i ][ currentWomanId ]->lockedBySolver )
			{
				possiblePartnerM = matches[ i ][ currentWomanId ]->man;
				possiblePartnerW = matches[ i ][ currentWomanId ]->woman;
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
						trace_msg( heuristic, 5, "Blocking path found" );
						blockingPaths->push_back( matches[ i ][ currentWomanId ] );
					}
					else
					{
						trace_msg( heuristic, 5, "Blocking path found but locked" );
					}
				}
			}
			else
			{
				trace_msg( heuristic, 5, "Blocking path found is already used or locked" );
			}
		}
	}

#ifdef TRACE_ON
	string out = "";
	for ( unsigned int i = 0; i < blockingPaths->size( ); i++ )
		out += VariableNames::getName( blockingPaths->at( i )->var ) + ", ";

	trace_msg( heuristic, 3, "All blocking paths: " << out );
#endif

	removeDominatedPaths( *blockingPaths, &blockingPathsUndominated );

#ifdef TRACE_ON
	out = "";
	for ( unsigned int i = 0; i < blockingPathsUndominated.size( ); i++ )
		out += VariableNames::getName( blockingPathsUndominated.at( i )->var ) + ", ";

	trace_msg( heuristic, 3, "All undominated blocking paths: " << out );
	trace_msg( heuristic, 3, "==========sim end==============" );
#endif

	return blockingPathsUndominated.size( );
}

bool
StableMarriageHeuristic::getBlockingPaths(
	vector< Match* >* blockingPathsUndominated,
	bool removeDominated )
{
	trace_msg( heuristic, 3, "Looking for blocking paths..." );
	vector< Match* > blockingPaths;

	Person* currentPartnerM;
	Person* currentPartnerW;
	Person* possiblePartnerM;
	Person* possiblePartnerW;

	for ( unsigned int i = 0; i < matchesInput.size( ); i++ )
	{
		trace_msg( heuristic, 4, "Check path " << VariableNames::getName( matchesInput[ i ].var ) );

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
					trace_msg( heuristic, 5, "Blocking path found" );
					blockingPaths.push_back( &matchesInput[ i ] );
				}
				else
				{
					trace_msg( heuristic, 5, "Blocking path found but current matches are locked" );
				}
			}
		}
		else
		{
			trace_msg( heuristic, 5, "Blocking path found is already used or locked" );
		}
	}

#ifdef TRACE_ON
	string out = "";
	for ( unsigned int i = 0; i < blockingPaths.size( ); i++ )
		out += VariableNames::getName( blockingPaths.at( i )->var ) + ", ";

	trace_msg( heuristic, 3, "All blocking paths: " << out );
#endif

	//-----------------------------------------------------------
	if ( removeDominated && blockingPaths.size( ) > 1 )
	{
		removeDominatedPaths( blockingPaths, blockingPathsUndominated );
	}
	else
	{
		*blockingPathsUndominated = blockingPaths;
	}

	if ( (*blockingPathsUndominated).size( ) == 0 )
		return false;
	return true;
}

bool
StableMarriageHeuristic::getBlockingPathRandom(
	Match** blockingPath )
{
	trace_msg( heuristic, 3, "Looking for blocking path..." );

	vector< unsigned int > matchesPositionCopy = matchesPosition;
	std::random_shuffle ( matchesPositionCopy.begin(), matchesPositionCopy.end() );

	Person* currentPartnerM;
	Person* currentPartnerW;
	Person* possiblePartnerM;
	Person* possiblePartnerW;
	bool found = false;
	unsigned int pos;

	while ( !found && matchesPositionCopy.size( ) > 0 )
	{
		pos = matchesPositionCopy.back( );
		trace_msg( heuristic, 4, "Check path " << VariableNames::getName( matchesInput[ pos ].var ) );

		if ( !matchesInput[ pos ].usedInLS && !matchesInput[ pos ].lockedBySolver )
		{
			possiblePartnerM = matchesInput[ pos ].man;
			possiblePartnerW = matchesInput[ pos ].woman;
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
					*blockingPath = &matchesInput[ pos ];
					trace_msg( heuristic, 5, "Blocking path found: " << VariableNames::getName( (*blockingPath)->var ) );
					found = true;
				}
				else
				{
					trace_msg( heuristic, 5, "Blocking path found but current matches are locked" );
				}
			}
		}
		else
		{
			trace_msg( heuristic, 5, "Blocking path found is already used or locked" );
		}

		matchesPositionCopy.pop_back( );
	}

	trace_msg( heuristic, 3, "Blocking Path found: " << VariableNames::getName( (*blockingPath)->var ) );

	if ( !found )
		return false;
	return true;
}

void
StableMarriageHeuristic::removeDominatedPaths(
	vector< Match* > blockingPaths,
	vector< Match* >* undomiatedPaths )
{
	vector< Match* >undominatedTemp;
	vector< Match* >undominated;
	Match* best;
	vector< int > consideredMen;
	vector< int > consideredWomen;

	trace_msg( heuristic, 3, "Removing dominated paths for " << ( startingGenderMale ? "men" : "women" ) << "..." );
	for ( unsigned int i = 0; i < blockingPaths.size( ); i++ )
	{
		if ( startingGenderMale )
		{
			trace_msg( heuristic, 4, "Considering " << VariableNames::getName( blockingPaths[ i ]->var ) );
			if ( find( consideredMen.begin( ), consideredMen.end( ), blockingPaths[ i ]->manId ) == consideredMen.end( ) )
			{
				consideredMen.push_back( blockingPaths[ i ]->manId );
				best = blockingPaths[ i ];
				trace_msg( heuristic, 4, "Removing dominated paths for " << best->man->name  << "..." );

				for ( unsigned int j = i + 1; j < blockingPaths.size( ); j++ )
				{
					trace_msg( heuristic, 5, "Considering " << VariableNames::getName( blockingPaths[ j ]->var ) << "..." );
					if ( blockingPaths[ j ]->manId == best->manId &&
						 ( best->man->preferncesInput.find( blockingPaths[ j ]->woman->name )->second > best->man->preferncesInput.find( best->woman->name )->second ) )
					{
						best = blockingPaths[ j ];
						trace_msg( heuristic, 6, "new best found" );
					}
				}

				trace_msg( heuristic, 4, "Setting " << VariableNames::getName( best->var ) << " as best for " << best->man->name );
				undominatedTemp.push_back( best );
			}
		}
		else
		{
			trace_msg( heuristic, 4, "Considering " << VariableNames::getName( blockingPaths[ i ]->var ) );
			if ( find( consideredWomen.begin( ), consideredWomen.end( ), blockingPaths[ i ]->womanId ) == consideredWomen.end( ) )
			{
				consideredWomen.push_back( blockingPaths[ i ]->womanId );
				best = blockingPaths[ i ];
				trace_msg( heuristic, 4, "Removing dominated paths for " << best->woman->name  << "..." );

				for ( unsigned int j = i + 1; j < blockingPaths.size( ); j++ )
				{
					trace_msg( heuristic, 5, "Considering " << VariableNames::getName( blockingPaths[ j ]->var ) << "..." );
					if ( blockingPaths[ j ]->womanId == best->womanId &&
						 ( best->woman->preferncesInput.find( blockingPaths[ j ]->man->name )->second > best->woman->preferncesInput.find( best->man->name )->second ) )
					{
						best = blockingPaths[ j ];
						trace_msg( heuristic, 6, "new best found" );
					}
				}

				trace_msg( heuristic, 4, "Setting " << VariableNames::getName( best->var ) << " as best for " << best->woman->name );
				undominatedTemp.push_back( best );
			}
		}
	}

#ifdef TRACE_ON
	string output = "";
	for ( unsigned int i = 0; i < undominatedTemp.size( ); i++ )
		output += VariableNames::getName( undominatedTemp.at( i )->var ) + ", ";
	trace_msg( heuristic, 3, "Dominated paths after first check: " << output );
#endif

	startingGenderMale = !startingGenderMale;

	trace_msg( heuristic, 3, "Removing dominated paths for " << ( startingGenderMale ? "men" : "women" ) << "..." );
	for ( unsigned int i = 0; i < undominatedTemp.size( ); i++ )
	{
		if ( startingGenderMale )
		{
			trace_msg( heuristic, 4, "Considering " << VariableNames::getName( undominatedTemp[ i ]->var ) );
			if ( find( consideredMen.begin( ), consideredMen.end( ), undominatedTemp[ i ]->manId ) == consideredMen.end( ) )
			{
				consideredMen.push_back( undominatedTemp[ i ]->manId );
				best = undominatedTemp[ i ];
				trace_msg( heuristic, 4, "Removing dominated paths for " << best->man->name  << "..." );

				for ( unsigned int j = i + 1; j < undominatedTemp.size( ); j++ )
				{
					trace_msg( heuristic, 5, "Considering " << VariableNames::getName( blockingPaths[ j ]->var ) << "..." );
					if ( undominatedTemp[ j ]->manId == best->manId &&
						 ( best->man->preferncesInput.find( undominatedTemp[ j ]->woman->name )->second > best->man->preferncesInput.find( best->woman->name )->second ) )
					{
						best = undominatedTemp[ j ];
						trace_msg( heuristic, 6, "new best found" );
					}
				}

				trace_msg( heuristic, 4, "Setting " << VariableNames::getName( best->var ) << " as best for " << best->man->name );
				undominated.push_back( best );
			}
		}
		else
		{
			trace_msg( heuristic, 4, "Considering " << VariableNames::getName( undominatedTemp[ i ]->var ) );
			if ( find( consideredWomen.begin( ), consideredWomen.end( ), undominatedTemp[ i ]->womanId ) == consideredWomen.end( ) )
			{
				consideredWomen.push_back( undominatedTemp[ i ]->womanId );
				best = undominatedTemp[ i ];
				trace_msg( heuristic, 4, "Removing dominated paths for " << best->woman->name  << "..." );

				for ( unsigned int j = i + 1; j < undominatedTemp.size( ); j++ )
				{
					trace_msg( heuristic, 5, "Considering " << VariableNames::getName( blockingPaths[ j ]->var ) << "..." );
					if ( undominatedTemp[ j ]->womanId == best->womanId &&
						 ( best->woman->preferncesInput.find( undominatedTemp[ j ]->man->name )->second > best->woman->preferncesInput.find( best->man->name )->second ) )
					{
						best = undominatedTemp[ j ];
						trace_msg( heuristic, 6, "new best found" );
					}
				}

				trace_msg( heuristic, 4, "Setting " << VariableNames::getName( best->var ) << " as best for " << best->woman->name );
				undominated.push_back( best );
			}
		}
	}

#ifdef TRACE_ON
	output = "";
	for ( unsigned int i = 0; i < undominated.size( ); i++ )
		output += VariableNames::getName( undominated.at( i )->var ) + ", ";
	trace_msg( heuristic, 3, "Dominated paths after second check: " << output );
#endif

	*undomiatedPaths = undominated;
}

bool
StableMarriageHeuristic::getBestPathFromNeighbourhood(
	Match** bestBlockingPath )
{
	vector< Match* > blockingPathsCurrent;
	vector< Match* > blockingPathsSimulated;
	vector< unsigned int > bpPosition;
    unsigned int bestBlockingPathValue = UINT_MAX;
    unsigned int bestBlockingPathScore = 0;
    unsigned int nbp = UINT_MAX;
    unsigned int score = 0;

	trace_msg( heuristic, 2, "Get blocking paths for current assignment..." );

	if ( !getBlockingPaths( &blockingPathsCurrent, true ) )
	{
		trace_msg( heuristic, 2, "No blocking paths - stable marriage found" );
		return false;
	}

	trace_msg( heuristic, 2, "There are " << blockingPathsCurrent.size( ) << " blocking paths." );

	if ( blockingPathsCurrent.size( ) > 1 )
	{
		Match* addedMatching1;
		Match* addedMatching2;
		Match* removedMatching1;
		Match* removedMatching2;
		Match* blockingPathCurrent;

		for ( unsigned int i = 0; i < blockingPathsCurrent.size( ); i++ )
		{
			bpPosition.push_back( i );
		}

		start = std::chrono::system_clock::now();
		bool stopSampling = false;

		while ( bpPosition.size( ) > 0 && !stopSampling )
		{
			unsigned int randPos = rand() % bpPosition.size( );
			blockingPathCurrent = blockingPathsCurrent[ bpPosition[ randPos ] ];
			bpPosition.erase( bpPosition.begin( ) + randPos );

			trace_msg( heuristic, 2, "Simulate removing blocking path " << VariableNames::getName( blockingPathCurrent->var ) << "..." );
			vector< Match* > toCheck;

			addedMatching1 = blockingPathCurrent;
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

			trace_msg( heuristic, 5, "Set " << addedMatching1->man->name << " and " << addedMatching1->woman->name << " as partner" );
			addedMatching1->man->currentPartner = addedMatching1->woman;
			addedMatching1->woman->currentPartner = addedMatching1->man;

			trace_msg( heuristic, 5, "Set " << removedMatching2->man->name << " and " << removedMatching1->woman->name << " as partner" );
			removedMatching2->man->currentPartner = removedMatching1->woman;
			removedMatching1->woman->currentPartner = removedMatching2->man;

			//----------------------------------------
			toCheck.push_back( addedMatching1 );
			toCheck.push_back( addedMatching2 );
			nbp = getBlockingPathsSampling( toCheck, &blockingPathsSimulated );
			//----------------------------------------

			trace_msg( heuristic, 3, "Reset matching changes" );

			addedMatching1->usedInLS = false;
			addedMatching2->usedInLS = false;
			removedMatching1->usedInLS = true;
			removedMatching2->usedInLS = true;

			trace_msg( heuristic, 5, "Set " << addedMatching1->man->name << " and " << removedMatching1->woman->name << " as partner" );
			addedMatching1->man->currentPartner = removedMatching1->woman;
			removedMatching1->woman->currentPartner = addedMatching1->man;

			trace_msg( heuristic, 5, "Set " << removedMatching2->man->name << " and " << addedMatching1->woman->name << " as partner" );
			removedMatching2->man->currentPartner = addedMatching1->woman;
			addedMatching1->woman->currentPartner = removedMatching2->man;


			trace_msg( heuristic, 3, "Removing " << VariableNames::getName( blockingPathCurrent->var ) << " leads to " << nbp << " blocking paths" );

			score = addedMatching1->man->preferncesInput.find( addedMatching1->woman->name )->second +
					addedMatching1->woman->preferncesInput.find( addedMatching1->man->name )->second;
			if ( nbp < bestBlockingPathValue || ( nbp == bestBlockingPathValue && score > bestBlockingPathScore ) )
			{
				trace_msg( heuristic, 3, "Found better blocking path " << VariableNames::getName( blockingPathCurrent->var ) << " with nbp " << nbp << " and score " << score );
				*bestBlockingPath = blockingPathCurrent;
				bestBlockingPathValue = nbp;
				bestBlockingPathScore = score;
			}

			end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end-start;

			if ( elapsed_seconds.count( ) > samplingTimeout )
			{
				trace_msg( heuristic, 2, "Sampling timeout..." );
				stopSampling = true;
			}
			else
			{
				trace_msg( heuristic, 2, "Current sampling time: " << elapsed_seconds.count( ) << " seconds" );
			}
		}

#ifdef TRACE_ON
		string out = "";
		for ( unsigned int i = 0; i < blockingPathsCurrent.size( ); i++ )
			out += VariableNames::getName( blockingPathsCurrent[ i ]->var ) + ", ";

		trace_msg( heuristic, 2, "Blocking path found: " << out << "(nbp: " << bestBlockingPathValue << ", score: " << bestBlockingPathScore << ")" );
		trace_msg( heuristic, 2, "Best blocking path to remove: " << VariableNames::getName( (*bestBlockingPath)->var ) );
#endif
	}
	else
	{
		*bestBlockingPath = blockingPathsCurrent[ 0 ];
		trace_msg( heuristic, 2, "Only one blocking path found: " << VariableNames::getName( blockingPathsCurrent[ 0 ]->var ) );
	}

	return true;
}

bool
StableMarriageHeuristic::getRandomBlockingPath(
	Match** randomBlockingPath )
{
	vector< Match* > blockingPathsCurrent;

	trace_msg( heuristic, 2, "Get blocking paths for current assignment..." );

	if ( !getBlockingPaths( &blockingPathsCurrent, true ) )
	{
		trace_msg( heuristic, 2, "No blocking paths - stable marriage found" );
		return false;
	}

	trace_msg( heuristic, 2, "There are " << blockingPathsCurrent.size( ) << " blocking paths." );

	int random = rand() % blockingPathsCurrent.size( );
	*randomBlockingPath = blockingPathsCurrent[ random ];

#ifdef TRACE_ON
	string out = "";
	for ( unsigned int i = 0; i < blockingPathsCurrent.size( ); i++ )
		out += VariableNames::getName( blockingPathsCurrent[ i ]->var ) + ", ";

	trace_msg( heuristic, 2, "Blocking path found: " << out );
	trace_msg( heuristic, 2, "Chosen path (random): " << VariableNames::getName( (*randomBlockingPath)->var ) );
#endif

	return true;
}

bool
StableMarriageHeuristic::simulatedAnnealingStep(
	Match** randomBlockingPath,
	bool sampling,
	bool useDiffInSampling )
{
	vector< Match* > blockingPathsCurrent;
	vector< Match* > blockingPathsSimulated;
	Match* addedMatching1;
	Match* addedMatching2;
	Match* removedMatching1;
	Match* removedMatching2;
	vector< Match* > toCheck;
	unsigned int nbpC = UINT_MAX;
	unsigned int nbpS = UINT_MAX;
	float randVal;

	if ( sampling )
	{
		trace_msg( heuristic, 2, "Get blocking paths for current assignment..." );

		if ( !getBlockingPathRandom( randomBlockingPath ) )
		{
			trace_msg( heuristic, 2, "No blocking paths - stable marriage found" );
			steps = maxSteps + 1;
			return false;
		}

		trace_msg( heuristic, 2, "Calculate blocking paths " << VariableNames::getName( (*randomBlockingPath)->var ) << "..." );

		addedMatching1 = *randomBlockingPath;
		removedMatching1 = matches[ addedMatching1->man->id ][ addedMatching1->man->currentPartner->id ];
		removedMatching2 = matches[ addedMatching1->woman->currentPartner->id ][ addedMatching1->woman->id ];
		addedMatching2 = matches[ removedMatching2->man->id ][ removedMatching1->woman->id ];

		toCheck.push_back( addedMatching1 );
		toCheck.push_back( addedMatching2 );
		nbpC = getBlockingPathsSampling( toCheck, &blockingPathsCurrent );

		trace_msg( heuristic, 2, "Simulate removing blocking path " << VariableNames::getName( (*randomBlockingPath)->var ) << "..." );

		trace_msg( heuristic, 3, "Add " << VariableNames::getName( addedMatching1->var )
										<< " and " << VariableNames::getName( addedMatching2->var )
										<< ", remove " << VariableNames::getName( removedMatching1->var )
										<< " and " << VariableNames::getName( removedMatching2->var ) );

		addedMatching1->usedInLS = true;
		addedMatching2->usedInLS = true;
		removedMatching1->usedInLS = false;
		removedMatching2->usedInLS = false;

		trace_msg( heuristic, 5, "Set " << addedMatching1->man->name << " and " << addedMatching1->woman->name << " as partner" );
		addedMatching1->man->currentPartner = addedMatching1->woman;
		addedMatching1->woman->currentPartner = addedMatching1->man;

		trace_msg( heuristic, 5, "Set " << removedMatching2->man->name << " and " << removedMatching1->woman->name << " as partner" );
		removedMatching2->man->currentPartner = removedMatching1->woman;
		removedMatching1->woman->currentPartner = removedMatching2->man;

		nbpS = getBlockingPathsSampling( toCheck, &blockingPathsSimulated );

		trace_msg( heuristic, 3, "Reset matching changes" );

		addedMatching1->usedInLS = false;
		addedMatching2->usedInLS = false;
		removedMatching1->usedInLS = true;
		removedMatching2->usedInLS = true;

		trace_msg( heuristic, 5, "Set " << addedMatching1->man->name << " and " << removedMatching1->woman->name << " as partner" );
		addedMatching1->man->currentPartner = removedMatching1->woman;
		removedMatching1->woman->currentPartner = addedMatching1->man;

		trace_msg( heuristic, 5, "Set " << removedMatching2->man->name << " and " << addedMatching1->woman->name << " as partner" );
		removedMatching2->man->currentPartner = addedMatching1->woman;
		addedMatching1->woman->currentPartner = removedMatching2->man;

		randVal = (float)rand()/(float)(RAND_MAX);

		if ( useDiffInSampling )
		{
			int diff = getBlockingPathDifference( blockingPathsCurrent, blockingPathsSimulated );
			trace_msg( heuristic, 3, "Removing " << VariableNames::getName( (*randomBlockingPath)->var ) << " leads to difference " << diff );
			trace_msg( heuristic, 3, "Boltz: " << exp( ( (double)nbpS - nbpC ) / temperature ) << ", rand: " << randVal );

			if ( diff <= 0 || randVal < exp( diff / temperature ) )
			{
				noMoveCount = 0;
				trace_msg( heuristic, 3, "Replace current marriage" );
				return true;
			}
			noMoveCount++;
			return false;
		}
		else
		{
			trace_msg( heuristic, 3, "Removing " << VariableNames::getName( (*randomBlockingPath)->var ) << " leads to " << nbpS << " blocking paths" );
			trace_msg( heuristic, 3, "NPBs: " << nbpS << ", NPBc: " << nbpC << ", Boltz: " << exp( ( (double)nbpS - nbpC ) / temperature ) << ", rand: " << randVal );

			if ( nbpS <= nbpC || randVal < exp( ( (double)nbpS - nbpC ) / temperature ) )
			{
				noMoveCount = 0;
				trace_msg( heuristic, 3, "Replace current marriage" );
				return true;
			}
			noMoveCount++;
			return false;
		}
	}
	else
	{
		trace_msg( heuristic, 2, "Get blocking paths for current assignment..." );

		if ( !getBlockingPaths( &blockingPathsCurrent, true ) )
		{
			trace_msg( heuristic, 2, "No blocking paths - stable marriage found" );
			steps = maxSteps + 1;
			return false;
		}

		trace_msg( heuristic, 2, "There are " << blockingPathsCurrent.size( ) << " blocking paths." );

		int random = rand() % blockingPathsCurrent.size( );
		*randomBlockingPath = blockingPathsCurrent[ random ];

		trace_msg( heuristic, 2, "Simulate removing blocking path " << VariableNames::getName( (*randomBlockingPath)->var ) << "..." );

		addedMatching1 = *randomBlockingPath;
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

		trace_msg( heuristic, 5, "Set " << addedMatching1->man->name << " and " << addedMatching1->woman->name << " as partner" );
		addedMatching1->man->currentPartner = addedMatching1->woman;
		addedMatching1->woman->currentPartner = addedMatching1->man;

		trace_msg( heuristic, 5, "Set " << removedMatching2->man->name << " and " << removedMatching1->woman->name << " as partner" );
		removedMatching2->man->currentPartner = removedMatching1->woman;
		removedMatching1->woman->currentPartner = removedMatching2->man;

		getBlockingPaths( &blockingPathsSimulated, true );

		trace_msg( heuristic, 3, "Reset matching changes" );

		addedMatching1->usedInLS = false;
		addedMatching2->usedInLS = false;
		removedMatching1->usedInLS = true;
		removedMatching2->usedInLS = true;

		trace_msg( heuristic, 5, "Set " << addedMatching1->man->name << " and " << removedMatching1->woman->name << " as partner" );
		addedMatching1->man->currentPartner = removedMatching1->woman;
		removedMatching1->woman->currentPartner = addedMatching1->man;

		trace_msg( heuristic, 5, "Set " << removedMatching2->man->name << " and " << addedMatching1->woman->name << " as partner" );
		removedMatching2->man->currentPartner = addedMatching1->woman;
		addedMatching1->woman->currentPartner = removedMatching2->man;

		trace_msg( heuristic, 3, "Removing " << VariableNames::getName( (*randomBlockingPath)->var ) << " leads to " << blockingPathsSimulated.size( ) << " blocking paths" );

		trace_msg( heuristic, 3, "The difference is " << (blockingPathsCurrent.size( ) - blockingPathsSimulated.size( )) << " and the boltzman condition is " <<
				exp( ( blockingPathsSimulated.size( ) - blockingPathsCurrent.size( ) ) / temperature ) );

		if ( blockingPathsSimulated.size( ) <= blockingPathsCurrent.size( ) ||
				( ((float)rand()/(float)(RAND_MAX)) * 1 ) < exp( ( (double)blockingPathsSimulated.size( ) - blockingPathsCurrent.size( ) ) / temperature ) )
		{
			trace_msg( heuristic, 3, "Replace current marriage" );
			return true;
		}
		return false;
	}
}

int
StableMarriageHeuristic::getBlockingPathDifference(
	vector< Match* > oldMatches,
	vector< Match* > newMatches )
{
	int diff = 0;

	for ( unsigned int i = 0; i < newMatches.size( ); i++ )
	{
		if ( std::find( oldMatches.begin( ), oldMatches.end( ), newMatches[ i ] ) == oldMatches.end( ) )
			diff++;
	}

	for ( unsigned int i = 0; i < oldMatches.size( ); i++ )
	{
		if ( std::find( newMatches.begin( ), newMatches.end( ), oldMatches[ i ] ) == newMatches.end( ) )
			diff--;
	}

	return diff;
}

/*
 * removes the given blocking path from the current matching
 * (updates usedInLS)
 */
void
StableMarriageHeuristic::removeBlockingPath(
	Match* blockingPath )
{
	Match* addedMatching1;
	Match* addedMatching2;
	Match* removedMatching1;
	Match* removedMatching2;

//	std::chrono::time_point<std::chrono::system_clock> saStart, saEnd;
//	saStart = std::chrono::system_clock::now();

	trace_msg( heuristic, 2, "Remove blocking path " << VariableNames::getName( blockingPath->var ) << "..." );

	addedMatching1 = blockingPath;
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

//	saEnd = std::chrono::system_clock::now();
//	std::chrono::duration<double> saTime = saEnd-saStart;
//	cout << "\t\t...(removing the path takes " << saTime.count( ) << " seconds)" << endl;
}

/*
 * solver found conflict
 */
void
StableMarriageHeuristic::conflictOccurred(
	)
{
	cout << "conf" << endl;
	minisat->conflictOccurred( );
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

void
StableMarriageHeuristic::onFinishedSolving(
	bool fromSolver )
{
	cout << "no move fallback: " << nmCount << endl;
	cout << stepCount << " steps in " << heuCount << " heuristic calls" << endl;
	cout << "Fallback: " << fallbackCount << ", Minisat calls: " << callMinisatCount << endl;
	std::chrono::duration<double> elapsed_seconds = end_init-start_init;
	cout << elapsed_seconds.count( ) << " seconds needed for initialization" << endl;
	cout << heuristic_time.count( ) << " seconds needed for all heuristics calls (" << ( heuristic_time.count( ) / heuCount ) << " on average)" << endl;
	minisat->onFinishedSolving( fromSolver );
}

void
StableMarriageHeuristic::galeShapley(
	)
{
	bool unmachted_found = false;

	do
	{
		unmachted_found = false;
		trace_msg( heuristic, 2, "Start iteration (gale-shapley)" );

		for ( unsigned int i = 0; i < size; i++ )
		{
			trace_msg( heuristic, 3, "Consider man " << men[ i ].name );
			trace_msg( heuristic, 4, "He is " << ( men[ i ].matched == true ? "matched": "unmachted" ) << " and "
					<< ( men[ i ].lastConsidered < (int)size ? "has" : "has no more" ) << " prefernces to consider" );

			if ( !men[ i ].matched && men[ i ].lastConsidered < (int)size )
			{
				unmachted_found = true;
				Person* possiblePartner = men[ i ].gs_preference[ ++men[ i ].lastConsidered ].first;
				trace_msg( heuristic, 4, "Consider woman " << possiblePartner->name );
				trace_msg( heuristic, 5, "She is " << ( possiblePartner->matched == true ? "matched": "unmachted" ) );
#ifdef TRACE_ON
				if( possiblePartner->matched )
				{
					trace_msg( heuristic, 5, "Current matching " << possiblePartner->currentPartner->name << "("
							<< to_string(possiblePartner->preferncesInput.find( possiblePartner->currentPartner->name )->second ) << "); " << "new preference would be "
							<< to_string(possiblePartner->preferncesInput.find( men[ i ].name )->second) );
				}
#endif

				if ( !possiblePartner->matched ||
						possiblePartner->preferncesInput.find( possiblePartner->currentPartner->name )->second <
						possiblePartner->preferncesInput.find( men[ i ].name )->second )
				{
					if ( possiblePartner->matched )
					{
						trace_msg( heuristic, 5, "Free old partner" );
						possiblePartner->currentPartner->matched = false;
						possiblePartner->currentPartner->currentPartner = 0;
					}

					trace_msg( heuristic, 5, "Set new partner" );
					possiblePartner->currentPartner = &men[ i ];
					men[ i ].currentPartner = possiblePartner;

					possiblePartner->matched = true;
					men[ i ].matched = true;
				}
			}
		}

		stepCount++;

	} while ( unmachted_found );

	matchesInMarriage.clear( );
	trace_msg( heuristic, 3, "[GS] marriage found: " );
	for ( unsigned int i = 0; i < size; i++ )
	{
		matchesInMarriage.push_back( matches[ men[ i ].id ][ men[ i ].currentPartner->id ] );
		trace_msg( heuristic, 4, i << ": " << VariableNames::getName( matches[ men[ i ].id ][ men[ i ].currentPartner->id ]->var ) );
	}
}

void
StableMarriageHeuristic::strongStableMarriage(
	)
{
	int level = 1;
	bool freePersons;
	vector< Match* > topMachtings;
	int topMatchingValue;
	int proposalsMade;
	bool found;

//	for ( unsigned int i = 0; i < men.size( ); i++ )
//	{
//		trace_msg( heuristic, 1, "[DEBUG] "<< men[ i ].name );
//		for ( unsigned int j = 0; j < men[ i ].strong_preferences.size( ); j++ )
//			trace_msg( heuristic, 2, "[DEBUG] " << men[ i ].strong_preferences[ j ].first->name << ", " << men[ i ].strong_preferences[ j ].second );
//	}
//
//	for ( unsigned int i = 0; i < women.size( ); i++ )
//	{
//		trace_msg( heuristic, 1, "[DEBUG] "<< women[ i ].name );
//		for ( unsigned int j = 0; j < women[ i ].strong_preferences.size( ); j++ )
//			trace_msg( heuristic, 2, "[DEBUG] " << women[ i ].strong_preferences[ j ].first->name << ", " << women[ i ].strong_preferences[ j ].second );;
//	}
//
//	trace_msg( heuristic, 1, "[DEBUG] EPrime/EC/Matching" );
//	for ( unsigned int i = 0; i < size; i++ )
//	{
//		for ( unsigned int j = 0; j < size; j++ )
//		{
//			if ( matches[ i ][ j ]->inEPrime )
//				cout << "+/";
//			else
//				cout << "-/";
//
//			if ( matches[ i ][ j ]->inEC )
//				cout << "+/";
//			else
//				cout << "-/";
//
//			if ( matches[ i ][ j ]->inMatching )
//				cout << "+\t";
//			else
//				cout << "-\t";
//		}
//
//		cout << endl;
//	}

	trace_msg( heuristic, 2, "[SM] Start looking for strong stable marriage" );
	do
	{
		trace_msg( heuristic, 2, "[SM] Begin next iteration" );
		trace_msg( heuristic, 2, "[SM] Phase one" );
		freePersons = true;
		proposalsMade = 0;

		//-------------------------------------
		// phase one (move edges from E' to Ec for men; remove strictly less prefered edges for women) - start

		while ( freePersons )
		{
			freePersons = false;

			for ( unsigned int i = 0; i < size; i++ )
			{
				trace_msg( heuristic, 3, "[SM] Check man "<< men[ i ].name );

#ifdef TRACE_ON
				if ( isFree( &men[ i ], false ) )
					trace_msg( heuristic, 4, "[SM] is free" );
				if ( men[ i ].strong_preferences.size( ) > 0 )
					trace_msg( heuristic, 4, "[SM] has preferences" );
#endif

				// process men if he is free and has preferences
				if ( isFree( &men[ i ],false ) && men[ i ].strong_preferences.size( ) > 0 )
				{
					freePersons = true;
					topMachtings.clear( );
					topMatchingValue = 0;

					// find the first top preference (delete it and continue if edge already removed)
					found = false;
					do
					{
						if ( men[ i ].strong_preferences.size( ) > 0 )
						{
							if ( matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->inEPrime )
							{
								trace_msg( heuristic, 4, "[SM] best preferences has edge " << VariableNames::getName( matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->var )
								                       << " with " << men[ i ].strong_preferences.back( ).second << " (move it)" );
								topMachtings.push_back( matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ] );
								matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->inEC = true;
								matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->inEPrime = false;
								matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->level = level;
								topMatchingValue = men[ i ].strong_preferences.back( ).second;
								men[ i ].strong_preferences.pop_back( );
								found = true;
								proposalsMade++;
							}
							else
							{
								trace_msg( heuristic, 4, "[SM] " << VariableNames::getName( matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->var ) << " already removed -> remove from list" );
								men[ i ].strong_preferences.pop_back( );
							}
						}
					} while ( men[ i ].strong_preferences.size( ) > 0 && !found );

					// get all other preferences with the same value as the one before and remove them
					found = true;
					while ( men[ i ].strong_preferences.size( ) > 0 && found )
					{
						if ( men[ i ].strong_preferences.back( ).second == topMatchingValue )
						{
							if ( matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->inEPrime )
							{
								trace_msg( heuristic, 4, "[SM] move edge " << VariableNames::getName( matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->var )
																		   << " (" << men[ i ].strong_preferences.back( ).second <<  ") too" );
								topMachtings.push_back( matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ] );
								matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->inEC = true;
								matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->inEPrime = false;
								matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->level = level;
								men[ i ].strong_preferences.pop_back( );
							}
							else
							{
								trace_msg( heuristic, 4, "[SM] " << VariableNames::getName( matches[ men[ i ].id ][ men[ i ].strong_preferences.back( ).first->id ]->var ) << " already removed -> remove from list" );
								men[ i ].strong_preferences.pop_back( );
							}
						}
						else
						{
							found = false;
						}
					}

#ifdef TRACE_ON
					string out = "";
					for ( unsigned int j = 0; j < topMachtings.size( ); j++)
					{
						out += VariableNames::getName( topMachtings[ j ]->var ) + ", ";
					}
					trace_msg( heuristic, 4, "[SM] Top matchings: " << out );
#endif

					for ( unsigned int j = 0; j < topMachtings.size( ); j++ )
					{
						Person* w = topMachtings[ j ]->woman;
						topMatchingValue = w->preferncesInput.find( topMachtings[ j ]->man->name )->second;
						found = true;

						trace_msg( heuristic, 4, "[SM] Remove strictly less prefered edges (woman " << w->name << ", " << topMatchingValue << ")" );

						while ( w->strong_preferences.size( ) > 0 && found )
						{
							if ( w->strong_preferences.back( ).second < topMatchingValue )
							{
								trace_msg( heuristic, 5, "[SM] Remove: " << VariableNames::getName( matches[ w->strong_preferences.back( ).first->id ][ w->id ]->var )
								                                         << " (" << w->strong_preferences.back( ).second << ")" );
								matches[ w->strong_preferences.back( ).first->id ][ w->id ]->inEC = false;
								matches[ w->strong_preferences.back( ).first->id ][ w->id ]->inEPrime = false;
								w->strong_preferences.pop_back( );
							}
							else
							{
								found = false;
							}
						}
					}
				}
			}
		}

		// phase one - end
		//-------------------------------------
		// phase two - start

		trace_msg( heuristic, 2, "[SM] Phase two" );
		for ( unsigned int i = 0; i < size; i++ )
		{
			int currentLevel;
			augmentedPathFound = false;
			augmentedPath.clear( );

			// consider only free man in the matching
			trace_msg( heuristic, 3, "[SM] Consider man " << men[ i ].name );
			if ( isFree( &men[ i ], true) )
			{
				currentLevel = getLevel( &men[ i ] );
				trace_msg( heuristic, 3, "[SM] is free and has level " << currentLevel );

				while ( currentLevel > 0 && !augmentedPathFound )
				{
					findAugmentedPath( &men[ i ], currentLevel );

					currentLevel--;
				}

				// try to find an augmented path
				if ( augmentedPathFound )
				{
#ifdef TRACE_ON
					string path = "";
					for ( unsigned int i = 0; i < augmentedPath.size( ); i++ )
						path += VariableNames::getName( augmentedPath[ i ]->var ) + ", ";
					trace_msg( heuristic, 3, "[SM] augmented path: " << path );
#endif
					for ( unsigned int i = 0; i < augmentedPath.size( ); i++ )
						augmentedPath[ i ]->inMatching = !augmentedPath[ i ]->inMatching;
				}
				// if there is none: find women that adjecent to alternatively reachable men starting from the current men
				else
				{
					alternatingReachableWomen.clear( );
					findAlternatingReachableWomen( &men[ i ] );
					int worstMatchingValue = 0;

					for ( unsigned int j = 0; j < alternatingReachableWomen.size( ); j++ )
					{
						Person* current = alternatingReachableWomen[ j ];

						found = false;
						do
						{
							if ( alternatingReachableWomen[ j ]->strong_preferences.size( ) > 0 )
							{
								if ( matches[ current->strong_preferences.back( ).first->id ][ current->id ]->inEPrime ||
										matches[ current->strong_preferences.back( ).first->id ][ current->id ]->inEC )
								{
									trace_msg( heuristic, 4, "[SM] worst preferences has edge " << VariableNames::getName( matches[ current->strong_preferences.back( ).first->id ][ current->id ]->var )
														   << " with " << current->strong_preferences.back( ).second << " (delete it)" );
									matches[ current->strong_preferences.back( ).first->id ][ current->id ]->inEC = false;
									matches[ current->strong_preferences.back( ).first->id ][ current->id ]->inEPrime = false;
									worstMatchingValue = current->strong_preferences.back( ).second;
									current->strong_preferences.pop_back( );
									found = true;
								}
								else
								{
									trace_msg( heuristic, 4, "[SM] " << VariableNames::getName( matches[ current->strong_preferences.back( ).first->id ][ current->id ]->var ) << " already removed -> remove from list" );
									current->strong_preferences.pop_back( );
								}
							}
						} while ( current->strong_preferences.size( ) > 0 && !found );

						// get all other preferences with the same value as the one before and remove them
						found = true;
						while ( current->strong_preferences.size( ) > 0 && found )
						{
							if ( ( matches[ current->strong_preferences.back( ).first->id ][ current->id ]->inEPrime ||
									matches[ current->strong_preferences.back( ).first->id ][ current->id ]->inEC ) &&
								 current->strong_preferences.back( ).second == worstMatchingValue )
							{
								trace_msg( heuristic, 4, "[SM] remove edge " << VariableNames::getName( matches[ current->strong_preferences.back( ).first->id ][ current->id ]->var )
								                                           << " (" << current->strong_preferences.back( ).second <<  ") too" );
								matches[ current->strong_preferences.back( ).first->id ][ current->id ]->inEC = false;
								matches[ current->strong_preferences.back( ).first->id ][ current->id ]->inEPrime = false;
								current->strong_preferences.pop_back( );
							}
							else
							{
								found = false;
							}
						}
					}
				}
			}
		}

		// phase two - end
		//-------------------------------------

		level++;
	} while ( proposalsMade > 0 );

	trace_msg( heuristic, 2, "[SM] No one has made a proposal -> fallback" );
}

void
StableMarriageHeuristic::findAugmentedPath(
	StableMarriageHeuristic::Person* start,
	int currentLevel )
{
	vector< Person* > augmentedPath;
	vector< Person* > endpoints;
	int path_index = 0;

	if ( start->male )
	{
		for ( unsigned int i = 0; i < size; i++ )
		{
			men[ i ].considered = women[ i ].considered = false;

			if ( getLevel( &women[ i ] ) == currentLevel && isFree( &women[ i ], true ) )
				endpoints.push_back( &women[ i ] );
		}
	}
	else
	{
		for ( unsigned int i = 0; i < size; i++ )
		{
			men[ i ].considered = women[ i ].considered = false;
			if ( getLevel( &men[ i ] ) == currentLevel && isFree( &men[ i ], true ) )
				endpoints.push_back( &men[ i ] );
		}
	}

	for ( unsigned int i = 0; i < endpoints.size( ); i++ )
	{
		if ( !augmentedPathFound )
			findAugmentedPath( start, endpoints[ i ], false, currentLevel, augmentedPath, path_index );
	}
}

void
StableMarriageHeuristic::findAugmentedPath(
	Person* start,
	Person* dest,
	bool inMatching,
	int currentLevel,
	vector< Person* > path,
	int &path_index )
{
	if ( !augmentedPathFound )
	{
		start->considered = true;
		path.push_back( start );
		path_index++;

		if ( start == dest )
		{
			augmentedPathFound = true;

			for ( int i = 0; i < path_index - 1; i++ )
			{
				if ( path[ i ]->male )
					augmentedPath.push_back( matches[ path[ i ]->id ][ path[ i+1 ]->id ] );
				else
					augmentedPath.push_back( matches[ path[ i+1 ]->id ][ path[ i ]->id ] );
			}
		}
		else
		{
			if ( start->male )
			{
				for ( unsigned int i = 0; i < size; i++ )
				{
					if ( matches[ start->id ][ i ]->inEC && !women[ i ].considered && getLevel( &women[ i ] ) >= currentLevel && matches[ start->id ][ i ]->inMatching == inMatching )
						findAugmentedPath( &women[ i ], dest, !inMatching, currentLevel, path, path_index );
				}
			}
			else
			{
				for ( unsigned int i = 0; i < size; i++ )
				{
					if ( matches[ i ][ start->id ]->inEC && !men[ i ].considered && getLevel( &men[ i ] ) >= currentLevel && matches[ i ][ start->id ]->inMatching == inMatching )
						findAugmentedPath( &men[ i ], dest, !inMatching, currentLevel, path, path_index );
				}
			}
		}

		path_index--;
		start->considered = false;
	}
}

void
StableMarriageHeuristic::findAlternatingReachableWomen(
	Person* start )
{
	for ( unsigned int i = 0; i < size; i++ )
	{
		men[ i ].considered = women[ i ].considered = false;
	}

	findAlternatingReachableWomen( start, true );
	findAlternatingReachableWomen( start, false );
}

void
StableMarriageHeuristic::findAlternatingReachableWomen(
	Person* start,
	bool inMatching )
{
	start->considered = true;

	if ( start->male )
	{
		for ( unsigned int i = 0; i < size; i++ )
		{
			if ( matches[ start->id ][ i ]->inEC )
			{
				if ( std::find( alternatingReachableWomen.begin( ), alternatingReachableWomen.end( ), &women[ i ] ) == alternatingReachableWomen.end( ) )
					alternatingReachableWomen.push_back( &women[ i ] );
			}
		}
	}
	else
	{
		if ( start->male )
		{
			for ( unsigned int i = 0; i < size; i++ )
			{
				if ( matches[ start->id ][ i ]->inEC && !women[ i ].considered && matches[ start->id ][ i ]->inMatching == inMatching )
					findAlternatingReachableWomen( &women[ i ], !inMatching );
			}
		}
		else
		{
			for ( unsigned int i = 0; i < size; i++ )
			{
				if ( matches[ i ][ start->id ]->inEC && !men[ i ].considered && matches[ i ][ start->id ]->inMatching == inMatching )
					findAlternatingReachableWomen( &men[ i ], !inMatching );
			}
		}
	}

	start->considered = false;
}

bool
StableMarriageHeuristic::isFree(
	Person* p,
	bool inMatching )
{
	if ( p->male )
	{
		for ( unsigned int i = 0; i < size; i++ )
		{
			if ( !inMatching )
			{
				if ( matches[ p->id ][ i ]->inEC )
					return false;
			}
			else
			{
				if ( matches[ p->id ][ i ]->inMatching )
					return false;
			}
		}
	}
	else
	{
		for ( unsigned int i = 0; i < size; i++ )
		{
			if ( !inMatching )
			{
				if ( matches[ i ][ p->id ]->inEC )
					return false;
			}
			else
			{
				if ( matches[ i ][ p->id ]->inMatching )
					return false;
			}
		}
	}

	return true;
}

int
StableMarriageHeuristic::getLevel(
	Person* p )
{
	int level = INT_MAX;

	if ( p->male )
	{
		for ( unsigned int i = 0; i < size; i++ )
		{
			if ( matches[ p->id ][ i ]->inEC && matches[ p->id ][ i ]->level < level )
				level = matches[ p->id ][ i ]->level;
		}
	}
	else
	{
		for ( unsigned int i = 0; i < size; i++ )
		{
			if ( matches[ i ][ p->id ]->inEC && matches[ i ][ p->id ]->level < level )
				level = matches[ i ][ p->id ]->level;
		}
	}

	if ( level == INT_MAX )
		return -1;

	return level;
}




























