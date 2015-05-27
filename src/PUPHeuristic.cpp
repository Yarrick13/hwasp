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

#include "Literal.h"
#include "Solver.h"

#include "util/VariableNames.h"

#include <string>

PUPHeuristic::PUPHeuristic( Solver& s ) :
    Heuristic( s ),  startAt( 0 ), index( 0 ), redo( 0 ), doRedo( false ), usedPartnerUnits( 0 ), maxPu( 2 ), maxElementsOnPu( 2 ),
	assignTo( -1 ), lastAssignTo( -1 ), handleConflict( false ), isConsitent( true )
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

	string zone("zone(");
	string sensor("doorSensor(");
	string z2s("zone2sensor(");

	string unit("comUnit(");
	string u2zP("unit2zone(");
	string u2zN("-unit2zone(");
	string u2sP("unit2sensor(");
	string u2sN("-unit2sensor(");

	if( name.compare( 0, 5, zone ) == 0 )
	{
		getName( name, &tmp );

		Node zone;
		zone.name = tmp;
		zone.var = v;
		zone.considered = 0;
		zone.type = ZONE;

		zones.push_back( zone );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( zone )" );
	}
	else if( name.compare( 0, 11, sensor ) == 0 )
	{
		getName( name, &tmp );

		Node sensor;
		sensor.name = tmp;
		sensor.var = v;
		sensor.considered = 0;
		sensor.type = SENSOR;

		sensors.push_back( sensor );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( sensor )" );
	}
	else if( name.compare( 0, 12, z2s ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		Connection c;
		c.from = tmp;
		c.to = tmp2;
		c.var = v;

		zone2sensor.push_back( c );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( zone2sensor )" );
	}
	else if( name.compare( 0, 8, unit ) == 0 )
	{
		getName( name, &tmp );

		Pu pu;
		pu.name = tmp;
		pu.var = v;

		partnerUnits.push_back( pu );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( partnerunit )" );
	}
	else if( name.compare( 0, 10, u2zP ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.positive = v;

		initUnitAssignments( za, POSITIVE, U2Z );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( unit2zone )" );
	}
	else if( name.compare( 0, 11, u2zN ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.negative = v;

		initUnitAssignments( za, NEGATIVE, U2Z );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( unit2zone )" );
	}
	else if( name.compare( 0, 12, u2sP ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.positive = v;

		initUnitAssignments( za, POSITIVE, U2S );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( unit2sensor )" );
	}
	else if( name.compare( 0, 13, u2sN ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.negative = v;

		initUnitAssignments( za, NEGATIVE, U2S );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( unit2sensor )" );
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
 * parse term of an unary atoms
 * 		( zones, sensors and partner units )
 *
 * 	@param atom 	the atom
 * 	@param name		the term
 */
void
PUPHeuristic::getName(
	string atom,
	string *name )
{
	unsigned int start = atom.find_first_of( "(" );
	unsigned int end = atom.find_last_of( ")" );

	assert_msg( start != string::npos && end != string::npos && start < end, "Error while processing " + atom );

	*name = atom.substr( start + 1, end - start - 1 );
}

/*
 * read terms of a binary atom
 * 		( unit2zone or unit2sensor )
 *
 * 	@param atom 	the atom
 * 	@param name1	the first term
 * 	@param name2 	the second term
 */
void
PUPHeuristic::getName(
	string atom,
	string *name1,
	string *name2 )
{
	unsigned int start = atom.find_first_of( "(" );
	unsigned int middle = atom.find_first_of( "," );
	unsigned int end = atom.find_last_of( ")" );

	assert_msg( start != string::npos && end != string::npos && start < end, "Error while processing " + atom );

	*name1 = atom.substr( start + 1, middle - start - 1 );
	*name2 = atom.substr( middle + 1, end - middle - 1 );
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
	usedPartnerUnits = 0;
	assignTo = -1;

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
	unsigned int sign,
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
				if ( sign == POSITIVE )
					unit2zone[ i ].positive = za.positive;
				else
					unit2zone[ i ].negative = za.negative;

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
				if ( sign == POSITIVE )
					unit2sensor[ i ].positive = za.positive;
				else
					unit2sensor[ i ].negative = za.negative;

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
 * creat new order
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
			cout << "Not all zones/sensors are connected!" << endl;
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
 * make choice for solver
 */
Literal
PUPHeuristic::makeAChoiceProtected( )
{
	Node *current;
	Pu pu;
	Var chosenVariable;
	Assignment a;
	bool reset = false;

	// handle conflict
	if ( handleConflict )
	{
		bool found = false;
		unsigned int pos;

		// check assignments starting from the beginning for the first which not true anymore
		// set heuristic to get a new value for this found node
		for ( pos = 0; pos < assignments.size( ) && !found; pos++ )
		{
			if ( solver.getTruthValue( assignments[ pos ].variable ) != TRUE )
			{
				found = true;
				index = assignments[ pos ].index;
				assignTo = assignments[ pos ].toUnit + 1;
				usedPartnerUnits = assignments[ pos ].usedPartnerUnits;
			}
		}

		// pop all assignments after and including the found one
		while ( pos <= assignments.size( ) )
		{
			trace_msg( heuristic, 3, "Remove variable "<< assignments.back( ).variable << " " << VariableNames::getName( assignments.back( ).variable ) << " from the chosen ones" );
			assignments.pop_back( );
		}

		handleConflict = false;
	}

	do
	{
		/*if ( doRedo )
		{
			bool isUndefined;

			do
			{
				trace_msg( heuristic, 3, "Redo " << assignments[ redo ].variable <<" " << Literal(assignments[ redo ].variable, POSITIVE) );
				isUndefined = solver.isUndefined( assignments[ redo ].variable );
				if ( !isUndefined )
					redo++;
			}
			while ( !isUndefined && redo < assignments.size( ) );

			if ( redo >= assignments.size( ) )
				doRedo = false;
			else
				return Literal( assignments[ redo++ ].variable, POSITIVE );
		}*/

		if ( !isConsitent )
			return Literal::null;

		assert_msg ( index < order.size( ), "COHERENT" );

		reset = false;
		current = order [ index ];
		chosenVariable = 0;

		// try new partner unit first
		// try all currently used partner units afterwards
		if ( assignTo == -1 )
		{
			if ( newPartnerUnitAvailable( &pu ) )
				chosenVariable = getVariable( &pu, current );
			else
				assignTo++;
		}

		if ( chosenVariable == 0 )
		{
			if ( ( ( unsigned int ) assignTo ) < usedPartnerUnits )
				chosenVariable = getVariable( &partnerUnits[ assignTo ], current );
			else
				reset = true;
		}

		// reset -> no assignment possible for the current element
		if ( reset )
		{
			/*cout << "assignments before" << endl;
			for ( unsigned int i = 0; i < assignments.size( ); i++ )
				cout << i << ": " << assignments[ i ].variable << " " << VariableNames::getName( assignments[ i ].variable ) << endl;*/

			// searching all assignment beginning form the last to find a node where another assignment is possible
			// i.e. not all used partner units at this time were used
			bool found = false;
			unsigned int pos = assignments.size( );
			while ( pos > 0 && !found)
			{
				pos--;

				if ( !solver.isUndefined( Literal( assignments[ pos ].variable, POSITIVE ) ) && assignments[ pos ].toUnit > -1 &&
						( ( unsigned int ) assignments[ pos ].toUnit ) < assignments[ pos ].usedPartnerUnits )
				{
					found = true;
					index = assignments[ pos ].index;
					assignTo = assignments[ pos ].toUnit + 1;
					usedPartnerUnits = assignments[ pos ].usedPartnerUnits;
				}

				trace_msg( heuristic, 3, "Remove variable "<< assignments.back( ).variable << " " << VariableNames::getName( assignments.back( ).variable ) << " from the chosen ones" );
				assignments.pop_back( );
			}

			// if no one was found reset the heuristic and calculate a new order
			if ( !found )
			{
				trace_msg( heuristic, 2, "No solution for current order - create new one" );
				isConsitent = resetHeuristic( );
			}
			else
			{
				// otherwise redo everthing to this point an continue
				/*cout << "assignments after" << endl;
				for ( unsigned int i = 0; i < assignments.size( ); i++ )
					cout << i << ": " << assignments[ i ].variable << " " << VariableNames::getName( assignments[ i ].variable ) << endl;*/

				trace_msg( heuristic, 3, "Found for redo " << assignments[ pos ].variable << " " << VariableNames::getName( assignments[ pos ].variable ) );

				//unsigned int varDecisionLevel = solver.getDecisionLevel( Literal( assignments[ pos ].variable, POSITIVE ) );

				//trace_msg( heuristic, 3, "Unroll from current decisionlevel ( " << solver.getCurrentDecisionLevel( ) << " ) to level " << varDecisionLevel );

				while( solver.getCurrentDecisionLevel() > 0 && !solver.isUndefined( assignments[ pos ].variable ) )
						solver.unrollOne();

				//solver.unroll( varDecisionLevel );

				/*redo = 0;
				doRedo = true;

				// what is the correctly to unroll this stuff????
				solver.unrollToZero( );
				solver.clearConflictStatus( );
				//*/
			}
		}
		else
		{
			a.index = index;
			a.toUnit = assignTo;
			a.usedPartnerUnits = usedPartnerUnits;
			a.variable = chosenVariable;

			assert_msg( chosenVariable != 0, "The chosen variable has not been set" );
			trace_msg( heuristic, 3, "Chosen variable is "<< chosenVariable <<" " << Literal(chosenVariable, POSITIVE) );

			incrementIndex( );

			if ( solver.getTruthValue( chosenVariable ) == FALSE )
			{
				decrementIndex( );
				trace_msg( heuristic, 3, "Chosen variable is already set to FALSE - get new one" );
			}
		}
	}
	while ( reset || !solver.isUndefined( chosenVariable ) );

	assignments.push_back( a );

	return Literal( chosenVariable, POSITIVE );
}

bool
PUPHeuristic::newPartnerUnitAvailable (
	Pu *pu )
{
	if ( usedPartnerUnits < partnerUnits.size( ) )
	{
		*pu = partnerUnits[ usedPartnerUnits ];
		usedPartnerUnits++;

		return true;
	}

	return false;
}

void
PUPHeuristic::incrementIndex (
	)
{
	index++;					// next node in order
	lastAssignTo = assignTo;
	assignTo = -1;				// begin again with new unit
}

void
PUPHeuristic::decrementIndex (
	)
{
	index--;					// previous node in order
	assignTo = lastAssignTo;
	if ( assignTo == -1 )
		usedPartnerUnits--;
	assignTo++;					// try next existing partner unit
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

	if ( (*node).type == ZONE )
	{
		for ( unsigned int i = 0; i < unit2zone.size( ) && !found; i++ )
		{
			if ( unit2zone[ i ].pu == (*unit).name && unit2zone[ i ].to == (*node).name )
			{
				var = unit2zone[ i ].positive;
				found = true;
			}
		}
	}
	else if ( (*node).type == SENSOR )
	{
		for ( unsigned int i = 0; i < unit2sensor.size( ) && !found; i++ )
		{
			if ( unit2sensor[ i ].pu == (*unit).name && unit2sensor[ i ].to == (*node).name )
			{
				var = unit2sensor[ i ].positive;
				found = true;
			}
		}
	}

	assert_msg ( found, "Variable for " << (*unit).name << " to " << (*node).name << " not found!" );

	return var;
}
