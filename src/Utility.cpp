#include "../include/Utility.h"
#include <fstream>
#include <string>
#include <iostream>
#include <ctime>
#include <sstream>

#ifdef _MSC_VER // Windows
#include <process.h>
#endif


int getInteger(string src, int* i) ;

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
vector<vector<int> > readGraphFromMatrix(char const * fileName) {
	/*we open the stream */
	std::fstream fs ;
	fs.open (fileName, std::fstream::in) ;

	/*we extract the number of nodes from the first line*/
	string tmp ; /* TODO : define the size of the line, it can be compute from the number of nodes */
	getline(fs, tmp) ;
	int index = 1 ;
	int n = getInteger(tmp, &index) ; /* and store it in n*/

	/* we initiate the matrix */
	vector<vector<int> > res ;
	for(int i = 0 ; i < n ; i++) {
		vector<int> tmp (n,0) ;
		res.push_back(tmp) ;
	}

	/*for each nodes we extract the vector of weight*/
	for(int i = 0 ; i < n ; i++) {
		getline(fs, tmp) ;

		int index = 1 ;
		int currNode = getInteger(tmp,&index) ;

		for(int neighboor = 0 ; neighboor < n ; neighboor++) {
			int weight = getInteger(tmp,&index) ;
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
vector<vector<int> > readGraphFromAdjList(char const * fileName) {
	/*we open the stream */
	std::fstream fs ;
	fs.open(fileName, std::fstream::in) ;

	/*we extract the number of nodes from the first line*/
	string tmp ;
	getline(fs, tmp) ;
	int index = 0 ;
	int n = getInteger(tmp, &index) ; /* and store it in n*/

	/* we initiate the matrix */
	vector<vector<int> > res ;
	for(int i = 0 ; i < n ; i++) {
		vector<int> tmp (n,10) ;
		res.push_back(tmp) ;
		res[i][i] = 0 ;
	}

	/*for each nodes we extract the adjacency list*/
	for(int i = 0 ; i < n ; i++) {
		getline(fs,tmp) ;
		int index = 0 ;
		int currNode = getInteger(tmp,&index) ;

		int neighboor = getInteger(tmp,&index) ;
		while(neighboor != -1) {
			res[currNode][neighboor] = 1 ;
			res[neighboor][currNode] = 1 ;
			neighboor = getInteger(tmp,&index) ;
		}
	}

	return res ;
}

int getInteger(string src, int* i) {
	int res = -1 ;
	/* we look for the next integer*/
	int tmp = src[(*i)] ;
	while((tmp < '0' || tmp > '9') && (tmp != '\0')) {// the value in tmp[i]
		tmp = src[++(*i)] ;
	}
	if((tmp >= '0' && tmp <= '9') && (tmp != '\0'))
		res++ ;
	/*and we extract it*/
	while((tmp >= '0' && tmp <= '9') && (tmp != '\0')) {
		res *= 10 ;
		res += tmp - '0' ;
		tmp = src[++(*i)] ;
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

void writeResult(const char* fileName, vector<double> t, vector<int> n, const char* fct) {
	std::filebuf fb;
	fb.open(fileName, std::ios::out) ;
	std::ostream os(&fb);

	os << "#" << fct << endl ;
	os << "# |V(G)| time" << endl ;
	for(int i = 0 ; i < t.size() ; i++) {
		os << n[i] << " " << t[i] << endl ;
	}
	fb.close() ;
}