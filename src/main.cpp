#include "../include/Algorithmic.h"
#include "../include/Utility.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <iostream>

using namespace std ;

int main(int argc, const char* argv[]) {
	//generateGraph(9, 0.8);
	//vector<vector<int> > graph = generateRandomGraph(10) ;
	vector<vector<int> > graph = generateGraph(15,0.6) ;
	for(int i=0; i<graph.size(); i++){
		cout << i << ": ";
		for (int j =0; j<graph[i].size(); j++){
			cout << graph[i][j] << " ";
		}
		cout << endl;
	}
	vector<int> res /*= bruteForce(graph) ;
	for(unsigned int i = 0 ; i < res.size() ; i++)
		printf("%d ", res[i]) */;

	res =  minimumSpanningTree(graph) ;
	for(unsigned int i = 0 ; i < res.size() ; i++){}
	//	printf("%d ", res[i]) ;
	
	return EXIT_SUCCESS ;
}
