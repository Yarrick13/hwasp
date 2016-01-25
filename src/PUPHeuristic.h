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
        unsigned int getTreshold( ){ return sNumberOfConflicts; };
        void onFinishedSolving( bool );
        bool isInputCorrect( ) { return inputCorrect; };
        bool isCoherent( ) { return coherent; };
        void reset( )
        {
        	sNumberOfConflicts = 0;
        	sNumberOfOrdersCreated = 0;
        	sNumberOfRecommendations = 0;
        	sNumberOfOrderMaxReached = 0;
        	resetHeuristic( true );
        };

    protected:
        Literal makeAChoiceProtected();

    private:
		unsigned int startAt;					// current starting node
		unsigned int index;						// current order index ( next node to be considered )
		unsigned int maxPu;						// maximum number of partner units per unit
		unsigned int maxElementsOnPu;			// maximum number of zones/sensors per unit
		unsigned int lowerBound;
		bool coherent;
		bool shrinkingPossible;
		bool conflictHandled;
		bool redoAfterAddingConstraint;
		bool redoAfterShrinking;
		bool inputCorrect;
		bool solutionFound;
		unsigned int resetLimit;
		unsigned int shrinkingIndex;

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

    public:
		// represents a partner unit
		struct Pu
		{
			string name;
			Var var;

			vector < ZoneAssignment* > usedIn;

			vector < Pu* > connectedTo;
			vector < Pu* > currentlyConnectedTo;
			unsigned int numberOfZones;
			unsigned int numberOfSensors;
			unsigned int numberOfPartners;
			bool removed;
			bool isUsed;
		};

		// represents either a zone or a sensor
		struct Node
		{
			string name;
			Var var;
			vector < Node* > connectedNodes;			// all sensors connected to this zone ( or vice versa )
			unsigned int type;					// ZONE or SENSOR
			unsigned int considered;
			unsigned int ignore;
			unsigned int resetTo;
			unsigned int currentOrderPosition;

			vector < ZoneAssignment* > usedIn;
			vector < Pu* > usedInUnit;
			vector < Pu* > untriedPredecessorUnits;
			Pu* assignedTo;
		};

    private:
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
			Pu* currentPu;
			vector < Var > triedUnits;
			bool triedNewUnit = false;
		};

		struct PartnerUnitConnection
		{
			Var var;
			string unit1;
			Pu* pu1;
			string unit2;
			Pu* pu2;
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
		vector < Var > shrinked;
		vector < Var > undefined;

		void initRelation ( );
		bool resetHeuristic ( bool createNewOrder );
		void processVariable ( Var v );
		bool checkInput( );
		void initUnitAssignments( ZoneAssignment pu, unsigned int type );
		bool createOrder ( );

		Var getVariable ( Pu *unit, Node *node );

		bool searchAndAddAssignment( Node* node, Var variable, Pu* pu, bool triedNewUnit );
		void getPredecessorUnits( Node* node );
		bool getTriedAssignments( vector < Var >* tried );
		bool newUnitTriedForCurrentNode( );
		unsigned int getUnusedPu( Pu** pu, Node* current );
		bool allUnitsUsed( );
		void resetUsedUnits( );
		unsigned int getUntriedPredecessorUnit( Pu** pu, Node* current );
		unsigned int getUntriedPu( Pu** pu, Node* current, const vector < Var >& tried );
		bool getPu( Var assignment, Pu** pu );
		void shrink( vector< Var >* trueInAS, vector< Var>* falseInAS, vector< Pu* >* removed, vector< Pu* >* notUsed);
		void printStatistics( );
		bool checkPartialAssignment( );
};

#endif

