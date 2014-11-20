#include "../include/Utility.h"
#include <stdlib.h>
#include <time.h>

vector<vector<int> > generateRandomGraph(int n, int maxWeight = 10) {
	/* initialize random seed: */
	srand (time(NULL));

	vector<vector<int> > res ;

	for(int i = 0 ; i < n ; i++) {
		for(int j = i ; j < n ; j++) {
			int tmp = rand() % maxWeight + 1 ;
			res[i][j] = tmp ;
			res[j][i] = tmp ;
		}
	}
}
