#include "../include/Algorithmic.h"
#include "../include/Utility.h"
#include <vector>
#include <iostream>
#include <cstring>
#include <string>

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
	vector<double> t;
	vector<int> n;
	if(ALGO == BRUTE) {
		for(int i = 10 ; i <= 100 ; i = i+10) {
			n.push_back(i);
			double res = 0 ;
			for(int j = 0 ; j < 10 ; j++) {
				vector<vector<int> > graph = generateGraph(i,0.6) ;
				clock_t t;
		  		t = clock();
		  		bruteForce(graph, DISCRETE);
		  		t = clock() - t;
		  		res += (double) ((float)t)/CLOCKS_PER_SEC;
			}
			res = res / 10;
			t.push_back(res);
			string fileName("discrete_brute_") ;
			fileName.append(to_string(i)) ;
			writeResult(fileName.c_str(), t, n, "brute for discrete") ;
		}
	}
	else if(ALGO == BT) {
		for(int i = 10 ; i <= 100 ; i = i+10) {
			n.push_back(i);
			double res = 0 ;
			for(int j = 0 ; j < 10 ; j++) {
				vector<vector<int> > graph = generateGraph(i,0.6) ;
				clock_t t;
		  		t = clock();
		  		backTracking(graph, DISCRETE);
		  		t = clock() - t;
		  		res += (double) ((float)t)/CLOCKS_PER_SEC;
			}
			res = res / 10;
			t.push_back(res);
			string fileName("discrete_bt_") ;
			fileName.append(to_string(i)) ;
			writeResult(fileName.c_str(), t, n, "backTracking for discrete") ;
		}

	}
	else if(ALGO == MST) {
		for(int i = 10 ; i <= 100 ; i = i+10) {
			n.push_back(i);
			double res = 0 ;
			for(int j = 0 ; j < 10 ; j++) {
				vector<vector<int> > graph = generateGraph(i,0.6) ;
				clock_t t;
		  		t = clock();
		  		minimumSpanningTree(graph);
		  		t = clock() - t;
		  		res += (double) ((float)t)/CLOCKS_PER_SEC;
			}
			res = res / 10;
			t.push_back(res);
			string fileName("discrete_mst_") ;
			fileName.append(to_string(i)) ;
			writeResult(fileName.c_str(), t, n, "minimumSpanningTree for discrete") ;
		}

	}
	else if (ALGO == sat) {
		for(int i = 10 ; i <= 100 ; i = i+10) {
			n.push_back(i);
			double res = 0 ;
			for(int j = 0 ; j < 10 ; j++) {
				vector<vector<int> > graph = generateGraph(i,0.6) ;
				clock_t t;
		  		t = clock();
		  		SAT(graph);
		  		t = clock() - t;
		  		res += (double) ((float)t)/CLOCKS_PER_SEC;
			}
			res = res / 10;
			t.push_back(res);
			string fileName("discrete_sat_") ;
			fileName.append(to_string(i)) ;
			writeResult(fileName.c_str(), t, n, "sat for discrete") ;
		}

	}
}

void useWeighted(Algorithms ALGO){
	vector<double> t;
	vector<int> n;
	if(ALGO == BRUTE) {
		for(int i = 10 ; i <= 100 ; i = i+10) {
			n.push_back(i);
			double res = 0 ;
			for(int j = 0 ; j < 10 ; j++) {
				vector<vector<int> > graph = generateRandomGraph(i,i*10) ;
				clock_t t;
		  		t = clock();
		  		bruteForce(graph);
		  		t = clock() - t;
		  		res += (double) ((float)t)/CLOCKS_PER_SEC;
			}
			res = res / 10;
			t.push_back(res);
			string fileName("weighted_brute_") ;
			fileName.append(to_string(i)) ;
			writeResult(fileName.c_str(), t, n, "brute for weighted") ;
		}
	}
	else if(ALGO == BT) {
		for(int i = 10 ; i <= 100 ; i = i+10) {
			n.push_back(i);
			double res = 0 ;
			for(int j = 0 ; j < 10 ; j++) {
				vector<vector<int> > graph = generateRandomGraph(i,i*10) ;
				clock_t t;
		  		t = clock();
		  		backTracking(graph);
		  		t = clock() - t;
		  		res += (double) ((float)t)/CLOCKS_PER_SEC;
			}
			res = res / 10;
			t.push_back(res);
			string fileName("weighted_bt") ;
			fileName.append(to_string(i)) ;
			writeResult(fileName.c_str(), t, n, "backTracking for weighted") ;
		}
	}
	else if(ALGO == MST) {
		for(int i = 10 ; i <= 100 ; i = i+10) {
			n.push_back(i);
			double res = 0 ;
			for(int j = 0 ; j < 10 ; j++) {
				vector<vector<int> > graph = generateRandomGraph(i,i*10) ;
				clock_t t;
		  		t = clock();
		  		minimumSpanningTree(graph);
		  		t = clock() - t;
		  		res += (double) ((float)t)/CLOCKS_PER_SEC;
			}
			res = res / 10;
			t.push_back(res);
			string fileName("weighted_mst_") ;
			fileName.append(to_string(i)) ;
			writeResult(fileName.c_str(), t, n, "mst for weighted") ;
		}
	}
}

int main(int argc, const char* argv[]){
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
	else{
		printUsage();		
	}

	return EXIT_SUCCESS;
}