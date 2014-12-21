//Quickperm implementation
//algorithm from www.quickperm.org

#include "../include/Algorithmic.h"
#include "../include/Utility.h"
#include <iostream>
#include <fstream>
#include <string.h>

#define WEIGHTED   0
#define DISCRETE   1


int poids(vector<vector<int> > graph, vector<int> solution, int type){
	int res = 0 ;
	int index = graph.size() - 1;

	if(type == WEIGHTED) {
		for(int i = 0 ; i < index ; i++) {
			res += graph[solution[i]][solution[i+1]] ;
		}
		return res + graph[solution[index]][solution[0]] ;
	} 
	else {
		for(int i = 0 ; i < index ; i++) {
			if( graph[solution[i]][solution[i+1]] == 10 ) {
				res += 1 ;
			}
		}
		if(graph[solution[index]][solution[0]] == 10 ) {
			return res + 1 ;
		}
		else
			return res ;
	}
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

vector<int> bruteForce(vector<vector<int> > graph, int type){
	int n = graph.size() ;
	vector<int> solution (n);
	vector<int> bestSol (n);

	vector<int> p (n-1);
	for (int i = 0; i < n -1 ; i++)
		p[i] = solution[i] = bestSol[i] = i;
	solution[n-1] = bestSol[n-1] = n-1 ;
	int min = poids(graph, solution, type);

	unsigned int i = 1 ;
	while(i < ((unsigned int) n - 1)){
		--p[i];
		int j = (i % 2 == 1) ? p[i] : 0;
		swap(&solution, i+1, j+1);

		int tmp = poids(graph, solution, type);

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


//Backtracking implementation
//algorithm from http://www.win.tue.nl/~kbuchin/teaching/2IL15/backtracking.pdf
vector<int> backTracking(vector<vector<int> > graph, int type) {
	vector<int> begin ;
	for(int i = 0 ; i < graph.size() ; i++)
		begin.push_back(i) ;

	return backTracking_(graph, type, begin, 0, 0, begin);
}

vector<int> backTracking_(vector<vector<int> > graph, int type, vector<int> A, int l, int lengthSoFar, vector<int> Sol) {
	int n = A.size() ;
	int minCost = poids(graph, Sol);
	if(l == n) {
		int newCost = lengthSoFar + graph[A[n-1]][A[0]] ;
		if(newCost < minCost)
			Sol = A ;
	}
	else {
		for(int i = l ; i < n ; i++) {
			swap(&A, l, i) ;
			int newLength = lengthSoFar + graph[A[l-1]][A[l]] ;
			if(newLength <= minCost) {
				vector<int> newSol = backTracking_(graph, type, A, l+1, newLength, Sol) ;
				if(poids(graph, newSol) < minCost) {
					Sol = newSol ;
				}
			}
			swap(&A, l, i) ;
		}
	}

	return Sol ;
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
	quickSort(&edges, 0, edges.size()-1, graph) ;

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
			int uComp = component[u] ;
			for(int j = 0 ; j < component.size() ; j++)
				if(component[j] == uComp)
					component[j] = component[v] ;
			count++ ;
		}
		i++ ;
	}

	//find a hamiltonian path from the minimum spanning tree
	vector<int> cycle ;
	cycle.push_back(edges[0][0]) ;
	cycle.push_back(edges[0][1]) ;

	i = 1 ;
	int cpt = 0 ;
	int cycleSize = mst.size() ;
	while(cpt < cycleSize - 1) {
		int u = mst[i][0] ;
		int v = mst[i][1] ;
		bool added = false ;
		for(int j = 0 ; j < cycle.size() ; j++) {
			if(u == cycle[j]) {
				if(j+1 == cycle.size()) {
					cycle.push_back(v) ;
					cycle.push_back(u) ;
				} else {
					cycle.insert(cycle.begin() + j+1,v) ;
					cycle.insert(cycle.begin() + j+2,u) ;
				}
				cpt++ ;
				added = true ;
				break ;
			}
			else if(v == cycle[j]) {
				if(j+1 == cycle.size()) {
					cycle.push_back(u) ;
					cycle.push_back(v) ;
				} else {
					cycle.insert(cycle.begin() + j+1,u) ;
					cycle.insert(cycle.begin() + j+2,v) ;
				}
				cpt++ ;
				added = true ;
				break ;
			}
		}
		if(!added)
			mst.push_back(edges[i]) ;
		i++ ;
	}
	return cycle ;
}

int SAT(vector<vector<int> > graph){
	for(int i = 0 ; i < graph.size(); i++) {
	  graphToSAT("tmpSAT", graph, i) ;
	 
	  FILE *fpipe;
	  char *command = "./glucose-syrup_static tmpSAT -model | grep \"s SATISFIABLE\"";
	  char path[1035];
	  if(0 == (fpipe = (FILE*)popen(command,"r"))){
	    printf("Failed to run glucose!\n");
	    exit(1);
	  }

	
	  while(fgets(path, sizeof(path)-1, fpipe) != NULL ){
	    if(strcmp(path, "s SATISFIABLE") == 0)
	      return i;
	  }
	  
	}
	return graph.size() ;
}

