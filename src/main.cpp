#include "../include/Algorithmic.h"
#include "../include/Utility.h"
#include <vector>
#include <iostream>
#include <string.h>

using namespace std ;

void printUsage(){
	cout<<"Usage: main mode type"<<endl;
	cout<<"Available modes: discrete, weighted, custom" << endl;
	cout<<"Available types for discrete: brute, bt, mst, sat" << endl;
	cout<<"Available types for weighted: brute, bt, mst" << endl;
	cout<<"Available types for custom: " << endl;
	cout<<"		weighted n w1 w2 (n: number of nodes, p1: minimum weight, p2: maximum weight)"<<endl;
	cout<<"		discrete n p (n: number of nodes, p: probability of pairing between 2 nodes)"<<endl;
}

void useDiscrete(Algorithms ALGO){
	//TODO
}

void useWeighted(Algorithms ALGO){
	//TODO
}

void create(Modes MODE, int n, int w1, int w2, int p){
	//TODO
	//when using weighted, use n, w1 and w2 parameters
	//when using discrete, use n and p parameters
}
int newMain(int argc, const char* argv[]){
	if(argc < 2)
		printUsage();		
	else if(strcmp(argv[1], "discrete") == 0){
		if(argc < 3)
			printUsage();
		else if(strcmp(argv[2], "brute") == 0){
			useDiscrete(BRUTE);
		}
		else if(strcmp(argv[2], "bt") == 0){
			useDiscrete(BT);
		}
		else if(strcmp(argv[2], "mst") == 0){
			useDiscrete(MST);
		}
		else if(strcmp(argv[2], "sat") == 0){
			useDiscrete(sat);
		}
		else printUsage();
	}
	else if(strcmp(argv[1], "weighted") == 0){
		if(argc < 3)
			printUsage();
		else if(strcmp(argv[2], "brute") == 0){
			useWeighted(BRUTE);
		}
		else if(strcmp(argv[2], "bt") == 0){
			useWeighted(BT);
		}
		else if(strcmp(argv[2], "mst") == 0){
			useWeighted(MST);
		}
		else printUsage();
	}
	else if(strcmp(argv[1], "create") == 0){
		if(argc < 3)
			printUsage();
		else if(strcmp(argv[2], "weighted") == 0){
			if(argc < 6)
				printUsage();
			else{
				int n = strtol(argv[3], NULL, 10);
				int w1 = strtol(argv[4], NULL, 10);
				int w2 = strtol(argv[5], NULL, 10);

				create(weighted, n, w1, w2, 1);
			}
		}
		else if(strcmp(argv[2], "discrete") == 0){
			if(argc < 5)
				printUsage();
			else{
				int n = strtol(argv[3], NULL, 10);
				int p = strtol(argv[4], NULL, 10);
				
				create(discrete, n, 0, 0, p);
			}
		}
		else printUsage();
	}
	else{
		printUsage();		
	}
}

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
