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

int poids(vector<vector<int> > graph, vector<int> solution, int type = WEIGHTED);

vector<int> bruteForce(vector<vector<int> > graph, int type = WEIGHTED);

vector<int> backTracking(vector<vector<int> > graph, int type = WEIGHTED);
vector<int> backTracking_(vector<vector<int> > graph, int type, vector<int> A, int l, int lengthSoFar, vector<int> Sol);
vector<int> minimumSpanningTree(vector<vector<int> > graph);

int SAT(vector<vector<int> > graph) ;

#endif /* INCLUDE_ALGORITHMIC_H_ */
