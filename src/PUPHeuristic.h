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

#ifndef PUPHEURISTIC_H
#define	PUPHEURISTIC_H

#define U2Z 1
#define U2S 2
#define ZONE 1
#define SENSOR 2

#include "util/Options.h"
#include "util/Trace.h"
#include "Literal.h"
#include "stl/Heap.h"
#include "Heuristic.h"

#include <cassert>
#include <iostream>
using namespace std;
class Solver;

class PUPHeuristic : public Heuristic
{
    public:
        PUPHeuristic( Solver& solver );
        ~PUPHeuristic();

        void onNewVariable( Var v );
        void onNewVariableRuntime( Var v );
        void onReadAtomTable ( Var v,  string name );
        void onFinishedParsing ( );
        void onLiteralInvolvedInConflict( Literal literal );
        void onUnrollingVariable( Var v );
        void incrementHeuristicValues( Var v );
        void simplifyVariablesAtLevelZero();
        void conflictOccurred();

    protected:
        Literal makeAChoiceProtected();

    private:
		unsigned int startAt;
		unsigned int index;
		unsigned int usedPartnerUnits;
		unsigned int update;
		unsigned int maxPu;
		unsigned int maxElem;
		int assignTo;
		int lastAssignTo;

		struct Node
		{
			string name;
			Var var;
			vector < Node* > children;
			unsigned int considered;
			unsigned int type;
		};

		struct Pu
		{
			string name;
			Var var;
		};

		struct Connection
		{
			string from;
			string to;
			Var var;
		};

		struct ZoneAssignment
		{
			string pu;
			string to;
			Var positive;
			Var negative;
			unsigned int considered;
		};

		/*struct PuAssignment
		{
			Pu *unit;
			vector < PuAssignment* > partnerUnits;
			vector < Node* > zones;
			vector < Node* > sensors;
		};*/

        vector < Node > zones;
		vector < Node > sensors;
		vector < Pu > partnerUnits;
		vector < Connection > zone2sensor;
		vector < ZoneAssignment > unit2zone;
		vector < ZoneAssignment > unit2sensor;

		vector < Node* > order;
		//vector < PuAssignment > model;

		void getName( string atom, string *name );
		void getName( string atom, string *name1, string *name2 );

		void initRelation ( );
		void initUnitAssignments( ZoneAssignment pu, unsigned int sign, unsigned int type );
		void createOrder ( );
		bool newPartnerUnitAvailable ( Pu *pu ); //bool newPartnerUnitAvailable ( PuAssignment *pua );
		//bool assignAndConnect ( PuAssignment *unit, Node *node );
		void incrementIndex ( );
		void decrementIndex ( );
		Var getVariable ( Pu *unit, Node *node );
};

#endif

