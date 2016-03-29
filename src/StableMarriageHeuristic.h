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

#ifndef STABLEMARRIAGEHEURISTIC_H
#define	STABLEMARRIAGEHEURISTIC_H

#include "Heuristic.h"

#include <map>
#include <chrono>

class StableMarriageHeuristic : public Heuristic
{
    public:
		StableMarriageHeuristic( Solver& solver, float randomWalkProbability, unsigned int maxSteps, unsigned int timeout );
        ~StableMarriageHeuristic( ) { minisat->~Heuristic( ); }
        void onNewVariable( Var v ){ variables.push_back( v ); minisat->onNewVariable( v ); }
        void onNewVariableRuntime( Var v ){ minisat->onNewVariableRuntime( v ); }
        void onLiteralInvolvedInConflict( Literal l ){ minisat->onLiteralInvolvedInConflict( l ); }
        void onUnrollingVariable( Var v ){ minisat->onUnrollingVariable( v ); }
        void incrementHeuristicValues( Var v ){ minisat->incrementHeuristicValues( v ); }
        void simplifyVariablesAtLevelZero( ){ minisat->simplifyVariablesAtLevelZero( ); }
        void conflictOccurred( );
        void onFinishedParsing( );
        unsigned int getTreshold( ) { return minisat->getTreshold( ); }
        void onFinishedSolving( bool fromSolver ) { minisat->onFinishedSolving( fromSolver ); };
        bool isInputCorrect( ){ return inputCorrect; }
        bool isCoherent( ){ return true; }
        void reset( ) { minisat->reset( ); }

    protected:
        Literal makeAChoiceProtected();

    public:
        struct Person
		{
        	Var var;
        	string name;
        	unsigned int id;			// id is {number}-1 for m_{number} and woman_{number}; {number} >= 1

        	map< string, int > preferncesInput;

        	Person* currentPartner;
		};

        struct Match
        {
        	Var var;
        	unsigned manId;
        	unsigned womanId;

        	Person* man;
        	Person* woman;

        	bool usedInLS;
        	bool lockedBySolver;
        };

    private:
        vector< Var > variables;
        vector< Person > men;
        vector< Person > women;
        vector< Match > matchesInput;
        vector< vector< Match* > > matches;
        vector< Match* > matchesUsedInLS;
        vector< Match* > matchesInMarriage;

        std::chrono::time_point<std::chrono::system_clock> start, starttime, end;

        Heuristic* minisat;

        float randWalkProb;
        unsigned int steps;
        unsigned int maxSteps;
        unsigned int timeout;
        unsigned int size;
        bool inputCorrect;

        unsigned int index;
        bool runLocalSearch;
        bool marriageFound;

        bool checkInput( );
        void processVariable( Var var );
        void initData( );

        void createFullAssignment( );
        bool getBestPathFromNeighbourhood( Match** bestBlockingPath );
        bool getRandomBlockingPath( Match** randomBlockingPath );
        bool getBlockingPath( vector< Match* >* blockingPairs );
        void removeBlockingPath( Match* blockingPath );
};

#endif
