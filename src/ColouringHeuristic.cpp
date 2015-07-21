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

#include "ColouringHeuristic.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include "Solver.h"
#include "util/Assert.h"
#include "util/Constants.h"
#include "util/VariableNames.h"
#include "util/HeuristicUtil.h"

ColouringHeuristic::ColouringHeuristic(
    Solver& s ) : Heuristic( s ), index( 0 ), firstChoiceIndex( 0 ), conflictOccured( false )
{
}

void
ColouringHeuristic::processVariable (
    Var v )
{
	string name = VariableNames::getName( v );

	name.erase(std::remove(name.begin(),name.end(),' '),name.end());

	string tmp;
	string tmp2;

	if( name.compare( 0, 5, "node(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp );

		Node node;
		node.variable = v;
		node.name = tmp;
		node.degree = 0;

		nodes.push_back( node );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( node )" );
	}
	else if( name.compare( 0, 5, "link(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		Link link;
		link.variable = v;
		link.from = tmp;
		link.to = tmp2;

		links.push_back( link );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( link )" );
	}
	else if( name.compare( 0, 7, "colour(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp );

		Colour col;
		col.variable = v;
		col.name = tmp;

		colours.push_back( col );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( colour )" );
	}
	else if( name.compare( 0, 13, "chosenColour(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		ColourAssignment ca;
		ca.variable = v;
		ca.node = tmp;
		ca.colour = tmp2;

		colourAssignments.push_back( ca );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( colour assignment )" );
	}
}

void
ColouringHeuristic::onFinishedParsing (
	)
{
	string order = "";

	trace_msg( heuristic, 1, "Initializing colouring heuristic" );
	trace_msg( heuristic, 2, "Start processing variables" );

	for ( Var variable : variables )
	{
		if ( !VariableNames::isHidden( variable ) )
			processVariable( variable );
	}

	trace_msg( heuristic, 2, "Initializing vertices" );

	initDegree( );
	initUsedIn( );

	trace_msg( heuristic, 1, "Start heuristic" );
	trace_msg( heuristic, 2, "Creating order" );

	quicksort( nodes, 0, nodes.size( ) );

	for ( unsigned int i = 0; i < nodes.size( ); i++ )
		order += nodes[ i ].name + ", ";

	trace_msg( heuristic, 3, "Considering order " + order );
}

void
ColouringHeuristic::initDegree(
	)
{
	for ( unsigned int i = 0; i < nodes.size( ); i++ )
	{
		for ( Link l : links )
		{
			if ( nodes[ i ].name == l.from )
				nodes[ i ].degree++;
		}
	}
}

void
ColouringHeuristic::initUsedIn(
	)
{
	for ( unsigned int i = 0; i < nodes.size( ); i++ )
	{
		for ( unsigned int j = 0; j < colourAssignments.size( ); j++ )
		{
			if ( nodes[ i ].name == colourAssignments[ j ].node )
			{
				nodes[ i ].usedIn.push_back( &colourAssignments[ j ] );
				nodes[ i ].tried.push_back( false );
			}
		}
	}
}

Literal
ColouringHeuristic::makeAChoiceProtected( )
{
	Var chosenVariable = 0;
	Node* current;
	bool found = false;

	if ( conflictOccured )
	{
		unsigned int conflictIndex = 0;

		for ( conflictIndex = 0; conflictIndex < nodes.size( ) && !found; conflictIndex++ )
		{
			if ( solver.getTruthValue( nodes[ conflictIndex ].usedIn[ nodes[ conflictIndex ].current ]->variable ) != TRUE )
			{
				trace_msg( heuristic, 3, "Reset to node " << nodes[ conflictIndex ].name << " due to conflict" );
				index = conflictIndex;
				found = true;
			}
		}

		found = true;
		for ( ; conflictIndex < nodes.size( ) && found; conflictIndex++ )
		{
			found = false;
			for ( unsigned int i = 0; i < nodes[ conflictIndex ].tried.size( ); i++ )
			{
				if ( nodes[ conflictIndex ].tried[ i ] == true )
				{
					nodes[ conflictIndex ].tried[ i ] = false;
					found = true;
				}
			}
		}

		conflictOccured = false;
	}

	do
	{
		chosenVariable = 0;

		do
		{
			current = &nodes[ index++ ];

			found = false;
			for ( unsigned int i = 0; i < current->usedIn.size( ) && !found; i++ )
			{
				if ( solver.getTruthValue( current->usedIn[ i ]->variable ) == TRUE )
				{
					found = true;
					current->tried[ i ] = true;
					current->current = i;

					trace_msg( heuristic, 3, "Node " << current->name << " is already assigned with "
													 << current->usedIn[ i ]->variable << " " << Literal(current->usedIn[ i ]->variable, POSITIVE)
													 << " -> continue with next node");
				}
			}
		}
		while( found );

		unsigned int choice = 0;
		if ( index == ( firstChoiceIndex + 1 ) )
		{
			choice = firstChoiceIndex % colours.size( );
			firstChoiceIndex++;
		}

		found = false;
		for ( unsigned int i = choice; i < current->usedIn.size() && !found; i++ )
		{
			if ( current->tried[ i ] == false )
			{
				current->tried[ i ] = true;
				current->current = i;

				found = true;
				chosenVariable = current->usedIn[ i ]->variable;
			}
		}

		if ( chosenVariable == 0 )
		{
			trace_msg( heuristic, 3, "Chosen variable is zero"  );

			if ( index > 1 )
			{
				trace_msg( heuristic, 3, "No more possibilities to place this node -> go one step back"  );
				index -= 2;		// -2 because the index has already been incremented

				while ( solver.getTruthValue( nodes[ index ].usedIn[ nodes[ index ].current ]->variable) != UNDEFINED )
					solver.unrollOne( );

				found = true;
				for ( unsigned int conflictIndex = index + 1 ; conflictIndex < nodes.size( ) && found; conflictIndex++ )
				{
					found = false;
					for ( unsigned int i = 0; i < nodes[ conflictIndex ].tried.size( ); i++ )
					{
						if ( nodes[ conflictIndex ].tried[ i ] == true )
						{
							nodes[ conflictIndex ].tried[ i ] = false;
							found = true;
						}
					}
				}
			}
			else
			{
				trace_msg( heuristic, 3, "Graph can not be coloured" );
				return Literal::null;
			}
		}
		else
		{
			trace_msg( heuristic, 3, "Chosen variable is "<< chosenVariable << " " << Literal( chosenVariable, POSITIVE ) );

			if ( solver.getTruthValue( chosenVariable ) == FALSE )
			{
				trace_msg( heuristic, 4, "Chosen variable is already set to FALSE - try another assignment" );
				index--;
			}
			if ( solver.getTruthValue( chosenVariable ) == TRUE )
			{
				trace_msg( heuristic, 4, "Chosen variable is already set to TRUE - continue with next node" );
			}
		}
	}
	while( chosenVariable == 0 || !solver.isUndefined( chosenVariable ) );

	return Literal( chosenVariable, POSITIVE );
}


void
ColouringHeuristic::quicksort(
	vector< Node >& nodes,
	unsigned int p,
	unsigned int q)
{
	unsigned int r;
    if ( p < q )
    {
        r = partition ( nodes, p, q );
        quicksort ( nodes, p, r );
        quicksort ( nodes, r + 1, q );
    }
}


int
ColouringHeuristic::partition(
	vector< Node >& nodes,
	unsigned int p,
	unsigned int q)
{
    Node node = nodes[ p ];
    unsigned int i = p;
    unsigned int j;

    for ( j = p + 1; j < q; j++ )
    {
        if ( nodes[ j ].degree > node.degree )
        {
            i = i + 1;
            swap ( nodes[ i ], nodes[ j ] );
        }

    }

    swap ( nodes[ i ], nodes[ p ] );
    return i;
}
