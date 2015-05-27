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
    Heuristic( s ),  startAt( 0 ), index( 0 ), usedPartnerUnits( 0 ), update( TRUE ), maxPu( 2 ), maxElem( 2 ), assignTo( -1 ), lastAssignTo( -1 )
{
	// nothing to to
}

PUPHeuristic::~PUPHeuristic()
{
	// nothing to do
}

void
PUPHeuristic::onNewVariable(
    Var v )
{
	// nothing to do
}

void
PUPHeuristic::onNewVariableRuntime(
    Var v )
{
	// nothing to do
}

void
PUPHeuristic::onReadAtomTable(
    Var v,
    string name )
{
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
	}
	else if( name.compare( 0, 12, z2s ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		Connection c;
		c.from = tmp;
		c.to = tmp2;
		c.var = v;

		zone2sensor.push_back( c );
	}
	else if( name.compare( 0, 8, unit ) == 0 )
	{
		getName( name, &tmp );

		Pu pu;
		pu.name = tmp;
		pu.var = v;

		partnerUnits.push_back( pu );
	}
	else if( name.compare( 0, 10, u2zP ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.positive = v;
		za.considered = 0;

		initUnitAssignments( za, POSITIVE, U2Z );
	}
	else if( name.compare( 0, 11, u2zN ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.negative = v;
		za.considered = 0;

		initUnitAssignments( za, NEGATIVE, U2Z );
	}
	else if( name.compare( 0, 12, u2sP ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.positive = v;
		za.considered = 0;

		initUnitAssignments( za, POSITIVE, U2S );
	}
	else if( name.compare( 0, 13, u2sN ) == 0 )
	{
		getName( name, &tmp, &tmp2 );

		ZoneAssignment za;
		za.pu = tmp;
		za.to = tmp2;
		za.negative = v;
		za.considered = 0;

		initUnitAssignments( za, NEGATIVE, U2S );
	}
}

void
PUPHeuristic::onFinishedParsing (
	)
{
	if ( startAt >= zones.size( ) )
	{
		// incoherent
		assert( 0 && "INCOHERENT!" );
	}

	initRelation( );
	createOrder( );

	/*for ( unsigned int i = 0; i < unit2zone.size( ); i++ )
		cout << "u2z " << unit2zone[ i ].pu << " to " << unit2zone[ i ].to << "; pos var: "
			 << unit2zone[ i ].positive << ", neg var: " << unit2zone[ i ].negative << endl;

	for ( unsigned int i = 0; i < unit2ssensor.size( ); i++ )
		cout << "u2s " << unit2ssensor[ i ].pu << " to " << unit2ssensor[ i ].to << "; pos var: "
			 << unit2ssensor[ i ].positive << ", neg var: " << unit2ssensor[ i ].negative << endl;*/

	cout << "consider order: ";

	for ( unsigned int i = 0; i < order.size(); i++ )
		cout << (*order[i]).name << ", ";

	cout << endl;

	startAt++;
}

void
PUPHeuristic::getName(
	string atom,
	string *name )
{
	unsigned int start = atom.find_first_of( "(" );
	unsigned int end = atom.find_last_of( ")" );

	if ( start == string::npos || end == string::npos || start >= end )
		ErrorMessage::errorDuringParsing( "Error in heuristic - error while parsing symbol table." );

	*name = atom.substr( start + 1, end - start - 1 );
}

void
PUPHeuristic::getName(
	string atom,
	string *name1,
	string *name2 )
{
	unsigned int start = atom.find_first_of( "(" );
	unsigned int middle = atom.find_first_of( "," );
	unsigned int end = atom.find_last_of( ")" );

	if ( start == string::npos || end == string::npos || start >= end )
		ErrorMessage::errorDuringParsing( "Error in heuristic - error while parsing symbol table." );

	*name1 = atom.substr( start + 1, middle - start - 1 );
	*name2 = atom.substr( middle + 1, end - middle - 1 );
}

void
PUPHeuristic::initRelation(
	)
{
	Node *z;
	Node *s;
	bool found;

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

void
PUPHeuristic::createOrder (
	)
{
	if ( startAt < zones.size( ) )
	{
		order.clear();
		unsigned int maxSize = zones.size( ) + sensors.size( );
		unsigned int nextNode = 1;
		unsigned int newConsidered = startAt + 1;

		Node *next = &zones[ startAt ];
		(*next).considered = newConsidered;
		order.push_back( next );

		while ( order.size( ) < maxSize )
		{
			for ( Node *child : (*next).children )
			{
				if ( (*child).considered < newConsidered )
				{
					(*child).considered = newConsidered;
					order.push_back( child );
				}
			}

			next = order[ nextNode ];			// if not possible error
			nextNode++;
		}
	}
	else
	{
		//error
		assert( 0 && "UNDEFINED STARTZONE" );
	}
}

void
PUPHeuristic::onLiteralInvolvedInConflict(
    Literal literal )
{
	solver.unrollOne( );
	decrementIndex( );
}

void
PUPHeuristic::conflictOccurred(
	)
{
	solver.unrollOne( );
	decrementIndex( );
	// ???
	//initHeuristic( );
}

void
PUPHeuristic::onUnrollingVariable(
    Var variable )
{
	// ???
}

Literal
PUPHeuristic::makeAChoiceProtected()
{
	Node *current;
	Pu pu;
	Var chosenVariable;

	do
	{
		if ( index >= order.size( ) )
		{
			assert( 0 && "COHERENT -> finalize" );
		}

		current = order [ index ];
		chosenVariable = 0;


		if ( assignTo == -1 )
		{
			if ( newPartnerUnitAvailable( &pu ) )
				chosenVariable = getVariable( &pu, current );
			else
				assignTo++;
		}

		if ( chosenVariable == 0 )
			chosenVariable = getVariable( &partnerUnits[ assignTo ], current );
		assert_msg( chosenVariable != 0, "The chosen variable has not been set" );
		trace_msg( weakconstraints, 1, "Chosen Var "<< chosenVariable <<" " << Literal(chosenVariable, POSITIVE) );
		incrementIndex( );
	}
	while ( !solver.isUndefined( chosenVariable ) );

	return Literal( chosenVariable, POSITIVE );

	/*bool consistent;
	Node *current;
	PuAssignment newUnit;
	Var chosenVariable = 0;

	if ( startAt == 0 )
		initHeuristic();

	do
	{
		// reset in case of another while iteration
		consistent = false;
		newUnit.partnerUnits.clear( );
		newUnit.zones.clear( );
		newUnit.sensors.clear( );

		if ( index >= order.size( ) )
		{
			// model found -> finalize
			for ( unsigned int i = 0; i < unit2zone.size( ); i++ )
			{
				if ( unit2zone[ i ].considered < startAt )
				{
					unit2zone[ i ].considered = startAt;
					chosenVariable = unit2zone[ i ].negative;
					return Literal( chosenVariable, POSITIVE );
				}
			}

			for ( unsigned int i = 0; i < unit2sensor.size( ); i++ )
			{
				if ( unit2sensor[ i ].considered < startAt )
				{
					unit2sensor[ i ].considered = startAt;
					chosenVariable = unit2sensor[ i ].negative;
					return Literal( chosenVariable, POSITIVE );
				}
			}
		}

		current = order[ index ];

		if ( newPartnerUnitAvailable( &newUnit ) )
		{
			if ( assignAndConnect( &newUnit, current ) )
			{
				chosenVariable = getVariable( newUnit.unit, current );
				consistent = true;
				usedPartnerUnits++;
			}
		}

		if ( !consistent )
		{
			for ( unsigned int i = 0; i < model.size( ) && !consistent; i++)
			{
				consistent = assignAndConnect( &model[ i ], current );

				if ( consistent )
					chosenVariable = getVariable( model[ i ].unit, current );
			}
		}

		if ( !consistent )
			initHeuristic( );

		index++;
	}
	while ( !solver.isUndefined( chosenVariable ) );

	return Literal( chosenVariable, POSITIVE );*/
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

/*bool
PUPHeuristic::assignAndConnect (
	PuAssignment *unit,
	Node *node )
{
	unsigned int freeSlots = maxElem * maxPu + maxElem;
	unsigned int usedSlots = 0;
	unsigned int foundChildren = 0;
	bool neededPartner;
	vector < PuAssignment* > neededPartners;

	if ( ( (*unit).sensors.size( ) >= maxElem && (*node).type == SENSOR) ||
		 ( (*unit).zones.size( ) >= maxElem && (*node).type == ZONE ) )
		return false;

	// second check
	for ( unsigned int i = 0; i < model.size( ); i++ )
	{
		if ( (*node).type == ZONE )
		{
			for ( Node *s : model[ i ].sensors )
			{
				neededPartner = false;

				for ( Node *c : (*node).children )
				{
					if ( (*c).name == (*s).name )
					{
						foundChildren++;
						neededPartner = true;
					}
				}

				if ( neededPartner && model[ i ].unit != (*unit).unit )
					neededPartners.push_back( &model[ i ] );
			}
		}
		else if ( (*node).type == SENSOR )
		{
			for ( Node *s : model[ i ].zones )
			{
				neededPartner = false;

				for ( Node *c : (*node).children )
				{
					if ( (*c).name == (*s).name )
					{
						foundChildren++;
						neededPartner = true;
					}
				}

				if ( neededPartner && model[ i ].unit != (*unit).unit )
					neededPartners.push_back( &model[ i ] );
			}
		}
	}

	for ( unsigned int i = 0; i < (*unit).partnerUnits.size( ); i++ )
	{
		if ( std::find( neededPartners.begin( ), neededPartners.end( ), (*unit).partnerUnits[ i ])
					== neededPartners.end( ) )
		{
			neededPartners.push_back( (*unit).partnerUnits[ i ] );
		}
	}

	if ( ( neededPartners.size() ) > maxPu )
		return false;

	for ( unsigned int i = 0; i < neededPartners.size( ); i++ )
	{
		if ( (*neededPartners[ i ]).partnerUnits.size( ) >= maxPu )
			return false;

		if ( (*node).type == ZONE )
			usedSlots += (*neededPartners[ i ]).sensors.size( );
		else if ( (*node).type == SENSOR )
			usedSlots += (*neededPartners[ i ]).zones.size( );
	}

	if ( freeSlots < ( (*node).children.size( ) - foundChildren + usedSlots ) )
		return false;

	if ( (*node).type == ZONE )
		(*unit).zones.push_back( node );
	else if ( (*node).type == SENSOR )
		(*unit).sensors.push_back( node );

	for ( unsigned int i = 0; i < neededPartners.size( ); i++ )
	{
		if ( std::find( (*unit).partnerUnits.begin( ), (*unit).partnerUnits.end( ), neededPartners[ i ] )
							== (*unit).partnerUnits.end( ) )
		{
			(*unit).partnerUnits.push_back( neededPartners[ i ] );
			(*neededPartners[ i ]).partnerUnits.push_back( unit );
		}
	}

	model.push_back( *unit );

	return true;
}*/

void
PUPHeuristic::incrementIndex (
	)
{
	index++;
	lastAssignTo = assignTo;
	assignTo = -1;
}

void
PUPHeuristic::decrementIndex (
	)
{
	index--;
	assignTo = lastAssignTo;
	if ( assignTo == -1 )
		usedPartnerUnits--;
}

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
				unit2zone[ i ].considered = startAt;
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
				unit2sensor[ i ].considered = startAt;
				found = true;
			}
		}
	}

	return var;
}

void
PUPHeuristic::incrementHeuristicValues(
	Var v )
{
	// nothing to do
}

void
PUPHeuristic::simplifyVariablesAtLevelZero()
{
	// ???
}
