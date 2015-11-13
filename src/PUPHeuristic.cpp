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
    Heuristic( s ),  startAt( 0 ), index( 0 ), maxPu( 2 ), maxElementsOnPu( 2 ), numberOfConflicts( 0 ), coherent( true ), conflictOccured( false ),
	conflictHandled( true ), redoAfterAddingConstraint( false ), inputCorrect( true ), solutionFound( false ),
	sNumberOfConflicts( 0 ), sNumberOfOrdersCreated( 0 ), sNumberOfRecommendations( 0 ), sNumberOfOrderMaxReached( 0 ), sFallback( 0 ),
	sAlreadyFalse( 0 ), sAlreadyTrue( 0 ), pre( 0 ), dec( 0 )
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

	try
	{
		name.erase(std::remove(name.begin(),name.end(),' '),name.end());

		string tmp;
		string tmp2;

		if( name.compare( 0, 6, "elem(z" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			Node zone;
			zone.name = tmp2;
			zone.var = v;
			zone.considered = 0;
			zone.type = ZONE;

			zones.push_back( zone );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( zone )" );
		}
		else if( name.compare( 0, 6, "elem(d" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			Node sensor;
			sensor.name = tmp2;
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

			pu.numberOfPartners = 0;
			pu.numberOfZones = 0;
			pu.numberOfSensors = 0;
			pu.removed = false;

			partnerUnits.push_back( pu );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( partnerunit )" );
		}
		else if( name.compare( 0, 10, "unit2zone(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			ZoneAssignment za;
			za.pu = tmp;
			za.to = tmp2;
			za.var = v;
			za.type = ZONE;

			initUnitAssignments( za, U2Z );

			trace_msg( heuristic, 3, "Processed variable " << v << " " << name << " ( unit2zone )" );
		}
		else if( name.compare( 0, 12, "unit2sensor(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			ZoneAssignment za;
			za.pu = tmp;
			za.to = tmp2;
			za.var = v;
			za.type = SENSOR;

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
		else if( name.compare( 0, 13, "partnerunits(" ) == 0 )
		{
			HeuristicUtil::getName( name, &tmp, &tmp2 );

			PartnerUnitConnection puc;
			puc.var = v;
			puc.unit1 = tmp;
			puc.unit2 = tmp2;

			partnerUnitConnections.push_back( puc );
		}
	}
	catch ( int e )
	{
		trace_msg( heuristic, 3, "Error while parsing " << name );
		inputCorrect = false;
	}
}

/*
 * compares to Nodes (pointers) for ordering based in their name
 */
bool compareNodes (
	PUPHeuristic::Node* n1,
	PUPHeuristic::Node* n2 )
{
	return std::stoi( n1->name ) < std::stoi( n2->name );
};

/*
 * initialize the heuristic after parsing the input
 * 		initialize zone assingment vectors and create the first order
 */
void
PUPHeuristic::onFinishedParsing (
	)
{
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

	trace_msg( heuristic, 1, "Initializing QuickPuP heuristic" );
	trace_msg( heuristic, 2, "Start processing variables" );

	for ( Var variable : variables )
	{
		if ( !VariableNames::isHidden( variable ) )
			processVariable( variable );
	}

	inputCorrect = checkInput( );

	if ( inputCorrect )
	{
		initRelation( );

		if ( zones.size( ) > ( partnerUnits.size( ) * maxElementsOnPu ) || sensors.size( ) > ( partnerUnits.size( ) * maxElementsOnPu ) )
			coherent = false;
		else
			coherent = resetHeuristic( );

		trace_msg( heuristic, 1, "Start heuristic" );
	}

	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
	pre += time_span;
}

bool
PUPHeuristic::checkInput(
	)
{
	if ( inputCorrect == false )
		return false;

	if ( zones.size( ) == 0 || sensors.size( ) == 0 || unit2zone.size( ) == 0 || unit2sensor.size( ) == 0 ||
			partnerUnits.size( ) == 0 || partnerUnitConnections.size( ) == 0 )
		return false;


	return true;
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

	for ( unsigned int i = 0; i < zones.size( ); i++ )
		std::sort ( zones[ i ].children.begin( ), zones[ i ].children.end(), compareNodes );

	for ( unsigned int i = 0; i < sensors.size( ); i++ )
		std::sort ( sensors[ i ].children.begin( ), sensors[ i ].children.end(), compareNodes );

	trace_msg( heuristic, 2, "Creating 'usedIn' relation" );

	ZoneAssignment* za;
	Pu* partner;
	for ( unsigned int i = 0; i < unit2zone.size( ); i++ )
	{
		za = &unit2zone[ i ];

		found = false;
		for ( unsigned int j = 0; j < partnerUnits.size( ) && !found; j++)
		{
			if ( partnerUnits[ j ].name == za->pu )
			{
				partnerUnits[ j ].usedIn.push_back( za );
				partner = &partnerUnits[ j ];
				found = true;
			}
		}

		found = false;
		for ( unsigned int j = 0; j < zones.size( ) && !found; j++ )
		{
			if ( zones[ j ].name == za->to )
			{
				zones[ j ].usedIn.push_back( za );
				zones[ j ].usedInUnit.push_back( partner );
				found = true;
			}
		}
	}

	for ( unsigned int i = 0; i < unit2sensor.size( ); i++ )
	{
		za = &unit2sensor[ i ];

		found = false;
		for ( unsigned int j = 0; j < partnerUnits.size( ) && !found; j++)
		{
			if ( partnerUnits[ j ].name == za->pu )
			{
				partnerUnits[ j ].usedIn.push_back( za );
				partner = &partnerUnits[ j ];
				found = true;
			}
		}

		found = false;
		for ( unsigned int j = 0; j < sensors.size( ) && !found; j++ )
		{
			if ( sensors[ j ].name == za->to )
			{
				sensors[ j ].usedIn.push_back( za );
				sensors[ j ].usedInUnit.push_back( partner );
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

	unrollHeuristic( );
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
				unit2zone[ i ].var = za.var;
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
				unit2sensor[ i ].var = za.var;
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

	sNumberOfOrdersCreated++;

	order.clear();
	unsigned int maxSize = zones.size( ) + sensors.size( );
	unsigned int nextNode = 1;
	unsigned int newConsidered = startAt + 1;

	Node *next = &zones[ startAt ];
	(*next).considered = newConsidered;
	order.push_back( next );

	string orderOutput = (*next).name + ( ( next->type == ZONE ) ? " ( zone )" : " ( sensor )" ) + ", ";

	while ( order.size( ) < maxSize )
	{
		if ( (*next).children.size( ) > ( maxPu * maxElementsOnPu + maxElementsOnPu ) )
		{
			trace_msg( heuristic, 1, (*next).name + " needs to more connections than available from partner units" );
			return false;
		}

		for ( unsigned int i = 0; i <  (*next).children.size( ); i++ )
		{
			Node* child = (*next).children[ i ];
			if ( child->considered < newConsidered )
			{
				child->considered = newConsidered;
				order.push_back( child );
				orderOutput += child->name + ( ( child->type == ZONE ) ? " ( zone )" : " ( sensor )" ) + ", ";
			}
		}

		next->ignore = false;
		for ( unsigned int i = 0; i < next->usedIn.size( ) && !next->ignore; i++ )
		{
			if ( solver.getTruthValue( next->usedIn[ i ]->var ) == TRUE )
				next->ignore = true;
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
	Pu pu,
	bool triedNewUnit )
{
	unsigned int current = index - 1;			// because index gets incremented each time a new node is acquired from the order

	if ( current < assignments.size( ) )
	{
		assignments[ current ].var = variable;
		assignments[ current ].currentPu = pu;
		if ( !assignments[ current ].triedNewUnit )
			assignments[ current ].triedNewUnit = triedNewUnit;

		if ( std::find( assignments[ current ].triedUnits.begin( ), assignments[ current ].triedUnits.end( ), pu.var ) == assignments[ current ].triedUnits.end( ) )
			assignments[ current ].triedUnits.push_back( pu.var );

		return true;
	}
	else
	{
		Assignment a;

		a.var = variable;
		a.currentPu = pu;
		a.triedNewUnit = triedNewUnit;
		a.triedUnits.push_back( pu.var );

		assignments.push_back( a );

		return false;
	}
}

/*
 * gets all tried partner units from the given node
 *
 * @param tried	the units tried by the current node ( out )
 * @return		true if there are tried units or false otherwise
 */
bool
PUPHeuristic::getTriedAssignments(
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
 * gets an unused partner unit for the given node
 *
 * @param pu	the partner unit ( out )
 * @param node	the current node
 * @return		true if there is an unused partner unit or false otherwise
 */
unsigned int
PUPHeuristic::getUnusedPu(
	Pu* pu,
	Node* current )
{
	bool used;
	Pu* partner;

	for ( unsigned int i = 0; i < current->usedInUnit.size( ); i++ )
	{
		used = false;
		partner = current->usedInUnit[ i ];

		for ( unsigned int j = 0; j < partner->usedIn.size( ) && !used; j++ )
		{
			if ( solver.getTruthValue( partner->usedIn[ j ]->var ) == TRUE )
				used = true;
		}

		if ( !used )
		{
			*pu = *partner;
			return current->usedIn[ i ]->var;
		}
	}

	return 0;
}

/*
 * determines if the partner is already in use
 *
 * @return	true if the unit is used or false otherwise
 */
bool
PUPHeuristic::isPartnerUsed(
	Pu* pu )
{
	bool found = false;

	for ( unsigned int i = 0; i < pu->usedIn.size( ) && !found; i++ )
	{
		if ( solver.getTruthValue( pu->usedIn[ i ]->var ) == TRUE )
			found = true;
	}

	return found;
}

/*
 * gets an unused partner unit for the given node
 *
 * @param pu	the partner unit ( out )
 * @param node 	the current node
 * @param tried	the partner units already tried
 */
unsigned int
PUPHeuristic::getUntriedPu(
	Pu* pu,
	Node* current,
	const vector < Var >& tried )
{
	Pu* partner;
	unsigned int index;

	for ( unsigned int i = 0; i < current->usedInUnit.size( ); i++ )
	{
		//index = current->usedInUnit.size( ) - 1 - i;
		index = i;

		partner = current->usedInUnit[ index ];

		if ( ( std::find( tried.begin(), tried.end(), partner->var ) == tried.end() ) && isPartnerUsed( partner ) )
		{
			*pu = *partner;
			return current->usedIn[ index ]->var;
		}
	}

	return 0;
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
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

	Pu pu;
	Node* current;
	Var chosenVariable;
	Assignment a;
	bool found;

	if ( redoAfterAddingConstraint )
	{
		for ( unsigned int i = 0; i < assignments.size( ); i++ )
		{
			chosenVariable = assignments[ i ].var;

			if ( solver.isUndefined( chosenVariable ) )
				return Literal( chosenVariable, POSITIVE );
		}

		conflictOccured = true;
		conflictHandled = false;
		chosenVariable = 0;
	}

	if ( solutionFound )
	{
		trace_msg( heuristic, 3, "Look for another solution - reset heuristic" );

		startAt = 0;
		index = 0;
		numberOfConflicts = 0;
		coherent = true;
		conflictOccured = false;
		conflictHandled = true;
		inputCorrect = false;
		solutionFound = false;
		sNumberOfConflicts = 0;
		sNumberOfOrdersCreated = 0;
		sNumberOfRecommendations = 0;
		sNumberOfOrderMaxReached = 0;

		assignments.clear( );

		for ( unsigned int i = 0; i < zones.size( ); i++ )
			zones[ i ].considered = 0;

		for ( unsigned int i = 0; i < sensors.size( ); i++ )
			sensors[ i ].considered = 0;

		createOrder( );
	}

	do
	{
		if ( !coherent )
		{
			trace_msg( heuristic, 4, "Heuristic can not find a solution" );
			sFallback = 2;
			return Literal::null;
		}

		chosenVariable = 0;

		do
		{
			found = false;

			// solution should already be found at this point - check assignments
			if ( index >= order.size( ) )
			{
				trace_msg( heuristic, 3, "All zones/sensors considered but no solution found - check assignments" );
				sNumberOfOrderMaxReached++;

				// check if all assignments are true
				// if one is FALSE raise conflict
				// if one is UNDEFINED go back to this point
				bool allTrue = true;
				for ( unsigned int i = 0; i < assignments.size( ) && allTrue; i++ )
				{
					if ( solver.getTruthValue( assignments[ i ].var ) == FALSE )
					{
						allTrue = false;
						conflictOccured = true;
						conflictHandled = false;
					}
					if ( solver.getTruthValue( assignments[ i ].var ) == UNDEFINED )
					{
						allTrue = false;
						index = i;

						while ( index < ( assignments.size( ) - 1 ) )
							assignments.pop_back( );
					}
				}

				if ( allTrue )
				{
					trace_msg( heuristic, 4, "PuP heuristic found solution but wasp did not recognized it - fall back to Minisat heuristic" );
					sFallback = 1;
					return Literal::null;
				}
				else
				{
					trace_msg( heuristic, 4, "solution not correct - reset" );
				}
			}

			// handle conflict and prepare for redo
			if ( conflictOccured )
			{
				redoAfterAddingConstraint = false;

				if ( !conflictHandled )
				{
					bool found = false;
					unsigned int pos = 0;
					unsigned int pos_undefined = 0;

					while ( pos < assignments.size( ) && !found )
					{
						if ( solver.getTruthValue( assignments[ pos ].var ) == FALSE )
						{
							found = true;
							index = pos;
							trace_msg( heuristic, 4, "Reset index to node " << order[ pos ]->name << " ( index " << pos << " ) due to conflict" );
						}
						else
						{
							if ( solver.getTruthValue( assignments[ pos ].var ) == UNDEFINED && pos_undefined == 0 )
								pos_undefined = pos;
							pos++;
						}
					}

					// pop assignments for zones/sensor after the current index
					while ( index < ( assignments.size( ) - 1 ) )
						assignments.pop_back( );

					conflictHandled = true;
					sNumberOfConflicts++;
				}

				conflictOccured = false;
				numberOfConflicts++;
			}

			// make a choice
			do
			{
				current = order[ index++ ];
				trace_msg( heuristic, 2, "Consider " << ( ( current->type == ZONE ) ? "zone " : "sensor " ) << current->name  );

				if ( current->ignore )
				{
					bool search = true;
					for ( unsigned int i = 0; i < current->usedIn.size( ) && search; i++ )
					{
						if ( solver.getTruthValue( current->usedIn[ i ]->var ) == TRUE )
						{
							search = true;

							searchAndAddAssignment( current->usedIn[ i ]->var, partnerUnits[ 0 ], false );
						}
					}

					trace_msg( heuristic, 3, "This zone/sensor will be ignored - get next one" );
				}
			}
			while ( current->ignore );

			// check if the current node is already assigned - take next if so
			for ( unsigned int i = 0; i < current->usedIn.size( ) && !found; i++ )
			{
				ZoneAssignment* za = current->usedIn[ i ];
				if ( solver.getTruthValue( za->var ) == TRUE )
				{
					found = true;

					trace_msg( heuristic, 3, "Node " << current->name << " is already assigned with "
							                         << za->var << " " << Literal(za->var, POSITIVE)
							                         << " -> continue with next zone/sensor" );

					getPu( za->var, &pu );

					searchAndAddAssignment( za->var, pu, false );
				}
			}
		}
		while( found );

		vector < Var > tried;


// version 1 start
// try unused unit first and used units afterwards ( asc )
		// get unused partner unit first
		if ( !getTriedAssignments( &tried ) )
		{
			chosenVariable = getUnusedPu( &pu, current );
			if ( chosenVariable != 0 )
				searchAndAddAssignment( chosenVariable, pu, true );
		}

		// try all used afterwards
		if ( chosenVariable == 0 )
		{
			chosenVariable = getUntriedPu( &pu, current, tried );
			if ( chosenVariable != 0 )
				searchAndAddAssignment( chosenVariable, pu, false );
		}
// version 1 end

// version 2 start
// try used units first ( asc ) and unused unit aftwards
//		// try all used units
//		getTriedAssignments( &tried );
//		chosenVariable = getUntriedPu( &pu, current, tried );
//		if ( chosenVariable != 0 )
//			searchAndAddAssignment( chosenVariable, pu, false );
//
//		// get unused partner unit
//		if ( chosenVariable == 0 && !newUnitTriedForCurrentNode( ) )
//		{
//			chosenVariable = getUnusedPu( &pu, current );
//			if ( chosenVariable != 0 )
//				searchAndAddAssignment( chosenVariable, pu, true );
//		}
// version 2 end

		// chosen variable is zero if all possible partner unit has been tried
		if ( chosenVariable == 0 )
		{
			trace_msg( heuristic, 3, "Chosen variable is zero" );

			if ( index > 1 )
			{
				string ass = "";
				index--;
				assignments.pop_back( );
				unrollHeuristic( );

				vector< Literal > l;
				for ( Assignment a : assignments )
				{
					if ( solver.isUndefined( a.var ) )
						l.push_back( Literal (a.var, NEGATIVE ) );

					ass += VariableNames::getName( a.var ) + ", ";
				}
				addClause( l );

				redoAfterAddingConstraint = true;
				trace_msg( heuristic, 3, "Not possible: " << ass );

				return Literal( assignments[ 0 ].var, POSITIVE );
			}
			else
			{
				trace_msg( heuristic, 3, "No solution for current order -> create new order" );
				coherent = resetHeuristic( );
			}
		}
		else
		{
			trace_msg( heuristic, 3, "Chosen variable is "<< chosenVariable << " " << Literal( chosenVariable, POSITIVE ) );

			if ( solver.getTruthValue( chosenVariable ) == FALSE )
			{
				trace_msg( heuristic, 4, "Chosen variable is already set to FALSE - try another assignment" );
				index--;
				sAlreadyFalse++;
			}
			if ( solver.getTruthValue( chosenVariable ) == TRUE )
			{
				trace_msg( heuristic, 4, "Chosen variable is already set to TRUE - continue with next zone/sensor" );
				sAlreadyTrue++;
			}
		}
	}
	while( chosenVariable == 0 || !solver.isUndefined( chosenVariable ) );

	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
	dec += time_span;

	sNumberOfRecommendations++;
	return Literal( chosenVariable, POSITIVE );
}

bool
PUPHeuristic::newUnitTriedForCurrentNode(
	)
{
	return assignments[ index - 1].triedNewUnit;
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
			var = za[ i ]->var;
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

void
PUPHeuristic::onFinishedSolving(
	)
{
	solutionFound = true;
	printStatistics( );

	trace_msg( heuristic, 2, "Adding constraint to avoid previous solution" );
	unrollHeuristic( );

	vector< Literal > l;
	for ( Pu pu : partnerUnits )
	{
		for ( ZoneAssignment* za : pu.usedIn)
		{
			if ( solver.getTruthValue( za->var ) == TRUE )
			{
				//cout << "add " << za->positive << " " << VariableNames::getName( za->positive ) << endl;
				if ( solver.isUndefined( za->var ) )
					l.push_back( Literal( za->var, NEGATIVE ) );
			}
		}
	}
	addClause( l );

//	vector < Var > trueInAS;
//	vector < Var > falseInAS;
//	vector < Pu* > removed;
//
//	trace_msg( heuristic, 1, "Minimize solution" );
//
//	cout << endl << endl << "after minimize " << endl;
//	minimize( &trueInAS, &falseInAS, &removed );
//
//	cout << "set to true" << endl;
//	for ( Var v : trueInAS )
//		cout <<"\t" << VariableNames::getName( v ) << endl;
//
//	cout << "set to false" << endl;
//	for ( Var v : falseInAS )
//		cout <<"\t" << VariableNames::getName( v ) << endl;
//
//	cout << "removed pu" << endl;
//	for ( Pu* pu : removed )
//		cout <<"\t" << pu->name << endl;
}

void
PUPHeuristic::minimize(
	vector< Var >* trueInAS,
	vector< Var>* falseInAS,
	vector< Pu* >* removed)
{
	unsigned int found = 0;
	unsigned int nZones;
	unsigned int nSensors;
	unsigned int nPartners;

	trace_msg( heuristic, 2, "Analyse partner unit connection" );
	for ( PartnerUnitConnection puc : partnerUnitConnections )
	{
		found = 0;

		if ( solver.getTruthValue( puc.var ) == TRUE )
		{
			Pu* unit1 = 0;
			Pu* unit2 = 0;
			for ( unsigned int i = 0; i < partnerUnits.size( ) && found < 2; i++ )
			{
				if ( puc.unit1 == partnerUnits[ i ].name && unit1 == 0 )
				{
					unit1 = &partnerUnits[ i ];
					found++;
				}
				else if ( puc.unit2 == partnerUnits[ i ].name && unit2 == 0 )
				{
					unit2 = &partnerUnits[ i ];
					found++;
				}
			}

			if ( std::find( unit1->connectedTo.begin( ), unit1->connectedTo.end( ), unit2 ) == unit1->connectedTo.end( ) )
				unit1->connectedTo.push_back( unit2 );
			if ( std::find( unit2->connectedTo.begin( ), unit2->connectedTo.end( ), unit1 ) == unit2->connectedTo.end( ) )
				unit2->connectedTo.push_back( unit1 );
		}
	}

	trace_msg( heuristic, 2, "Count connected zones/sensors/partners" );
	for ( unsigned int i = 0; i < partnerUnits.size( ); i++ )
	{
		for ( ZoneAssignment* za : partnerUnits[ i ].usedIn )
		{
			if ( solver.getTruthValue( za->var ) == TRUE )
			{
				if ( za->type == ZONE )
					partnerUnits[ i ].numberOfZones++;
				else
					partnerUnits[ i ].numberOfSensors++;
			}
		}

		partnerUnits[ i ].numberOfPartners = partnerUnits[ i ].connectedTo.size( );
	}

	trace_msg( heuristic, 2, "Start minimizing" );
	for ( unsigned int i = 0; i < partnerUnits.size( ); i++ )
	{
		// add nodes/sensors from unit i to minimized ones (they can not be removed
		if ( partnerUnits[ i ].removed == false )
		{
			for ( ZoneAssignment* za : partnerUnits[ i ].usedIn )
			{
				if ( solver.getTruthValue( za->var ) == TRUE )
					trueInAS->push_back( za->var );
			}
		}

		// check following partner units (remove if neccesary)
		for ( unsigned int j = i + 1; j < partnerUnits.size( ); j++ )
		{
			nPartners = partnerUnits[ i ].numberOfPartners + partnerUnits[ j ].numberOfPartners;
			if ( std::find( partnerUnits[ i ].connectedTo.begin( ), partnerUnits[ i ].connectedTo.end( ), &partnerUnits[ j ] ) != partnerUnits[ i ].connectedTo.end( ) )
				nPartners-=2;
			nZones = partnerUnits[ i ].numberOfZones + partnerUnits[ j ].numberOfZones;
			nSensors = partnerUnits[ i ].numberOfSensors + partnerUnits[ j ].numberOfSensors;

			if ( nZones <= maxElementsOnPu &&
				 nSensors <= maxElementsOnPu &&
				 nPartners <= maxPu &&
				 partnerUnits[ i ].removed == false &&
				 partnerUnits[ j ].removed == false )
			{
				trace_msg( heuristic, 3, "Merge unit " << partnerUnits[ i ].name << " and unit " << partnerUnits[ j ].name << " (unit " << partnerUnits[ j ].name << " is removed)" );

				partnerUnits[ i ].numberOfZones = nZones;
				partnerUnits[ i ].numberOfSensors = nSensors;
				partnerUnits[ i ].numberOfPartners = nPartners;
				partnerUnits[ j ].removed = true;

				removed->push_back( &partnerUnits[ j ] );

				for ( ZoneAssignment* za1 : partnerUnits[ i ].usedIn )
				{
					for ( ZoneAssignment* za2 : partnerUnits[ j ].usedIn )
					{
						// add nodes/sensors from unit2 to unit1 (and minimized ones)
						if ( solver.getTruthValue( za2->var ) == TRUE && za1->to == za2->to )
						{
							trace_msg( heuristic, 4, "Move " << ( za2->type == ZONE ? "zone " : "sensor ") << za2->to << " from unit " << partnerUnits[ i ].name << " to unit " << partnerUnits[ j ].name );

							trueInAS->push_back( za1->var );
							falseInAS->push_back( za2->var );
						}
					}
				}
			}
		}
	}
}

void
PUPHeuristic::printStatistics(
	)
{
	unsigned int partnerUnitsUsed = 0;
	bool found;

	for ( Pu pu : partnerUnits )
	{
		found = false;
		for ( unsigned int i = 0; i < pu.usedIn.size( ) && !found; i++ )
		{
			if ( solver.getTruthValue( pu.usedIn[ i ]->var ) == TRUE )
			{
				found = true;
				partnerUnitsUsed++;
			}
		}
	}

	cout << "Heuristic time (preprocessing) " << pre.count() << " seconds" << endl;
	cout << "Heuristic time (decision making) " << dec.count() << " seconds" << endl;

	cout << sNumberOfConflicts << " conflicts occured" << endl;
	cout << sNumberOfOrdersCreated << " orders were created" << endl;
	cout << sNumberOfRecommendations << " heuristic decisions made" << endl;
	cout << sAlreadyFalse << " suggestions where already set to false " << sAlreadyFalse / sNumberOfRecommendations << " (on average)" << endl;
	cout << sAlreadyTrue << " suggestions where already set to true " << sAlreadyTrue / sNumberOfRecommendations << " (on average)" << endl;
	cout << partnerUnits.size( ) << " partnerunits were specified" << endl;
	cout << partnerUnitsUsed << " partnerunits were used" << endl;
	cout << sNumberOfOrderMaxReached << " times the last element in the order was handled without finding a solution (current solution was checked)" << endl;
	cout << "fallback to minisat: ";
	if ( sFallback == 0 )
		cout << "no" << endl;
	else if ( sFallback == 1 )
		cout << "yes (to finalize solution)" << endl;
	else
		cout << "yes (no solution found)" << endl;
}
