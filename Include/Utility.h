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

void readGraph(char* src, int type = SIMPLE) ;

void generateGraph(int n) ;

void writeResult(int res[], int t) ;

vector<vector<int> > generateRandomGraph(int n, int maxWeight = 10) ;

#endif /* INCLUDE_UTILITY_H_ */
