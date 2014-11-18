//Quickperm implementation
//algorithm from www.quickperm.org

#include "../include/Algorithmic.h"

int poids(vector<vector<int>> graph, vector<int> solution){
	//TODO
	return 0;
}

void swap(vector<int> solution, int i, int j){
	//TODO
}

void bruteForce(vector<vector<int>> graph){
	int n = graph.size;
	vector<int> solution;
	vector<int> bestSol;
	int min = poids(graph, solution);

	vector<int> p;
	for (int i = 0; i < n; i++)
		p[i] = solution[i] = bestSol[i] = i;
	p[n] = n;

	for (int i = 1; i < n; i++){
		--p[i];
		int j = (i % 2 == 1) ? p[i] : 0;
		swap(solution, i, j);
		int tmp = poids(graph, solution);
		if (tmp < min){
			min = tmp;
			bestSol = solution.data; //is that similar to using a copy function?
		}
		i = 1; //What the fuck??
		while (p[i] == 0){
			p[i] = i;
			i++;
		}
	}
}
//le cycle commence du sommet zero