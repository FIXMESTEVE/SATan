//Quickperm implementation
//algorithm from www.quickperm.org

#include "../include/Algorithmic.h"

int poids(vector<vector<int> > graph, vector<int> solution){
	int res = 0 ;
	int index = graph.size() - 1;

	for(int i = 0 ; i < index ; i++)
		res += graph[solution[i]][solution[i+1]] ;

	return res + graph[index][0] ;
}

void swap(vector<int> solution, int i, int j){
	int tmp = solution[i] ;
	solution[i] = solution[j] ;
	solution[j] = tmp ;
}

vector<int> bruteForce(vector<vector<int> > graph){
	int n = graph.size() ;
	vector<int> solution;
	vector<int> bestSol;
	int min = poids(graph, solution);

	vector<int> p;
	for (int i = 0; i < n -1 ; i++)
		p[i] = solution[i] = bestSol[i] = i;
	p[n - 1] = n;
	solution[n-1] = bestSol[n-1] = n-1 ;

	int i = 1 ;
	while(i < n){
		--p[i];
		int j = (i % 2 == 1) ? p[i] : 0;
		swap(solution, i+1, j+1);
		int tmp = poids(graph, solution);
		if (tmp < min){
			min = tmp;
			bestSol = solution ; //is that similar to using a copy function?
		}
		i = 1; //What the fuck??
		while (p[i] == 0){
			p[i] = i;
			i++;
		}
	}
	return bestSol ;
}
//le cycle commence du sommet zero
