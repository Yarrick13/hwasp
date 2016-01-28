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

#ifndef CPPHEURISTIC_H
#define	CPPHEURISTIC_H

#include "Heuristic.h"

class CCPHeuristic : public Heuristic
{
    public:
		CCPHeuristic( Solver& solver, bool useO = true, bool useA1 = false );
        ~CCPHeuristic( ) { };
        void onNewVariable( Var v ){ variables.push_back( v ); }
        void onNewVariableRuntime( Var ){ }
        void onLiteralInvolvedInConflict( Literal ){ }
        void onUnrollingVariable( Var ){ }
        void incrementHeuristicValues( Var ){  }
        void simplifyVariablesAtLevelZero( ){  }
        void conflictOccurred( );
        void onFinishedParsing( );
        unsigned int getTreshold( ) { return nConflicts; }
        void onFinishedSolving( bool fromSolver )
        {
        	if ( !fromSolver )
        	{
        		cout << "CCP" << endl;
        		cout << "fallback" << endl;
        		cout << "Number of conflict: " << nConflicts << endl;
        		cout << "Number of Choices: " << nChoices << endl;
        		cout << "Number of Iterations: " << nIterations << endl;
        	}
        };
        bool isInputCorrect( ){ return inputCorrect; }
        bool isCoherent( ){ return true; }
        void reset( )
        {
        	nConflicts = 0;
        	nChoices = 0;
        	nIterations = 0;
        	unrollHeuristic( );
        	resetHeuristic( );
        	orderingValid = createOrder( );
        };

    protected:
        Literal makeAChoiceProtected();

    private:
        unsigned int nrOfColors;
		unsigned int nrOfBins;
        unsigned int maxBinSize;
        bool inputCorrect;
        unsigned int index;
        unsigned int currentColour;

        unsigned int nConflicts;
        unsigned int nChoices;
        unsigned int nIterations;

        bool fallback;
        bool orderingValid;

        bool useOrdering;
		bool useA1A2;

    public:
        struct VertexColour
		{
			string vertex;
			unsigned int colour;
			Var var;
		};

        struct VertexBin
        {
        	string vertex;
        	unsigned int bin;
        	Var var;
        };

        struct Vertex
		{
        	string name;
        	Var var;
        	unsigned int size;

        	bool considered;
        	unsigned int orderingValue;
        	bool inOrder;
        	unsigned int inPath;			// 0 for no path or >1 for the number of the path

        	vector< Vertex* > neighbours;
        	unsigned int nPredecessors;
        	unsigned int nSuccessors;
        	vector< VertexColour* > allColours;
        	vector< VertexBin* > allBins;
		};

    private:
        struct VertexSize
		{
			string vertex;
			unsigned int size;
			Var var;
		};

        struct BinAssignment
        {
        	string vertex;
        	unsigned int vertexSize;
        	unsigned int colour;
        	unsigned int bin;
        	Var var;
        };

        struct Bin
        {
        	unsigned int name;

        	vector< BinAssignment* > allVertices;
        };

        struct Edge
        {
        	string from;
        	string to;
        	Var var;
        };

        struct Area
		{
			string area;
			Var var;

			bool selected;

			unsigned int total;
			unsigned int counter;
		};

        struct EdgeMatching
        {
        	string area;
        	string borderelement;
        	Var var;

        	Area* realtedArea;
        };

        struct Borderelement
		{
			string element;
			Var var;

			vector< EdgeMatching* > usedIn;
		};

        Vertex* currentV;
        Borderelement* currentBe;

        vector< Var > variables;
        vector< Vertex > vertices;
        vector< Edge > edges;
        vector< vector< Bin > > binsByColour; 	// one vector of bins for each colour
        vector< BinAssignment > binAssignments;
        vector< VertexSize > vertexSizes;
        vector< VertexColour > vertexColours;
        vector< VertexBin > vertexBins;
        vector< Area > areas;
        vector< Borderelement > borderelements;
        vector< EdgeMatching > edgeMatchings;
        vector< string > path1;
        vector< string > path2;

        vector< Vertex* > startingVertices;
        vector< Vertex* > order;
        vector< Vertex* > queue;

        void processVariable( Var v );
        bool initData( );
        bool checkInput( );

        void queuePushBack( Vertex* vertex );
        Vertex* queueGetFirst ( );
        void queueEraseFirst( );
        void queueAddNeighbours( Vertex* vertex );
        unsigned int getUsedBinSize( unsigned int bin, unsigned int colour );

        void resetHeuristic( );
        bool createOrder( );
        void print( );
        unsigned int getVertexColour( Vertex* vertex );
        unsigned int getVertexBin( Vertex* vertex );

        Literal greedyMatching( );
        Literal greedyCBPC( );
};

#endif
