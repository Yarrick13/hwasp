/*
 *
 *  Copyright 2013 Mario Alviano, Carmine Dodaro, Wolfgang Faber, Nicola Leone, Francesco Ricca, and Marco Sirianni.
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

/* 
 * File:   SequenceBasedRestartsStrategy.h
 * Author: Carmine Dodaro
 *
 * Created on 6 August 2013, 21.15
 */

#ifndef SEQUENCEBASEDRESTARTSSTRATEGY_H
#define	SEQUENCEBASEDRESTARTSSTRATEGY_H

#include "RestartsStrategy.h"

class SequenceBasedRestartsStrategy : public RestartsStrategy
{
    public:
        inline SequenceBasedRestartsStrategy( unsigned int threshold = 32 );
        virtual bool onLearningClause();
        virtual void onLearningUnaryClause();
        
    protected:
        virtual void computeNextRestartValue();

    private:
        unsigned int computeRestartNumber( unsigned int i );
        unsigned int numberOfRestarts;
};

SequenceBasedRestartsStrategy::SequenceBasedRestartsStrategy( 
    unsigned int threshold ) : RestartsStrategy( threshold ), numberOfRestarts( 1 )
{
    nextRestartValue = computeRestartNumber( numberOfRestarts ) * threshold;
}

#endif	/* SEQUENCEBASEDRESTARTSSTRATEGY_H */
