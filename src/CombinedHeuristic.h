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

#define NONE 0
#define CONFLICT 1
#define TIME 2

#include "Heuristic.h"
#include "Solver.h"
#include <string>
#include <chrono>

class CombinedHeuristic : public Heuristic
{
    public:
		CombinedHeuristic( Solver& solver, unsigned int useTreshold, unsigned int treshold = 100, bool altnerate = false );
        ~CombinedHeuristic();
        void onNewVariable( Var v );
        void onNewVariableRuntime( Var v );
        void onLiteralInvolvedInConflict( Literal );
        void onUnrollingVariable( Var v );
        void incrementHeuristicValues( Var v );
        void simplifyVariablesAtLevelZero( );
        void conflictOccurred( );
        void onFinishedParsing( );
        unsigned int getTreshold( );
        void onFinishedSolving( bool fromSolver );
        bool isInputCorrect( );
        bool isCoherent( ) { return true; };
        void reset( ) { };

        void addHeuristic( Heuristic* h );
        bool addHeuristic( string h );

    protected:
        Literal makeAChoiceProtected( );

    private:
        unsigned int index;
        unsigned int th;
        unsigned int useTh;
        unsigned int thReached;
        unsigned int nConflicts;
        bool alt;
        std::chrono::time_point<std::chrono::system_clock> start, starttime, end;

        Heuristic* minisat;
        vector< Heuristic* > heuristics;
        vector< string > heursisticsNames;

        bool tresholdReached( unsigned int useTh, unsigned int th );
};

#endif


