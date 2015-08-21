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

#include "HeuristicUtil.h"
#include "Assert.h"

/*
 * parse term of an unary atoms
 * 		( zones, sensors and partner units )
 *
 * 	@param atom 	the atom
 * 	@param name		the term
 */
void
HeuristicUtil::getName(
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
HeuristicUtil::getName(
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

char
HeuristicUtil::tolower(
	char in )
{
	if(in<='Z' && in>='A')
		return in-('Z'-'z');
	return in;
}
