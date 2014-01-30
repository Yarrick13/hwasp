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

#ifndef SOLVER_H
#define	SOLVER_H

#include <cassert>
#include <vector>
#include <iomanip>
using namespace std;

#include "Clause.h"
#include "Variable.h"
#include "Variables.h"
#include "Literal.h"
#include "util/Options.h"
#include "util/Trace.h"
#include "WaspRule.h"
#include "stl/List.h"
#include "stl/UnorderedSet.h"
#include "Learning.h"
#include "outputBuilders/OutputBuilder.h"
#include "util/Assert.h"
#include "Satelite.h"
#include "Restart.h"
#include "MinisatHeuristic.h"
#include "util/Statistics.h"
#include "util/Time.h"

extern time_t initial_time;

class Solver
{
    public:
        inline Solver();
        ~Solver();
        
        Clause* clauseFromModel;

        inline void greetings(){ if( !multi ) outputBuilder->greetings(); }
        
        inline void init();
        bool solve();
        void propagate( Variable* variable );
        void propagateAtLevelZero( Variable* variable );
        void propagateAtLevelZeroSatelite( Variable* variable );

        inline bool preprocessing();
        inline void attachWatches();
        
        inline void addVariable( const string& name );
        inline void addVariable();        
        
        inline bool addClause( Clause* clause );
        inline bool addClauseFromModel( Clause* clause );
        inline void addLearnedClause( Clause* learnedClause );
        bool addClauseFromModelAndBackjump();
//        bool addClauseFromModelAndRestart();
//        Clause* computeClauseFromModel();
        
        inline Literal getLiteral( int lit );
        inline Variable* getVariable( unsigned int var );
        
        inline Variable* getNextVariableToPropagate();
        inline bool hasNextVariableToPropagate() const;        
        
        inline Literal getOppositeLiteralFromLastAssignedVariable();
        inline bool hasNextAssignedVariable() const;
        inline void startIterationOnAssignedVariable();

        inline unsigned int getCurrentDecisionLevel();
        inline void incrementCurrentDecisionLevel();
        
        inline void assignLiteral( Literal literal );
        inline void assignLiteral( Clause* implicant );
        
        inline bool propagateLiteralAsDeterministicConsequence( Literal literal );
        inline bool propagateLiteralAsDeterministicConsequenceSatelite( Literal literal );
        
        inline void onLearningClause( Literal literalToPropagate, Clause* learnedClause, unsigned int backjumpingLevel );
        inline void onLearningUnaryClause( Literal literalToPropagate, Clause* learnedClause );        
        inline void doRestart();        
        
        inline unsigned int numberOfClauses();
        inline unsigned int numberOfLearnedClauses();        
        inline unsigned int numberOfAssignedLiterals();
        inline unsigned int numberOfVariables();
        inline unsigned int numberOfAuxVariables();
        
        inline void setAChoice( Literal choice );        
        
        inline void foundEmptyClause(){ conflictAtLevelZero = true; }
        inline bool analyzeConflict();
        inline void clearConflictStatus();
        inline bool chooseLiteral();
        inline bool conflictDetected();
        inline void foundIncoherence();
        inline bool hasUndefinedLiterals();
        inline Variable* getFirstUndefined() { return variables.getFirstUndefined(); }
        inline Variable* getNextUndefined( Variable* v ) { return variables.getNextUndefined( v ); }
        inline void printAnswerSet();        
        
        void unroll( unsigned int level );
        inline void unrollOne();
        inline void unrollLastVariable();
        
        /* OPTIONS */
        inline void setOutputBuilder( OutputBuilder* value );        
        
        typedef vector< Clause* >::iterator ClauseIterator;
        typedef vector< Clause* >::reverse_iterator ClauseReverseIterator;
        typedef vector< Clause* >::const_iterator ConstClauseIterator;
        typedef vector< Clause* >::const_reverse_iterator ConstClauseReverseIterator;
        
        inline unsigned int numberOfClauses() const { return clauses.size(); }
        inline Clause* clauseAt( unsigned int i ) { assert( i < numberOfClauses() ); return clauses[ i ]; }
        
        inline ClauseIterator clauses_begin() { return clauses.begin(); }
        inline ClauseIterator clauses_end() { return clauses.end(); }
        inline ClauseReverseIterator clauses_rbegin() { return clauses.rbegin(); }
        inline ClauseReverseIterator clauses_rend() { return clauses.rend(); }
        inline ClauseIterator learnedClauses_begin() { return learnedClauses.begin(); }
        inline ClauseIterator learnedClauses_end() { return learnedClauses.end(); }
        inline ClauseReverseIterator learnedClauses_rbegin() { return learnedClauses.rbegin(); }
        inline ClauseReverseIterator learnedClauses_rend() { return learnedClauses.rend(); }
        inline ConstClauseIterator clauses_begin() const { return clauses.begin(); }
        inline ConstClauseIterator clauses_end() const { return clauses.end(); }
        inline ConstClauseReverseIterator clauses_rbegin() const { return clauses.rbegin(); }
        inline ConstClauseReverseIterator clauses_rend() const { return clauses.rend(); }
        inline ConstClauseIterator learnedClauses_begin() const { return learnedClauses.begin(); }
        inline ConstClauseIterator learnedClauses_end() const { return learnedClauses.end(); }
        inline ConstClauseReverseIterator learnedClauses_rbegin() const { return learnedClauses.rbegin(); }
        inline ConstClauseReverseIterator learnedClauses_rend() const { return learnedClauses.rend(); }

        inline void deleteLearnedClause( ClauseIterator iterator );
        inline void deleteClause( Clause* clause );
        inline void removeClauseNoDeletion( Clause* clause );
        void deleteClauses();
        void updateActivity( Clause* learnedClause );
        inline void decrementActivity(){ deletionCounters.increment *= deletionCounters.decrement; }
        inline void onLearning( Clause* learnedClause );
        inline bool hasToDelete();
        inline void markClauseForDeletion( Clause* clause ){ satelite->onDeletingClause( clause ); clause->markAsDeleted(); }
        
        void printProgram() const;
        
//        inline void initClauseData( Clause* clause ) { assert( heuristic != NULL ); heuristic->initClauseData( clause ); }
//        inline Heuristic* getHeuristic() { return heuristic; }
        inline void onLiteralInvolvedInConflict( Literal l ) { minisatHeuristic.onLiteralInvolvedInConflict( l ); }
        inline void finalizeDeletion( unsigned int newVectorSize ) { learnedClauses.resize( newVectorSize ); }        
        
        inline void setRestart( Restart* r );
        
        void simplifyOnRestart();
        void removeSatisfied( vector< Clause* >& clauses );

        inline void onEliminatingVariable( Variable* variable, unsigned int sign, Clause* definition );
        inline void completeModel();
        
        inline Clause* newClause();
        inline void releaseClause( Clause* clause );
        
        inline void setQuery( unsigned int q ) { this->query = q; }
        
//        inline void addPreferredChoice( Variable* var ) { var->setCautiousConsequenceCandidate( true ); preferredChoices.push_back( var ); }
//        inline unsigned int numberOfPreferredChoices() const { return preferredChoices.size(); }
        
        inline void addVariableInLowerEstimate( Variable* var ) { var->setCautiousConsequence( true ); lowerEstimate.push_back( var ); }
        inline const vector< Variable* >& getLowerEstimate() const { return lowerEstimate; }
        
        inline unsigned int getQueryType() const { return query; }
        inline bool hasQuery() const { return query != NOQUERY; }
        inline bool claspApproachForQuery() const { return query == CLASPQUERY || query == CLASPQUERYRESTART; }
        inline bool waspApproachForQuery() const { return query == WASPQUERY; }
        inline bool waspFirstModelApproachForQuery() const { return query == WASPQUERYFIRSTMODEL; }
        inline bool hybridApproachForQuery() const { return query == HYBRIDQUERY; }
        inline bool iterativeApproachForQuery() const { return query == ITERATIVEQUERY; }
        
        inline void printLowerEstimate() const;
        inline void printUpperEstimate() const;
        
        inline void setFirstChoiceFromQuery( bool value ){ firstChoiceFromQuery = value; }
        inline void setShuffleAtEachRestart( bool value ){ shuffleAtEachRestart = value; }
        inline bool propagateLiteralOnRestart( Literal literal );
        
        inline void setMultiSolver( bool m ){ multi = m; }
        inline bool hasMultiSolver() const{ return multi; }                
        inline void printLearnedClauseForMultiSolver( Clause* clause, bool unary ) const;
        inline void printLiteralForMultiSolver( Literal lit ) const;
        
        inline void shrinkUpperEstimate();        
        inline unsigned int removeDeterministicConsequencesFromUpperEstimate();
        
        inline unsigned int upperEstimateSize() { return clauseFromModel->size(); }
        inline void createClauseFromModel() { assert( clauseFromModel == NULL ); clauseFromModel = newClause(); clauseFromModel->canBeSimplified = false; }
        inline void addInClauseFromModel( Variable* var ) { assert( clauseFromModel != NULL ); var->setCautiousConsequenceCandidate( true ); clauseFromModel->addLiteral( Literal( var, NEGATIVE ) ); }
        void printNames() const;
        
    private:
        inline Variable* addVariableInternal();
        
        Solver( const Solver& ) : learning( *this )
        {
            assert( "The copy constructor has been disabled." && 0 );
        }

        unsigned int currentDecisionLevel;
        Variables variables;
        
        vector< Clause* > clauses;
        vector< Clause* > learnedClauses;
        
        vector< unsigned int > unrollVector;
        
        Literal conflictLiteral;
        Clause* conflictClause;
        
        Learning learning;
        OutputBuilder* outputBuilder;        
        
        MinisatHeuristic minisatHeuristic;
        Restart* restart;
        Satelite* satelite;
        
        bool conflictAtLevelZero;
        unsigned int getNumberOfUndefined() const;
        bool allClausesSatisfied() const;
        
        bool checkForNewMessages();
        
        unsigned int assignedVariablesAtLevelZero;
        int64_t nextValueOfPropagation;
        
        uint64_t literalsInClauses;
        uint64_t literalsInLearnedClauses;
        
        unsigned int query;

        vector< Variable* > eliminatedVariables;
        vector< Clause* > poolOfClauses;
        
        //vector< Variable* > preferredChoices;
        vector< Variable* > lowerEstimate;

        bool firstChoiceFromQuery;
        bool multi;
        bool shuffleAtEachRestart;
                
        struct DeletionCounters
        {
            Activity increment;
            Activity decrement;

            double learnedSizeFactor;
            double learnedSizeIncrement;
            double maxLearned;
            unsigned int learnedSizeAdjustStartConfl;
            double learnedSizeAdjustConfl;
            double learnedSizeAdjustIncrement;
            unsigned int learnedSizeAdjustCnt;
            
            void init()
            {
                increment = 1 ;
                decrement = 1/0.999;
                learnedSizeFactor = ( ( double ) 1 / ( double) 3 );
                learnedSizeIncrement = 1.1;
                maxLearned = 0.0;
                learnedSizeAdjustStartConfl = 100;
                learnedSizeAdjustConfl = 0.0;
                learnedSizeAdjustCnt = 0;
                learnedSizeAdjustIncrement = 1.5;
            }
        } deletionCounters; 
};

Solver::Solver() 
: 
    clauseFromModel( NULL ),
    currentDecisionLevel( 0 ),
    conflictLiteral( NULL ),
    conflictClause( NULL ),
    learning( *this ),
    outputBuilder( NULL ),
    restart( NULL ),
    conflictAtLevelZero( false ),
    assignedVariablesAtLevelZero( MAXUNSIGNEDINT ),
    nextValueOfPropagation( 0 ),
    literalsInClauses( 0 ),
    literalsInLearnedClauses( 0 ),
    query( NOQUERY ),
    firstChoiceFromQuery( false ),
    shuffleAtEachRestart( true )
{
    satelite = new Satelite( *this );
    deletionCounters.init();
}

void
Solver::setOutputBuilder(
    OutputBuilder* value )
{
    assert( value != NULL );
    if( outputBuilder != NULL )
        delete outputBuilder;
    outputBuilder = value;
}

Variable*
Solver::addVariableInternal()
{
    Variable* variable = new Variable( variables.numberOfVariables() + 1 );
    variables.push_back( variable );
    assert( variables.numberOfVariables() == variable->getId() );
    minisatHeuristic.onNewVariable( variable );
    learning.onNewVariable();
    satelite->onAddingVariable( variable );
    return variable;
}

void
Solver::addVariable( 
    const string& name )
{    
    Variable* variable = addVariableInternal();
    VariableNames::setName( variable, name );    
}

void
Solver::addVariable()
{
    addVariableInternal();
}

Literal
Solver::getLiteral(
    int lit )
{
    assert( "Lit is out of range." && static_cast< unsigned >( abs( lit ) ) <= variables.numberOfVariables() && abs( lit ) > 0);
    return lit > 0 ? Literal( variables[ lit ], POSITIVE ) : Literal( variables[ -lit ], NEGATIVE );
}

Variable*
Solver::getVariable(
    unsigned int var )
{
    assert_msg( ( var > 0 && var <= variables.numberOfVariables() ), "Variable id " << var << " is greater than the number of variables: " << numberOfVariables() );
    return variables[ var ];
}

void
Solver::init()
{
    variables.init();    
}

void
Solver::assignLiteral(
    Literal literal )
{
    if( !variables.assign( currentDecisionLevel, literal ) )
    {
        conflictLiteral = literal;
        conflictClause = NULL; 
    }
}

void
Solver::assignLiteral(
    Clause* implicant )
{
    assert( implicant != NULL );
    if( !variables.assign( currentDecisionLevel, implicant ) )
    {
        conflictLiteral = implicant->getAt( 0 );
        conflictClause = implicant;        
    }
}

bool
Solver::addClause(
    Clause* clause )
{
    assert( clause != NULL );
    unsigned int size = clause->size();    
    if( size > 1 )
    {
        statistics( onAddingClause( size ) );
//        clause->attachClause();
        clause->attachClauseToAllLiterals();
        clause->setPositionInSolver( clauses.size() );
        clauses.push_back( clause );
        return true;
    }

    if( size == 1 )
    {
        Literal literal = clause->getAt( 0 );
        if( !literal.isTrue() && !propagateLiteralAsDeterministicConsequence( literal ) )
        {
            conflictLiteral = literal;
        }
        else
        {
            releaseClause( clause );
//            delete clause;
            return true;
        }
    }

    conflictAtLevelZero = true;
    releaseClause( clause );
//    delete clause;
    return false;
}

bool
Solver::addClauseFromModel(
    Clause* clause )
{
    assert( clause != NULL );
    unsigned int size = clause->size();
    if( size > 1 )
    {
        statistics( onAddingClause( size ) );
        clause->attachClause();
        clause->setPositionInSolver( clauses.size() );
        clauses.push_back( clause );
        return true;
    }

    if( size == 1 )
    {
        assignLiteral( clause->getAt( 0 ) );
        releaseClause( clause );

        clearConflictStatus();
        while( hasNextVariableToPropagate() )
        {
            nextValueOfPropagation--;
            Variable* variableToPropagate = getNextVariableToPropagate();
            propagate( variableToPropagate );

            if( conflictDetected() )            
                return false;
        }
    }

    return true;
}

void
Solver::addLearnedClause( 
    Clause* learnedClause )
{
    assert( learnedClause != NULL );
    learnedClause->attachClause();
    learnedClauses.push_back( learnedClause );
    literalsInLearnedClauses += learnedClause->size();
}

Variable*
Solver::getNextVariableToPropagate()
{
    assert( variables.hasNextVariableToPropagate() );
    return variables.getNextVariableToPropagate();
}
        
bool
Solver::hasNextVariableToPropagate() const
{
    return variables.hasNextVariableToPropagate();
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
    unrollVector.push_back( variables.numberOfAssignedLiterals() );
    
    assert( currentDecisionLevel == unrollVector.size() );
}

void
Solver::unrollLastVariable()
{    
    minisatHeuristic.onUnrollingVariable( variables.unrollLastVariable() );
}

void
Solver::unrollOne()
{
    unroll( currentDecisionLevel - 1 );
}

#include <cstdio>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
bool hasNewInput();

void
Solver::doRestart()
{
    statistics( onRestart() );
    trace( solving, 2, "Performing restart.\n" );
    restart->onRestart();
    unroll( 0 );
}

void
Solver::deleteLearnedClause( 
    ClauseIterator iterator )
{
    Clause* learnedClause = *iterator;
    trace_msg( solving, 4, "Deleting learned clause " << *learnedClause );
    learnedClause->detachClause();
    literalsInLearnedClauses -= learnedClause->size();
    releaseClause( learnedClause );
//    delete learnedClause;
//    learnedClauses.erase( iterator );
}

void
Solver::deleteClause( 
    Clause* clause )
{
    unsigned int position = clause->getPositionInSolver();
    assert_msg( position < clauses.size(), "Position " << position << " is greater than size of the clauses vector " << clauses.size() );
    assert_msg( clause == clauses[ position ], "The clause to delete " << *clause << " is not equal to the clause " << clauses[ position ] << " in position " << position  );
    trace_msg( solving, 4, "Deleting clause " << *clause );

    clauses[ position ] = clauses.back();
    trace_msg( solving, 6, "Swapping clause " << *clause << " and " << *clauses[ position ] );
    clauses[ position ]->setPositionInSolver( position );
    clauses.pop_back();
//    delete clause;
    releaseClause( clause );
}

void
Solver::removeClauseNoDeletion( 
    Clause* clause )
{
    satelite->onDeletingClause( clause );
    unsigned int position = clause->getPositionInSolver();
    assert_msg( position < clauses.size(), "Position " << position << " is greater than size of the clauses vector " << clauses.size() );
    assert_msg( clause == clauses[ position ], "The clause to delete " << *clause << " is not equal to the clause " << clauses[ position ] << " in position " << position  );
    trace_msg( solving, 4, "Deleting clause " << *clause );

    clauses[ position ] = clauses.back();
    trace_msg( solving, 6, "Swapping clause " << *clause << " and " << *clauses[ position ] );
    clauses[ position ]->setPositionInSolver( position );
    clauses.pop_back();
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

bool
Solver::conflictDetected()
{
    return conflictLiteral != Literal::null;
}

bool
Solver::hasUndefinedLiterals()
{
    return variables.numberOfAssignedLiterals() < variables.numberOfVariables();
}

void
Solver::printAnswerSet()
{
    variables.printAnswerSet( outputBuilder );
}

void
Solver::foundIncoherence()
{
    outputBuilder->onProgramIncoherent();
}

bool
Solver::chooseLiteral()
{
    Literal choice;
    if( currentDecisionLevel == 0 && multi )
        if( !checkForNewMessages() )
            return false;

    if( currentDecisionLevel == 0 && firstChoiceFromQuery )
    {
        assert( clauseFromModel->size() != 0 );
        if( shuffleAtEachRestart )
        {
            static unsigned int counter = 0;
            if( hybridApproachForQuery() && ++counter % 32 != 0 )
            {
                goto normalChoice;
            }
            unsigned int oldSize = lowerEstimate.size();
            unsigned int maxIndex = removeDeterministicConsequencesFromUpperEstimate();
            
            if( oldSize != lowerEstimate.size() )
                printLowerEstimate();

            if(  clauseFromModel->size() == 0 )
            {
                if( hybridApproachForQuery() )
                    goto normalChoice; 
                else
                    return false;
            }

            clauseFromModel->swapLiteralsNoWatches( maxIndex, 0 );
//            Variable* tmp = preferredChoices[ maxIndex ];
//            preferredChoices[ maxIndex ] = preferredChoices[ 0 ];
//            preferredChoices[ 0 ] = tmp;
            
        }
        else
        {
            static unsigned int initialSize = clauseFromModel->size();
            if( !clauseFromModel->getAt( 0 ).isUndefined() || initialSize != clauseFromModel->size() )
            {
                unsigned int oldSize = lowerEstimate.size();
                unsigned int maxIndex = removeDeterministicConsequencesFromUpperEstimate();

                if( oldSize != lowerEstimate.size() )
                    printLowerEstimate();

                if( clauseFromModel->size() == 0 )
                    return false;

                clauseFromModel->swapLiteralsNoWatches( 0, maxIndex );                
                initialSize = clauseFromModel->size();
            }
        }
        
        choice = clauseFromModel->getAt( 0 );//minisatHeuristic.makeAChoice( preferredChoices );
    }
    else
    {
        normalChoice:;        
        choice = minisatHeuristic.makeAChoice();
    }
    trace( solving, 1, "Choice: %s.\n", toString( choice ).c_str() );
    setAChoice( choice );    
    statistics( onChoice() );
    
    return true;
}

bool
Solver::analyzeConflict()
{
    Clause* learnedClause = learning.onConflict( conflictLiteral, conflictClause );
    assert( "Learned clause has not been calculated." && learnedClause != NULL );
    statistics( onLearning( learnedClause->size() ) );
    
    if( learnedClause->size() == 1 )
    {
        if( multi )
            printLearnedClauseForMultiSolver( learnedClause, true );

        doRestart();
        
        Literal lit = learnedClause->getAt( 0 );        
        releaseClause( learnedClause );

        clearConflictStatus();
        
        if( !propagateLiteralOnRestart( lit ) )
            return false;
//        while( hasNextVariableToPropagate() )
//        {
//            nextValueOfPropagation--;            
//            Variable* variableToPropagate = getNextVariableToPropagate();
//            propagate( variableToPropagate );
//
//            if( conflictDetected() )
//                return false;
//        }
        
        simplifyOnRestart();                
    }
    else
    {
        //Be careful. UIP should be always in position 0.
        assert( learnedClause->getAt( 0 ).getDecisionLevel() == currentDecisionLevel );
        assert( learnedClause->getAt( 1 ).getDecisionLevel() == learnedClause->getMaxDecisionLevel( 1, learnedClause->size() ) );
        addLearnedClause( learnedClause );
        
        if( multi && learnedClause->size() == 2 )
            printLearnedClauseForMultiSolver( learnedClause, false );


        if( restart->hasToRestart() )
        {
            doRestart();
            simplifyOnRestart();
        }
        else
        {
            assert( learnedClause->getAt( 1 ).getDecisionLevel() != 0 );
            assert( "Backjumping level is not valid." && learnedClause->getAt( 1 ).getDecisionLevel() < currentDecisionLevel );
            trace( solving, 2, "Learned clause and backjumping to level %d.\n", learnedClause->getAt( 1 ).getDecisionLevel() );
            unroll( learnedClause->getAt( 1 ).getDecisionLevel() );    
            
            assert( "Each learned clause has to be an asserting clause." && learnedClause->getAt( 0 ) != Literal::null );
            
            assignLiteral( learnedClause );
            
            onLearning( learnedClause );  // FIXME: this should be moved outside
        }
    }

    if( --deletionCounters.learnedSizeAdjustCnt == 0 )
    {
        deletionCounters.learnedSizeAdjustConfl *= deletionCounters.learnedSizeAdjustIncrement;
        deletionCounters.learnedSizeAdjustCnt = ( unsigned int ) deletionCounters.learnedSizeAdjustConfl;
        deletionCounters.maxLearned *= deletionCounters.learnedSizeIncrement;
    }
    
    clearConflictStatus();
    
    return true;
}

void
Solver::clearConflictStatus()
{
    conflictLiteral = Literal::null;
    conflictClause = NULL;
}

unsigned int
Solver::numberOfAssignedLiterals()
{
    return variables.numberOfAssignedLiterals();
}

unsigned int
Solver::numberOfVariables()
{
    return variables.numberOfVariables();
}

void
Solver::setAChoice(
    Literal choice )
{
    assert( choice != Literal::null );
    incrementCurrentDecisionLevel();
    assert( choice.isUndefined() );
    assignLiteral( choice );
}

Literal
Solver::getOppositeLiteralFromLastAssignedVariable()
{
    return variables.getOppositeLiteralFromLastAssignedVariable();
}

bool
Solver::hasNextAssignedVariable() const
{
    return variables.hasNextAssignedVariable();
}

void
Solver::startIterationOnAssignedVariable()
{
    variables.startIterationOnAssignedVariable();
}

bool
Solver::propagateLiteralAsDeterministicConsequence(
    Literal literal )
{
    assignLiteral( literal );
    if( conflictDetected() )
        return false;

    while( hasNextVariableToPropagate() )
    {
        Variable* variableToPropagate = getNextVariableToPropagate();
        propagateAtLevelZero( variableToPropagate );

        if( conflictDetected() )
            return false;
    }    
    assert( !conflictDetected() );

    return true;
}

bool
Solver::propagateLiteralAsDeterministicConsequenceSatelite(
    Literal literal )
{
    assignLiteral( literal );
    if( conflictDetected() )
        return false;

    while( hasNextVariableToPropagate() )
    {
        Variable* variableToPropagate = getNextVariableToPropagate();        
        propagateAtLevelZeroSatelite( variableToPropagate );

        if( conflictDetected() )
            return false;
    }    
    assert( !conflictDetected() );

    return true;
}

void
Solver::attachWatches()
{
    for( unsigned int i = 0; i < clauses.size(); )
    {
        Clause* current = clauses[ i ];
        if( current->hasBeenDeleted() )
        {
            deleteClause( current );
        }
        else
        {
            literalsInClauses += current->size();
            current->attachClause();
            ++i;
        }
    }    
}

bool
Solver::preprocessing()
{
    if( conflictDetected() || conflictAtLevelZero )
    {
        trace( solving, 1, "Conflict at level 0.\n" );
        return false;
    }    

    statistics( beforePreprocessing( numberOfVariables() - numberOfAssignedLiterals(), numberOfClauses() ) );
    assert( satelite != NULL );        
    if( !satelite->simplify() )
        return false;

    minisatHeuristic.simplifyVariablesAtLevelZero();    
    attachWatches();    
    
    assignedVariablesAtLevelZero = numberOfAssignedLiterals();
    
    deletionCounters.maxLearned = numberOfClauses() * deletionCounters.learnedSizeFactor;
    deletionCounters.learnedSizeAdjustConfl = deletionCounters.learnedSizeAdjustStartConfl;
    deletionCounters.learnedSizeAdjustCnt = ( unsigned int ) deletionCounters.learnedSizeAdjustConfl;
    
    statistics( afterPreprocessing( numberOfVariables() - numberOfAssignedLiterals(), numberOfClauses() ) );

    return true;
}

void
Solver::onLearning(
    Clause* learnedClause )
{
    updateActivity( learnedClause );
    decrementActivity();    
}

bool
Solver::hasToDelete()
{
    return ( ( int ) ( numberOfLearnedClauses() - numberOfAssignedLiterals() ) >= deletionCounters.maxLearned );
}

void
Solver::setRestart(
    Restart* r )
{
    if( restart != NULL )
        delete restart;
    
    assert( r != NULL );    
    restart = r;
}

void
Solver::onEliminatingVariable(
    Variable* variable,
    unsigned int sign,
    Clause* definition )
{
    variables.onEliminatingVariable( variable, sign, definition );
    eliminatedVariables.push_back( variable );
}

void
Solver::completeModel()
{
    trace_msg( satelite, 5, "Completing the model for eliminated variables" );  
    for( int i = eliminatedVariables.size() - 1; i >= 0; i-- )    
    {
        Variable* back = eliminatedVariables[ i ];

        assert( back->hasBeenEliminated() );
        unsigned int sign = back->getSignOfEliminatedVariable();
    
        trace_msg( satelite, 5, "Processing variable " << *back );
        if( sign == ELIMINATED_BY_DISTRIBUTION )
        {
            trace_msg( satelite, 5, "Eliminated by distribution " << *back );
            bool found = false;            
            Literal positiveLiteral( back, POSITIVE );
            positiveLiteral.startIterationOverOccurrences();
            while( positiveLiteral.hasNextOccurrence() )
            {
                Clause* clause = positiveLiteral.nextOccurence();
                assert( clause->hasBeenDeleted() );
                if( !clause->isSatisfied() )
                {
                    back->setUndefinedBrutal();                    
                    #ifndef NDEBUG
                    bool result =
                    #endif
                    positiveLiteral.setTrue();
                    assert( result );
                    found = true;
                    
                    trace_msg( satelite, 5, "Clause " << *clause << " is not satisfied: inferring " << positiveLiteral );
                    break;
                }
            }            
            if( !found )
            {
                Literal negativeLiteral( back, NEGATIVE );
                negativeLiteral.startIterationOverOccurrences();
                while( negativeLiteral.hasNextOccurrence() )
                {
                    Clause* clause = negativeLiteral.nextOccurence();                    
                    assert( clause->hasBeenDeleted() );
                    if( !clause->isSatisfied() )                        
                    {
                        back->setUndefinedBrutal();

                        #ifndef NDEBUG
                        bool result =
                        #endif
                        negativeLiteral.setTrue();                            
                        assert( result );
                        
                        trace_msg( satelite, 5, "Clause " << *clause << " is not satisfied: inferring " << negativeLiteral );
                        break;
                    }
                }
            }
        }
        else
        {            
            assert( sign == POSITIVE || sign == NEGATIVE );
        
            Literal literal( back, sign );
            back->setUndefinedBrutal();
            const Clause* definition = back->getDefinition();
            trace_msg( satelite, 5, "Considering variable " << *back << " and its definition " << *definition << " which is " << ( definition->isSatisfied() ? "satisfied" : "unsatisfied" ) );
            #ifndef NDEBUG
            bool result =
            #endif
            definition->isSatisfied() ? literal.getOppositeLiteral().setTrue() : literal.setTrue();                

            assert( result );
            trace_msg( satelite, 5, "Inferring " << ( definition->isSatisfied() ? literal.getOppositeLiteral() : literal ) );            
        }
    }
}

Clause*
Solver::newClause()
{
    if( poolOfClauses.empty() )
    {
        unsigned int bufferSize = 20;
        for( unsigned int i = 0; i < bufferSize; i++ )
            poolOfClauses.push_back( new Clause() );       
    }
    
    Clause* back = poolOfClauses.back();
    poolOfClauses.pop_back();
    return back;
}

void
Solver::releaseClause(
    Clause* clause )
{
    assert( find( poolOfClauses.begin(), poolOfClauses.end(), clause ) == poolOfClauses.end() );
    clause->free();    
    poolOfClauses.push_back( clause );
}

void
Solver::printLowerEstimate() const
{
    if( multi )
    {
        static unsigned i = 0;
        cout << "c";
        for( ; i < lowerEstimate.size(); i++ )
            cout << " " << lowerEstimate[ i ]->getId();
        cout << endl;
    }
    else
    {
        printTime( cout );
        cout << "Certain answers (" << lowerEstimate.size() << "):" << endl;
        for( unsigned int i = 0; i < lowerEstimate.size(); i++ )
            cout << *lowerEstimate[ i ] << " ";
        cout << endl;
    }
}

void
Solver::printUpperEstimate() const
{
    if( multi )
    {
        cout << "p";
        for( unsigned int i = 0; i < clauseFromModel->size(); i++ )
            cout << " " << clauseFromModel->getAt( i ).getVariable()->getId();
        cout << endl;
    }
    else
    {
        printTime( cout );
        cout << "Possible answers (" << ( clauseFromModel->size() + lowerEstimate.size() ) << "; " << clauseFromModel->size() << "):" << endl;
        for( unsigned int i = 0; i < clauseFromModel->size(); i++ )
            cout << " " << *( clauseFromModel->getAt( i ).getVariable() );
        cout << endl;    
    }
}

bool
Solver::propagateLiteralOnRestart(
    Literal literal )
{
    assignLiteral( literal );
    
    unsigned int initialLowerEstimateSize = lowerEstimate.size();
    while( hasNextVariableToPropagate() )
    {
        nextValueOfPropagation--;            
        Variable* variableToPropagate = getNextVariableToPropagate();
        
        if( variableToPropagate->isTrue() && variableToPropagate->isCautiousConsequenceCandidate() )
            addVariableInLowerEstimate( variableToPropagate );

        propagate( variableToPropagate );

        if( conflictDetected() )
            return false;
    }
   
    if( lowerEstimate.size() > initialLowerEstimateSize )
        printLowerEstimate();
    simplifyOnRestart();    
    
    return true;
}

void
Solver::printLiteralForMultiSolver(
    Literal lit ) const
{
    cout << ( lit.getSign() == NEGATIVE ? "-" : "" ) << lit.getVariable()->getId();
}

void
Solver::printLearnedClauseForMultiSolver(
    Clause* learnedClause,
    bool unary ) const
{
    if( unary )
    {
        cout << "u ";
        printLiteralForMultiSolver( learnedClause->getAt( 0 ) );
        cout << endl;
    }
    else
    {
        assert( learnedClause->size() == 2 );
        cout << "b ";
        printLiteralForMultiSolver( learnedClause->getAt( 0 ) );
        cout << " ";
        printLiteralForMultiSolver( learnedClause->getAt( 1 ) );
        cout << endl;
    }
}

void
Solver::shrinkUpperEstimate()
{
    for( unsigned int i = 0; i < clauseFromModel->size(); )
    {
        Variable* var = clauseFromModel->getAt( i ).getVariable();
        assert( !var->isUndefined() );
        if( !var->isTrue() )
        {
            var->setCautiousConsequenceCandidate( false );
            clauseFromModel->swapLiteralsNoWatches( i, clauseFromModel->size() - 1 );
            clauseFromModel->removeLastLiteralNoWatches();
        }
        else
            ++i;
    }
}

unsigned int
Solver::removeDeterministicConsequencesFromUpperEstimate()
{
    Activity maxAct = 0;
    unsigned int maxIndex = 0;

    for( unsigned int i = 0; i < clauseFromModel->size(); )
    {
        Variable* var = clauseFromModel->getAt( i ).getVariable();
        if( !var->isUndefined() )
        {
            assert( var->getDecisionLevel() == 0 );                    
//            if( var->isTrue() && !hybridApproachForQuery() )                                    
//                addVariableInLowerEstimate( var );

            var->setCautiousConsequenceCandidate( false );
            clauseFromModel->swapLiteralsNoWatches( i, clauseFromModel->size() - 1 );
            clauseFromModel->removeLastLiteralNoWatches();
        }
        else
        {
            if( clauseFromModel->getAt( i ).getVariable()->activity() > maxAct )
            {
                maxAct = clauseFromModel->getAt( i ).getVariable()->activity();
                maxIndex = i;
            }
            ++i;
        }
    }
    
    return maxIndex;
}

#endif	/* SOLVER_H */

