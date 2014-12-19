#include "../include/Algorithmic.h"
#include "../include/Utility.h"
#include <vector>
#include <iostream>

using namespace std ;

int main(int argc, const char* argv[]) {
	vector<vector<int> > graph = generateRandomGraph(10) ;
	for(unsigned int i=0; i<graph.size(); i++){
		cout << i << ": ";
		for (int j =0; j<graph[i].size(); j++){
			cout << graph[i][j] << " ";
		}
		cout << endl;
	}
	vector<int> res ;

	cout << endl << "**************************" << endl; 

	res = bruteForce(graph) ;
	for(unsigned int i = 0 ; i < res.size() ; i++)
		cout << res[i] << " " ;
	cout << endl ;
	cout << poids(graph, res, WEIGHTED,true) ;
	cout << endl ;

	cout << endl << "**************************" << endl; 

	int resBT =  backTracking(graph) ;
	cout << resBT ;
	cout << endl ;

	cout << endl << "**************************" << endl; 

	return EXIT_SUCCESS ;
}
