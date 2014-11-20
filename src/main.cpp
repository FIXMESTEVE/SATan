#include "../include/Algorithmic.h"
#include "../include/Utility.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>

using namespace std ;

int main(int argc, const char* argv[]) {
	vector<vector<int> > graph = generateRandomGraph(8) ;
	vector<int> res = bruteForce(graph) ;
	for(int i = 0 ; i < res.size() ; i++)
		printf("%s ", res[i]) ;
}
