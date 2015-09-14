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

#include "Heuristic.h"
#include "Solver.h"

Literal
Heuristic::makeAChoice()
{
    if( !preferredChoices.empty() )
    {
        for( unsigned int i = 0; i < preferredChoices.size(); i++ )
            if( solver.isUndefined( preferredChoices[ i ].getVariable() ) )
                return preferredChoices[ i ];
    }

    return makeAChoiceProtected( );
}

void
Heuristic::addClause(
	const vector< Literal >& literals
)
{
	if ( solver.getCurrentDecisionLevel( ) != 0 )
		solver.unrollToZero( );

	Clause* clause = new Clause( literals.size( ) );

	for ( Literal l : literals )
		clause->addLiteral( l );

	solver.addClauseRuntime( clause );
}

void
Heuristic::unrollHeuristic(
	)
{
	trace_msg( heuristic, 1, "Unroll heuristic" );

	if ( solver.getCurrentDecisionLevel( ) > 0 )
		solver.unroll( solver.getNumberOfAssumptions( ) );
}
