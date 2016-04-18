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
	unsigned int samplingTimeoutDefault ) : Heuristic( s ), randWalkProb( randomWalkProbability ), steps( 0 ), maxSteps( maxSteps ), timeout( timeoutDefault ), samplingTimeout( samplingTimeoutDefault ),
	                         size( 0 ), inputCorrect( true ), index( 0 ), runLocalSearch( true ), sendToSolver( false ), marriageFound( false ), startingGenderMale( true )
{
	minisat = new MinisatHeuristic( s );
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
				p.id = strtoul( tmp.substr(1).c_str(), NULL, 0 ) - 1;
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
				p.preferncesInput.insert( std::pair< string, int >( tmp2, strtoul( tmp3.c_str(), NULL, 0 ) ) );

				women.push_back( p );
			}
		}
		else if( name.compare( 0, 6, "match(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			Match m;
			m.var = v;
			m.manId = strtoul( tmp.substr(1).c_str(), NULL, 0 ) - 1;;
			m.womanId = strtoul( tmp2.substr(1).c_str(), NULL, 0 ) - 1;;

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

void
StableMarriageHeuristic::initData(
	)
{
	bool found = false;

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
			m.push_back( &matchesInput[ i * size + j ] );

		matches.push_back( m );
	}
}

/*
 * make a choice for the solver
 */
Literal
StableMarriageHeuristic::makeAChoiceProtected(
	)
{
	if ( !runLocalSearch )
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
			trace_msg( heuristic, 2, "Starting step " << (steps++) << "..." );

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

		trace_msg( heuristic, 2, "Prepare assignment to send to solver..." );
		for ( unsigned int i = 0; i < matchesInput.size( ); i++ )
		{
			if ( matchesInput[ i ].usedInLS )
				matchesInMarriage.push_back( &matchesInput[ i ] );
		}

		index = 0;
		runLocalSearch = false;
		sendToSolver = true;
		start = std::chrono::system_clock::now();
		trace_msg( heuristic, 1, "Fallback..." );
	}


	if ( sendToSolver )
	{
		for ( unsigned int i = 0; i < matchesInMarriage.size( ); i++ )
		{
			if ( solver.getTruthValue( matchesInMarriage[ index ]->var ) == UNDEFINED )
			{
				trace_msg( heuristic, 3, "index " << index << " of size " << matchesInMarriage.size( ) );
				trace_msg( heuristic, 3, "truth value: " << solver.getTruthValue( matchesInMarriage[ index ]->var ) );
				trace_msg( heuristic, 3, "Send " << VariableNames::getName( matchesInMarriage[ index ]->var ) << " as " << (index+1) << "th" );
				return Literal( matchesInMarriage[ index++ ]->var, POSITIVE );
			}
		}

		sendToSolver = false;
	}

	if ( index < matchesInMarriage.size( ) )
	{
		trace_msg( heuristic, 3, "Send " << VariableNames::getName( matchesInMarriage[ index ]->var ) << " as " << (index+1) << "th" );
		return Literal( matchesInMarriage[ index++ ]->var, POSITIVE );
	}

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
//	matchesUsedInLS.push_back( matches[ 0 ][ 2 ] );
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
	vector< Match* > bpToCheck )
{
	trace_msg( heuristic, 3, "==========sim start==============" );
	vector< Match* > blockingPaths;
	vector< Match* > blockingPathsUndominated;

	unsigned nbp = 0;
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
						blockingPaths.push_back( matches[ currentManId ][ i ] );
						nbp++;
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
						blockingPaths.push_back( matches[ i ][ currentWomanId ] );
						nbp++;
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
	for ( unsigned int i = 0; i < blockingPaths.size( ); i++ )
		out += VariableNames::getName( blockingPaths.at( i )->var ) + ", ";

	trace_msg( heuristic, 3, "All blocking paths: " << out );
#endif

	removeDominatedPaths( blockingPaths, &blockingPathsUndominated );

#ifdef TRACE_ON
	out = "";
	for ( unsigned int i = 0; i < blockingPathsUndominated.size( ); i++ )
		out += VariableNames::getName( blockingPathsUndominated.at( i )->var ) + ", ";

	trace_msg( heuristic, 3, "All undominated blocking paths: " << out );
	trace_msg( heuristic, 3, "==========sim end==============" );
#endif

	return nbp;
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
			nbp = getBlockingPathsSampling( toCheck );
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
}

/*
 * solver found conflict
 */
void
StableMarriageHeuristic::conflictOccurred(
	)
{
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
