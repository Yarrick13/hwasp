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

#ifndef BINPACKINGHEURISTIC_H
#define	BINPACKINGHEURISTIC_H

#include "Heuristic.h"

class BinPackingHeuristic : public Heuristic
{
    public:
		BinPackingHeuristic( Solver& solver );
        ~BinPackingHeuristic() { };
        void onNewVariable( Var v ){ variables.push_back( v ); }
        void onNewVariableRuntime( Var ){ }
        void onFinishedParsing ( );
        void onLiteralInvolvedInConflict( Literal ){ };
        void onUnrollingVariable( Var ){ };
        void incrementHeuristicValues( Var ){ };
        void simplifyVariablesAtLevelZero(){ };
        void conflictOccurred(){ conflictOccured = true; }

    protected:
        Literal makeAChoiceProtected();

    private:
        unsigned int index;				// current index for the heuristic (items)
        bool conflictOccured;			// true after conflict, otherwise false

        struct Item2Bin
		{
			Var variable;
			int bin;
			int item;
			int itemsize;
		};

        struct Bin
		{
        	Var variable;
        	int name;
        	int size;

        	vector< Item2Bin* > usedIn;
		};

        struct Item
        {
        	Var variable;
        	int name;
        	int size;

        	vector< Item2Bin* > usedIn;
        	vector< Bin* > correspondingBin;
        	vector< bool > tried;
        	unsigned int current;
        };

        vector< Var > variables;
        vector< Bin > bins;
        vector< Item > items;
        vector< Item2Bin > item2bin;

        vector< Item* > order;

        void processVariable( Var variable );
        void initUsedIn( );
        int getCurrentBinContentSize( Bin* bin );

        void quicksort( vector< Item > &items, unsigned int p, unsigned int q );
		int partition( vector< Item > &items, unsigned int p, unsigned int q);
};

#endif
