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

#include "Solver.h"

Solver::~Solver()
{
//    for( unsigned int i = 0; i < auxLiterals.size(); i++ )
//    {
//        delete auxLiterals[ i ];
//    }
    
    while( !clauses.empty() )
    {
        delete clauses.back();
        clauses.pop_back();
    }
    
    while( !learnedClauses.empty() )
    {
        delete learnedClauses.back();
        learnedClauses.pop_back();
    }
        
    if( outputBuilder != NULL )
        delete outputBuilder;
        
    if( heuristic != NULL )
        delete heuristic;
}

void
Solver::unroll(
    unsigned int level )
{
    assert( "Level is not valid." && level < unrollVector.size() && currentDecisionLevel >= level );
    assert( "Vector for unroll is inconsistent" && variables.numberOfAssignedLiterals() >= unrollVector[ level ] );    
    unsigned int toUnroll = variables.numberOfAssignedLiterals() - unrollVector[ level ];
    unsigned int toPop = currentDecisionLevel - level;
    
    currentDecisionLevel = level;
    
    while( toUnroll > 0 )
    {
        unrollLastVariable();
        toUnroll--;
    }
    
    while( toPop > 0 )
    {
        unrollVector.pop_back();
        toPop--;
    }
    
    variables.onUnroll();
}

bool
Solver::addClauseFromModelAndRestart()
{
    assert( variables.numberOfAssignedLiterals() > 0 );
    
    trace_msg( enumeration, 2, "Creating the clause representing the model." );
    Clause* clause = new Clause();
    
    for( unsigned int i = 1; i <= variables.numberOfVariables(); i++ )
    {
        Variable* v = variables[ i ];
        assert( !v->isUndefined() );
        
        trace_msg( enumeration, 3, "Checking literal " << *v << " with decision level " << v->getDecisionLevel() << " and its implicant is " << ( v->hasImplicant() ? "null" : "not null" ) );
        if( !v->hasImplicant() && v->getDecisionLevel() != 0 )
        {
            if( v->isTrue() )
            {
                Literal lit( v, NEGATIVE );
                trace_msg( enumeration, 2, "Adding literal " << lit << " in clause." );
                clause->addLiteral( lit );
            }
            else
            {
                Literal lit( v );
                trace_msg( enumeration, 2, "Adding literal " << lit << " in clause." );
                clause->addLiteral( lit );
            }
        }
    }
    
    this->doRestart();
    return addClause( clause );
}

bool 
Solver::solve()
{
    trace( solving, 1, "Starting solving.\n" );
    if( conflictDetected() || conflictAtLevelZero )
    {
        trace( solving, 1, "Conflict at level 0.\n" );
        return false;
    }

    while( hasUndefinedLiterals() )
    {
        /*
        static unsigned int PROVA = 0;
        static time_t PROVA_TIME = time( 0 );


        unsigned int end = 3000000;
        unsigned int printValue = 10000;

        if( ++PROVA > end ) {
            cerr << "PROVA END!" << endl;
            return false;
        }
        else if( ++PROVA % printValue == 0 )
        {
            cout << PROVA << " " << learnedClauses.size() <<  " " << ( time( 0 ) - PROVA_TIME ) << endl;
        }
        //*/

        assert( !conflictDetected() );
        chooseLiteral();
        
        while( hasNextVariableToPropagate() )
        {
            Variable* variableToPropagate = getNextVariableToPropagate();            
            propagate( variableToPropagate );

            if( conflictDetected() )
            {
                trace( solving, 1, "Conflict detected.\n" );
                if( getCurrentDecisionLevel() == 0 )
                {
                    trace( solving, 1, "Conflict at level 0: return. \n");
                    return false;
                }

                analyzeConflict();
                assert( hasNextVariableToPropagate() || getCurrentDecisionLevel() == 0 );
            }
        }
    }
    
    assert_msg( getNumberOfUndefined() == 0, "Found a model with " << getNumberOfUndefined() << " undefined variables." );
    assert_msg( allClausesSatisfied(), "The model found is not correct." );
    
    return true;
}

void
Solver::propagate(
    Variable* variable )
{
    assert( "Variable to propagate has not been set." && variable != NULL );
    trace( solving, 2, "Propagating: %s.\n", toString( *variable ).c_str() );
    
    Literal complement = Literal::createOppositeFromAssignedVariable( variable );
    
    variable->unitPropagationStart();
    assert( !conflictDetected() );
    while( variable->unitPropagationHasNext() && !conflictDetected() )
    {
        Clause* clause = variable->unitPropagationNext();
        assert( "Next clause to propagate is null." && clause != NULL );
        trace( solving, 5, "Considering clause %s.\n", toString( *clause ).c_str() );
        if( clause->onLiteralFalse( complement ) )
        {
            assignLiteral( clause );
            heuristic->onUnitPropagation( clause );
        }
        else
            assert( !conflictDetected() );
    }
}

void
Solver::propagateAtLevelZero(
    Variable* variable )
{
    assert( "Variable to propagate has not been set." && variable != NULL );    
    Literal literal = Literal::createFromAssignedVariable( variable );
    trace_msg( solving, 2, "Propagating " << literal << " as true at level 0" );
    literal.startIterationOverOccurrences();

    while( literal.hasNextOccurrence() )
    {
        Clause* clause = literal.nextOccurence();
        trace_msg( solving, 5, "Considering clause " << *clause );
        clause->detachClauseToAllLiterals( literal );
        deleteClause( clause );
    }

    assert( !conflictDetected() );
    Literal complement = Literal::createOppositeFromAssignedVariable( variable );
    trace_msg( solving, 2, "Propagating " << complement << " as false at level 0" );
    complement.startIterationOverOccurrences();

    while( complement.hasNextOccurrence() && !conflictDetected() )
    {
        Clause* clause = complement.nextOccurence();
        assert( "Next clause to propagate is null." && clause != NULL );
        trace( solving, 5, "Considering clause %s.\n", toString( *clause ).c_str() );
        
        clause->removeLiteral( complement );
        if( clause->size() == 1 )
        {
            if( !clause->getAt( 0 ).isTrue() )
                assignLiteral( clause->getAt( 0 ) );
            clause->detachClauseToAllLiterals( Literal::null );
            deleteClause( clause );
        }
        else
            assert( !conflictDetected() );
    }
}

void
Solver::printProgram() const
{
    for( ConstClauseIterator it = clauses.begin(); it != clauses.end(); ++it )
    {
        cout << *( *it ) << endl;
    }
}

unsigned int
Solver::getNumberOfUndefined() const
{
    unsigned countUndef = 0;
    for( unsigned int i = 1; i <= variables.numberOfVariables(); i++ )
    {
        Variable const* var = variables[ i ];
        if( var->isUndefined() )
        {
            countUndef++;
        }
    }

    return countUndef;
}

bool
Solver::allClausesSatisfied() const
{
    for( ConstClauseIterator it = clauses.begin(); it != clauses.end(); ++it )
    {
        Clause& clause = *( *it );

        if( clause.isUnsatisfied() )
            return false;
    }

    return true;
}
