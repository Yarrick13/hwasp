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
		StableMarriageHeuristic( Solver& solver, float randomWalkProbability, unsigned int maxSteps, unsigned int timeout, unsigned int samplingTimeout, bool useSimulatedAnnealing );
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
        void onFinishedSolving( bool fromSolver );
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

        	// for gale-shapley
        	bool matched;
        	vector< pair< Person*, int > > gs_preference;
        	int lastConsidered;

        	// for strong stable marriage computation
        	vector< pair< Person*, int > >strong_preferences;
        	bool male;
        	bool considered;
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

        	// for strong stable marriages computation
        	bool inEPrime;
        	bool inEC;
        	int level;
        	bool inMatching;
        };

    private:
        vector< Var > variables;
        vector< Person > men;
        vector< Person > women;
        vector< Match > matchesInput;
        vector< vector< Match* > > matches;
        vector< Match* > matchesInMarriage;
        vector< unsigned int > matchesPosition;

        // augmented path
        bool augmentedPathFound;
        vector< Match* > augmentedPath;
        vector< Person* > alternatingReachableWomen;

        std::chrono::time_point<std::chrono::system_clock> start, starttime, end;
        std::chrono::time_point<std::chrono::system_clock> start_heuristic, end_heuristic, start_init, end_init;
        std::chrono::duration<double> heuristic_time = end-start;

        Heuristic* minisat;

        float randWalkProb;
        unsigned int steps;
        unsigned int maxSteps;
        unsigned int stepCount;
        unsigned int heuCount;
        unsigned int timeout;
        unsigned int samplingTimeout;
        unsigned int size;
        bool inputCorrect;
        int noMoveCount;

        unsigned int index;
        bool runLocalSearch;
        bool sendToSolver;
        bool marriageFound;
        bool startingGenderMale;
        bool simAnnealing;
        float temperature;
        unsigned int fallbackCount;
        unsigned int callMinisatCount;
        unsigned int nmCount;
        bool gs_finished;

        bool checkInput( );
        void processVariable( Var var );
        void initData( );

        void createFullAssignment( );
        bool getBestPathFromNeighbourhood( Match** bestBlockingPath );
        bool getRandomBlockingPath( Match** randomBlockingPath );
        bool getBlockingPaths( vector< Match* >* blockingPaths, bool removeDominated );
        void removeDominatedPaths( vector< Match* > blockingPaths, vector< Match* >* undomiatedPaths );

        unsigned int getBlockingPathsSampling( vector< Match* > bpToCheck, vector< Match* >* blockingPaths );
        bool getBlockingPathRandom( Match** blockingPaths );

        void removeBlockingPath( Match* blockingPath );

        int getBlockingPathDifference( vector< Match* > oldMatches, vector< Match* > newMatches );

        bool simulatedAnnealingStep( Match** bestBockingPath, bool sampling, bool useDiffInSampling );

        void galeShapley( );

        void strongStableMarriage( );
        bool isFree( Person* p, bool inMatching );
        int getLevel( Person* p );
        void findAugmentedPath( Person* start, int currentLevel );
        void findAugmentedPath( Person* start, Person* dest, bool inMatching, int currentLevel, vector< Person* > path, int &path_index);
        void findAlternatingReachableWomen( Person* start );
        void findAlternatingReachableWomen( Person* start, bool inMatching );
};

#endif
