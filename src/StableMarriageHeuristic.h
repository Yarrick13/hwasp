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

class StableMarriageHeuristic : public Heuristic
{
    public:
		StableMarriageHeuristic( Solver& solver );
        ~StableMarriageHeuristic( ) { };
        void onNewVariable( Var v ){ variables.push_back( v ); }
        void onNewVariableRuntime( Var ){ }
        void onLiteralInvolvedInConflict( Literal ){ }
        void onUnrollingVariable( Var ){ }
        void incrementHeuristicValues( Var ){  }
        void simplifyVariablesAtLevelZero( ){  }
        void conflictOccurred( );
        void onFinishedParsing( );
        unsigned int getTreshold( ) { return 0; }//nConflicts; }
        void onFinishedSolving( bool fromSolver ) { };
        bool isInputCorrect( ){ return inputCorrect; }
        bool isCoherent( ){ return true; }
        void reset( ) { }

    protected:
        Literal makeAChoiceProtected();

    public:
        struct Person
		{
        	Var var;
        	string name;
        	unsigned int id;			// id is {number}-1 for m_{number} and woman_{number}; {number} >= 1

        	map< string, int > preferncesInput;
        	vector< pair< Person*, int > > prefsByID;
        	vector< pair< Person*, int > > prefsByPref;
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

        unsigned int size;
        bool inputCorrect;

        bool checkInput( );
        void resetHeuristic( );
        void processVariable( Var var );
        void initData( );

        void createFullAssignment( );
};

#endif
