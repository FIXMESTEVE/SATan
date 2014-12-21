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
	ofstream os ;
	os.open(fileName) ;

	os << "#" << fct << endl ;
	os << "# |V(G)| time" << endl ;
	for(int i = 0 ; i < t.size() ; i++) {
		os << n[i] << " " << t[i] << endl ;
	}
	os.close() ;
	cout << "done writing results" << endl ;
}

void writeGraph(const char* fileName, vector<vector<int> > graph) {
	ofstream os ;
	os.open(fileName) ;

	os << "N = " << graph.size() << endl;
	for(int i = 0 ; i < graph.size(); i++) {
		os << i << " : " ;
		for(int j = 0 ; j < graph[i].size() ; j++)
			os << graph[i][j] << " " ;
		os << endl;
	}
	os.close();
}

void graphToSAT(const char* fileName, vector<vector<int> > graph, int k) {
	int n = graph.size() ;
	int m = n * (n-1) / 2 ;
	int nbVar = m * n ; //nb d'arêtes x taille du cycle <=> nb de x_i,u,v
	nbVar += k * n ; //taille du cycle * nb d'arêtes de poids 10 ;
	int nbC = n ;
	nbC += n * (m * (m-1) / 2) ;
	nbC += m * (n * (n-1) / 2) ;
	nbC += n * m * 2 ;
	nbC += n * k ;
	nbC += k ;
	nbC += n * (k * (k-1) / 2) ;
	nbC += k * n * (n-1)/2 ;

	int cpt = 1 ;
	vector<vector<vector<int> > > var1 ;
	vector<vector<int> > var2 ;
	for(int i1 = 1 ; i1 <= n ; i1++) {
		vector<vector<int> > tmp1 ;
		for(int i2 = 1 ; i2 <= n ; i2++) {
			vector<int> tmp2 ;
			for(int i3 = 1 ; i3 <= n ; i3++) {
				if(i3 > i2) {
					tmp2.push_back(cpt) ;
					cpt++ ;
				}
				else
					tmp2.push_back(0);
			}
			tmp1.push_back(tmp2) ;
		}
		var1.push_back(tmp1) ;
	} 

	for(int i2 = 1 ; i2 <= k ; i2++) {
		vector<int> tmp2 ;
		for(int i3 = 1 ; i3 <= n ; i3++) {
			tmp2.push_back(cpt) ;
			cpt++ ;
		}
		var2.push_back(tmp2) ;
	} 

	ofstream os ;
	os.open(fileName) ;
	os << "p cnf " << nbVar << " " << nbC << endl;

	/*	*****	*/
	/* on commence à générer la formule sat*/
	/*	*****	*/

	cpt = 0;
	/*à chaque position une arête*/
	for(int p = 0 ; p < n ; p++) { 
		for(int u = 0 ; u < n -1; u++) {
			for(int v = u + 1 ; v < n ; v++) {
				os << var1[p][u][v] << " ";
			}
		}
		os << "0" << endl; 
		cpt++ ;
	}

	cpt = 0;
	/* à cahque position une seul arête*/
	for(int p = 0 ; p < n ; p++) {
		for(int u1 = 0 ; u1 < n -1; u1++) {
			for(int v1 = u1 + 1 ; v1 < n ; v1++ ) {
				int u2 = u1 ;
				for(int v2 = v1+1 ; v2 < n ; v2++) {
					cpt++ ;
					os << "-" << var1[p][u1][v1] << " " << "-" << var1[p][u2][v2] << " 0" << endl;
				}
				for(u2 = u1 + 1; u2 < n -1; u2++) {
					for(int v2 = u2+1; v2 < n ; v2++) {
						cpt++ ;
						os << "-" << var1[p][u1][v1] << " " << "-" << var1[p][u2][v2] << " 0" << endl;
					}
				}
			}
		}
	}

	cpt = 0 ;
	/* à chaque arête une position au max*/
	for(int u = 0; u < n -1; u++) {
		for(int v = u+1; v < n ; v++) {
			for(int p1 = 0 ; p1 < n - 1; p1++) {
				for(int p2 = p1 + 1; p2 < n ; p2++){
					cpt++;
					os << "-" << var1[p1][u][v] << " " << "-" << var1[p2][u][v] << " 0" << endl;
				}
			}
		}
	}

	cpt = 0 ;
	/* on veut un cycle */
	for(int p = 0 ; p < n ; p++) {
		for(int u = 0 ; u < n -1; u++) {
			for(int v = u+1 ; v < n ; v++) {
				os << "-" << var1[p][u][v] << " ";
				for(int w = 0 ; w < n ; w++) {
					if(w < v) {
						os << var1[(p+1)%n][w][v] << " ";
					}
					else if (v < w) {
						os << var1[(p+1)%n][v][w] << " ";
					}
				} 
				os << "0" << endl ;
				cpt++;
				os << "-" << var1[p][u][v] << " ";
				for(int w = 0 ; w < n ; w++) {
					if(w < u) {
						os << var1[(p+n-1)%n][w][u] << " ";
					}
					else if (u < w) {
						os << var1[(p+n-1)%n][u][w] << " ";
					}
				} 
				os << "0" << endl ;
				cpt++;
			}
		}
	}

	cpt = 0 ;
	/* soit en position p il n'y a pas d'arête de poids 10 soit x_ij est vrai*/ 
	for(int p = 0 ; p < n ; p++) {
		for( int j = 0 ; j < k ; j++) {
			os << "-" << var2[j][p] << " " ;
			for(int u = 0 ; u < n -1; u++) {
				for(int v = u + 1; v < n ; v++) {
					if(graph[u][v] == 10)
						os << var1[p][u][v] << " " ;
				}
			}
			cpt++;
			os << "0" << endl ;
		}
	}

	cpt = 0;
	/* pour chaque j au moins une p valide, on a bien k arête de poids  10*/
	for(int j = 0 ; j < k ; j++) {
		for(int p = 0 ; p < n ; p++) {
			os << var2[j][p] << " ";
		}
		cpt++;
		os << "0" << endl;
	}

	cpt = 0;
	/*pour chaque position de poids 10, on a au plus une arête*/
	for(int p = 0 ; p < n ; p++ ) {
		for(int j1 = 0 ; j1 < k -1; j1++) {
			for(int j2 = j1 + 1 ; j2 < k ; j2++) {
				cpt++;
				os << "-" << var2[j1][p] << " " << "-" << var2[j2][p] << " 0" << endl ;
			}
		}
	}

	cpt = 0 ;
	/* pour chaque position de poids 10 on a au plus une arête dans le cycle*/  
	for(int p1 = 0 ; p1 < n -1; p1++) {
		for(int p2 = p1 + 1 ; p2 < n ; p2++) {
			for(int j = 0 ; j < k ; j++) {
				cpt++;
				os << "-" << var2[j][p1] << " " << "-" << var2[j][p2] << " 0" << endl ;
			}
		}
	}

	os.close() ;
}