/*
 * Algorithmic.h
 *
 *  Created on: 13 nov. 2014
 *      Author: Jérémy
 */

#ifndef INCLUDE_ALGORITHMIC_H_
#define INCLUDE_ALGORITHMIC_H_

#include <vector>

#define WEIGHTED   0
#define DISCRETE   1

using namespace std ;

vector<int> bruteForce(vector<vector<int> > graph, int type = WEIGHTED);

vector<int> backTracking(vector<vector<int> > graph, int type = WEIGHTED, int l = 0, int currLength = -1, int minCost = 1);

vector<int> minimumSpanningTree(vector<vector<int> > graph);

void SAT(vector<vector<int> > graph) ;

#endif /* INCLUDE_ALGORITHMIC_H_ */
