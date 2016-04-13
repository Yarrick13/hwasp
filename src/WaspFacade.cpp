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

#include "WaspFacade.h"

#include "Restart.h"

#include "input/GringoNumericFormat.h"

#include "outputBuilders/WaspOutputBuilder.h"
#include "outputBuilders/SilentOutputBuilder.h"
#include "outputBuilders/ThirdCompetitionOutputBuilder.h"
#include "outputBuilders/CompetitionOutputBuilder.h"
#include "outputBuilders/DimacsOutputBuilder.h"

#include "MinisatHeuristic.h"
#include "PUPHeuristic.h"
#include "ColouringHeuristic.h"
#include "BinPackingHeuristic.h"
#include "CombinedHeuristic.h"
#include "CCPHeuristic.h"
#include "StableMarriageHeuristic.h"
#include "outputBuilders/MultiOutputBuilder.h"
#include "QueryInterface.h"

#include <vector>
#include <string>

void
WaspFacade::readInput()
{
    char tmp;
    cin >> tmp;

    if( !cin.good() && !cin.eof() )
    {   
        ErrorMessage::errorDuringParsing( "Unexpected symbol." );
    }    

    cin.putback( tmp );
    switch ( tmp )
    {
        case COMMENT_DIMACS:
        case FORMULA_INFO_DIMACS:
        {
            DimacsOutputBuilder* d = new DimacsOutputBuilder();
            solver.setOutputBuilder( d );
            Dimacs dimacs( solver );
            dimacs.parse();
            if( dimacs.isMaxsat() )
                d->setMaxsat();
            greetings();
            break;
        }

        default:
        {
            GringoNumericFormat gringo( solver );
            gringo.parse();
//            solver.setOutputBuilder( new WaspOutputBuilder() );
            greetings();
            break;
        }
    }
}

void
WaspFacade::solve()
{
    if( printProgram )
    {
        solver.printProgram();
        return;
    }   
    
    if( solver.preprocessing() )
    {
        if( printDimacs )
        {
            solver.printDimacs();
            return;
        }
        
        if( queryAlgorithm != NO_QUERY )
        {
            QueryInterface queryInterface( solver );
            queryInterface.computeCautiousConsequences( queryAlgorithm );
            return;
        }
        
        if( !solver.isOptimizationProblem() )
        {            
            while( solver.solve() == COHERENT )
            {
                solver.printAnswerSet();
                trace_msg( enumeration, 1, "Model number: " << numberOfModels + 1 );
                if( ++numberOfModels >= maxModels )
                {
                    trace_msg( enumeration, 1, "Enumerated " << maxModels << "." );
                    break;
                }
                else if( !solver.addClauseFromModelAndRestart() )
                {
                    trace_msg( enumeration, 1, "All models have been found." );
                    break;
                }
            }
        }
        else
        {
            unsigned int result = solveWithWeakConstraints();
            switch( result )
            {
                case COHERENT:
                    cerr << "ERROR";
                    break;
                
                case INCOHERENT:
                    solver.foundIncoherence();
                    break;
                    
                case OPTIMUM_FOUND:
                default:
                    solver.optimumFound();
                    break;
            }
            return;
        }
    }

    if( numberOfModels == 0 )
    {
        trace_msg( enumeration, 1, "No model found." );
        solver.foundIncoherence();
    }
    
//    solver.printLearnedClauses();
}

void
WaspFacade::setDeletionPolicy(
    DELETION_POLICY deletionPolicy,
    unsigned int /*deletionThreshold*/ )
{
    switch( deletionPolicy )
    {
//        case RESTARTS_BASED_DELETION_POLICY:
//            heuristic->setDeletionStrategy( new RestartsBasedDeletionStrategy( solver ) );
//            break;
//            
//        case MINISAT_DELETION_POLICY:
//            heuristic->setDeletionStrategy( new MinisatDeletionStrategy( solver ) );
//            break;
//            
//        case GLUCOSE_DELETION_POLICY:
//            heuristic->setDeletionStrategy( new GlueBasedDeletionStrategy( solver, deletionThreshold ) );
//            break;
//
//        case AGGRESSIVE_DELETION_POLICY:
//        default:
//            heuristic->setDeletionStrategy( new AggressiveDeletionStrategy( solver ) );
//            break;
    }
}

void
WaspFacade::setDecisionPolicy(
    DECISION_POLICY decisionPolicy,
    unsigned int /*threshold*/ )
{
    switch( decisionPolicy )
    {
//        case HEURISTIC_BERKMIN:
//            assert( threshold > 0 );
//            heuristic->setDecisionStrategy( new BerkminHeuristic( solver, threshold ) );
//            break;
//            
//        case HEURISTIC_BERKMIN_CACHE:
//            assert( threshold > 0 );
//            heuristic->setDecisionStrategy( new BerkminHeuristicWithCache( solver, threshold ) );            
//            break;
//        
//        case HEURISTIC_FIRST_UNDEFINED:
//            heuristic->setDecisionStrategy( new FirstUndefinedHeuristic( solver ) );
//            break;
        
        case HEURISTIC_PUP:
        	{
				if ( heuristic_fallback )
				{
					CombinedHeuristic* cbin = new CombinedHeuristic( solver, NONE );
					cbin->addHeuristic( "pup" );
					solver.setHeuristic( cbin );
				}
				else
				{
					if ( heuristic_option.compare( "QUICKPUP" ) == 0 )
						solver.setHeuristic( new PUPHeuristic( solver, true, false, false, false ) );
					else if ( heuristic_option.compare( "QUICKPUP*" ) == 0 )
						solver.setHeuristic( new PUPHeuristic( solver, false, true, false, false ) );
					else if ( heuristic_option.compare( "PRED*" ) == 0 )
						solver.setHeuristic( new PUPHeuristic( solver, false, false, true, true ) );
					else if ( heuristic_option.compare( "PRED" ) == 0 )
						solver.setHeuristic( new PUPHeuristic( solver ) );
					else
					{
						cerr << "Invalid value for PUP heuristic option. Use QUICKPUP, QUICKPUP*, PRED or PRED*" << endl;
						exit( 0 );
					}
				}
			}
            break;

        case HEURISTIC_COLOURING:
        	{
        		if ( heuristic_fallback )
				{
					CombinedHeuristic* cbin = new CombinedHeuristic( solver, NONE );
					cbin->addHeuristic( "colouring" );
					solver.setHeuristic( cbin );
				}
				else
				{
					solver.setHeuristic( new ColouringHeuristic( solver ) );
				}
			}
        	break;

        case HEURISTIC_BINPACKING:
        	{
        		if ( heuristic_fallback )
				{
					CombinedHeuristic* cbin = new CombinedHeuristic( solver, NONE );
					cbin->addHeuristic( "binpacking" );
					solver.setHeuristic( cbin );
				}
				else
				{
					solver.setHeuristic( new BinPackingHeuristic( solver ) );
				}
        	}
        	break;

        case HEURISTIC_STABLEMARRIAGE:
        	solver.setHeuristic( new StableMarriageHeuristic( solver, 0.5, 500, 10 ) );
        	break;

        case HEURISTIC_CCP:
        	{
        		CombinedHeuristic* cbin;
        		if ( heuristic_option.compare( "A1A2" ) == 0 )
        		{
        			cbin = new CombinedHeuristic( solver, TIME, 10, false );
        			cbin->addHeuristic( "ccp heuristic", new CCPHeuristic( solver, false, true ) );
        		}
        		else if ( heuristic_option.compare( "A2F" ) == 0 )
        		{
        			cbin = new CombinedHeuristic( solver, TIME, 10, false );
        			cbin->addHeuristic( "ccp heuristic", new CCPHeuristic( solver, false, false ) );
        		}
        		else if ( heuristic_option.compare( "A2FO" ) == 0 )
				{
        			cbin = new CombinedHeuristic( solver, TIME, 10, false );
					cbin->addHeuristic( "ccp heuristic", new CCPHeuristic( solver, true, false ) );
				}
        		else if ( heuristic_option.compare( "A2AFO" ) == 0 )
        		{
        			cbin = new CombinedHeuristic( solver, TIME, 10, true );
        			cbin->addHeuristic( "ccp heuristic", new CCPHeuristic( solver, true, false ) );
        		}
        		else
        		{
					cerr << "Invalid value for CPP heuristic option. Use A1A2, A2F, A2FO or A2AFO" << endl;
					exit( 0 );
        		}

        		solver.setHeuristic( cbin );
			}
			break;

        case HEURISTIC_COMBINED:
        	{
				CombinedHeuristic* combined = new CombinedHeuristic( solver, CONFLICT, 100 );
				bool found = true;

				vector< string > heuristics;
				string h;

				for( char& c : heuristic_option )
				{
					if ( c == ';' )
					{
						heuristics.push_back( h );
						h = "";
					}
					else
						h += c;
				}
				heuristics.push_back( h );

				for ( string h : heuristics )
				{
					if ( !combined->addHeuristic( h ) )
					{
						found = false;
						cout << "Can not find heuristic " << h << endl;
					}
				}

				if ( found && heuristics.size( ) > 0 )
					solver.setHeuristic( combined );
				else
				{
					cout << "Can not use combined heuristic - using Minisat";
					decisionPolicy = HEURISTIC_MINISAT;
					solver.setHeuristic( new MinisatHeuristic( solver ) );
				}

				solver.setMinisatHeuristic( );
        	}
        	break;
        
        case HEURISTIC_MINISAT:
            solver.setHeuristic( new MinisatHeuristic( solver ) );
            solver.setMinisatHeuristic();
            break;
//    
//        default:
//            heuristic->setDecisionStrategy( new BerkminHeuristic( solver, 512 ) );
//            break;
    }
}

void
WaspFacade::setOutputPolicy(
    OUTPUT_POLICY outputPolicy )
{
    switch( outputPolicy )
    {
        case COMPETITION_OUTPUT:
            solver.setOutputBuilder( new CompetitionOutputBuilder() );
            break;
            
        case DIMACS_OUTPUT:
            solver.setOutputBuilder( new DimacsOutputBuilder() );
            break;
            
        case SILENT_OUTPUT:
            solver.setOutputBuilder( new SilentOutputBuilder() );
            break;
            
        case THIRD_COMPETITION_OUTPUT:
            solver.setOutputBuilder( new ThirdCompetitionOutputBuilder() );
            break;
            
        case MULTI:
            solver.setOutputBuilder( new MultiOutputBuilder() );
            break;
            
        case WASP_OUTPUT:
        default:
            solver.setOutputBuilder( new WaspOutputBuilder() );
            break;
    }
}

void
WaspFacade::setRestartsPolicy(
    RESTARTS_POLICY restartsPolicy,
    unsigned int threshold )
{
    assert( threshold > 0 );
    Restart* restart;
    switch( restartsPolicy )
    {
        case GEOMETRIC_RESTARTS_POLICY:
            restart = new Restart( threshold, false );
            solver.setRestart( restart );
            break;            

        case SEQUENCE_BASED_RESTARTS_POLICY:
        default:
            restart = new Restart( threshold, true );
            solver.setRestart( restart );
            break;
    }
}
