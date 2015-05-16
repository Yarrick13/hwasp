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

#ifndef PUPHEURISTIC_H
#define	PUPHEURISTIC_H

#include "Heuristic.h"

class PUPHeuristic : public Heuristic
{
    public:
        PUPHeuristic( Solver& solver );
        ~PUPHeuristic();
        void onNewVariable( Var ){ assert( 0 && "IMPLEMENT ME!" ); }
        void onNewVariableRuntime( Var ){ assert( 0 && "IMPLEMENT ME!" ); }
        void onLiteralInvolvedInConflict( Literal ){ assert( 0 && "IMPLEMENT ME!" ); }
        void onUnrollingVariable( Var ){ assert( 0 && "IMPLEMENT ME!" ); }
        void incrementHeuristicValues( Var ){ assert( 0 && "IMPLEMENT ME!" ); }
        void simplifyVariablesAtLevelZero(){ assert( 0 && "IMPLEMENT ME!" ); }
        void conflictOccurred(){ assert( 0 && "IMPLEMENT ME!" ); }

    protected:
        Literal makeAChoiceProtected() { assert( 0 && "IMPLEMENT ME!" ); return Literal::null; }

    private:

};

#endif

