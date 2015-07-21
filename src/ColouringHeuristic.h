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

#ifndef COLOURINGHEURISTIC_H
#define	COLOURINGHEURISTIC_H

#include "Heuristic.h"

class ColouringHeuristic : public Heuristic
{
    public:
		ColouringHeuristic( Solver& solver );
        ~ColouringHeuristic() { };
        void onNewVariable( Var v ) { variables.push_back( v ); }
        void onNewVariableRuntime( Var ) { };
        void onFinishedParsing ( );
        void onLiteralInvolvedInConflict( Literal ){ }
        void onUnrollingVariable( Var ){ }
        void incrementHeuristicValues( Var ){ }
        void simplifyVariablesAtLevelZero( ){ }
        void conflictOccurred(){ conflictOccured = true; }

    protected:
        Literal makeAChoiceProtected();

    private:
        unsigned int index;
        unsigned int firstChoiceIndex;
        bool conflictOccured;

        struct ColourAssignment
		{
			Var variable;
			string node;
			string colour;
		};

        struct Node
		{
			Var variable;
			string name;
			unsigned int degree;

			vector< ColourAssignment* > usedIn;
			vector< bool > tried;
			unsigned int current;
		};

        struct Link
        {
        	Var variable;
        	string from;
        	string to;
        };

        struct Colour
        {
        	Var variable;
        	string name;
        };

        struct Assignment
        {
        	Var variable;
        	ColourAssignment* current;
        	vector< ColourAssignment* > tried;
        };

        vector< Var > variables;
        vector< Node > nodes;
        vector< Link > links;
        vector< Colour > colours;
        vector< ColourAssignment > colourAssignments;

        vector< Node* > order;

        void processVariable( Var v );
        void initDegree( );
        void initUsedIn( );

        void quicksort( vector< Node > &nodes, unsigned int p, unsigned int q );
        int partition( vector< Node > &nodes, unsigned int p, unsigned int q);
};

#endif
