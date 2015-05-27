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

#ifndef HEURISTIC_H
#define	HEURISTIC_H

#include "util/Assert.h"
#include "Literal.h"
#include <vector>
using namespace std;

class Solver;

class Heuristic
{
    public:
        inline Heuristic( Solver& s ) : solver( s ){}
        virtual ~Heuristic(){};

        Literal makeAChoice();
        virtual void onNewVariable( Var v ) = 0;
        virtual void onNewVariableRuntime( Var v ) = 0;
        virtual void onReadAtomTable ( Var v,  string name ) = 0;
        virtual void onFinishedParsing ( ) = 0;
        virtual void onLiteralInvolvedInConflict( Literal literal ) = 0;
        virtual void onUnrollingVariable( Var var ) = 0;
        virtual void incrementHeuristicValues( Var var ) = 0;
        virtual void simplifyVariablesAtLevelZero() = 0;
        virtual void conflictOccurred() = 0;
        inline void addPreferredChoice( Literal lit ){ assert( lit != Literal::null ); preferredChoices.push_back( lit ); }
        inline void removePrefChoices() { preferredChoices.clear(); }

    protected:
        virtual Literal makeAChoiceProtected() = 0;
        Solver& solver;

    private:
        vector< Literal > preferredChoices;    
};

#endif
