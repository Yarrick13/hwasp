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
	bool useTreshold,
	unsigned int treshold ) : Heuristic( s ), index( 0 ), th( treshold ), useTh( useTreshold )
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
	if ( index < heuristics.size( ) )
		heuristics[ index ]->conflictOccurred( );

	minisat->conflictOccurred( );
}

Literal
CombinedHeuristic::makeAChoiceProtected(
	)
{
	Literal lit = Literal::null;

	if ( useTh && getTresholdStatistics( ) >= th )
		index++;

	while ( lit == Literal::null && index < heuristics.size( ) )
	{
		lit = heuristics[ index ]->makeAChoice( );
		if ( lit == Literal::null )
			index++;
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
		heursisticsNames.push_back( "ccp packing heuristic" );
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
	)
{
	minisat->onFinishedSolving( );

	for ( Heuristic* h : heuristics )
		h->onFinishedSolving( );
}

double
CombinedHeuristic::getTresholdStatistics(
	)
{
#ifdef STATS_ON
	return (double) solver.getNumberOfRestarts( ) / solver.getNumberOfChoices( );
#else
	return getTreshold( );
#endif
}
