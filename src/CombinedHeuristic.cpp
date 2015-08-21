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
#include "util/HeuristicUtil.h"

CombinedHeuristic::CombinedHeuristic(
    Solver& s ) : Heuristic( s )
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
	minisat->onLiteralInvolvedInConflict( l );

	for ( Heuristic* h : heuristics )
		h->onLiteralInvolvedInConflict( l );
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
	minisat->incrementHeuristicValues( v );

	for ( Heuristic* h : heuristics )
		h->incrementHeuristicValues( v );
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
	minisat->conflictOccurred( );

	for ( Heuristic* h : heuristics )
		h->conflictOccurred( );
}

Literal
CombinedHeuristic::makeAChoiceProtected(
	)
{
	cout << "number of restarts: " << solver.getNumberOfRestarts( ) << endl;
	cout << "number of choices: " << solver.getNumberOfChoices( ) << endl;
	assert( 0 && "IMPLEMENT ME!" );
	return Literal::null;
}

void
CombinedHeuristic::onFinishedParsing(
	)
{
	minisat->onFinishedParsing( );

	for ( Heuristic* h : heuristics )
		h->onFinishedParsing( );
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
		heuristics.push_back( new PUPHeuristic( solver ) );
	else if ( h == "colouring" )
		heuristics.push_back( new ColouringHeuristic( solver ) );
	else if ( h == "binpacking" )
		heuristics.push_back( new BinPackingHeuristic( solver ) );
	else
		return false;

	return true;

}
