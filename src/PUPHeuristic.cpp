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

#include "PUPHeuristic.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include "Solver.h"
#include "util/Assert.h"
#include "util/Constants.h"
#include "util/VariableNames.h"
#include "util/HeuristicUtil.h"

PUPHeuristic::PUPHeuristic( Solver& s ) :
    Heuristic( s ),  startAt( 0 ), index( 0 ), maxPu( 2 ), maxElementsOnPu( 2 ), isConsitent( true ), conflictOccured( false ),
	conflictHandled( true ), assignedSinceConflict( 0 ), redoAfterConflict( false )
{ }

/*
 * Processes all variables related to the PUP
 * 		read zones, sensors, partner units and all unit2zone/unit2sensors
 * 		and put them in to the corresponding vectors
 *
 * 	@param v	the variable to process
 */
void
PUPHeuristic::processVariable (
    Var v )
{
	string name = VariableNames::getName( v );

	name.erase(std::remove(name.begin(),name.end(),' '),name.end());

	string tmp;
	string tmp2;

	if( name.compare( 0, 5, "zone(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp );

		Node zone;
		zone.name = tmp;
		zone.var = v;
		zone.considered = 0;
		zone.type = ZONE;

		zones.push_back( zone );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( zone )" );
	}
	else if( name.compare( 0, 11, "doorSensor(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp );

		Node sensor;
		sensor.name = tmp;
		sensor.var = v;
		sensor.considered = 0;
		sensor.type = SENSOR;

		sensors.push_back( sensor );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( sensor )" );
	}
	else if( name.compare( 0, 12, "zone2sensor(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		Connection c;
		c.from = tmp;
		c.to = tmp2;
		c.var = v;

		zone2sensor.push_back( c );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( zone2sensor )" );
	}
	else if( name.compare( 0, 8, "comUnit(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp );

		Pu pu;
		pu.name = tmp;
		pu.var = v;

		partnerUnits.push_back( pu );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( partnerunit )" );
	}
	else if( name.compare( 0, 10, "unit2zone(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.positive = v;

		initUnitAssignments( za, U2Z );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( unit2zone )" );
	}
	else if( name.compare( 0, 12, "unit2sensor(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.positive = v;

		initUnitAssignments( za, U2S );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( unit2sensor )" );
	}
	else if( name.compare( 0, 12, "maxElements(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp );

		maxElementsOnPu = strtoul( tmp.c_str(), NULL, 0 );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( maxElements )" << " with max. " << maxElementsOnPu << " elements on a PU");
	}
	else if( name.compare( 0, 6, "maxPU(" ) == 0 )
	{
		HeuristicUtil::getName( name, &tmp );

		maxPu = strtoul( tmp.c_str(), NULL, 0 );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( maxPu )" << " with max. " << maxPu << " partners on a PU" );
	}
}

/*
 * initialize the heuristic after parsing the input
 * 		initialize zone assingment vectors and create the first order
 */
void
PUPHeuristic::onFinishedParsing (
	)
{
	trace_msg( heuristic, 1, "Initializing QuickPuP heuristic" );
	trace_msg( heuristic, 2, "Start processing variables" );

	for ( Var variable : variables )
	{
		if ( !VariableNames::isHidden( variable ) )
			processVariable( variable );
	}

	initRelation( );

	if ( zones.size( ) > ( partnerUnits.size( ) * maxElementsOnPu ) || sensors.size( ) > ( partnerUnits.size( ) * maxElementsOnPu ) )
		isConsitent = false;
	else
		isConsitent = resetHeuristic( );

	trace_msg( heuristic, 1, "Start heuristic" );
}

/*
 * initialize the connection between zones and sensors
 */
void
PUPHeuristic::initRelation(
	)
{
	Node *z;
	Node *s;
	bool found;

	trace_msg( heuristic, 2, "Creating zone-to-sensor relation" );

	for ( Connection c : zone2sensor)
	{
		found = false;
		for ( unsigned int i = 0; i < zones.size() && !found; i++ )
		{
			if ( c.from == zones[ i ].name )
			{
				z = &zones[ i ];
				found = true;
			}
		}

		found = false;
		for ( unsigned int i = 0; i < sensors.size() && !found; i++ )
		{
			if ( c.to == sensors[ i ].name )
			{
				s = &sensors[ i ];
				found = true;
			}
		}

		z->children.push_back( s );
		s->children.push_back( z );
	}

	trace_msg( heuristic, 2, "Creating 'usedIn' relation" );

	ZoneAssignment* za;
	for ( unsigned int i = 0; i < unit2zone.size( ); i++ )
	{
		za = &unit2zone[ i ];

		found = false;
		for ( unsigned int j = 0; j < zones.size( ) && !found; j++ )
		{
			if ( zones[ j ].name == za->to )
			{
				zones[ j ].usedIn.push_back( za );
				found = true;
			}
		}

		found = false;
		for ( unsigned int j = 0; j < partnerUnits.size( ) && !found; j++)
		{
			if ( partnerUnits[ j ].name == za->pu )
			{
				partnerUnits[ j ].usedIn.push_back( za );
				found = true;
			}
		}
	}

	for ( unsigned int i = 0; i < unit2sensor.size( ); i++ )
	{
		za = &unit2sensor[ i ];

		found = false;
		for ( unsigned int j = 0; j < sensors.size( ) && !found; j++ )
		{
			if ( sensors[ j ].name == za->to )
			{
				sensors[ j ].usedIn.push_back( za );
				found = true;
			}
		}

		found = false;
		for ( unsigned int j = 0; j < partnerUnits.size( ) && !found; j++)
		{
			if ( partnerUnits[ j ].name == za->pu )
			{
				partnerUnits[ j ].usedIn.push_back( za );
				found = true;
			}
		}
	}
}

/*
 * reset heuristic
 * 		( in case of creating a new order )
 */
bool
PUPHeuristic::resetHeuristic (
	)
{
	assignments.clear( );
	index = 0;

	solver.unrollToZero( );
	solver.clearConflictStatus( );

	return createOrder( );
}

/*
 * initialize zone assignments
 *
 * @param za	the zone assingment
 * @param sign	positive or negative atom
 * @param type 	sensor or zone
 */
void
PUPHeuristic::initUnitAssignments(
	ZoneAssignment za,
	unsigned int type )
{
	bool found = false;
	unsigned int i;

	if ( type == U2Z )
	{
		for ( i = 0; i < unit2zone.size( ) && !found; i++ )
		{
			if ( unit2zone[ i ].pu == za.pu && unit2zone[ i ].to == za.to )
			{
				unit2zone[ i ].positive = za.positive;
				found = true;
			}
		}
	}
	else if ( type == U2S )
	{
		for ( i = 0; i < unit2sensor.size( ) && !found; i++ )
		{
			if ( unit2sensor[ i ].pu == za.pu && unit2sensor[ i ].to == za.to )
			{
				unit2sensor[ i ].positive = za.positive;
				found = true;
			}
		}
	}

	if ( !found )
	{
		if ( type == U2Z)
			unit2zone.push_back( za );
		else if ( type == U2S )
			unit2sensor.push_back( za );
	}
}

/*
 * create new order
 */
bool
PUPHeuristic::createOrder (
	)
{
	trace_msg( heuristic, 2, "Creating order" );

	if ( startAt >= zones.size( ) )
	{
		trace_msg( heuristic, 1, "No more starting zones" );
		return false;
	}

	order.clear();
	unsigned int maxSize = zones.size( ) + sensors.size( );
	unsigned int nextNode = 1;
	unsigned int newConsidered = startAt + 1;

	Node *next = &zones[ startAt ];
	(*next).considered = newConsidered;
	order.push_back( next );

	string orderOutput = (*next).name + ", ";

	while ( order.size( ) < maxSize )
	{
		if ( (*next).children.size( ) > ( maxPu * maxElementsOnPu + maxElementsOnPu ) )
		{
			trace_msg( heuristic, 1, (*next).name + " needs to more connections than available from partner units" );
			return false;
		}

		for ( Node *child : (*next).children )
		{
			if ( (*child).considered < newConsidered )
			{
				(*child).considered = newConsidered;
				order.push_back( child );
				orderOutput += (*child).name + ", ";
			}
		}

		if ( nextNode >= order.size( ) )
		{
			trace_msg( heuristic, 1, "Not all zones/sensors are connected" );
			return false;
		}

		next = order[ nextNode ];
		nextNode++;
	}

	startAt++;

	trace_msg( heuristic, 3, "Considering order " + orderOutput );

	return true;
}

/*
 * adding variable to assignments
 * if some unit was already assign to the sensor/zone, add the variable to the tried ones
 * or add a new assignment otherwise
 *
 * @param var	the variable ( unit2zone or unit2sensor )
 * @param pu	the pu ( from unit2zone or unit2sensor )
 */
bool
PUPHeuristic::searchAndAddAssignment(
	Var variable,
	Pu pu )
{
	unsigned int current = index - 1;			// because index gets incremented each time a new node is acquired from the order

	if ( current < assignments.size( ) )
	{
		assignments[ current ].variable = variable;
		assignments[ current ].currentPu = pu;

		if ( std::find( assignments[ current ].triedUnits.begin( ), assignments[ current ].triedUnits.end( ), pu.var ) == assignments[ current ].triedUnits.end( ) )
			assignments[ current ].triedUnits.push_back( pu.var );

		return true;
	}
	else
	{
		Assignment a;

		a.variable = variable;
		a.currentPu = pu;
		a.triedUnits.push_back( pu.var );

		assignments.push_back( a );

		return false;
	}
}

/*
 * gets all tried partner units from the given node
 *
 * @param node	the node ( zone or sensor )
 * @param trie	the units tried by this node ( out )
 * @return		true if there are tried units or false otherwise
 */
bool
PUPHeuristic::getTriedAssignments(
	Node* node,
	vector < Var >* tried )
{
	unsigned int current = index - 1;			// because index gets incremented each time a new node is acquired from the order

	if ( current < assignments.size( ) )
	{
		*tried = assignments[ current ].triedUnits;
		return true;
	}

	return false;
}

/*
 * gets an unused partner unit
 *
 * @param pu	the partner unit ( out )
 * @return		true if there is an unused partner unit or false otherwise
 */
bool
PUPHeuristic::getUnusedPu(
	Pu* pu )
{
	bool used = false;

	for ( unsigned int i = 0; i < partnerUnits.size( ); i++ )
	{
		used = false;

		for ( unsigned int j = 0; j < partnerUnits[ i ].usedIn.size( ) && !used; j++ )
		{
			if ( solver.getTruthValue( partnerUnits[ i ].usedIn[ j ]->positive ) == TRUE )
				used = true;
		}

		if ( !used )
		{
			*pu = partnerUnits[ i ];
			return true;
		}
	}

	return false;
}

/*
 * determines if the partner is already in use
 *
 * @return	true if the unit is used or false otherwise
 */
bool
PUPHeuristic::isPartnerUsed(
	Pu pu )
{
	bool found = false;

	for ( unsigned int i = 0; i < pu.usedIn.size( ) && !found; i++ )
	{
		if ( solver.getTruthValue( pu.usedIn[ i ]->positive ) == TRUE )
			found = true;
	}

	return found;
}

/*
 * gets an unused partner unit
 *
 * @param pu	the partner unit ( out )
 * @param tried	the partner units already tried
 */
bool
PUPHeuristic::getUntriedPu(
	Pu* pu,
	vector < Var > tried )
{
	bool found = false;

	for ( unsigned int i = 0; i < partnerUnits.size( ) && !found; i++ )
	{
		if ( ( std::find( tried.begin(), tried.end(), partnerUnits[ i ].var ) == tried.end() ) && isPartnerUsed( partnerUnits[ i ] ) )
		{
			*pu = partnerUnits[ i ];
			found = true;
		}
	}

	return found;
}

bool
PUPHeuristic::getPu(
	Var assignment,
	Pu *pu )
{
	string unit;
	string node;
	bool found = false;

	HeuristicUtil::getName( VariableNames::getName( assignment ), &unit, &node );

	for ( unsigned int i = 0; i < partnerUnits.size( ) && !found; i++ )
	{
		if ( partnerUnits[ i ].name == unit )
			*pu = partnerUnits[ i ];
	}

	return found;
}

/*
 * make choice for solver
 */
Literal
PUPHeuristic::makeAChoiceProtected( )
{
	Pu pu;
	Node* current;
	Var chosenVariable;
	Assignment a;
	bool found;

	do
	{
		if ( !isConsitent )
			return Literal::null;

		chosenVariable = 0;

		do
		{
			found = false;

			// solution should already be found at this point - check assignments
			if ( index >= order.size( ) )
			{
				// all assignments are TRUE but something is UNDEFINED - set all UNDEFINED to FALSE
				if ( undefined.size( ) > 0 )
				{
					for ( unsigned int i = 0; i < undefined.size( ); i++ )
					{
						if ( solver.getTruthValue( undefined[ i ] ) == UNDEFINED )
							return Literal( undefined[ i ], NEGATIVE );
					}

					assert ( "Solution found" );
				}

				// check if all assignments are true
				// if one is FALSE raise conflict
				// if one is UNDEFINED go back to this point
				bool allTrue = true;
				for ( unsigned int i = 0; i < assignments.size( ) && allTrue; i++ )
				{
					if ( solver.getTruthValue( assignments[ i ].variable ) == FALSE )
					{
						allTrue = false;
						conflictOccured = true;
					}
					if ( solver.getTruthValue( assignments[ i ].variable ) == UNDEFINED )
					{
						allTrue = false;
						index = i;

						while ( index < ( assignments.size( ) - 1 ) )
							assignments.pop_back( );
					}
				}

				// if all assignments are TRUE, get all that are UNDEFINED
				if ( allTrue )
				{
					for ( unsigned int i = 0; i < variables.size( ); i++ )
					{
						if ( solver.isUndefined( variables[ i ] ) )
							undefined.push_back( variables[ i ] );
					}

					return Literal( undefined[ 0 ], NEGATIVE );
				}
			}

			// handle conflict and prepare for redo
			if ( conflictOccured )
			{
				if ( !conflictHandled )
				{
					bool found = false;
					unsigned int pos = 0;

					while ( pos < assignments.size( ) && !found )
					{
						if ( solver.getTruthValue( assignments[ pos ].variable ) == FALSE )
						{
							found = true;
							index = pos;
							trace_msg( heuristic, 4, "Reset index to node " << order[ pos ]->name << " ( index " << pos << " ) due to conflict" );
						}
						else
							pos++;
					}

					// pop assignments for zones/sensor after the current index
					while ( index < ( assignments.size( ) - 1 ) )
						assignments.pop_back( );

					conflictHandled = true;
				}

				conflictOccured = false;
				assignedSinceConflict = 0;
				redoAfterConflict = true;
			}

			// redo all UNDEFINED up to the current assignemnt
			// after the current zone/sensor has been assigned (otherwise the same conflict can occur again)
			if ( redoAfterConflict && assignedSinceConflict > 0 )
			{
				for ( unsigned int i = 0; i < assignments.size( ); i++ )
				{
					chosenVariable = assignments[ i ].variable;

					if ( solver.isUndefined( chosenVariable ) )
						return Literal( chosenVariable, POSITIVE );
				}

				redoAfterConflict = false;
				chosenVariable = 0;
			}

			// make a choice
			current = order[ index++ ];

			// check if the current node is already assigned - take next if so
			for ( unsigned int i = 0; i < current->usedIn.size( ) && !found; i++ )
			{
				ZoneAssignment* za = current->usedIn[ i ];
				if ( solver.getTruthValue( za->positive ) == TRUE )
				{
					found = true;

					trace_msg( heuristic, 3, "Node " << current->name << " is already assigned with "
							                         << za->positive << " " << Literal(za->positive, POSITIVE)
							                         << " -> continue with next zone/sensor");

					getPu( za->positive, &pu );
					searchAndAddAssignment( za->positive, pu );
				}
			}
		}
		while( found );

		vector < Var > tried;

		// get unused partner unit first
		if ( !getTriedAssignments( current, &tried ) )
		{
			if ( getUnusedPu( &pu ) )
			{
				chosenVariable = getVariable( &pu, current );
				searchAndAddAssignment( chosenVariable, pu );
			}
		}

		// try all used afterwards
		if ( chosenVariable == 0 )
		{
			if ( getUntriedPu( &pu, tried ) )
			{
				chosenVariable = getVariable( &pu, current );
				searchAndAddAssignment( chosenVariable, pu );
			}
		}

		// chosen variable is zero if all possible partner unit has been tried
		if ( chosenVariable == 0 )
		{
			trace_msg( heuristic, 3, "Chosen variable is zero"  );

			if ( index > 1 )
			{
				trace_msg( heuristic, 3, "No more possibilities to place this zone/sensor -> go one step back"  );
				index -= 2;		// -2 because the index has already been incremented

				while ( solver.getTruthValue( assignments[ index ].variable ) != UNDEFINED )
					solver.unrollOne( );

				assignments.pop_back( );

				conflictOccured = true;
			}
			else
			{
				trace_msg( heuristic, 3, "No solution for current order -> create new one" );
				isConsitent = resetHeuristic( );
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
				trace_msg( heuristic, 4, "Chosen variable is already set to TRUE - continue with next zone/sensor" );
			}
		}
	}
	while( chosenVariable == 0 || !solver.isUndefined( chosenVariable ) );

	assignedSinceConflict++;
	return Literal( chosenVariable, POSITIVE );
}

/*
 * return the variable for assigning the given node to the given unit
 *
 * @param  node 	the node to assign to
 * @param  unit		the unit
 * @return the variable
 */
Var
PUPHeuristic::getVariable (
	Pu *unit,
	Node *node )
{
	Var var = 0;
	bool found = false;
	vector < ZoneAssignment* > za = node->usedIn;

	for ( unsigned int i = 0; i < za.size( ) && !found; i++ )
	{
		if ( za[ i ]->pu == unit->name )
		{
			found = true;
			var = za[ i ]->positive;
		}
	}

	return var;
}

/*
 * handle conflict
 */
void
PUPHeuristic::conflictOccurred(
	)
{
	conflictOccured = true;
	conflictHandled = false;
}
