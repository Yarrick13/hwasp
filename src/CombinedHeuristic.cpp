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

#include "CombinedHeuristic.h"
#include "MinisatHeuristic.h"
#include "PUPHeuristic.h"
#include "ColouringHeuristic.h"
#include "BinPackingHeuristic.h"
#include "CCPHeuristic.h"
#include "util/HeuristicUtil.h"

CombinedHeuristic::CombinedHeuristic(
    Solver& s,
	unsigned int useTreshold,
	unsigned int treshold,
	bool alternate ) : Heuristic( s ), index( 0 ), th( treshold ), useTh( useTreshold ), thReached( 0 ), nConflicts( 0 ), alt( alternate )
{
	minisat = new MinisatHeuristic( s );
}

CombinedHeuristic::~CombinedHeuristic()
{
	minisat->~Heuristic();

	for ( Heuristic* h : heuristics )
		h->~Heuristic( );
}

void
CombinedHeuristic::onNewVariable(
	Var v )
{
	minisat->onNewVariable( v );

	for ( Heuristic* h : heuristics )
		h->onNewVariable( v );
}

void
CombinedHeuristic::onNewVariableRuntime(
	Var v )
{
	minisat->onNewVariableRuntime( v );

	for ( Heuristic* h : heuristics )
		h->onNewVariableRuntime( v );
}

void
CombinedHeuristic::onLiteralInvolvedInConflict(
	Literal l )
{
	if ( index < heuristics.size( ) )
		heuristics[ index ]->onLiteralInvolvedInConflict( l );

	minisat->onLiteralInvolvedInConflict( l );
}

void
CombinedHeuristic::onUnrollingVariable(
	Var v )
{
	minisat->onUnrollingVariable( v );

	for ( Heuristic* h : heuristics )
		h->onUnrollingVariable( v );
}

void
CombinedHeuristic::incrementHeuristicValues(
	Var v )
{
	if ( index < heuristics.size( ) )
		heuristics[ index ]->incrementHeuristicValues( v );

	minisat->incrementHeuristicValues( v );
}

void
CombinedHeuristic::simplifyVariablesAtLevelZero(
	)
{
	minisat->simplifyVariablesAtLevelZero( );

	for ( Heuristic* h : heuristics )
		h->simplifyVariablesAtLevelZero( );
}

void
CombinedHeuristic::conflictOccurred(
	)
{
	nConflicts++;

	if ( index < heuristics.size( ) )
	{
		heuristics[ index ]->conflictOccurred( );
	}
	else
	{
		minisat->conflictOccurred( );
	}
}

Literal
CombinedHeuristic::makeAChoiceProtected(
	)
{
	Literal lit = Literal::null;

	if ( useTh != NONE && tresholdReached( useTh, th ) )
	{
		trace_msg( heuristic, 1, "Treshold reached - get next heuristic" );
		start = std::chrono::system_clock::now();

		if ( index < heuristics.size( ) )
		{
			cout << heursisticsNames[ index ] << " conflicts: " << nConflicts << endl;
			heuristics[ index ]->onFinishedSolving( false );
		}
		else
		{
			cout << "minisat conflicts: " << nConflicts << endl;
		}
		nConflicts = 0;

		if ( alt )
		{
			if ( index >= heuristics.size( ) )
			{
				index = 0;
			}
			else
			{
				index++;
			}

			if ( index < heuristics.size( ) )
				heuristics[ index ]->reset( );
		}
		else
		{
			index++;
		}
	}

	while ( lit == Literal::null && index < heuristics.size( ) )
	{
		lit = heuristics[ index ]->makeAChoice( );
		if ( lit == Literal::null )
		{
			index++;
			nConflicts = 0;
		}
		else
			return lit;
	}

	return minisat->makeAChoice( );
}

void
CombinedHeuristic::onFinishedParsing(
	)
{
	minisat->onFinishedParsing( );

	for ( Heuristic* h : heuristics )
		h->onFinishedParsing( );

	isInputCorrect( );

	starttime = std::chrono::system_clock::now();
	start = std::chrono::system_clock::now();
}

bool
CombinedHeuristic::isInputCorrect(
	)
{
	bool isCorrect = true;

	for ( unsigned int i = 0; i < heuristics.size( ); i++ )
	{
		if ( !heuristics[ i ]->isInputCorrect( ) )
		{
			cout <<  heursisticsNames[ i ] << " will not be used because the input was not in the correct format" << endl;
			heuristics.erase( heuristics.begin( ) + i );
			heursisticsNames.erase( heursisticsNames.begin( ) + i );
			isCorrect = false;
		}
	}

	return isCorrect;
}

void
CombinedHeuristic::addHeuristic(
	Heuristic* h )
{
	heuristics.push_back( h );
}

bool
CombinedHeuristic::addHeuristic(
	string h )
{
	std::transform(h.begin(), h.end(), h.begin(), HeuristicUtil::tolower);

	if ( h == "pup" )
	{
		heuristics.push_back( new PUPHeuristic( solver ) );
		heursisticsNames.push_back( "PuP heuristic" );
	}
	else if ( h == "colouring" )
	{
		heuristics.push_back( new ColouringHeuristic( solver ) );
		heursisticsNames.push_back( "colouring heuristic " );
	}
	else if ( h == "binpacking" )
	{
		heuristics.push_back( new BinPackingHeuristic( solver ) );
		heursisticsNames.push_back( "bin packing heuristic" );
	}
	else if ( h == "ccp" )
	{
		heuristics.push_back( new CCPHeuristic( solver ) );
		heursisticsNames.push_back( "ccp heuristic" );
	}
	else
		return false;

	return true;
}

unsigned int
CombinedHeuristic::getTreshold(
	)
{
	if ( index < heuristics.size( ) )
		return heuristics[ index ]->getTreshold( );
	else
		return minisat->getTreshold( );
};

void
CombinedHeuristic::onFinishedSolving(
	bool fromSolving )
{
	minisat->onFinishedSolving( fromSolving );

	for ( Heuristic* h : heuristics )
		h->onFinishedSolving( fromSolving );

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-starttime;
	cout << "solving time: " << elapsed_seconds.count( ) << endl;
	cout << "fallback to minisat: " << ( index >= heuristics.size( ) ? "yes" : "no" ) << endl;
	cout << "treshold reached: " << thReached << " times " << endl;
}

bool
CombinedHeuristic::tresholdReached(
	unsigned int useTh,
	unsigned int th )
{
	bool reached = true;

	if ( useTh == TIME )
	{
		end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end-start;
		if ( elapsed_seconds.count( ) > th )
		{
			reached = true;
		}
		else
			reached = false;
	}
	else
	{
		#ifdef STATS_ON
			if ( ( (double) solver.getNumberOfRestarts( ) / solver.getNumberOfChoices( ) ) > th )
			{
				reached = true;
				thReached++;
			}
			else
				reached = false;
		#else
			if ( getTreshold( ) > th )
			{
				reached = true;
			}
			else
				reached = false;
		#endif
	}

	if ( reached && index < heuristics.size( ) )
		thReached++;

	return reached;
}
