/*
 *
 *  Copyright 2013 Mario Alviano, Carmine Dodaro, Wolfgang Faber, Nicola Leone, Francesco Ricca, and Marco Sirianni.
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

/* 
 * File:   Solver.h
 * Author: Carmine Dodaro
 *
 * Created on 21 July 2013, 17.36
 */

#ifndef SOLVER_H
#define	SOLVER_H

#include <cassert>
#include <vector>
#include <list>
using namespace std;

#include "Clause.h"
#include "Literal.h"
#include "stl/List.h"
#include "stl/UnorderedSet.h"
#include "learning/LearningStrategy.h"
#include "learning/FirstUIPLearningStrategy.h"
#include "PositiveLiteral.h"
#include "learning/SequenceBasedRestartsStrategy.h"
#include "learning/DeletionStrategy.h"
#include "learning/AggressiveDeletionStrategy.h"
#include "LearnedClause.h"

class Variable;

class Solver
{
    public:
        inline Solver();
        ~Solver();
        
        virtual void init() = 0;
        virtual void solve() = 0;        
        
        void addVariable( const string& name );
        void addVariable();
        
        inline void addClause( Clause* clause );
        inline void addLearnedClause( LearnedClause* learnedClause );        
        
        Literal* getLiteral( int lit );
        
        inline Literal* getNextLiteralToPropagate();
        inline bool hasNextLiteralToPropagate() const;        
        
        inline unsigned int getCurrentDecisionLevel();
        inline void incrementCurrentDecisionLevel();
        
        void onLiteralAssigned( Literal* literal, TruthValue truthValue, Clause* implicant );
        
        void decreaseLearnedClausesActivity();
        void onDeletingLearnedClausesThresholdBased();
        void onDeletingLearnedClausesAvgBased();
        inline void onLearningClause( Literal* literalToPropagate, LearnedClause* learnedClause, unsigned int backjumpingLevel );
        inline void onLearningUnaryClause( Literal* literalToPropagate, LearnedClause* learnedClause );        
        inline void onRestarting();        
        
        inline unsigned int numberOfClauses();
        inline unsigned int numberOfLearnedClauses();
        
        void unroll( unsigned int level );
        inline void unrollOne();
        
        void printProgram()
        {
            for( list< Clause* >::iterator it = clauses.begin(); it != clauses.end(); ++it )
            {
                cout << *( *it ) << endl;
            }
        }
        
    private:
        Solver( const Solver& )
        {
            assert( "The copy constructor has been disabled." && 0 );
        }
                
        void addVariableInternal( PositiveLiteral* posLiteral );
        inline void deleteLearnedClause( LearnedClause* learnedClause, List< LearnedClause* >::iterator iterator );
        
        list< Literal* > literalsToPropagate;
        unsigned int currentDecisionLevel;
        
    protected:
        LearningStrategy* learningStrategy;
        DeletionStrategy* deletionStrategy;
        
        list< PositiveLiteral* > assignedLiterals;
        UnorderedSet< PositiveLiteral* > undefinedLiterals;
        vector< unsigned int > unrollVector;
        bool conflict;
        Literal* conflictLiteral;
        
        /* Data structures */
        vector< PositiveLiteral* > positiveLiterals;
        List< Clause* > clauses;
        List< LearnedClause* > learnedClauses;
};

Solver::Solver() : currentDecisionLevel( 0 ), conflict( false ), conflictLiteral( NULL )
{
    //Add a fake position.
    positiveLiterals.push_back( NULL );
    learningStrategy = new FirstUIPLearningStrategy( new SequenceBasedRestartsStrategy( 32 ) );
    deletionStrategy = new AggressiveDeletionStrategy();    
}

void
Solver::addClause(
    Clause* clause )
{
    clauses.push_back( clause );
}

void
Solver::addLearnedClause( 
    LearnedClause* learnedClause )
{    
    learnedClauses.push_back( learnedClause );
}

Literal*
Solver::getNextLiteralToPropagate()
{
    assert( !literalsToPropagate.empty() );
    Literal* tmp = literalsToPropagate.back();
    literalsToPropagate.pop_back();
    return tmp;
}
        
bool
Solver::hasNextLiteralToPropagate() const
{
    return !literalsToPropagate.empty();
}

unsigned int
Solver::getCurrentDecisionLevel()
{
    return currentDecisionLevel;
}

void
Solver::incrementCurrentDecisionLevel()
{
    currentDecisionLevel++;
    unrollVector.push_back( assignedLiterals.size() );
    
    assert( currentDecisionLevel == unrollVector.size() );
}

void
Solver::unrollOne()
{
    unroll( currentDecisionLevel - 1 );
}

void
Solver::onLearningClause( 
    Literal* literalToPropagate, 
    LearnedClause* learnedClause, 
    unsigned int backjumpingLevel )
{
    assert( "Backjumping level is not valid." && backjumpingLevel < currentDecisionLevel );
    unroll( backjumpingLevel );    
    
    assert( "Each learned clause has to be an asserting clause." && literalToPropagate != NULL );
    assert( "Learned clause has not been calculated." && learnedClause != NULL );
    
    Clause* clause = static_cast< Clause* >( learnedClause );
    onLiteralAssigned( literalToPropagate, TRUE, clause );
    
    deletionStrategy->onLearning( *this, learnedClause );
}

void
Solver::onLearningUnaryClause(
    Literal* literalToPropagate,
    LearnedClause* learnedClause )
{
    onRestarting();
    onLiteralAssigned( literalToPropagate, TRUE, NULL );

    assert( "Learned clause has not been calculated." && learnedClause != NULL );    
    delete learnedClause;
}

void
Solver::onRestarting()
{
    deletionStrategy->onRestarting();
    unroll( 0 );
}

void
Solver::deleteLearnedClause( 
    LearnedClause* learnedClause,
    List< LearnedClause* >::iterator iterator )
{
    learnedClause->detachClause();
    delete learnedClause;
    learnedClauses.erase( iterator );
}

unsigned int
Solver::numberOfClauses()
{
    return clauses.size();
}

unsigned int
Solver::numberOfLearnedClauses()
{
    return learnedClauses.size();
}

#endif	/* SOLVER_H */

