/*
 * Utility.h
 *
 *  Created on: 13 nov. 2014
 *      Author: Jérémy
 */

#ifndef INCLUDE_UTILITY_H_
#define INCLUDE_UTILITY_H_

#include <vector>

using namespace std ;

#define SIMPLE 0
#define Matrix 1

vector<vector<int> > readGraphFromMatrix(char* fileName) ;

vector<vector<int> > readGraphFromAdjList(char* fileName) ;

vector<vector<int> > generateRandomGraph(int n, int maxWeight = 10) ;

vector<vector<int> > generateGraph(int n, float p);
#endif /* INCLUDE_UTILITY_H_ */
