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

#ifndef COMBINEDHEURISTIC_H
#define	COMBINEDHEURISTIC_H

#include "Heuristic.h"
#include "Solver.h"
#include <string>

class CombinedHeuristic : public Heuristic
{
    public:
		CombinedHeuristic( Solver& solver );
        ~CombinedHeuristic();
        void onNewVariable( Var v );
        void onNewVariableRuntime( Var v );
        void onLiteralInvolvedInConflict( Literal l );
        void onUnrollingVariable( Var v );
        void incrementHeuristicValues( Var v );
        void simplifyVariablesAtLevelZero( );
        void conflictOccurred( );
        void onFinishedParsing( );
        unsigned int getTreshold( );

        void addHeuristic( Heuristic* h );
        bool addHeuristic( string h );

    protected:
        Literal makeAChoiceProtected( );

    private:
        unsigned int index;

        Heuristic* minisat;
        vector< Heuristic* > heuristics;

        double getTresholdStatistics( );
};

#endif


