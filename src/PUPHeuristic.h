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

#include <string>
#include <vector>
#include <chrono>

#define U2Z 1
#define U2S 2
#define ZONE 1
#define SENSOR 2

#include "util/Constants.h"
#include "util/Options.h"
#include "util/Trace.h"
#include "Literal.h"
#include "stl/Heap.h"
#include "Heuristic.h"

#include <cassert>
#include <iostream>
using namespace std;
class Solver;

/*
 * QuickPUP heuristic for solving the partner units problem
 * Problem encoding: pup polynomial ( ASP competition 2011 )
 * ( at most 2 partner units and 2 zones/sensors per unit )
 */
class PUPHeuristic : public Heuristic
{
    public:
        PUPHeuristic( Solver& solver );
        ~PUPHeuristic() { };

        void onNewVariable( Var v ) { variables.push_back( v ); };
        void onNewVariableRuntime( Var ) { };
        void onFinishedParsing ( );
        void onLiteralInvolvedInConflict( Literal ) { };
        void onUnrollingVariable( Var ) { };
        void incrementHeuristicValues( Var ) { };
        void simplifyVariablesAtLevelZero() { };
        void conflictOccurred( );
        unsigned int getTreshold( ){ return numberOfConflicts; };
        void onFinishedSolving( );
        bool isInputCorrect( ) { return inputCorrect; };
        bool isCoherent( ) { return coherent; };

    protected:
        Literal makeAChoiceProtected();

    private:
		unsigned int startAt;					// current starting node
		unsigned int index;						// current order index ( next node to be considered )
		unsigned int maxPu;						// maximum number of partner units per unit
		unsigned int maxElementsOnPu;			// maximum number of zones/sensors per unit
		unsigned int numberOfConflicts;
		bool coherent;
		bool conflictOccured;
		bool conflictHandled;
		unsigned int assignedSinceConflict;
		bool redoAfterConflict;
		bool redoAfterAddingConstraint;
		bool inputCorrect;
		bool solutionFound;

		unsigned int sNumberOfConflicts;
		unsigned int sNumberOfOrdersCreated;
		unsigned int sNumberOfRecommendations;
		unsigned int sNumberOfOrderMaxReached;
		unsigned int sFallback;
		double sAlreadyFalse;
		double sAlreadyTrue;

		std::chrono::duration<double> pre;
		std::chrono::duration<double> dec;

		// represents a zone to sensor connection ( positive and negative variable )
		struct ZoneAssignment
		{
			string pu;
			string to;
			Var var;
			unsigned int type;
		};

		// represents a partner unit
		struct Pu
		{
			string name;
			Var var;

			vector < ZoneAssignment* > usedIn;

			vector < Pu* > connectedTo;
			unsigned int numberOfZones;
			unsigned int numberOfSensors;
			unsigned int numberOfPartners;
			bool removed;
		};

		// represents either a zone or a sensor
		struct Node
		{
			string name;
			Var var;
			vector < Node* > children;			// all sensors connected to this zone ( or vice versa )
			unsigned int type;					// ZONE or SENSOR
			unsigned int considered;
			unsigned int ignore;

			vector < ZoneAssignment* > usedIn;
			vector < Pu* > usedInUnit;
		};

		// represents a zone to sensor connection
		struct Connection
		{
			string from;
			string to;
			Var var;
		};

		// represents an assignment done by the heuristic
		struct Assignment
		{
			Var var;
			Pu currentPu;
			vector < Var > triedUnits;
		};

		struct PartnerUnitConnection
		{
			Var var;
			string unit1;
			string unit2;
		};

		vector < Var > variables;
        vector < Node > zones;
		vector < Node > sensors;
		vector < Pu > partnerUnits;
		vector < Connection > zone2sensor;		// input relation for creating zone assignments
		vector < ZoneAssignment > unit2zone;
		vector < ZoneAssignment > unit2sensor;
		vector < PartnerUnitConnection > partnerUnitConnections;

		vector < Node* > order;					// current node order
		vector < Assignment > assignments;		// current assignments done by the heuristic

		void initRelation ( );
		bool resetHeuristic ( );
		void processVariable ( Var v );
		bool checkInput( );
		void initUnitAssignments( ZoneAssignment pu, unsigned int type );
		bool createOrder ( );

		Var getVariable ( Pu *unit, Node *node );

		bool searchAndAddAssignment( Var variable, Pu pu );
		bool getTriedAssignments( vector < Var >* tried );
		unsigned int getUnusedPu( Pu* pu, Node* current );
		bool isPartnerUsed( Pu* pu );
		unsigned int getUntriedPu( Pu* pu, Node* current, const vector < Var >& tried );
		bool getPu( Var assignment, Pu *pu );
		void minimize( vector< Var >* trueInAS, vector< Var>* falseInAS, vector< Pu* >* removed);
		void printStatistics( );
};

#endif

