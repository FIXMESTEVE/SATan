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
	while(i < ((unsigned int) n - 1)){
		--p[i];
		int j = (i % 2 == 1) ? p[i] : 0;
		swap(&solution, i+1, j+1);

		int tmp = poids(graph, solution);
		++cpt ;

		time_t t2 ;
		time(&t2) ;
		if(difftime(t2,t) >= 1.) {
			t = t2 ;
			printf("%d/%ld\n",cpt,NbPerm) ;
		}

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

int edgeWeight(vector<int> edge, vector<vector<int> > graph) {
	return graph[edge[0]][edge[1]] ;
}

int partitionner(vector<vector<int> > *edges, int begin, int end, int pivot, vector<vector<int> > graph) {
	vector<int> tmp = (*edges)[pivot] ;
	(*edges)[pivot] = (*edges)[end] ;
	(*edges)[end] = tmp ;
	int j = begin ;
	for(int i = begin ; i < end - 1 ; i++) {
		if( edgeWeight((*edges)[i],graph) <= edgeWeight((*edges)[end],graph) ) {
			tmp = (*edges)[i] ;
			(*edges)[i] = (*edges)[j] ;
			(*edges)[j++] = tmp ;
		}
	}
	tmp = (*edges)[end] ;
	(*edges)[end] = (*edges)[j] ;
	(*edges)[j] = tmp ;
	return j ;
}

void quickSort(vector<vector<int> > *edges, int begin, int end, vector<vector<int> > graph) {
	if(begin < end) {
		int pivot = begin ;
	    pivot = partitionner(edges, begin, end, pivot, graph) ;
	    quickSort(edges ,begin, pivot-1, graph) ;
	    quickSort(edges, pivot+1, end, graph) ;
	}
}

vector<int> minimumSpanningTree(vector<vector<int> > graph) {
	vector<vector<int> > mst ;
	vector<vector<int> > edges ;
	vector<int> component ;

	//we put the edges in 'edges'
	for(int i = 0 ; i < graph.size() ; i++) {
		for(int j = i + 1 ; j < graph.size() ; j++) {
			vector<int> currEdge ;
			currEdge.push_back(i) ;
			currEdge.push_back(j) ;
			edges.push_back(currEdge) ;
		}
	}
	//we sort 'edges'
	quickSort(&edges, 0, edges.size(), graph) ;

	//we initialize the component
	for(int i = 0 ; i < graph.size() ; i++)
		component.push_back(i) ;

	int count = 0 ;
	int i = 0 ;
	while(count < (graph.size() - 1) && i < edges.size()) {
		int u = edges[i][0] ;
		int v = edges[i][1] ;
		if(component[u] != component[v]) {
			mst.push_back(edges[i]) ;
			component[u] = component[v] ;
		}
	}

	//find a hamiltonian path from the minimum spanning tree
}
