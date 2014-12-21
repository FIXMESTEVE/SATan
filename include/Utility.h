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

enum Algorithms { BRUTE, BT, MST, sat };
enum Modes { WEIGHTEDMODE, DISCRETEMODE };

vector<vector<int> > readGraphFromMatrix(char* fileName) ;

vector<vector<int> > readGraphFromAdjList(char* fileName) ;

vector<vector<int> > generateRandomGraph(int n, int maxWeight = 10) ;

vector<vector<int> > generateGraph(int n, float p);

void writeResult(const char* fileName, vector<double> t, vector<int> n, const char* fct);
void writeGraph(const char* fileName, vector<vector<int> > graph) ;

void graphToSAT(const char* fileName, vector<vector<int> > graph, int k);

#endif /* INCLUDE_UTILITY_H_ */

/*
	int nbC = n ;
	nbC += n * (m * (m-1) / 2) ;
	nbC += m * (n * (n-1) / 2) ;
	nbC += n * m * 2 ;
	nbC += n * k ;
	nbC += k ;
	nbC += n * (k * (k-1) / 2) ;
	nbC += k ;
	*/