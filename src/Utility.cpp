#include "../include/Utility.h"
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <string>
#include <iostream>

#ifdef _MSC_VER // Windows
#include <process.h>
#endif

int getInteger(char* src, int* i, int size) ;

vector<vector<int> > generateRandomGraph(int n, int maxWeight /*= 10*/) {
	/* initialize random seed: */
	srand (time(NULL));

	vector<vector<int> > res ;

	for(int i = 0 ; i < n ; i++) {
		vector<int> tmp (n,0);
		res.push_back(tmp) ;
	}

	for(int i = 0 ; i < n ; i++) {
		for(int j = i + 1; j < n ; j++) {
			int tmp = rand() % maxWeight + 1 ;
			res[i][j] = tmp ;
			res[j][i] = tmp ;
		}
	}
	return res ;
}

/*the files describing a graph must be as follow :
 * N = n (n being the number of nodes)
 * 0 : ... (a node ':' the adjacency list corresponding to this node)
 * ...
 * i : x0, x1, ... , x(n-1) (a node ':' xj = the weight of the edge between the nodes i and j)
 * ...
 * n - 1 : ...
 */
vector<vector<int> > readGraphFromMatrix(char* fileName) {
	/*we open the stream */
	std::fstream fs ;
	fs.open (fileName, std::fstream::in) ;

	/*we extract the number of nodes from the first line*/
	char tmp[256] ; /* TODO : define the size of the line, it can be compute from the number of nodes */
	fs.getline(tmp,256) ;
	int index = 1 ;
	int n = getInteger(tmp, &index, 256) ; /* and store it in n*/

	/* we initiate the matrix */
	vector<vector<int> > res ;
	for(int i = 0 ; i < n ; i++) {
		vector<int> tmp (n,0) ;
		res.push_back(tmp) ;
	}

	/*for each nodes we extract the vector of weight*/
	for(int i = 0 ; i < n ; i++) {
		fs.getline(tmp,256) ;

		int index = 1 ;
		int currNode = getInteger(tmp,&index, 256) ;

		for(int neighboor = 0 ; neighboor < n ; neighboor++) {
			int weight = getInteger(tmp,&index, 256) ;
			res[currNode][neighboor] = weight ;
		}
	}

	return res ;
}

/*the files describing a graph must be as follow :
 * N = n (n being the number of nodes)
 * 0 : ... (a node ':' the adjacency list corresponding to this node)
 * ...
 * i : a b c
 * ...
 * n - 1 : ...
 */
vector<vector<int> > readGraphFromAdjList(char* fileName) {
	/*we open the stream */
	std::fstream fs ;
	fs.open (fileName, std::fstream::in) ;

	/*we extract the number of nodes from the first line*/
	char tmp[256] ; /* TODO : define the size of the line, it can be computed from the number of nodes */
	fs.getline(tmp,256) ;
	int index = 0 ;
	int n = getInteger(tmp, &index, 256) ; /* and store it in n*/

	/* we initiate the matrix */
	vector<vector<int> > res ;
	for(int i = 0 ; i < n ; i++) {
		vector<int> tmp (n,10) ;
		res.push_back(tmp) ;
		res[i][i] = 0 ;
	}

	/*for each nodes we extract the adjacency list*/
	for(int i = 0 ; i < n ; i++) {
		fs.getline(tmp,256) ;
cout << "tmp == " << tmp << endl;
		int index = 0 ;
		int currNode = getInteger(tmp,&index, 256) ;
		cout << currNode << " : ";
		while(index < 256) {
			int neighboor = getInteger(tmp,&index, 256) ;
			cout << neighboor << " ";
			if(neighboor != -1) {
				res[currNode][neighboor] = 1 ;
				res[neighboor][currNode] = 1 ;
			}
		}
		cout << endl;
	}

	return res ;
}

int getInteger(char* src, int* i, int size) {
	int res = -1 ;
	/* we look for the next integer*/
	int index = *i ;
	int tmp = src[index] ;
	while((tmp < '0' || tmp > '9') && (index < size && tmp != '\0')) {// the value in tmp[i]
		(*i) = ++index ;
		tmp = src[index] ;
	}
	if((tmp >= '0' && tmp <= '9') && (index < size && tmp != '\0'))
		res++ ;
	else
		(*i) = size;
	/*and we extract it*/
	while((tmp >= '0' && tmp <= '9') && (index < size && tmp != '\0')) {
		res *= 10 ;
		res += tmp - '0' ;
		(*i) = ++index ;
		tmp = src[index] ;
	}
	return res ;
}


vector<vector<int> > generateGraph(int n, float p){
	std::string nStr;
	std::string pStr;

	/*
	std::ostringstream ss;
	ss << p;
	std::string s(ss.str());
	*/

	nStr = std::to_string(n);
	pStr = std::to_string(p);

	std::string cmd = "./gengraph -format list random "+ nStr + " "+ pStr +" > out.txt";
	system(cmd.c_str());
	vector<vector<int> > ret = readGraphFromAdjList("./out.txt");
	return ret;
}
