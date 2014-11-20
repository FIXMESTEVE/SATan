//Quickperm implementation
//algorithm from www.quickperm.org

#include "../include/Algorithmic.h"
#include <stdio.h>
#include <time.h>

int poids(vector<vector<int> > graph, vector<int> solution){
	int res = 0 ;
	int index = graph.size() - 1;

	for(int i = 0 ; i < index ; i++)
		res += graph[solution[i]][solution[i+1]] ;

	return res + graph[index][0] ;
}

void swap(vector<int> *solution, int i, int j){
	int tmp = (*solution)[i] ;
	(*solution)[i] = (*solution)[j] ;
	(*solution)[j] = tmp ;
}

long int factorielle(long int n) {
	if(n <= 1)
		return 1 ;
	return n * factorielle(n-1) ;
}

vector<int> bruteForce(vector<vector<int> > graph){
	int n = graph.size() ;
	vector<int> solution (n);
	vector<int> bestSol (n);

	vector<int> p (n-1);
	for (int i = 0; i < n -1 ; i++)
		p[i] = solution[i] = bestSol[i] = i;
	solution[n-1] = bestSol[n-1] = n-1 ;
	int min = poids(graph, solution);

	int cpt = 0 ;
	long int NbPerm = factorielle(n-1) ;

	time_t t ;
	time(&t) ;
	printf("%d/%ld\n",++cpt,NbPerm) ;

	unsigned int i = 1 ;
	while(i < n - 1){
		--p[i];
		int j = (i % 2 == 1) ? p[i] : 0;
		swap(&solution, i+1, j+1);

		time_t t2 ;
		time(&t2) ;
		if(difftime(t2,t) >= 1.) {
			t = t2 ;
			printf("%d/%ld\n",++cpt,NbPerm) ;
		}

		int tmp = poids(graph, solution);
		if (tmp < min){
			min = tmp;
			bestSol = solution ;
		}
		i = 1;
		while (i < p.size() && p[i] == 0){
			p[i] = i;
			i++;
		}
	}
	return bestSol ;
}
//le cycle commence du sommet zero
