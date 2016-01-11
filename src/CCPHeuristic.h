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
		CCPHeuristic( Solver& solver );
        ~CCPHeuristic( ) { };
        void onNewVariable( Var v ){ variables.push_back( v ); }
        void onNewVariableRuntime( Var ){ }
        void onLiteralInvolvedInConflict( Literal ){ }
        void onUnrollingVariable( Var ){ }
        void incrementHeuristicValues( Var ){  }
        void simplifyVariablesAtLevelZero (){  }
        void conflictOccurred( );
        void onFinishedParsing( );
        unsigned int getTreshold( ){ return nConflicts; }
        void onFinishedSolving( ){ cout << "Number of conflict: " << nConflicts << endl; cout << "Number of Choices: " << nChoices << endl; };
        bool isInputCorrect( ){ return inputCorrect; }
        bool isCoherent( ){ return true; }

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

    private:
        struct Vertex
		{
        	string name;
        	Var var;
        	unsigned int size;

        	bool considered;

        	vector< Vertex* > predecessor;
        	vector< Vertex* > successors;
        	vector< VertexColour* > allColours;
        	vector< VertexBin* > allBins;
		};

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
        void print( );
        unsigned int getVertexColour( Vertex* vertex );
        unsigned int getVertexBin( Vertex* vertex );

        Literal greedyMatching( );
        Literal greedyCBPC( );
};

#endif
