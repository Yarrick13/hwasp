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

PUPHeuristic::PUPHeuristic( Solver& s ) :
    Heuristic( s ),  startAt( 0 ), index( 0 ), usedPartnerUnits( 0 ), maxPu( 2 ), maxElementsOnPu( 2 ), isConsitent( true ), conflictOccured( false ),
	conflictHandled( true ), conflictIndex( 0 )
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
		getName( name, &tmp );

		Node zone;
		zone.name = tmp;
		zone.var = v;
		zone.considered = 0;
		zone.type = ZONE;

		zones.push_back( zone );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( zone )" );
	}
	else if( name.compare( 0, 11, "doorSensor(" ) == 0 )
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
	else if( name.compare( 0, 12, "zone2sensor(" ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		Connection c;
		c.from = tmp;
		c.to = tmp2;
		c.var = v;

		zone2sensor.push_back( c );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( zone2sensor )" );
	}
	else if( name.compare( 0, 8, "comUnit(" ) == 0 )
	{
		getName( name, &tmp );

		Pu pu;
		pu.name = tmp;
		pu.var = v;

		partnerUnits.push_back( pu );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( partnerunit )" );
	}
	else if( name.compare( 0, 10, "unit2zone(" ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.positive = v;

		initUnitAssignments( za, POSITIVE, U2Z );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( unit2zone )" );
	}
	else if( name.compare( 0, 11, "-unit2zone(" ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.negative = v;

		initUnitAssignments( za, NEGATIVE, U2Z );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( unit2zone )" );
	}
	else if( name.compare( 0, 12, "unit2sensor(" ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.positive = v;

		initUnitAssignments( za, POSITIVE, U2S );

		trace_msg( heuristic, 3, "Processed variable " << v << " " << name << "( unit2sensor )" );
	}
	else if( name.compare( 0, 13, "-unit2sensor(" ) == 0 )
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
	usedPartnerUnits = 0;

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
			//cout << "Not all zones/sensors are connected!" << endl;
			return false;
		}

		next = order[ nextNode ];
		nextNode++;
	}

	startAt++;

	trace_msg( heuristic, 3, "Considering order " + orderOutput );
	//cout << "Considering order " + orderOutput << endl;

	return true;
}

/*
 * adding variable to assignments
 * if some unit was already assign to the sensor/zone, add the variable to the tried ones
 * or add a new assignment otherwise
 *
 * @param var	the variable ( unit2zone or unit2sensor )
 */
bool
PUPHeuristic::searchAndAddAssignment(
	Var variable)
{
	string unit, node;
	unsigned int current = index - 1;			// because index gets incremented each time a new node is acquired from the order

	getName( VariableNames::getName( variable ), &unit, &node );

	if ( current < assignments.size( ) )
	{
		assignments[ current ].variable = variable;

		if ( std::find( assignments[ current ].triedUnits.begin( ), assignments[ current ].triedUnits.end( ), unit ) == assignments[ current ].triedUnits.end( ) )
			assignments[ current ].triedUnits.push_back( unit );

		return true;
	}
	else
	{
		Assignment a;

		a.variable = variable;
		a.name = node;
		a.triedUnits.push_back( unit );

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
	vector < string >* tried )
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
	vector < string > tried )
{
	bool found = false;

	for ( unsigned int i = 0; i < partnerUnits.size( ) && !found; i++ )
	{
		if ( std::find( tried.begin(), tried.end(), partnerUnits[ i ].name ) == tried.end() )//&& isPartnerUsed( partnerUnits[ i ] ) )
		{
			*pu = partnerUnits[ i ];
			found = true;
		}
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

			if ( index >= order.size( ) )
			{
				cout << "assert index" << endl;
				if ( undefined.size( ) > 0 )
				{
					for ( unsigned int i = 0; i < undefined.size( ); i++ )
					{
						if ( solver.getTruthValue( undefined[ i ] ) == UNDEFINED )
							return Literal( undefined[ i ], NEGATIVE );
					}

					assert_msg ( index < order.size( ), "COHERENT" );
				}

				bool allTrue = true;
				for ( unsigned int i = 0; i < assignments.size( ) && allTrue; i++ )
				{
					if ( solver.getTruthValue( assignments[ i ].variable ) != TRUE )
						allTrue = false;
				}

				if ( allTrue )
				{
					for ( unsigned int i = 0; i < variables.size( ); i++ )
					{
						if ( solver.isUndefined( variables[ i ] ) )
							undefined.push_back( variables[ i ] );
					}

					return Literal( undefined[ 0 ], NEGATIVE );
				}
				else
				{
					conflictOccured = true;
					conflictHandled = false;
					conflictIndex = 0;
				}
			}

			if ( conflictOccured )
			{
				if ( !conflictHandled )
				{
					bool found = false;
					unsigned int pos = 0;

					while ( pos < assignments.size( )  && !found )
					{
						if ( solver.getTruthValue( assignments[ pos ].variable ) == FALSE )
						{
							found = true;
							index = pos;
							trace_msg( heuristic, 4, "Reset index to node " << order[ pos ]->name << " ( index " << pos << " ) due to conflict" );
							//cout << "Reset index to node " << order[ pos ]->name << " ( index " << pos << " ) due to conflict" << endl;
						}
						else
							pos++;
					}

					while ( index < ( assignments.size( ) - 1 ) )
						assignments.pop_back( );

					conflictHandled = true;
				}

				while ( conflictIndex < index )
				{
					chosenVariable = assignments[ conflictIndex++ ].variable;

					if ( solver.isUndefined( chosenVariable ) )
						return Literal( chosenVariable, POSITIVE );
				}

				chosenVariable = 0;

				if ( conflictIndex >= index )
					conflictOccured = false;
			}

			current = order[ index++ ];

			for ( unsigned int i = 0; i < current->usedIn.size( ) && !found; i++ )
			{
				ZoneAssignment* za = current->usedIn[ i ];
				if ( solver.getTruthValue( za->positive ) == TRUE )
				{
					found = true;

					trace_msg( heuristic, 3, "Node " << current->name << " is already assigned with "
							                         << za->positive << " " << Literal(za->positive, POSITIVE)
							                         << " - get next node");

					//cout << "Node " << current->name << " is already assigned with "
					//         << za->positive << " " << Literal(za->positive, POSITIVE)
					//         << " - get next node" << endl;

					searchAndAddAssignment( za->positive );
				}
			}
		}
		while( found );

		vector < string > tried;

		if ( !getTriedAssignments( current, &tried ) )
		{
			if ( getUnusedPu( &pu ) )
			{
				chosenVariable = getVariable( &pu, current );
				searchAndAddAssignment( chosenVariable );
			}
		}

		if ( chosenVariable == 0 )
		{
			if ( getUntriedPu( &pu, tried ) )
			{
				chosenVariable = getVariable( &pu, current );
				searchAndAddAssignment( chosenVariable );
			}
		}

		if ( chosenVariable == 0 )
		{
			/*cout << "assignments" << endl;
			for ( unsigned int i = 0; i < assignments.size( ); i++ )
			{
				cout << i << ": " << assignments[ i ].variable << " " << VariableNames::getName( assignments[ i ].variable ) << " is "
						<< solver.getTruthValue( assignments[ i ].variable ) << endl;
			}

			cout << endl << "for the nodes" << endl;
			for ( unsigned int i = 0; i < order.size( ); i++ )
			{
				cout << "node " << order[ i ]->name << endl;
				for ( unsigned int j = 0; j < (*order[ i ]).usedIn.size( ); j++ )
				{
					cout << j << ": " << (*order[ i ]).usedIn[ j ]->positive << " " << VariableNames::getName( (*order[ i ]).usedIn[ j ]->positive ) << " is "
							<< solver.getTruthValue( (*order[ i ]).usedIn[ j ]->positive ) << endl;
				}
				cout << endl;
			}*/

			isConsitent = resetHeuristic( );
		}
		else
		{
			trace_msg( heuristic, 3, "Chosen variable is "<< chosenVariable << " " << Literal(chosenVariable, POSITIVE) );
			//cout <<  "Chosen variable is "<< chosenVariable << " " << Literal(chosenVariable, POSITIVE) << endl;

			if ( solver.getTruthValue( chosenVariable ) == FALSE )
			{
				trace_msg( heuristic, 4, "Chosen variable is already set to FALSE - try another assignment" );
				//cout << "Chosen variable is already set to FALSE - try another assignment" << endl;
				index--;
			}
			if ( solver.getTruthValue( chosenVariable ) == TRUE )
			{
				trace_msg( heuristic, 4, "Chosen variable is already set to TRUE - continue" );
				//cout << "Chosen variable is already set to TRUE - continue" << endl;
			}
		}
	}
	while( chosenVariable == 0 || !solver.isUndefined( chosenVariable ) );

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
	conflictIndex = 0;
}
