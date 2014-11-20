/*
 * Algorithmic.h
 *
 *  Created on: 13 nov. 2014
 *      Author: Jérémy
 */

#ifndef INCLUDE_ALGORITHMIC_H_
#define INCLUDE_ALGORITHMIC_H_

#include <vector>

using namespace std ;

vector<int> bruteForce(vector<vector<int> > graph);

void approximation(vector<vector<int> > graph);

void SAT(vector<vector<int> > graph) ;

#endif /* INCLUDE_ALGORITHMIC_H_ */
