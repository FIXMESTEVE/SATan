/*utf8*

   Graph Generator - v3.3 - © Cyril Gavoille - Juillet 2014

     Use: gengraph [-option] graph_name [parameter_list]

     A free command-line program to generate undirected graphs in many
     formats: plain text, .dot, .pdf, .fig, .svg ... Formats like .pdf
     or .fig are produced thanks to GraphViz and allow visualization.
     Easy to install, there is a single .c file to compile. There is a
     on-line manual available in the command (for the moment the
     manual is in French only and included at the end of the source).

                                -----

     Petit programme libre en ligne de commande permettant de générer
     des graphes non orientés dans plein de formats différents: texte,
     .dot, .pdf, .fig, .svg ... Certains formats (comme .pdf ou .jpg),
     produit grâce à GraphViz, permettent la visualiser des graphes.
     Très simple à installer, il n'y a qu'un seul fichier .c à
     compiler. Pour le reste il faut voir l'aide en ligne qui est
     aussi incluse à la fin du source.


   Comment l'installer / le compiler ?

     (MacOs)  gcc gengraph.c -o gengraph
     (Linux)  gcc gengraph.c -lm -o gengraph

*/

#include <stdio.h>  /* pour printf(), sprintf() ... */
#include <stdlib.h> /* pour system(), RAND_MAX ... */
#include <string.h> /* pour strcomp() ... */
#include <unistd.h> /* pour getpid() ... */
#include <math.h>   /* pour cos(), acos(), hypot() ... */
#include <float.h>  /* pour DBL_MAX ... */
#include <limits.h> /* pour strtod(), INT_MAX, LONG_MAX, DBL_MAX ... */

/* types */

/* graphe ou famille de graphes */
typedef struct _graph {
  int id;    // numéro du graphe, utilisé pour les familles de graphes 
  int n;     // nb de sommets, <0 si non défini
  int m;     // nb d'arêtes, <0 si non défini
  int sort;  // vrai ssi les listes d'adjacences sont triées
  int *d;    // tableau des degrés, =NULL si non défini
  int **L;   // liste d'adjacence, =NULL si non défini
             // L[u][i]: i-ème voisin du sommet u
  int f;     // nb de graphes de la famille, =0 si graphe normal
  int sym;   // vrai ssi les listes d'adjacence du graphe sont symétriques
  double **W;// tableau de poids des arcs, =NULL si non défini 
             // W[u][i]: poids du i-ème voisin du sommet u
  double *xpos,*ypos; // tableau de positions des sommets (graphe géométrique)
  struct _graph **G; // tableau de graphes, =NULL si non défini

  // les champs suivants sont utilisés pour communiquer des valeurs
  // ou des résultats à travers les appels de fonctions

  int int1;  // paramètre: entier
  int* pint1;// paramètre: tableau d'entiers
} graph;

typedef int test(graph*); /* fonction de test sur un graphe */

/* chemin simple d'un graphe G */
typedef struct{
  int n; /* nb de sommets du chemin */
  int *P; /* liste ordonnée des sommets du chemin */
  int *V; /* V[u]=i si le sommet u de G est le i-ème (i dans [0,G->n[)
	     sommet du chemin (V[P[i]]=i), V[u]=-1 si u n'est dans le
	     chemin  */
  int **aux; /* tableau auxiliaire utilisé (et géré) par NextPath() */
  int nG; /* nb de sommets du graphe G, sert pour free_path() */
} path;

typedef int adjacence(int,int); /* fonction d'adjacence */
typedef struct _list { int item,type; struct _list *next; } list; /* liste chaînée */
typedef struct { double x,y; } point; /* un point du plan */
typedef struct { int u,v; } edge; /* une arête */
typedef struct { unsigned int r:8,g:8,b:8; } color;
/* une couleur est un int accessible par les champs r,g,b de 8 bits */

/* constantes */

#define DIMAX   32  /* nb maximum de paramètres pour un graphe */
#define CMDMAX  512 /* nb maximum de caractères sur la ligne de commande */
#define NAMEMAX 20  /* nb maximum de caractères d'un nom de sommet */
#define PARAMSIZE 1024 /* taille mémoire des buffers FPARAM et CPARAM (en octets) */

/* constantes pour le format dot (-visu) */

#define GRAPHPDF "g.pdf" /* nom du graphe de sortie par défaut */
double VSIZESTD=0.05; /* taille standard des sommets */
double VSIZEXY=0.12;  /* taille des sommets pour les graphes géométriques */
double LEN=1.0; /* longueur des arêtes avec dot neato */
#define VSIZEK 5.5 /* rapport entre taille max et min des sommets (-vsize) */
#define C32 32 /* Coefficient par lequel sont multipliées les
	          coordonnées des points dans le cas des graphes
	          géométriques et le format dot. Plus la valeur est
	          élevée, plus les sommets paraissent petits et les
	          arêtes fines. */

/* codes pour les formats de sorties possibles */

#define F_standard 0
#define F_list     1
#define F_matrix   2
#define F_smatrix  3
#define F_dot      4
#define F_xy       5
#define F_no       6

/* codes pour les algorithmes possibles */

#define CHECK_BFS        2
#define CHECK_DFS        3
#define CHECK_DEGENERATE 4
#define CHECK_GCOLOR     5
#define CHECK_DEG        6
#define CHECK_PATHS      7
#define CHECK_ISO        8
#define CHECK_SUB        9
#define CHECK_ISUB      10
#define CHECK_MINOR     11
#define CHECK_TWDEG     12
#define CHECK_TW        13
#define CHECK_PS1       14 /* NB: les valeurs PS1? doivent être consécutives */
#define CHECK_PS1b      15
#define CHECK_PS1c      16
#define CHECK_PS1x      17
#define CHECK_GIRTH     18
#define CHECK_PATHS2    19
#define CHECK_BELLMAN   20
#define CHECK_MAINCC    21
#define CHECK_KCOLOR    22
#define CHECK_KCOLORSAT 23

/* macros utiles */

#define EQUAL(s)   (strcmp(ARGV[i],s)==0)
#define PREFIX(s)  (strncmp(ARGV[i],s,strlen(s))==0)
#define MIN(i,j)   (((i)<(j))?(i):(j))
#define MAX(i,j)   (((i)>(j))?(i):(j))
#define SWAP(x,y,z) do{z=x;x=y;y=z;}while(0) /* échange les valeurs x et y, via z */
#define VIDE(s)    *s='\0'    /* vide le tableau de caractères s */ 
#define ESTVIDE(s) (*s=='\0') /* vrai ssi s est vide */
#define NONVIDE(s) (*s!='\0') /* vrai ssi s est non vide */
#define RAND01     ((double)random()/(double)RAND_MAX) /* réel aléatoire dans [0,1[ */
#define COLORNB    ((int)sizeof(COLORCHAR)-1) /* nb de couleurs dans la palette de base */
#define MEM(mem,pos,type) (*(type*)(mem+pos)) /* écrit/lit la mémoire mem+pos */

/* alloue à la variable T un tableau de n cases de type t */
#define ALLOC(T,n,t)						\
  do{								\
    if(((T)=(t*)malloc((n)*sizeof(t)))==NULL) Erreur(3);	\
  } while(0)

/* comme ALLOC mais initialise le tableau avec z */
#define ALLOCZ(T,n,t,z)						\
  do{								\
    if(((T)=(t*)malloc((n)*sizeof(t)))==NULL) Erreur(3);	\
    int _i;for(_i=0;_i<n;(T)[_i]=z,_i++);			\
  } while(0)

/* un realloc() qui évite de faire un free() non souhaité à un
   pointeur, et donc qui évite les erreurs "double free". */
#define REALLOC(P,n,t) realloc(P,MAX(n,1)*sizeof(t))

/* Alloue à la valiable T une matrice de n x s valeurs de type t,
   c'est-à-dire un tableau de n tableaux de s valeurs de type t. */
#define ALLOCMAT(T,n,s,t)		\
  do{					\
    int _k=0,_n=n;			\
    ALLOC(T,_n,t*);			\
    for(;_k<_n;_k++) ALLOC(T[_k],s,t);	\
  } while(0)

/* Libère un tableau de pointeurs. On ne peut pas faire de fonction,
   le type de X n'étant pas pré-déterminé. */
#define FREE2(X,n)			\
  do{					\
    if(X!=NULL){			\
      int _j=0;				\
      for(;_j<(n);_j++) free(X[_j]);	\
      free(X);				\
    }					\
  } while(0)

/* Comme FREE2() mais pour un tableau de dimension 3.
   Ex: FREE3(X,n,m[_i]); */
#define FREE3(X,n,m)			\
  do{					\
    if(X!=NULL){			\
      int _i=0;				\
      for(;_i<(n);_i++) FREE2(X[_i],m);	\
      free(X);				\
    }					\
  } while(0)

/* ajoute une arête uv au graphe G en supposant qu'il y a assez de place */
#define ADD_EDGE(G,u,v)	   	\
  do{				\
    G->L[u][G->d[u]++]=v;	\
    G->L[v][G->d[v]++]=u;	\
  }while(0)

/* comme ADD_EDGE */
#define ADD_ARC(G,u)	   	\
  do{				\
    G->L[u][G->d[u]++]=v;	\
  }while(0)

/* macros de débuggage */

// affiche une variable int
#define PRINT(v)					\
  do{							\
    printf(#v " = %i\n",v);				\
  } while(0)

// affiche un tableau
#define PRINTT(T,n)					\
  do{							\
    int _i;						\
    printf(#T " =");					\
    if((T)==NULL) printf(" NULL"); else			\
      for(_i=0;_i<(n);_i++) printf(" %i",(T)[_i]);	\
    printf("\n");					\
  } while(0)

// affiche une liste chaînée
#define PRINTLIST(L)		        	\
  do{						\
    list *_L=L;                                 \
    printf(#L " = {");				\
    while(_L!=NULL){				\
      printf(" (%i,%i)",_L->item,_L->type);     \
      _L=_L->next;                              \
      } printf(" }\n");				\
  } while(0)

#define PAUSE scanf("%*c") /* appuyer sur RETURN pour continuer */
#define STRTOI(s) ((int)strtol(s,NULL,10)) 
#define STRTOD(s) strtod(s,NULL)

/* variables globales */

adjacence *adj;  /* nom de la fonction d'adjacence */
int N;           /* N=nb de sommets du graphes avant suppression */
int NF;          /* nb de sommets final du graphes (donc après suppression) */
int *V;          /* étiquette des sommets, en principe V[i]=i */
int *VF;         /* VF[j] est l'indice i du j-ème sommet non supprimé */
int *INC;        /* INC[i]=deg(i). Si =0, alors i est un sommet isolé */
int SHIFT=0;     /* début de la numérotation des sommets */
int PARAM[DIMAX];/* liste des paramètres du graphe */
void* CPARAM=NULL; /* liste de paramètres (pointeur tout type, en octets) pour -check */
void* FPARAM=NULL; /* liste de paramètres (pointeur tout type, en octets) pour -filter */
int CVALUE;    /* sert pour la valeur dans -filter */
int PVALUE;    /* 1 ssi on affiche la valeur du paramètre dans -filter */
test *FTEST;   /* pour l'option -filter */
double DPARAM[DIMAX]; /* idem PARAM, mais si paramètres de type double */
double DELE=0.0; /* proba de supprimer une arêtes */
double DELV=0.0; /* proba de supprimer un sommet */
double REDIRECT=0.0; /* proba de rediriger une arête */
int DIRECTED=0;  /* vrai ssi graphe orienté for(i=0 ... for(j=...) ) */
int LOOP=0;      /* vrai ssi ont n'affiche pas les boucles */
int PERMUTE=0;   /* vrai ssi -permute */
int NOT=0;       /* vrai ssi -not */
int STAR=0;      /* paramètre de -star */
int APEX=0;      /* paramètre de -apex */
int POS=0;       /* vrai ssi -pos */
int LABEL=0;     /* vrai ssi affiche les labels des sommets (-label) */
int CHECK=0;     /* vrai ssi option -check */
int NORM=2;      /* Norme pour les graphes géométriques: L_2 par défaut */
int FORMAT=F_standard; /* type de la sortie, par défaut le format standard */
int HEADER=0;    /* par défaut pas de préambule, sinon HEADER=1 */
int WIDTH=12;    /* nb maximum d'arêtes/sommets isolés affichés par ligne */
int LIGNE=0;     /* compteur d'arêtes/sommets isolés par ligne */
int LAST=-1;     /* extrémité de la dernière arête affichée */
char *FILEXY=NULL;  /* pointeur sur le nom de fichier */
char *DOTFILTER="neato"; /* pointeur sur le filtre "dot" par défaut */
char NAME[NAMEMAX]; /* nom du sommet retourné par adj(i,-2) */
char SPARAM[32]; /* un paramètre de graphe de type string */
char *STRsep1;   /* séparateur d'arête: "" (pour standard) ou ";" (dot) */
char *STRsep2;   /* autre séparateur */
char *STRedge;   /* caractère pour une arête: "-" (pour standard) ou "--" (dot) */
int ARGC;        /* variable globale pour argc */
char **ARGV;     /* variable globale pour argv */
double *XPOS=NULL,*YPOS=NULL; /* tableaux (X,Y) pour les graphes géométriques */
double SCALEX=-1.0,SCALEY; /* pour le redimensionement: <0 signifie pas de redimen. */
double NOISEr=-1.0,NOISEp; /* paramètre pour -xy noise: <0 signifie pas de "noise". */
double XMIN=0,YMIN=0; /* Bounding Box par défaut */
double XMAX=1,YMAX=1; /* Bounding Box par défaut */
int ROUND=10; /* valeur impossible. L'arrondi de XPOS/YPOS à 10^-ROUND près */
int XYtype=1; /* type de génération des points, par défaut=1=uniforme */
int XYunique=0; /* =1 ssi on élimine les points doubles */
int XYgrid=0; /* =1 ssi on affiche une grille grisée */
double XYvsize=1.0; /* facteur d'échelle pour la taille des sommets dans F_dot */
int SEEDk;  /* nombre de graînes */
double SEEDp; /* loi puissance pour les graines */
double *XSEED=NULL,*YSEED=NULL; /* tableaux de doubles pour les graînes */
int WRAP[DIMAX]; /* WRAP[k]=1 ssi la dimension k est "torique" */
list *L0,*L1,*L2; /* listes utilisées pour -check (tête,dernier,avant-dernier) */
graph *LOAD=NULL;   /* graphe pour "load file" */
graph *GF=NULL;     /* graphe pour l'option -check */
graph *FAMILY=NULL; /* graphe pour l'option -filter */
int *TREE; /* tableau global pour les arbres (sert aussi pour les outerplanars) */
int **REP; /* Associe à chaque sommet i une représentation sous forme
              de tableau d'entiers (REP[i] est donc un tabeau). Sert
              pour les représentations implicites de graphes, les
              arbres, les graphes d'intersections, etc. */
double **DREP; /* comme REP mais avec des doubles */
int VSIZE=0; /* code pour la taille des sommets */
int VCOLOR=0; /* code couleur pour les sommets */
char COLORCHAR[]="redjykugfocatbhsqvmpinzxlw";
color COLORBASE[]={ /* HTML Color Names */
  /* palette des couleurs de bases, l'ordre doit être celui de COLORCHAR */
  {255,0,0},     // r=red
  {210,105,30},  // e=chocolate
  {255,140,0},   // d=darkorange
  {255,165,0},   // j=orange
  {255,255,0},   // y=yellow
  {240,230,140}, // k=khaki
  {154,205,50},  // u=yellowgreen
  {0,255,0},     // g=green (lime)
  {34,139,34},   // f=forestgreen
  {128,128,0},   // o=olive
  {0,255,255},   // c=cyan
  {127,255,212}, // a=aquamarine
  {0,128,128},   // t=teal
  {0,0,255},     // b=blue
  {255,105,180}, // h=hotpink
  {250,128,114}, // s=salmon
  {255,192,203}, // q=pink
  {238,130,238}, // v=violet
  {255,0,255},   // m=magenta
  {128,0,128},   // p=purple
  {75,0,130},    // i=indigo
  {0,0,128},     // n=navy
  {0,0,0},       // z=black
  {128,128,128}, // x=gray
  {230,230,250}, // l=lavender
  {255,255,255}  // w=white
};
color *PALETTE=COLORBASE; /* palette par défaut */
int NPAL=COLORNB; /* taille de la palette par défaut */

/***********************************

         ROUTINES EN VRAC

***********************************/

void Erreur(int error){
  char *s="Erreur : code d'erreur inconnue."; /* erreur par défaut, ne
						 devrait jamais se
						 produire */
  switch(error){
  case 1: s="Erreur : option -xy non reconnue."; break;
  case 2: s="Erreur : option non reconnue."; break;
  case 3: s="Erreur : espace mémoire insuffisant."; break;
  case 4: s="Erreur : nombre trop grand de paramètres."; break;
  case 5: s="Erreur : format de sortie inconnu."; break;
  case 6: s="Erreur : valeur de paramètre incorrecte."; break;
  case 7: s="Erreur : ouverture du fichier impossible."; break;
  case 8: s="Erreur : tableau de coordonnées inexistant."; break;
  case 9: s="Erreur : option -vcolor non reconnue."; break;
  case 10: s="Erreur : nom de graphe inconnu."; break;
  case 11: s="Erreur : option -algo non reconnue."; break;
  case 12: s="Erreur : option -check non reconnue."; break;
  case 13: s="Erreur : format de famille de graphes invalide."; break;
  case 14: s="Erreur : option -filter non reconnue."; break;
  case 15: s="Erreur : graphe(s) non trouvé(s)."; break;
  case 16: s="Erreur : plage de valeurs incorrecte."; break;
  case 17: s="Erreur : nom de sommets trop grand."; break;
  case 18: s="Erreur : opérateurs -star et -apex incompatibles."; break;
  }
  printf("%s\n",s);
  exit(EXIT_FAILURE);
}


int GraphFromArray(int i,int j,int *T){
/*
  Fonction générique pour générer des graphes fixés de petite
  taille. C'est le tableau T qui définit le graphe G.  En gros,
  GraphFromArray(i,j,T)=1 (soit "i-j") ssi i et j se suivent dans le
  tableau T (il existe k tel que T[k]=i et T[k+1]=j ou le
  contraire). On utilise le nombre de sommets N (pour le code -4). Les
  arêtes du graphe sont ainsi couvertes par des chemins éventuellement
  non élémentaires. Les valeurs négatives du tableau ont des
  significations particulières.

  -1: fin d'un chemin ou d'une séquence.
  -2: fin du tableau et donc du graphe.
  -3: sommets consécutifs. Une suite "a,-3,n" est équivalent à "a,a+1,...,a+n,-1"
  -4: cycle hamiltonien, équivalent à "0,-3,N-1,0,-1"
  -5: étoile, "a,-5,a_1,...,a_n,-1" représente une étoile de centre a
      et de feuilles a_1...a_n. C'est équivalent à "a, a_1, a, a_2,
      a,..., a, a_n, -1." On peut remplacer le -1 par n'importe
      qu'elle valeur négative qui peut ainsi être enchaînée.
  -6: comme -5, sauf que les sommets a_i,a_{i+1} sont adjacents.

  Une bonne stratégie pour trouver un code assez court (c'est-à-dire
  un tableau T de petite taille) pour un graphe est de l'éplucher par
  degré maximum jusqu'à obtenir des chemins ou cycles, les sommets
  isolés n'ont pas à être codés. Les sommets enlevés peuvent être
  codés par -5 ou -6, les cycles et les chemins par -3.
*/

  int k=0,a,n;

  while(T[k]!=-2){
    if(T[k]==-3){
      a=T[k-1];
      n=T[++k];
      if((a<=i)&&(i<a+n)&&(j==(i+1))) return 1; /* suppose que i<j */
      goto next;
    }
    if(T[k]==-4){
      if(j==(i+1)) return 1; /* suppose que i<j */
      if((i==0)&&(j==(N-1))) return 1;/* utilise N ici */
      goto next;
    }
    if(T[k]==-5){
      a=T[k-1];
      while(T[++k]>=0){
	if((i==a)&&(j==T[k])) return 1;
	if((j==a)&&(i==T[k])) return 1;
      }
      continue;
    }
    if(T[k]==-6){
      a=n=T[k-1];
      while(T[++k]>=0){
	if(((i==a)||(i==n))&&(j==T[k])) return 1;
	if(((j==a)||(j==n))&&(i==T[k])) return 1;
	n=T[k];
      }
      continue;
    }
    if((i==T[k])&&(j==T[k+1])) return 1;
    if((j==T[k])&&(i==T[k+1])) return 1;
  next:
    k++;
  }
  return 0;
}


void Permute(int *T,int n){
/*
  Permute aléatoirement les n premiers éléments de T.
*/
  int i,j,k;
  for(i=0;i<n;i++){
    j=i+(random()%(n-i));
    SWAP(T[i],T[j],k);
  }
}


void NAME_Vector(int *R,int n,char *sep,char *par,int d,char *f){
/*
  Ecrit dans la variable globale NAME le nom représenté par les n
  premiers entiers (lettres) du tableau R. Les lettres (chiffres) sont
  séparées par la chaîne "sep" (qui peut être vide). Le mot est
  parenthésé par la chaîne "par" qui doit être composé de deux
  caractères (ou bien vide): par[0]=parenthèse gauche,
  par[1]=parenthèse droite. Si d>0, les lettres sont écrites dans le
  sens croissant des indices (vers la droite), si d<0 c'est dans le
  sens des indices décroissant. La chaîne f indique le format
  d'affichage (avec printf) de l'entier REP[i]. En général, il s'agit
  de "%i".

  Ex: R={3,6,1}, n=3, d=1, f="%i".
  sep="," et par="{}": alors NAME="{3,6,1}"
  sep="-" et par="": alors NAME="3-6-1"
  sep="" et par="": alors NAME="361"
*/
  int i;
  char s[NAMEMAX];
  char vide[2]="\0\0"; /* vide=00 */

  VIDE(NAME);
  for(i=0;i<n;i++){
    sprintf(s,f,R[i]);
    if(d>0) strcat(NAME,s);
    else{
      strcat(s,NAME);
      strcpy(NAME,s);
    }
    if(i<n-1) strcat(NAME,sep);
  }

  if(ESTVIDE(par)) par=vide;
  s[0]=par[0]; s[1]='\0';
  strcat(s,strcat(NAME,par+1));
  strcpy(NAME,s);
  return;
}


void NAME_Base(int u,int b,int n,char *sep,char *par,int d)
/*
  Comme NAME_Vector(...,n,sep,par,d,"%i") sauf que NAME est l'écriture
  de la valeur de u en base b.

  Ex: R=361, b=10, n=3 et d=1.
  sep="," et par="{}": alors NAME="{3,6,1}"
  sep="-" et par="": alors NAME="3-6-1"
  sep="" et par="": alors NAME="361"
*/
{
  int i;
  int R[NAMEMAX];

  if(b<2) return;
  for(i=0;i<n;i++){
    R[i]=u%b;
    u /= b;
  }

  NAME_Vector(R,n,sep,par,d,"%i");
  return;
}


int LoadXY(char *file){
/*
  Remplit les tableaux XPOS et YPOS à partir d'un fichier, et renvoie
  le nombre de valeurs lues. Le programme peut être amené à lire une
  valeur de trop (et donc vide) si le "n" du fichier est > aux nombres
  de valeurs qu'il contient réellement et que le fichier contient un
  retour de ligne finale.  */
  FILE *f=stdin;
  int n,i,r;

  if(strcmp(file,"-")) f=fopen(file,"r"); /* ouvre en lecture */
  if(f==NULL) Erreur(7);
  i=fscanf(f,"%i",&n); /* lit n */
  if((n<0)||(i<0)) n=0;
  ALLOC(XPOS,n,double);
  ALLOC(YPOS,n,double);
  for(i=0;(i<n)&&(!feof(f));i++){
    if(fscanf(f,"//%*[^\n]\n")>0) continue;
    r=fscanf(f,"%lf %lf",XPOS+i,YPOS+i);
  }
  fclose(f);
  return i; /* i=minimum entre n et le nb de valeurs lues */
}


list *Insert(list *p,int v,int t){
/*
  Ecrit (v,t) dans l'élément de la liste chaînée pointée par p (donc p
  initialement doit pointer sur un emplacement libre), puis crée et
  chaîne puis retourne le prochain pointeur libre. Si p=NULL, alors on
  crée un nouvel élément que l'on retourne.  */
  list *l;
  ALLOC(l,1,list);
  l->next=NULL;
  if(p==NULL){
    l->item=v;
    l->type=t;
  }
  else{
    p->item=v;
    p->type=t;
    p->next=l;
  }
  return l;
}


graph *new_graph(int n){
  /*
    Renvoie un objet de type "graph". Les champs sont initialisés. Si
    n>0, alors les tableaux ->d et ->L de taille n sont alloués.
  */
  graph *G;

  ALLOC(G,1,graph);
  G->n=n;
  G->m=-1;
  G->sort=0;
  G->id=-1;
  G->d=NULL;
  G->L=NULL;
  G->W=NULL;
  G->xpos=NULL;
  G->ypos=NULL;
  G->f=0;
  G->sym=1;
  G->G=NULL;

  G->pint1=NULL;
  G->int1=-1;

  if(n>0){
    ALLOC(G->d,n,int);
    ALLOC(G->L,n,int*);
  }

  return G;
}


void free_graph(graph *G)
/*
  Libère tous les tableaux d'un graphe G.  Dans le cas d'une famille,
  chaque graphe est aussi libéré (de manière récursive).
*/
{
  if(G==NULL) return;

  /* Remarque: ce n'est pas grave de faire free() sur un ptr NULL */

  int i;
  free(G->d);
  free(G->pint1);
  free(G->xpos);
  free(G->ypos);
  FREE2(G->L,G->n);
  FREE2(G->W,G->n);
  for(i=0;i<G->f;i++) free_graph(G->G[i]);
  free(G->G);
  free(G);
}


path *new_path(graph *G,int *P,int n){
  /*
    Créer un chemin d'un graphe G, défini par une liste P de n sommets
    de G. Attention, le pointeur P est utilisé par le chemin (struct
    path) renvoyé. Il ne faut pas détruire P après cet appel. P sera
    libéré par free_path(). Si P=NULL, alors le champs ->P de taille n
    est alloué, et le champs ->n=0. C'est une façon de créer un chemin
    simple et vide. Dans ce cas n représente la longueur maximum d'un
    chemin de G. Le champs ->V (de taille G->n) est initialisé selon
    P, si P<>NULL, ou à -1 sinon.
  */
  if(G==NULL) return NULL;
  path *Q;
  ALLOC(Q,1,path);
  ALLOCZ(Q->V,G->n,int,-1);
  Q->aux=NULL;
  Q->nG=G->n;

  if(P==NULL){
    ALLOC(Q->P,n,int);
    Q->n=0;
  }
  else{
    int i;
    Q->P=P;
    for(i=0;i<n;i++) Q->V[P[i]]=i;
    Q->n=n;
  }

  return Q;
}


void free_path(path *P)
{
  if(P==NULL) return;
  free(P->P);
  free(P->V);
  FREE2(P->aux,P->nG);
  free(P);
}


int fcmp_int(const void *P,const void *Q)
/* Compare deux entiers, pour qsort(). */
{
  return (*(int*)P - *(int*)Q);
}


int fcmp_point(const void *P,const void *Q)
/* Compare deux points, pour qsort(). */
{
  if(((point*)P)->x < ((point*)Q)->x) return -1;
  if(((point*)P)->x > ((point*)Q)->x) return 1;
  if(((point*)P)->y < ((point*)Q)->y) return -1;
  if(((point*)P)->y > ((point*)Q)->y) return 1;
  return 0;
}


int fcmp_profile(const void *P,const void *Q)
/*
  Compare deux profiles, pour qsort(). Les profiles de plus grande
  longueur sont classés avant les plus courts, ceux-ci étant plus
  discriminant.
*/
{
  int* A=*(int**)P;
  int* B=*(int**)Q;

  if(*A>*B) return -1; /* si long(A)>long(B), alors A<B */
  if(*A<*B) return 1; /* si long(A)<long(B), alors A>B */
  /* ici, profiles de même longueur n=A[0] */

  int u=2,n=*A; /* surtout ne pas utiliser A[1] */

  for(;u<n;u++){
    if(A[u]<B[u]) return -1;
    if(A[u]>B[u]) return 1;
  }

  return 0;
}


int fcmp_graphid(const void *P,const void *Q)
/* Compare les identifiants de deux graphes. Sert pour qsort() et
   bsearch(). Ici, P et Q sont des pointeurs de (graph*). */
{
  return ( ((*(graph**)P)->id) - ((*(graph**)Q)->id) );
}


int ReadRange(char *s,int *R)
/*
  Lit une chaîne de caractères décrivant un intervalle de valeurs
  entières, et renvoie dans le tableau d'entiers R les valeurs et les
  codes correspondant pour que la fonction InRange(x,R) puisse
  fonctionner. En quelque sorte cette fonction prépare la fonction
  InRange(). On ignore les caractères non reconnus (pas d'erreur). On
  renvoie le nombre d'opérations décodées, c'est-à-dire le nombre de
  valeurs écrites dans le tableau R, nombre qui est aussi écrit dans
  la première entrée de R.

  Ex: s="1,-3,5-7,>30,<50" (on interprète les "," comme des "ou")
  => R={12,code=,1,code=,-3,code-,5,7,code>,30,code<,50} (12=taille(R))

  La valeur des codes d'opérations (code=,code<...) sont données par
  la fonction InRange().
*/
{
  if(R==NULL) return 0;
  if(s==NULL){ R[0]=0; return 0; }

  int i,r,p,x,start,c;
  i=x=c=0;
  r=start=p=1;

  /* r=indice de R[] */
  /* i=indice de s[] */
  /* x=valeur entière lue */
  /* c=1 ssi le code d'opération a été détecté */
  /* start=1 ssi on va commencer à lire un entier */
  /* p=1 ou -1, signe de x */

  while(s[i]!='\0'){
    if(s[i]=='='){ R[r++]=0; c=start=p=1; }
    if(s[i]=='<'){ R[r++]=1; c=start=p=1; }
    if(s[i]=='>'){ R[r++]=2; c=start=p=1; }
    if(s[i]=='-'){
      if(start) p=-p;
      else{ R[r++]=3; R[r++]=x; c=start=p=1; }
    }
    if(s[i]=='t'){ x=4; c=r=1; break; } /* t=true, pour avoir false faire "not" et "t" */
    if(s[i]=='p') PVALUE=1; /* pas de code pour "p" */
    if(s[i]==','){
      if(c==0) R[r++]=0; /* code '=' par défaut */
      R[r++]=x; c=0; start=p=1;
    }
    if((s[i]>='0')&&(s[i]<='9')){
      if(start) start=0;
      x=x*10+p*(s[i]-'0'); /* on lit x en base 10 en tenant compte du signe p */
    }
    if(start) x=0;
    i++;
  }

  if(PVALUE==i){ x=4;c=1; } /* si s="p", alors comme "t" */
  if(c==0) R[r++]=0;
  R[r++]=x; /* on ajoute le dernier opérande */
  R[0]=r;
  return r;
}


int InRange(int x,int* R)
/*
  Détermine si x appartient aux valeurs décrites par le "range" R.
  R[0] est la taille de R, R[0] compris.
*/
{
  int i,n,t;
  n=R[t=0]; /* n=taille(R) */
  i=1; /* commence à lire R[1] */
  CVALUE=x;

  while(i<n){
    switch(R[i++]){ /* lit le code d'opération */
    case 0: t=(x==R[i++]); break;
    case 1: t=(x<R[i++]); break;
    case 2: t=(x>R[i++]); break;
    case 3: t=((R[i]<=x)&&(x<=R[i+1])); i+=2; break;
    case 4: return 1;
    default: Erreur(16); /* ne devrait jamais se produire */
    }
    if(t) break;
  }
  return t;
}


void PrintGraphList(graph *G)
/*
  Affiche le graphe G sous la forme d'une liste d'adjacence. Tient
  compte de SHIFT.
*/
{
  if(G==NULL){ printf("NULL\n"); return; }
  int u,d,i,n=G->n;

  printf("N=%i\n", n);
  for(u=0;u<n;u++){
    printf("%i:",u+SHIFT);
    for(i=0,d=G->d[u];i<d;i++){
      printf(" %i",G->L[u][i]+SHIFT);
    }
    printf("\n");
  }
  return;
}


void PrintGraphMatrix(graph *G)
/*
  Affiche le graphe G sous la forme d'une matrice d'adjacence complète
  ou triangulaire supérieure (en tennant compte du FORMAT, smatrix ou
  matrix). La complexité en espace est seulement de O(n).
*/
{
  int u,d,i,z,t,n=G->n;
  int *M;

  ALLOCZ(M,n,int,0);
  t=(FORMAT==F_smatrix);

  for(u=z=0;u<n;u++){
    if(t) z=u;
    for(i=0,d=G->d[u];i<d;M[G->L[u][i++]]=1);
    for(i=0;i<n;i++)
      if(i<z) printf(" ");
      else printf("%c",'0'+M[i]);
    for(i=0;i<d;M[G->L[u][i++]]=0); /* remet rapidement M[] tout à 0 */
    printf("\n");
  }

  free(M);
  return;
}


void PrintPath(graph *G,path *P)
/*
  Affiche le chemin P d'un graphe G.
  Sert pour le débugage.
*/
{
  if((G==NULL)||(P==NULL)) printf("NULL\n");
  int i,j,u,d;
  for(i=0;i<P->n;i++)
    if(P->V[P->P[i]]!=i) break;
  if(i<P->n) goto error;
  for(u=0;u<G->n;u++)
    if((P->V[u]>=0)&&(P->P[P->V[u]]!=u)) break;
  if(u<G->n) goto error;
  printf("P->aux:");
  if(P->aux==NULL) printf(" NULL\n");
  else{
    printf("\n");
    for(i=0;i<P->n;i++){
      u=P->P[i];
      d=P->aux[u][0];
      printf("  %i:",u);
      for(j=1;j<=d;j++){
	printf(" %i",P->aux[u][j]);
      }
      printf("\n");
    }
  }
  return;
 error:
  printf("Chemin incohérent.\n");
  return;
}


void SortGraph(graph *G)
/*
  Trie la liste d'adjacence d'un graphe G, c'est-à-dire pour chaque
  sommet u, G->L[u] est une liste d'entiers triés par ordre croissant.
  L'algo, simplissime et basé sur qsort(), est à peu près en
  O(n+m*log(m/n)).

  [A FINIR: Il faudrait améliorer la fonction pour enlever les boucles
   et arêtes multiples éventuelles]
*/
{
  if(G==NULL) return;
  int u,n=G->n;
  for(u=0;u<n;u++)
    qsort(G->L[u],G->d[u],sizeof(int),fcmp_int);
  G->sort=1;
  return;
}


void GraphRealloc(graph *G,int *D)
/*
  Redimensionne le graphe G à G->n sommets suivant le tableau de degré
  D. On réajuste en premier les tableaux G->d et G->L pour qu'ils
  aient une taille G->n, puis on réajuste les listes d'adjacences des
  sommets de G suivant le tableau des degrés D (qui doit être de
  taille au moins G->n). Si D[u] est plus petit que G->d[u], alors la
  liste G->L[u] est tronquée. Si D[u] est plus grand que G->d[u],
  alors G->L[u] est réajusté. Le degré G->d[u] est initialisé au
  minimum de G->d[u] et de D[u]. NB: le nb d'arêtes G->m, qui a pu
  changer, est réinitialisé à -1.

  Pour plonger G dans un graphe complet faire:
     int *D=ALLOCZ(D,n,int,n-1);
     GraphRealloc(G,D);
     ...
     free(D);
 */
{
  int u,d,n=G->n;
  for(u=0;u<n;u++){
    d=D[u];
    G->L[u]=REALLOC(G->L[u],d,int);
    G->d[u]=MIN(G->d[u],d);
  }

  /* Il ne faut pas réajuster G->d et G->L avant la boucle for(u=...)
     car potentiellement on libère G->d et G->L. Or il est possible
     d'avoir D=G->d. */

  G->d=REALLOC(G->d,n,int);
  G->L=REALLOC(G->L,n,int*);
  G->m=-1; /* le nb d'arêtes n'est plus à jour */
  return;
}


graph *new_fullgraph(int n){
  /*
    Retourne un graphe G comme new_graph(n), mais en plus alloue
    G->L[u] de taille MAX(n-1,1), et initialise G->d[u]=0 pour tous
    les sommets u. Une fois le graphe construit, on peut
    rédimensionner le graphe grâce à GraphRealloc, comme dans
    l'exemple:

    graph *G=new_fullgraph(n);
      ...
      ADD_EDGE(G,u1,v1);
      ADD_EDGE(G,u2,v2);
      ...
    GraphRealloc(G,G->d);
    ...
    free_graph(G);
  */
  if(n<1) return NULL;
  graph *G=new_graph(n);
  int u,n1=MAX(n-1,1);

  for(u=0;u<n;u++){
    G->d[u]=0;
    ALLOC(G->L[u],n1,int);
  }
  
  return G;
}


graph *ExtractSubgraph(graph *G,int *T,int k,int code){
  /*
    Construit, à partir d'un graphe G et d'une liste T de k sommets,
    un nouveau graphe S correspondant au sous-graphe de G induit par
    les sommets de T (code=1) ou de V(G)\T (si code=0). Les sommets de
    S sont dans [0,k[ (ou [0,n-k[ si code=0).

    Effet de bord: S->pint1 est alloué si T<>NULL. Dans ce cas on
    renvoie dans S->pint1 un tableau X de taille G->n indiquant la
    renumérotation de G: pour tout sommet u de G (u dans [0,G->n[)
    S->pint1[u]=0 si u est un sommet abscent de S et S->pint1[u]=d>0
    si u est numéroté d-1>=0 dans S. Le nombre d'arêtes S->m du graphe
    S renvoyé est à jour. L'ordre relatif des listes de G est
    préservé. En particulier, si G->sort=1, alors le sous-graphe sera
    aussi trié. G->sym est aussi copié.

    Remarque: pour renvoyer une copie du graphe G, faire:
    graph *C=ExtractSubgraph(G,NULL,0,0);
  */
  if(G==NULL) return NULL;
  int *X;
  int n=G->n;
  int u,v,d,i,s,ns,m;
  graph *S;

  ALLOC(X,n,int);
  for(u=1-code,i=0;i<n;i++) X[i]=u;
  if(T!=NULL) for(i=0;i<k;i++) X[T[i]] ^= 1;
  for(i=d=0;i<n;i++) if(X[i]) X[i]=++d; 
  /* ici X[i]=0 si i est un sommet à supprimer */
  /* ici X[i]=d>0 si i doit être renuméroté en d-1>=0 */

  ns=(code)?k:n-k;
  S=new_fullgraph(ns);

  for(s=u=m=0;u<n;u++)
    if(X[u]){ /* si u existe, s=X[u]-1 */
      d=G->d[u];
      for(i=0;i<d;i++){
	v=G->L[u][i];
	if(X[v]){ m++; S->L[s][S->d[s]++]=X[v]-1; } /* si v existe */
      }
      s++;
    }

  /* réduit la taille des listes */
  GraphRealloc(S,S->d);

  S->pint1=X;
  S->sort=G->sort;
  S->sym=G->sym;
  S->m=(m>>1);
  return S;
}


graph *List2Graph(list *L,int code){
  /*
    Retourne un graphe à partir d'un graphe G défini par une liste
    (voir File2List() pour le codage précis du type "list"). Met à
    jour G->n, le nombre de sommets du graphe. Certaines opérations
    sont effectuées sur L en fonction de la valeur binaire de code
    (voir ci-dessous).

    Effet de bord: si code&16=0, alors L est libérée.
    
    Pour calculer le graphe (et sa liste d'adjacence) on effectue
    plusieurs passes sur L: 1) on détermine n; 2) les degrés; et 3) on
    remplit G et on vide la liste L.

    Les bits de "code" ont la signification suivante:

    - code&1 =1: optimisation des listes du graphe (tri par ordre croissant)
             =0: sans optimisation
    - code&2 =1: auto-détection du shift dans L (pour "load file")
             =0: pas d'auto-détection du shift
    - code&4 =1: gestion d'un sous-graphe (V,NF) => code&2=0
             =0: pas de sous-graphe
    - code&8 =1: tri de la famille suivant les identifiants (sert pour List2Family)
             =0: pas de tri de la famille
    - code&16=1: ne libère pas la liste L (sert aussi pour List2Family)
             =0: libère la liste L (par défaut)
    - code&32=1: renvoie toujours un graphe, le 1er si c'est une famille
             =0: renvoie une famille si c'est une famille

  */
  if(L==NULL) return NULL; /* si pas de cellule, ne rien faire */
  int u,v,x,n,*D;
  graph *G;
  list *p;

  u=INT_MAX;
  if(code&4){ /* si sous-graphe */
    p=L;
    while(p!=NULL){
      p->item=V[p->item]-SHIFT;
      p=p->next;
    }
    n=NF; /* on connaît n */
  }
  else{ /* sinon, on calcule n, et on lit les valeurs min (=u) et
	   valeur max (=v) de L */
    p=L; v=0;
    while(p!=NULL){
      x=p->item;
      if(x<u) u=x;
      if(x>v) v=x;
      p=p->next;
    }
    if(code&2){ /* on décale les valeurs dans [0,n[ */
      p=L;
      while(p!=NULL){
	p->item -= u;
	p=p->next;
      }
      n=v+1-u;
    } else n=v+1;
  }

  ALLOCZ(D,n,int,0);

  /* on lit les degrés (sortant) des sommets */
  p=L;
  while(p!=NULL){
    v=p->item;
    if(p->type==1){ D[u]++; D[v]++; } /* u-v */
    if(p->type==4) D[u]++; /* u->v */
    u=v;
    p=p->next;
  }

  /* on initialise la liste d'adjacence G */
  /* on se sert de D[u] pour indiquer la prochaine position libre dans G[u][] */

  G=new_graph(n); /* G->n=n, alloue G->d et G->L */
  for(u=0;u<n;u++){
    ALLOC(G->L[u],D[u],int); /* alloue une liste pour chaque sommet */
    G->d[u]=D[u]; /* G->d[u]=deg(u) */
    D[u]=0; /* prochaine position libre dans G[u] */
  }

  /* remplit G et libère en même temps L (si code&16=0). On pourrait
     tester à la volée si les listes sont triées et mettre à jour
     G->sort */
  p=L;
  while(p!=NULL){
    v=p->item;
    if(p->type==1){ G->L[u][D[u]++]=v; G->L[v][D[v]++]=u; } /* u-v */
    if(p->type==4){ G->L[u][D[u]++]=v; G->sym=0; } /* u->v */
    u=v;
    L=p;
    p=p->next;
    if(!(code&16)) free(L);
  }

  free(D);
  if(code&1) SortGraph(G);
  return G;
}


graph *List2Family(list *L,int code){
  /*
    Transforme une liste en famille de graphes.  Si L représente un
    graphe simple (pas de type 2 ou 3), alors un graphe simple est
    retournée (plutôt qu'une famille à un seul élément). Donc,
    List2Family() généralise List2Graph(). On utilise List2Graph() comme
    sous-routine. Pour "code" voir List2Graph().
    
    Effet de bord:
    - la liste L est libérée si code&16=0
    - la famille est triée par ID croissant si code&8=1
    - on retourne un graphe si code&32=1 (plutôt qu'une famille)
  */
  if(L==NULL) return NULL;
  if(L->type!=3) return List2Graph(L,code);
  
  int n=L->item; /* nb de graphes dans la liste */
  if(n<=0) return NULL; /* potentiellement on ne libère pas L ... */

  int i,id;
  graph *F=new_graph(0);
  list *T,*P;

  F->f=n;
  ALLOC(F->G,n,graph*); /* F->G[0..n[: tableau de n pointeurs de graphes */
  T=L; L=L->next;
  if(!(code&16)) free(T); /* on libère l'élément (n,3), L=prochain élément */

  for(i=0;i<n;i++){
    /* ici L pointe normalement sur un élément (id,2) */
    if((L==NULL)||(L->type!=2)) Erreur(13);
    id=L->item; /* identifiant du graph i */
    T=L->next;
    if(!(code&16)) free(L); /* on libère l'élément (id,2) */
    P=L=T; /* P=L=T=tête courante du graphe i */
    while((L!=NULL)&&(L->type!=2)){ P=L; L=L->next; } /* cherche la fin du graphe i */
    /* ici le graphe i va de T à P */
    P->next=NULL; /* on coupe la liste */
    F->G[i]=List2Graph(T,code);
    F->G[i]->id=id;
    }

  /* éventuellement trie la famille suivant les IDs */
  if(code&8) qsort(F->G,F->f,sizeof(graph*),fcmp_graphid);

  /* extrait le premier graphe */
  if(code&32){
    graph *G=ExtractSubgraph(F->G[0],NULL,0,0); /* copie le premier graphe */
    free_graph(F); /* libère complètement la famille F */
    F=G; /* F=premier graphe */
  }

  return F;
}

list *File2List(char *file){
/*
  Lit le fichier "file" de graphes au format standard orienté ou non
  (sans l'option -label 1, mais potentiellement avec -shift) et
  retourne le contenu dans une liste. Il est possible de spécifier un
  "range" pour "file" avec la forme: "file:range" où "range" est une
  liste de valeurs ayant la même signification que pour "-filter F id
  value". Par exemple, "file:5" spécifie le graphe d'identifiant 5, et
  "file:5-10" est la famille de graphes contenant les graphes
  d'identifiant 5 à 10. Notez que "-:5" est le graphe d'identifiant 5
  de la famille lue depuis l'entrée standard.

  Chaque élément de la liste est une paire d'entiers (item,type). La
  signification de type est la suivante:

  - type=0: item est un sommet u
  - type=1: item est un sommet v connecté au précédent de la liste par une arête (u-v)
  - type=2: item est l'identifiant d'un nouveau graphe d'une famille
  - type=3: item est le nombre de graphes de la famille
  - type=4: item est un sommet v connecté au précédent de la liste par un arc (u->v)

  Si, par exemple, le graphe est "0-1 2 1-2-3 5->4" alors la liste
  retournée sera {(0,0), (1,1), (2,0), (1,0), (2,1), (3,1), (5,0),
  (4,4)}.

  La fonction est généralisée à la lecture d'une famille de
  graphes. Si le fichier contient "[5] 0-1 [8] 0->2-1" alors la liste
  contiendra {(2,3), (5,2), (0,0), (1,1), (8,2), (0,0), (2,4), (1,1)},
  où le premier élément (n,3) signifie qu'il s'agit d'une famille de n
  graphes, et où (u,2) signifie que u est l'identifiant d'un nouveau
  graphe.
*/
  FILE *f;
  list *T; /* tête de la liste */
  list *L; /* élément courant */
  list *P; /* sauvegarde le dernier élément */
  int v,c=0,read=1; /* par défaut on lit tout */
  char *r=NULL;
  int range[CMDMAX]={2,4}; /* par défaut: toujours vrai */

  T=P=L=Insert(NULL,0,0); /* crée la liste */

  /* ouverture du fichier: file ou file:range */

  f=strcmp(file,"-")? fopen(file,"r"):stdin;
  if(f==NULL){
    /* on a pas réussit à ouvrir file */
    //fclose(f); /* il faut fermer le fichier, même si c'est NULL ! */
    if((r=strchr(file,':'))==NULL) Erreur(7);
    *r='\0'; /* coupe file en (préfix,r=range) */
    f=strcmp(file,"-")? fopen(file,"r"):stdin;
    if(f==NULL) Erreur(7);
    *r++ = ':'; /* rétablit le nom de fichier original de file */
    ReadRange(r,range);
  }

  /* BUG: si on met "//% ..." dans fscanf() alors 1 commentaire sur 2
     n'est pas enlevés correctement. Le format "->%i" dans fscanf()
     marche moins bien que ">%i" */

  /* lecture du fichier */

  while(!feof(f)){
    if(fscanf(f," /%*[^\n]\n")>0) continue; /* commentaire */
    if(fscanf(f," [%i] ",&v)>0){
      read=InRange(v,range);
      if(read){ L=Insert(P=L,v,2); c++; }
      continue; }
    if(fscanf(f,"-%i ",&v)>0){ if(read) L=Insert(P=L,v,1); continue; }
    if(fscanf(f,">%i ",&v)>0){ if(read) L=Insert(P=L,v,4); continue; }
    if(fscanf(f," %i ",&v)>0){ if(read) L=Insert(P=L,v,0); continue; }
    v=fscanf(f,"%*c"); /* lit 1 caractère */
  }
  fclose(f); /* on ferme le fichier */

  free(L); /* supprime le dernier élément */
  if(L==T) return NULL; /* si on a lu aucun élément */
  P->next=NULL;

  if(c>0){ /* il s'agit d'une famille */
    /* on ajoute un nouvelle élément en tête de la liste */
    L=Insert(NULL,0,0);
    L->item=c;
    L->type=3;
    L->next=T;
    T=L; /* nouvelle tête */
  }

  return T; /* on retourne la tête */
}


graph *File2Graph(char *file,int code){
  /*
    Renvoie un graphe (ou une famille) à partir d'un fichier. Pour
    "code" voir List2Graph(). La liste intermédiaire est toujours
    libérée.
  */
  graph *G=List2Family(File2List(file),code&(63-16)); /* annule le bit-4 */
  if(G==NULL) Erreur(15);
  return G;
}


double PosAspect(){
/*
  Donne le coefficient par lequel les positions XPOS/YPOS vont être
  multipliées pour le format dot pour permettre une taille de sommets
  raisonable. On tient compte de N et de SCALEX/Y.
*/
  double w;
  w=C32*sqrt((double)N); /* la largeur est en sqrt(N) */
  if((SCALEX>0.0)&&(SCALEY>0.0)) w /= MIN(SCALEX,SCALEY);
  if(LABEL>0) w *= 3; /* augmente l'aspect si besoin des LABELs (et POS) */
  return w;
}

void BoundingBox(){
/*
  Calcule XMIN,YMIN,XMAX,YMAX des tableaux XPOS/YPOS.
*/
  int i;
  XMIN=XMAX=XPOS[0];
  YMIN=YMAX=YPOS[0];
  for(i=1;i<N;i++){
    if(XPOS[i]<XMIN) XMIN=XPOS[i];
    if(YPOS[i]<YMIN) YMIN=YPOS[i];
    if(XPOS[i]>XMAX) XMAX=XPOS[i];
    if(YPOS[i]>YMAX) YMAX=YPOS[i];
  }
}

void InitXY(void){
/*
  Initialise les tableaux XPOS et YPOS suivants les options
  éventuelles de -xy.  La valeur de N est éventuellement recalculée
  (si -xy load), et les BoundingBox (XMIN,XMAX,YMIN,YMAX) mises à
  jour. On met aussi à jour VSIZESTD et VSIZEK.
*/
  int i,k;
  double sx,sy,tx,ty;

  if(FILEXY!=NULL) N=LoadXY(FILEXY); /* charge à partir d'un fichier et met à jour N */
  else{ /* sinon, remplissage aléatoire dans [0,1[ */
    if(N<0) N=0;
    ALLOC(XPOS,N,double);
    ALLOC(YPOS,N,double);

    if(XYtype==1) /* random uniforme dans [0,1[ */
      for(i=0;i<N;i++){
	XPOS[i]=RAND01;
	YPOS[i]=RAND01;
      }

    if(XYtype==2){ /* loi puissance autour des graines */
      /* on choisit les graines dans [0,1[ */
      ALLOC(XSEED,SEEDk,double);
      ALLOC(YSEED,SEEDk,double);
      sx=sy=0.0; /* calcule (sx,sy), le barycentre des SEEDk graines */
      for(i=0;i<SEEDk;i++){
	XSEED[i]=RAND01;sx+=XSEED[i];
	YSEED[i]=RAND01;sy+=YSEED[i];
      }
      sx /= (double)SEEDk; sy /= (double)SEEDk;
      sx -= 0.5; sy -= 0.5;
      for(i=0;i<SEEDk;i++){ /* centre par rapport au barycentre */
	XSEED[i] -= sx; /* enlève le barycentre puis décale de 0.5 */
	YSEED[i] -= sy;
      }
      /* on génère les points autour des graines */
      tx=sqrt(log((double)(SEEDk+1.0))/(double)(SEEDk+1.0)); /* rayon r */
      for(i=0;i<N;i++){
	k=random()%SEEDk;    /* on choisit la graine numéro k au hasard */
	sx=2.0*M_PI*RAND01;  /* angle aléatoire */
	sy=tx*pow(RAND01,SEEDp); /* longueur aléatoire */
	XPOS[i]=XSEED[k]+sy*cos(sx);
	YPOS[i]=YSEED[k]+sy*sin(sx);
      }
    }

    if(XYtype==3){ /* permutation */
      int *P;
      ALLOC(P,N,int);
      for(i=0;i<N;i++) XPOS[i]=(double)(P[i]=i);
      Permute(P,N);
      for(i=0;i<N;i++) YPOS[i]=(double)P[i];
      free(P);
    }

  }

  if(NOISEr>0.0){ /* "noise" doit être avant "scale" */
    for(i=0;i<N;i++){
      sx=2.0*M_PI*RAND01; /* angle aléatoire */
      sy=NOISEr*pow(RAND01,NOISEp); /* longueur aléatoire */
      XPOS[i] += sy*cos(sx); /* décale XPOS */
      YPOS[i] += sy*sin(sx); /* décale YPOS */
    }
  }

  if((SCALEX>0.0)&&(SCALEY>0.0)){ /* "scale" doit être après "noise" */
    BoundingBox(); /* calcule les BB */
    tx=(XMAX-XMIN); if(tx==0.0) tx=0.5;
    ty=(YMAX-YMIN); if(ty==0.0) ty=0.5;
    sx=2.0*sqrt(N+1); /* le +1 est pour être sûr d'avoir sx>0 */
    tx /= sx; /* tx=largeur de la bande vide */
    ty /= sx; /* ty=hauteur de la bande vide */
    sx=SCALEX/(XMAX-XMIN+2.0*tx); /* sx ne peut pas être nul */
    sy=SCALEY/(YMAX-YMIN+2.0*ty); /* sy ne peut pas être nul */

    for(i=0;i<N;i++){
      XPOS[i]=sx*(XPOS[i]-XMIN+tx);
      YPOS[i]=sy*(YPOS[i]-YMIN+ty);
    }

    XMAX=SCALEX;
    YMAX=SCALEY;

    if(ROUND<10){
      sx=pow(10.0,ROUND);
      for(i=0;i<N;i++){
	XPOS[i]=rint(XPOS[i]*sx)/sx;
	YPOS[i]=rint(YPOS[i]*sx)/sx;
      }
    }
  }

  if(XYunique){ /* élimine les doubles, en triant les points */
    point *P,p;
    ALLOC(P,N,point);
    for(i=0;i<N;i++) P[i].x=XPOS[i],P[i].y=YPOS[i];
    qsort(P,N,sizeof(point),fcmp_point); /* tri les points */
    p=P[0];p.x=p.x-1.0; /* ici p<>du premier élément */
    for(i=k=0;i<N;i++)
      if(fcmp_point(P+i,&p)){ /* copie que si différent de l'élément p */
	p=P[i];
	XPOS[k]=p.x;
	YPOS[k++]=p.y;
      }
    free(P);
    N=k;
    XPOS=REALLOC(XPOS,N,double);
    YPOS=REALLOC(YPOS,N,double);
  }

  /* on recalcule éventuellement les BB */
  if((FILEXY!=NULL)||(XYtype>1)||(NOISEr>0.0)) BoundingBox();

  /* mise à jour de la taille des sommets */
  VSIZESTD *= XYvsize;
  VSIZEXY  *= XYvsize;

  return;
}


color *GradColor(color *T,int n,int m){
/*
  Retourne un tableau de m couleurs formant un dégradé obtenu à partir
  d'un tableau de n couleurs. Pour avoir un dégradé simple d'une
  couleur T[0] à T[1] il faut initialiser T[0],T[1] et poser n=2. Pour
  avoir un dégradé cyclique, il suffit de répéter la couleur T[0] en
  dernière position de T (et ajouter 1 à n, donc d'avoir
  T[n-1]=T[0]). Il faut dans tous les cas n>1 et m>0. On peut avoir
  m<n. Dans ce cas on prend la première couleur de T, puis la i-ème
  couleur est (i*(n-1))/(m-1).
*/
  color *P,c,c1,c2;
  int i,j,k,r,q,n1,m1,dr,dg,db;

  if(T==NULL) return NULL; /* normalement ne sert à rien */
  ALLOC(P,m,color);
  c2=P[0]=T[0];
  if(m==1) return P;
  /* maintenant m >= 2 */

  m1=m-1; n1=n-1; /* utilisés souvent */
  k=1; /* indice dans P de la prochaine couleur libre */

  if(m<=n){ /* cas où il y a moins de couleurs demandées que dans T */
    for(i=1;i<m;i++) /* m-1 fois */
      P[k++]=T[(i*n1+m1-1)/m1]; /* le "+m-2" est pour arrondir à l'entier sup. */
    return P;
  }

  /*
    Cas m>n.  Soient B1,B2,...B_(n-1) les n-1>0 blocs de couleurs, B_i
    commençant juste après la couleurs T[i-1] et se terminant avec la
    couleur T[i]. On doit répartir m-1 couleurs dans ces n-1 blocs, la
    couleurs T[0] étant déjà dans P. On met alors floor((m-1)/(n-1))
    couleurs par blocs, et une de plus si i<=(m-1)%(n-1).
   */
  r=m1%n1; /* il reste r couleurs */
  q=(m1/n1)+(r>0); /* nombre de couleurs par blocs (+1 au départ si r>0) */
  for(i=j=1;i<n;i++){ /* on traite le bloc B_i */
    c1=c2; /* c1=T[i-1]. Au départ c2 vaut T[0] */
    c2=T[i];
    dr=c2.r-c1.r;
    dg=c2.g-c1.g;
    db=c2.b-c1.b;
    for(j=1;j<=q;j++){ /* extrapolation linéaire de q points dans
			  l'intervalle ]c1,c2] */
      c.r=c1.r+(j*dr)/q;
      c.g=c1.g+(j*dg)/q;
      c.b=c1.b+(j*db)/q;
      P[k++]=c;
    }
    if(i==r) q--; /* les blocs B_{r+1}...B_{n-1} auront 1 couleur de
		     moins */
  }
  return P;
}


/***********************************

       ROUTINES POUR LES
       FONCTIONS adj()

***********************************/

double Norme(int i,int j){
/*
  Calcule la distance entre les points (XPOS[i],YPOS[i]) et
  (XPOS[j],YPOS[j]) selon la norme définie par la constante NORM
  (=1,2,...).  Dans le cas de la L_2 (NORM=2) c'est le carré de la
  norme qui est retourné pour éviter l'extraction d'une racine carrée.

  NORM=1 -> L_1
  NORM=2 -> L_2
  NORM=3 -> L_max
  NORM=4 -> L_min
*/
  double x,y;
  x=XPOS[i]-XPOS[j];
  y=YPOS[i]-YPOS[j];

  if(NORM==2) return (double)(x*x) + (double)(y*y); /* norme L_2 */

  /* x=fasb(x), y=fabs(y) */
  if(x<0.0) x=-x;
  if(y<0.0) y=-y;

  if(NORM==1) return (x+y);    /* norme L_1 */
  if(NORM==3) return MAX(x,y); /* norme L_max */
  if(NORM==4) return MIN(x,y); /* norme L_min */
  return 0.0; /* si aucune NORM trouvée */
}


double distgone(int u,int v,int i,int p,int k,double w){
/*
  Calcule la distance P_i(u,v). Il s'agit de la "distance p-gone" (un
  polygone régulier à p cotés) relative à la direction i (axe d'angle
  i*2pi/k) entre les points u et v, restreint au cône de visibilité
  d'angle w*(p-2)*pi/p (d'angle w*pi si p est infini, c'est-à-dire
  p<3), w=0...1 (voir aussi la définition du thetagone dans l'aide en
  ligne). Ici, k est le nombre de directions.  L'algorithme est en
  O(1).

  Soient a_j (j=0...p-1) les sommets du p-gone P_i de rayon unité avec
  a_0=u et numérotés consécutivement en tournant dans le sense
  positif. Soit c le centre de P_i. Donc dist(u,c)=1, les a_j sont sur
  un cercle de rayon unité. On remarque que l'angle (u,a_j) est
  indépendant du rayon du p-gone. En fait, l'angle (a_j,u,a_{j+1})
  vaut la moitié de l'angle (a_j,c,a_{j+1}), soit pi/p. Et donc
  l'angle entre deux cotés consécutifs d'un p-gone vaut
  (p-2)*pi/p. Pour le calcul de distgone(u,v) on fait:

  1. Trouver la direction j tq (u,v) est dans le cône de visibilité et
     dans la région [(u,a_j),(u,a_{j+1})[.  Si j n'existe pas, alors
     on renvoit une distance infinie. Si p est infini, a_j est
     simplement sur la droite (u,v).

  2. On calcule l'intersection v' entre les droites (u,v) et
     (a_j,a_{j+1}). Si p est infini, v'=a_j. L'intersection existe
     forcément. Eventuellement v est sur la droite (u,a_j).

  3. distgone(u,v)=dist(u,v)/dist(u,v').

*/
  int j;
  double xu,xv,dxc,dxa,dxb,dxv;
  double yu,yv,dyc,dya,dyb,dyv;
  double hv,A,Ac,Ap,Aw;

  xu=XPOS[u];yu=YPOS[u]; /* coordonnées de u */
  xv=XPOS[v];yv=YPOS[v]; /* coordonnées de v */
  Ac=(double)i*2.0*M_PI/(double)k; /* angle (u,c), c=centre de P_i */
  dxc=cos(Ac);dyc=sin(Ac); /* coordonnées du centre c dans le repère u */

  dxv=xv-xu;dyv=yv-yu; /* coordonnées de v dans repère u */
  hv=hypot(dxv,dyv); /* |v-u|=dist(u,v) */
  if(hv==0.0) return 0.0; /* si u,v ont les mêmes coordonnées */

  /*
    Rappel: Si a et b sont deux vecteurs, alors le produit scalaire
    (dot product) est le réel a.b = xa*xb + ya*yb = |a|*|b|*cos(a,b),
    où |a|=sqrt(xa^2 + ya^2). Donc le signe de cos(a,b) est celui de
    xa*xb + ya*yb. Pour calculer sin(a,b) il faut faire une rotation
    de +pi/2 au vecteur a et calculer cos(a',b) où a'=(-ya,xa). Donc
    sin(a,b) = cos(a',b) = (-ya*xb + xa*yb) / (|a|*|b|). Et le signe
    de sin(a,b) est celui de xa*yb - ya*xb.
  */

  /* Aw=demi-angle du cône de visibilité */
  Aw=0.5*w*M_PI; /* si p infini */
  if(p>2) Aw *= (double)(p-2)/(double)p; /* si p est fini */

  /*
    Il faut bien sûr que (u,v) soit dans le cône de visibilité. La
    bissectrice de ce cône est l'axe (u,c) et son angle est
    w*(p-2)pi/p (w*pi si p infini). On note (w1,u,w2) le cône en
    question. Il faut que (u,v) soit entre (u,w1) (compris) et (u,w2)
    (non compris). Donc si sin(w1,u,v) < 0 ou si sin(w2,u,v) > 0 alors
    il n'existe pas de j (et donc on retourne une distance infinie).
  */

  A=Ac-Aw; /* A=angle (c,w1) */
  dxa=cos(A);dya=sin(A); /* coordonnées de w1 relatif à u */
  if(dxa*dyv<dya*dxv) return DBL_MAX; /* v avant w1 */

  A=Ac+Aw; /* A=angle (c,w2) */
  dxa=cos(A);dya=sin(A); /* coordonnées de w2 relatif à u */
  if(dxa*dyv>=dya*dxv) return DBL_MAX; /* v après ou sur w2 */

  /*
    Ici v est dans le cône de visibilité et donc la droite (uv)
    intersecte P_i en un point v'.
  */

  Ac -= M_PI; /* Ac=Ac-pi */

  /* Cas p infini */
  if(p<3){
    /*
      On raisone dans le repère u.  On pose c'=(dyc,-dxc),
      c'est-à-dire une rotation de -pi/2 de (u,v). On a |uc'|=1. On
      calcule l'angle A=(uv,uc'), en fait cos(A). On obtient v' en
      tournant autour de c d'un angle Ac-M_PI+2A ...
     */
    A=acos((dxv*dyc-dyv*dxc)/hv);
    A=Ac+2.0*A;
    dxa=dxc+cos(A);dya=dyc+sin(A);
    return hv/hypot(dxa,dya);
  }

  /*
    Cas p fini.  On cherche j de sorte qu'en tournant dans le sens
    positif, le vecteur (u,v) soit compris entre (u,a_j) (compris) et
    (u,a_{j+1}) (non compris). La droite (u,v) intersecte le segment
    [a_j, a_{j+1}[. L'indice j recherché est l'entier tq: (j-1)*pi/p
    <= angle(a_1,u,a_j) < j*pi/p. Et donc, j = 1 +
    floor{arcos(a_1,u,v)/(pi/p)}.
  */

  Ap=2.0*M_PI/(double)p; /* valeur souvent utilisée */
  A=Ac + Ap; /* angle (c,a_1) */
  dxa=dxc+cos(A);dya=dyc+sin(A); /* coordonnées de a_1 relatif à u */

  /* Aw=cos(a_1,u,v) = (a_1-u).(v-u) / dist(a_1,u)*dist(u,v) */
  Aw=(dxa*dxv+dya*dyv)/(hypot(dxa,dya)*hv);
  j=(int)((acos(Aw)*(double)p)/M_PI); /* en fait, la variable j vaut "j-1" */
  A += Ap*(double)j; /* angle (c,a_j): on part de a_1, donc on décale de j-1 cônes */
  dxa=dxc+cos(A);dya=dyc+sin(A); /* coordonnées de a_j relatif à u */
  A += Ap; /* angle (c,a_{j+1}) */
  dxb=dxc+cos(A)-dxa;dyb=dyc+sin(A)-dya; /* vecteur (a_j,a_{j+1}) */

  /*
    Calcule l'unique intersection entre la droite Dv définie par le
    vecteur (u,v) et la droite Dj définie par le vecteur
    (a_j,a_{j+1}). Dans le repère u, les équations sont:

    Dv: dyv*X - dxv*Y = 0
    Dj: dyb*X - dxb*Y = B avec B=dyb*dxa-dxb*dya,

    car a_j=(dxa,dya) appartient à Dj.
    L'intersection (x0,y0) est, dans le repère u:
    en faisant dxv*Dj-dxb*Dv, on a: x0=dxv*B/(dxv*dyb-dxb*dyv)
    en faisant dyb*Dv-dyv*Dj, on a: y0=dyv*B/(dxv*dyb-dxb*dyv)
  */

  A=(dyb*dxa-dxb*dya)/(double)(dxv*dyb-dxb*dyv); /* A=B/(...) */
  return hv/hypot(dxv*A,dyv*A);
}


void TraverseRegTree(int h,int k,int r){
/*
  Parcoure récursivement un arbre régulier de profondeur h, degré
  interne k et de racine r. Cette fonction fixe REP des sommets de
  tous les sous-arbres du sommet dont le nom est N, c'est-à-dire
  REP[N][i]. Après l'appel N vaut le nombre de sommets de l'arbre
  parcouru, ou encore le nom du prochain sommet. La racine possèdera r
  fils, les autres k fils.
*/
  int p,i;
  p=N++; /* Mémorise le nom du père, puis incrémente N. Après N vaut
	    le nom du prochain sommet, et s'il n'y a plus personne, le
	    nombre de sommets du graphe */
  if(h==0) return;
  for(i=0;i<r;i++){ /* tous les fils ont pour père p */
    REP[N][0]=p;
    TraverseRegTree(h-1,k,k); /* sommet interne, donc de degré k */
  }
  return;
}


int Dick(int *B,int n)
/*
  Remplit un tabeau B aléatoire de n entiers >=0 dont la somme fait n
  se qui représenté en unaire correspond à un mot de Dick (autant de 0
  que de 1). Le début du mot (racine) commence à l'indice r renvoyé
  par la fonction.

  Ex: pour n=4, on tire B=[0,1,2,1], ce qui en unaire (x -> 1^x0)
  donne le mot "01011010". Pour obtenir un mot de Dick, il faut
  décaler B cycliquement de r=1 positions vers la gauche. Ici r=1,
  pour avoir le mot "10110100".
*/
{
  int i,r,s,h;

  /* remplit B de valeurs dont la somme fait n */

  for(i=0;i<n;B[i++]=0); /* met tout à zéro */
  for(i=0;i<n;i++) B[random()%n]++; /* on incrémente n fois les positions aléatoires */

  /* cherche la première racine r, et donc la hauteur minimum */

  r=-1;
  for(i=s=h=0;i<n;i++){
    s += B[i]-1;
    if(s<h) { h=s; r=i; }
  }
  
  return (r+1)%n;
}

int** RandomTree(int n,int **T,int code)
/*
  Renvoie un arbre plan enraciné aléatoire à n sommets. Les sommets
  forment un DFS à partir de la racine numérotée 0. On remplit la
  matrice nx1 T où T[i][0] est le père du sommet i. Si T=NULL, on
  alloue une matrice nx1. Dans tous les cas on renvoie T. Le père de
  la racine est -1, donc T[0][0]=-1. Si code=1, le tableau interne
  TREE est effacé (libéré) sinon il est préservé (sert à outerplanar).

   Ex: Arbre avec N=5 sommets, soit n=4 arêtes.  On tire un tableau
   B=[0,1,2,1] (de taille n et dont la somme fait n), ce qui en unaire
   (x -> 1^x0) donne le mot de Dick "01011010". On décale le mot sur
   un minimum (r=1 pour ce B), d'où le mot "10110100":

   Mot     3   4        Arbre   0
          / \ / \              / \
     1   2   2   2            1   2
    / \ /         \              / \
   0   0           0            3   4

   Ici TREE=[0,1,0,2,3,2,4,2,0] (taille 2N-1): c'est la liste des
   sommets rencontrés sur une marche le long du mot. On remarque que i
   et j sont voisins ssi on trouve i,j voisins dans le tableau
   TREE. Si Si i<j, alors pas montant, sinon pas descendant.
   Remarque: 0 est toujours la racine, et le sommet N-1 est la
   dernière feuille.

   Note: en fait, ce n'est pas aléatoire uniforme car pour obtenir le
   tableau B=[1,1,1] par exemple, il n'y a qu'une seule solution.
   Alors que pour B=[0,2,1] il y a trois façons de faire, en ajoutant
   1 aux positions de B: 1,2,1 ou 1,1,2 ou 2,1,1.

*/
{
  int n1,i,r,k,s,m,t,h;
  int *B,*H; /* tableaux auxiliaires */

    if(n<2){ /* fin si moins de 2 sommets */
      if(!code) ALLOC(TREE,1,int);
      ALLOCMAT(T,1,1,int);
      T[0][0]=-1; /* racine du père=-1 */
      return T;
    }

    ALLOC(TREE,(n<<1)-1,int); /* il faut n>0 */
    if(T==NULL) ALLOCMAT(T,n,1,int); /* l'arbre final */
    n1=n-1;
    ALLOC(B,n1,int);
    ALLOC(H,n+1,int); /* hauteurs possibles: 0...n */
    r=Dick(B,n1);

    /*
      Remplissage de TREE: on parcours le mot à partir de la racine r.
      - H[h]=dernier sommet rencontré de hauteur h.
      - h=hauteur du dernier sommet de H
      - s=numéro du prochain sommet
      - t=index libre du tableau TREE.
    */
    TREE[0]=H[0]=h=0; /* on traite le sommet 0 */
    s=t=1;

    for(i=0;i<n1;i++){
      m=B[(i+r)%n1];
      for(k=0;k<m;k++){
	TREE[t++]=s;
	H[++h]=s++;
      }
      TREE[t++]=H[--h];
    }

    free(B); /* B et H ne servent plus à rien */
    free(H);

    /* Remplit T en parcourant le tableau TREE. Soient r,s deux
       valeurs consécutives dans TREE. Si r=n-1, alors tous les
       sommets ont été parcouru.  Si s>r, alors c'est un pas montant,
       et donc T[s][0]=r. */

    T[r=i=0][0]=-1; /* pas de père pour la racine */
    while(r<n-1){
      s=TREE[++i];
      if(s>r) T[s][0]=r;
      r=s;
    }

    /* libère la mémoire */
    if(code) free(TREE);
    return T;
}


int NextPermutation(int *P,int n,int *C)
/*
  Génère, à partir d'une permutation P, la prochaine dans l'ordre
  lexicographique suivant les contraintes définies par le tableau C
  (voir ci-après). On renvoie 1 ssi la dernière permutation a été
  atteinte. Dans ce cas la permutation la plus petite selon l'ordre
  lexicographique est renvoyée. On permute les éléments de P que si
  leurs positions sont entre C[j] et C[j+1] (exclu) pour un certain
  indice j. On peut initialiser P avec ALLOCZ(P,k,int,_i) ou si le
  tableau P existe déjà avec NextSet(P,-1,k).

  Ex: C={2,3,5}. Les permutations sous la contrainte C sont:
  (on peut permuter les indices {0,1}{2}{3,4})

                 0 1 2 3 4 (positions dans P)
	      P={a,b,c,d,e}
	        {b,a,c,d,e}
		{a,b,c,e,d}
		{b,a,c,e,d}
  
  Evidemment, il y a beaucoup moins de permutations dès que les
  contraintes augmentent. Par exemple, si C contient k intervalles de
  même longueur, alors le nombre de permutations sera de (n/k)!^k au
  lieu de n!. Le rapport des deux est d'environ k^n.

  Concrêtement, pour:
  - n=9 et k=3, on a 216 permutations au lieu de 362.880 (k^n=19.683)
  - n=12 et k=3, on a 13.824 permutations au lieu de 479.001.600 (k^n=531.441)

  Le dernier élément de C doit être égale à n-1 (sentinelle), le
  premier étant omis car toujours = 0. Donc C est un tableau à au plus
  n éléments. Si C=NULL, alors il n'y a pas de contrainte
  particulière, ce qui est identique à poser C[0]=n.

  On se base sur l'algorithme classique (lorsque C=NULL, sinon on
  l'applique sur l'intervalle de positions [C[j],C[j+1][):

  1. Trouver le plus grand index i tel que P[i] < P[i+1].
     S'il n'existe pas, la dernière permutation est atteinte.
  2. Trouver le plus grand indice j tel que P[i] < P[j].
  3. Echanger P[i] avec P[j].
  4. Renverser la suite de P[i+1] jusqu'au dernier élément.

*/
{
  int i,j,a,b,c,T[1];

  if(C==NULL){
    T[0]=n;
    C=T;
  }

  b=C[i=j=0]; /* j=indice de la prochaine valeur à lire dans C */
  c=-1;

  /* étape 1: on cherche l'intervalle [a,b[ avec i tq P[i]<P[i+1] */
 etape1:
  for(a=i;i<b-1;i++) if(P[i]<P[i+1]) c=i; /* on a trouvé un i tq P[i]<P[i+1] */
  if(c<0){ /* le plus grand i tq P[i]<[i+1] n'existe pas */
    for(i=a;i<b;i++) P[i]=i; /* on réinitialise P[a]...P[b-1] */
    if(b==n) return 1; /* alors on a fini d'examiner C */
    b=C[++j]; /* b=nouvelle borne */
    goto etape1;
  }
  i=c; /* i=le plus grand tq P[i]<P[i+1] avec a<=i,i+1<b */

  /* étape 2: cherche j=le plus grand tq P[i]<P[j] */
  for(j=i+1;j<b;j++) if(P[i]<P[j]) c=j;
  j=c;

  /* étape 3: échange P[i] et P[j] */
  SWAP(P[i],P[j],c);

  /* étape 4: renverse P[i+1]...P[b-1] */
  for(++i,--b;i<b;i++,b--) SWAP(P[i],P[b],c);

  return 0;
}


int NextSet(int *S,int n,int k){
/*
  Calcule les sous-ensembles de k entiers de [0,n[. Si n<0, alors S
  est initialisé au plus petit ensemble possible, soit S={0,1,2,
  ...,k-1}. L'idée est de maintenir une sorte de compteur S qui
  représente le sous-ensemble courant et qui va passer par tous les
  sous-ensembles possibles. Les éléments sont rangés dans l'ordre
  croissant. On renvoie 1 ssi S était le dernier sous-ensemble. Dans
  ce cas l'ensemble le plus petit est écrit dans S.

  On peut facilement en déduire un algorithme pour générer des
  multi-ensembles, comme par exemple tous les multi-ensembles du type
  [2,0,0,2,1,0] comprennant 3 fois 0, 1 fois 1 et 2 fois 2:
  NextMultiSet(S,C,k) avec C=[3,1,2] (voir la fonction ggosset()).

  La stratégie pour "incrémenter" S est la suivante : on essaye
  d'incrémenter le plus petit élément de S[i] tout en restant
  strictement inférieur à l'élément suivant S[i+1]. Si c'est possible
  on a trouvé le nouveau sous-ensemble. Sinon, on réinitialise
  S[0],...,S[i] à leur plus petites valeurs: S[0]=0,...,S[i]=i. Si on
  a pas pu incrémenter S[k-1] sans dépasser n on a atteint le dernier
  sous-ensemble.

  L'algorithme est en O(k) dans le pire des cas, mais de manière
  amortie c'est beaucoup moins car on incrémente moins souvent S[j]
  que S[i] si j>i.
*/
  int i=0,j,s;

  if(n<0){
    for(;i<k;i++) S[i]=i;
    return 0;
  }
  
  while(i<k){
    s=++S[j=i++];
    if(i==k){
      if(s<n) return 0;
      goto fin;
    }
    if(s<S[i]) return 0;
  fin:
    S[j]=j;
  }
  return 1;
}


int NextArrangement(int *S,int *P,int n,int k){
/*
  Permet de générer tous les arrangements de k entiers de
  [0,n[. L'arrangement est représenté par les tableaux S et P de k
  entiers. S représente un ensemble de k entiers et P une permutation
  de [0,k[. Ainsi, l'arrangement A=(4,2,7,3) est représenté par
  S=(2,3,4,7) et P=(2,0,3,1). Autrement dit A[i]=S[P[i]] pour tout
  i=0...k-1.

  L'idée est d'essayer d'incrémenter le double compteur S,P. On essaye
  d'incrémenter P en premier avec NextPermutation(). Si on est arrivé
  à la fin de P, on incrémente S avec NextSet(). Si n<0, alors S et P
  sont initialisés au plus petit arrangement possible, soit
  S=P=(0,1,2, ...,k-1). On renvoie 1 ssi S,P était le dernier
  arrangement. Dans ce cas l'arrangement le plus petit est écrit dans
  S,P.
*/
  if(n<0){
    int i;
    for(i=0;i<k;i++) S[i]=P[i]=i;
    return 0;
  }
  if(NextPermutation(P,k,NULL)) return NextSet(S,n,k);
  return 0;
}


int *NextPart(int *S,int n,int s,int *C)
/*
  Permet de générer toutes les suites S de n entiers >=0 dont la somme
  fait s et dont la i-ème part S[i] ne dépasse pas C[i]. Il faut que s
  <= sum_{i=0}^(n-1) C[i], n=1 est possible.

  S est la suite courante de somme s et on renvoie dans S la prochaine
  suite (la fonction renvoie aussi S). On renvoie NULL si on a atteint
  la dernière suite, et on remplit S avec le première suite. Si
  S=NULL, alors S est allouée et initialisée à la première suite. La
  première suite de somme s est obtenue en remplissant autant que
  possible les parts S[n-1],S[n-2],...

  L'algorithme est le suivant:
  1. on détermine le plus grand indice j tq S[j]>0
  2. si j=0, alors on a fini: on fait 5. avec x=s+1 et i=-1
  3. on détermine le plus grand indice i<j tq S[i] peut être augmenté
  4. on calcule x = sum_{j=i+1}^(n-1) S[i]
  5. on incrémente S[i]
  6. on remplit S[i+1]...S[n-1] avec la première suite de somme x-1

  Exemple: s=n=5

  C=1 2 2 1 1
  S=0 1 2 1 1
    0 2 1 1 1
    0 2 2 0 1
    0 2 2 1 0
    1 0 2 1 1
    1 1 1 1 1
    1 1 2 0 1
    1 1 2 1 0
    1 2 0 1 1
    1 2 1 0 1
    1 2 1 1 0
    1 2 2 0 0
*/
{
  int x,i,j,r;

  i=0;
  r=(S==NULL);
  if(r) ALLOC(S,n,int);
  else i=n-1;
  
  /* calcule le plus grand indice i tq S[i]>0 */
  while((i>0)&&(S[i]==0)) i--;
  x=S[i--]; /* rem: si i=0, alors i -> -1 */

  /* calcule le plus grand indice j<i tq S[j] peut être augmenté */ 
  while((i>=0)&&(S[i]==C[i])) x += S[i--];

  if(i>=0){ S[i]++; s=x-1; } /* si cet indice n'existe pas i<0 => FIN */
  
  /* écrit la première suite de somme s dans S[i+1]...S[n-1] */
  for(j=n-1;j>i;j--){
    x=MAX(s,0);
    x=MIN(C[j],x);
    S[j]=x;
    s -= x;
  }

  /* on retourne S sauf si i<0 et r=0 (<=> FIN ) */
  return ((i<0)&&(!r))? NULL : S;
}


int SetCmp(int *T1,int *T2,int n1,int n2)
/*
  Compare deux tableaux d'entiers T1 et T2 de taille n1 et n2 triés
  par ordre croissant. Les tableaux peuvent être de taille nulle. La
  valeur renvoyée est un entier interprété en binaire comme suit:

  bit-0: 1 ssi T1 intersecte T2 (possède un élément commun)
  bit-1: 1 ssi T1 égale T2
  bit-2: 1 ssi T1 est strictement inclu dans T2
  bit-3: 1 ssi T2 est strictement inclu dans T1

  Les valeurs possibles sont donc: 0,1,2,3,4,5,8,9 (=[0,9]\{6,7})
  La complexité est O(n1+n2).
*/
{
  if(n1==0) return (n2==0)? 2:4;
  if(n2==0) return 8;
  /* ici T1 et T2 contiennent au moins 1 élément */

  if((T1[n1-1]<T2[0])||(T2[n2-1]<T1[0])) return 0; /* cas trivial de disjonction */

  int i1,i2,r;
  i1=i2=0;
  r=14; /* tous les bit à 1 sauf b0 */

  while((i1<n1)&&(i2<n2)){
    if(T1[i1]==T2[i2]){
      i1++; i2++; /* T1 et T2 ont un élément en commun */
      r |= 1; continue; /* met b0 */
    }
    r &= 13; /* annule b1 (15-2) car T1<>T2 */
    if(T1[i1]<T2[i2]){
      i1++; /* T1 contient des éléments qui ne sont pas dans T2 */
      r &= 11; /* annule b2 (15-4) car T1 ne peux pas contenir T1 */
    }else{
      i2++; /* T2 contient des éléments qui ne sont pas dans T1 */
      r &= 7; /* annule b3 (15-8) car T2 ne peux pas contenir T1 */
    }
  }

  if(i1<n1) r &= 9; /* annule b2 et b1 (15-4-2) */
  if(i2<n2) r &= 5; /* annule b3 et b1 (15-8-2) */
  if(r&2) r &= 3; /* annule b3 et b2 (15-8-4) */

  return r;
}

int *SetInter(int *T1,int *T2,int n1,int n2)
/*
  Détermine si les tableaux d'entiers T1 et T2 de taille n1 et n2
  triés par ordre croissant ont un élément commun (intersection
  non-vide). S'ils sont disjoints on renvoie NULL. Sinon on renvoie un
  pointeur vers un élement commun de T1 (si n1<=n2) ou de T2 (si
  n1>n2). La complexité est O(min{n1,n2}*log max{n1,n2}).
*/
{
  int u,*t,*tf;
  if(n1>n2){ SWAP(n1,n2,u); SWAP(T1,T2,t); }
  for(tf=(t=T1)+n1;t<tf;t++) /* pour chaque élément (*t) de T1 */
    if(bsearch(t,T2,n2,sizeof(int),fcmp_int)!=NULL) return t;
  return NULL;
}


int Binom(int n,int k)
/*
  Calcule l'entier B={n choose k}.  L'algorithme utilisé ici est en
  O(k). Il n'utilise que des multiplications et divisions entières sur
  des entiers en O(B), sans aucun tableau.

  L'algorithme issu du Triangle de Pascal est lui en O(n*k), utilise
  un espace en O(k) (tableaux d'entiers en O(B)). Par contre il
  n'utilise que des additions sur des entiers en O(B).

  Principe: B = n x (n-1) x ... x (n-k+1) / k x (k-1) x ... x 1

  On réecrit en (((((n/1) x (n-1)) / 2) x (n-2)) / 3) ...
  c'est-à-dire en multipliant le plus grand numérateur et en divisant
  par le plus petit numérateur. Chaque terme est ainsi un certain
  binomial, et donc un entier.
*/
{
  int B,s,i;
  for(B=s=n,i=2;i<=k;i++) B=(B*(--s))/i;
  return B;
}

#define SORT_FREQv  0
#define SORT_FREQe  1
#define SORT_INDEXe 2
#define SORT_INDEXi 3 /* tri normal */
#define SORT_RANKv  4
#define SORT_RANKe  5
#define SORT_INC 6 /* tri croissant */
#define SORT_DEC 7 /* tri décroissant */

int *SortInt(int *T,int *R,int n,int a,int *m,const int code)
/*
  Tri par ordre croissant un tableau T de taille n dont les valeurs
  sont des entiers de [a,a+m[. Si m=NULL, alors les valeurs de T sont
  supposées être dans [a,a+n[, sinon m=*m. La complexité en temps est
  O(n+m). Le tableau T n'est pas modifié. Le résultat est rangé dans
  le tableau R qui doit être de taille au moins n, et aussi renvoyé
  par la fonction. Il R=NULL, alors R est alloué et retourné par la
  fonction. Pour être compétitif avec qsort() pour m=n il faut,
  théoriquement, n>32.

  L'opération de tri dépend de la constante "code" qui est interprétée
  comme suit (la complexité est le nombre d'étapes dans les boucles
  "for"):

    v=valeur d'un élément, v dans [a,a+m[
    d=nombre réel de valeurs v distinctes dans T, d<=m
    e=élément du tableau, e dans [0..n[, v=T[e]
    i=indice dans un tableau, i dans [0..n[, R[i]=T[e]
    r=rang dans un tableau, r dans [0..d[

  - code=SORT_FREQv: renvoie le tableau F[0..m[ de fréquence où F[v]
    est le nombre de fois où la valeur v+a apparaît dans T. Le tableau
    R et la variable m ne sont pas modifiés. Complexité: m+n.

  - code=SORT_FREQe: renvoie dans R[0..n[ un tableau de fréquence où
    R[e] est le nombre de fois où la valeur T[e] apparaît dans T. La
    variable m n'est pas modifiée. Complexité: m+2n.

  - code=SORT_INDEXi: renvoie dans R[0..n[ un tableau d'indices où
    R[i] est l'élément e dans l'ordre i de T. Autrement dit, R est le
    tableau T trié. La variable m n'est pas modifiée. Complexité:
    2m+2n.

  - code=SORT_INDEXe: renvoie dans R[0..n[ un tableau d'indices où
    R[e] est l'ordre i de l'élément e dans T. La variable m n'est pas
    modifiée. Complexité: 2m+2n.

  - code=SORT_RANKv: renvoie dans R[0..d[ un tableau de rangs où R[v]
    est le rang r de la valeur v+a dans T. Le tableau R est modifié et
    on renvoie d dans m. Complexité: 3m+n.

  - code=SORT_RANKe: renvoie dans R[0..n[ un tableau de rangs où R[e]
    est le rang r de l'élément e dans T. Le tableau R est modifié et
    on renvoie d dans m. Complexité: 3m+2n.

    BUG:
    Ne marche pas si a=0,m=n, T=2 1 2 3 2 5 6.
    Retourne                  R=1 0 1 2 1 5 4 (le 5 devrait être un 3).
*/
{
  int *F,i,r,t;
  int k=(m==NULL)? n:*m;

  ALLOCZ(F,k,int,0);
  for(i=0;i<n;i++) F[T[i]-a]++;
  if(code==SORT_FREQv) return F;

  if((R==NULL)&&(code!=SORT_RANKv)) ALLOC(R,n,int);

  if(code==SORT_FREQe){
    for(i=0;i<n;i++) R[i]=F[T[i]-a];
    free(F);
    return R;
  }

  if(code==SORT_INC){ /* R=tableau T trié */
    for(i=r=0;i<k;i++) for(t=F[i];t>0;t--) R[r++]=i+a;
    free(F);
    return R;
  }

  if(code==SORT_DEC){ /* R=tableau T trié, ordre décroissant */
    for(i=0,r=n;i<k;i++) for(t=F[i];t>0;t--) R[--r]=i+a;
    free(F);
    return R;
  }

  for(i=r=0;i<k;i++){ t=F[i]; F[i]=r; r += t; }

  /* Ici F[i] indique que le prochain élément e de valeur i+a, i.e. tq
     T[e]-a=i, doit être mis dans R à la position F[i]. Autrement dit,
     il faut faire R[e]=F[i] puis incrémenter F[i]. */

  if(code==SORT_INDEXi){
    for(i=0;i<n;i++) R[F[T[i]-a]++]=i;
    goto fin;
  }

  if(code==SORT_INDEXe){
    for(i=0;i<n;i++) R[i]=F[T[i]-a]++;
    goto fin;
  }

  for(t=-1,i=r=0;i<k;i++) if(F[i]!=t) { t=F[i]; F[i]=r++; }
  if(m!=NULL) *m=r; /* m=nb de valeurs différentes */

  /* Ici F[i] indique le rang (dans [0,r[) de la valeur i+a dans T. */

  if(code==SORT_RANKv) return F;
  if(code==SORT_RANKe){
    for(i=0;i<n;i++) R[i]=F[T[i]-a];
    goto fin;
  }

  R=NULL;

 fin:
  free(F);
  return R;
}


/***********************************

       ROUTINES SUR
       LES GRAPHES

***********************************/


void PrintGraph(graph *G)
/*
  Affiche un graphe ou une famille de graphes au format standard sous
  forme compacte. Utilise WIDTH. Effet de bord: le (ou les) graphes
  sont triés, et donc G->sort=1.
*/
{
  if(G==NULL){ printf("NULL\n"); return; }

  int u,v,i,k,n,ligne,nk=(G->f>0)?G->f:1;
  graph *H;
  int *P;

  for(k=0;k<nk;k++){

    if(G->f>0){
      H=G->G[k];
      printf("[%i]",H->id);
    } else H=G;

    SortGraph(H);
    n=H->n;
    ALLOCZ(P,n,int,0);
    i=u=ligne=0;
    v=-1;

    while(i<n){
      /* si u==i, alors u=tête d'un nouveau chemin */
      while((v<u)&&(P[u]<H->d[u])) v=H->L[u][P[u]++];
      if(v<u){ /* on a pas trouvé v => fin d'un chemin */
	if(H->d[u]==0){ /* cas des sommets isolés */
	  printf(" %i",u);
	  if(++ligne==WIDTH){ printf("\n"); ligne=0; }
	}
	u=(i+=(u==i));
	v=-1;
      }
      else{ /* u a un voisin v>u */
	if((u==i)||(ligne==0)) printf(" %i",u); /* on affiche la tête */
	printf("-%i",v); /* on affiche v */
	if(++ligne==WIDTH){ printf("\n"); ligne=0; }
	u=v; /* on continu avec v */
	v=-1;
      }
    } /* fin du while */

    if(ligne>0) printf("\n"); /* newline si fini avant la fin de ligne */
    free(P);
  }

  G->sort=1; /* effet de bord */
  return;
}


int NbEdges(graph *G)
/*
  Retourne le nombre d'arêtes de G, G->m si positif. Si G->m<0, alors
  G->m est mis à jour à partir de la somme des G->d[i].
*/
{ int m=G->m;
  if(m<0){
    int i,n=G->n;
    for(i=m=0;i<n;i++)
      m += G->d[i];
    G->m=(m>>=1);
  }
  return m;
}


int Degree(graph *G,int max)
/*
  Renvoie le degré maximum (si max=1) ou minimum (si max=0) d'un
  graphe G. On renvoie -1 si G est nul ou est une famille de graphes.
*/
{ if((G==NULL)||(G->f>0)) return -1;
  int i=1,n=G->n,d=G->d[0];
  for(;i<n;i++)
    if(max) d=MAX(d,G->d[i]);
    else d=MIN(d,G->d[i]);
  return d;
}


/***********************************

       BFS, DFS, ...
       (DIJKSTRA)

***********************************/


typedef struct{
  int root;  /* racine du BFS. */
  int rad;   /* rad=eccentricité du sommet source s. */
  int girth; /* girth=longueur (>2) plus petit cycle passant par
		root. Si girth<0, alors la girth est infinie. */
  int nc;    /* nombre de sommets parcourus */
  int *D;    /* D[u]=distance entre u et root. Les sommets u avec
	        D[u]=-1 sont à une distance infinie de root. En
	        entrée, les sommets avec D[u]=-1 doivent être
	        considérés inexsitant dans G. Si D=NULL, D est alloué
	        et initialisé à -1. */
  int *P;    /* P[u]=père de u dans un arbre BFS de racine root, avec
	        P[root]=-1. Seuls les sommets u avec D[u]>=0 ont un
	        père défini (sauf si u=root). Si P=NULL, alors P est
	        alloué. */
  //int *s;    /* s[i]=u si dans le i-ème sommet parcouru est u */
  //int *i;    /* i[u]=i si s[i]=u */
} param_bfs;


param_bfs *new_param_bfs(int n)
/*
  Si n>0, alloue le tableau P.  Le tableau D peut être utilisé pour
  effacer des sommets.
*/
{
  param_bfs *X;
  ALLOC(X,1,param_bfs);

  X->D=X->P=NULL;
  //X->s=X->i=NULL;
  X->rad=X->nc=0;
  X->girth=X->root=-1;
  if(n>0){
    ALLOC(X->P,n,int);
    //ALLOC(X->s,n,int);
    //ALLOC(X->i,n,int);
  }

  return X;
}

void free_param_bfs(param_bfs *X){
  if(X==NULL) return;
  free(X->D);
  free(X->P);
  //free(X->s);
  //free(X->i);
  free(X);
}


param_bfs *bfs(graph *G,int source,param_bfs *X)
/*
  Effectue un BFS sur G depuis source. Si X=NULL, X est alloué et
  renvoyé. On l'utilise comme ceci:

  param_bfs *p=bfs(G,s,NULL); // BFS depuis le sommet s
  ... // p->D[u]=distance entre s et u
  free_param_bfs(p);

  ou pour un BFS d'un sous-graphe de G évitant les sommets de T:

  param_bfs *p=new_param_bfs(G->n);
  ALLOCZ(p->D,G->n,int,-1);
  for(i=0;i<G->n;i++) p->D[T[i]]=-2;
  bfs(G,s,p);
  ... // p->D[u]=distance entre s et u dans G\T
  free_param_bfs(p);

ALGO:
- prendre le sommet u en tête de file
- pour tout ses voisins v non marqués:
  - enfiler v
  - marquer v
  
  Si D<>NULL, alors D n'est pas initialisé et D[u]=-2 sert à supprimer u
  de G pendant le BFS.

  Pour déterminer la girth, on calcule la longueur L du premier cycle
  crée. On remarque que la girth peut être seulement L ou L-1. C'est
  L-1 seulement si u et v sont à la même hauteur. Une fois le niveau
  terminé, L ne peut plus être modifiée et la girth est
  déterminée. Pour aller plus vite, une variable g détermine si c'est
  la peine de calculer la girth.
*/
{
  int i,u,v,d,ff,tf,g,h,n=G->n;
  int *file; /* file */

  if(X==NULL) X=new_param_bfs(n);
  if(X->D==NULL) ALLOCZ(X->D,n,int,-1); /* on initialise que si D==NULL */

  for(u=0;u<n;u++) X->P[u]=-1;
  X->D[source]=0; /* distance=0 */
  X->root=source;
  X->girth=-1; /* valeur par défaut */
  g=1; /* g=vrai ssi X->girth n'est pas encore correcte */
  h=n; /* h=hauteur en dessous de laquelle le premier cycle peut apparaître */
  ALLOC(file,n,int); /* réserve une file */
  tf=ff=0; /* tf=tête de file, pointe sur la tête, ff=fin de file,
	      pointe sur prochain élément libre. */
  file[ff++]=source; /* enfile le 1er sommet = la source */

  while(tf<ff){
    u=file[tf]; /* défile la tête de file */
    //X->s[u]=tf; /* indice du sommet u dans le parcours */
    //X->i[tf]=u; /* sommet u du graphe d'indice tf dans le parcours */
    tf++;
    for(i=0,d=G->d[u];i<d;i++){ /* pour tout voisin v de u */
      v=G->L[u][i];
      if(X->D[v]==-1){ /* si v voisin non marqué, si =-2 on saute le sommet */
	X->P[v]=u; /* père de v = u */
	X->D[v]=X->D[u]+1; /* hauteur(v)=1+hauteur(père(v)) */
	file[ff++]=v; /* enfile v */
      }else /* si v a déjà été visité */
	if(g){ /* si g=faux, pas la peine de regarder la girth */
	  if((X->D[u]<h)&&(v!=X->P[u])){ /* sinon X->girth ne peut pas être modifiée */
	    if(X->girth<0) h=X->D[u]+1,X->girth=(h<<1); /* on suppose X->D[v]>X->D[u] */
	    if(X->D[v]<h) X->girth--,g=0; /* la girth est + petite et donc g=0 */
	  }
	}
    }
  }

  X->nc=ff; /* nb de sommets parcourus */
  X->rad=X->D[u]; /* hauteur du dernier sommet */
  free(file);
  return X;
}


typedef struct{
  int nc; // nc=nb de composantes connexes du graphe
  int na; // nombre de sommets d'articulation (ou cut-vertex)
  int *C; // C[u]=couleur de la composante de u, entier de [0,nc[
  int *P; // P[u]=parent de u dans une forêt couvrante, P[racine]=-1
  int *R; // R[i]=i-ème sommet racine dans la forêt couvrante, i dans [0,nc[
  int *d; // d[u]=date de début de visite du sommet u, entier de [0,n[
  int *A; // A[u]=vrai ssi u est un sommet d'articulation, u dans [0,n[
} param_dfs;


param_dfs *new_param_dfs(int n)
{
  param_dfs *X;
  ALLOC(X,1,param_dfs);

  X->C=X->P=X->R=X->A=X->d=NULL;
  X->nc=X->na=0;

  if(n>0){
    ALLOC(X->P,n,int);
    ALLOC(X->R,n,int);
    ALLOC(X->A,n,int);
    ALLOC(X->d,n,int);
  }
  return X;
}


void free_param_dfs(param_dfs *X){
  if(X==NULL) return;
  free(X->C);
  free(X->P);
  free(X->R);
  free(X->A);
  free(X->d);
  free(X);
}


param_dfs *dfs(graph *G,int source,param_dfs *X)
/*
  Effectue un parcours en profondeur du graphe G depuis le sommet
  source. Version non récursive. On détermine également tous les
  sommets d'articulations (voir la définition de param_dfs pour lire
  le résultat). On l'utilise comme suit:

  param_dfs *p=dfs(G,s,NULL); // DFS dans G depuis s
  ...
  free_param_dfs(p);

  ou alors, pour un DFS dans G évitant les sommets de T:

  param_dfs *p=new_param_dfs(G->n);
  ALLOCZ(p->C,G->n,int,-1);
  for(i=0;i<G->n;i++) p->C[T[i]]=-2;
  dfs(G,s,p);
  ...
  free_param_dfs(p);

  Si p->C[u]=-2, alors le sommet u n'est pas parcouru (il est
  virtuellement supprimé de G). Les autres sommets v (non supprimés)
  doivent avoir p->C[v]=-1. Si p->C==NULL (par défaut), alors ce
  tableau est alloué et initialisé à -1. Il sera libéré par
  free_param_dfs(p).
*/
{
  if(G==NULL) return NULL;
  int u,i,d,v,k,t,n=G->n,r=0;
  int tete,nc,na,b;
  int *pile,*next,*level;

  if(X==NULL){ r=1; X=new_param_dfs(n); }
  if(X->C==NULL) ALLOCZ(X->C,n,int,-1);
  for(i=0;i<n;i++) X->A[i]=0;

  nc=na=0;
  ALLOC(pile,n,int); /* pile */
  ALLOC(next,n,int); /* next[u]=prochain voisin de u à visiter */
  ALLOC(level,n,int); /* level[u]=... */
  t=tete=-1;

  for(k=0;k<n;k++,source=(source+1)%n)
    /* on essaye tous les sommets à partir de source */
    if(X->C[source]==-1){ /* si ==-2 ou >=0 alors on saute le sommet */
      pile[++tete]=source;
      next[source]=0; /* premier voisin à visiter */
      X->P[source]=-1;
      v=X->R[nc]=source;

      while(tete>=0){ /* tant que la pile n'est pas vide */
	u=pile[tete]; /* u=sommet courant */
	i=next[u]; /* indice du prochain voisin de u à visiter */
	if(i==0){
	  X->C[u]=nc; /* couleur de la composante courante */
	  level[u]=X->d[u]=++t; /* date de début de visite */
	}
	d=G->d[u]; /* degré de u */
	b=1; /* sentiennelle pour savoir si on a trouvé un v */
	while(i<d){ /* on cherche le prochain voisin v de u non visité */
	  v=G->L[u][i++]; /* pour tous les voisins v de u */
	  if(X->C[v]==-1){ /* si v n'a jamais été visité */
	    if((u==source)&&(t>X->d[u])&&(!X->A[u])) na++,X->A[u]=1; /* u=cut-vertex */
	    X->P[v]=u; /* père(v)=u */
	    pile[++tete]=v; /* on empile v */
	    next[v]=b=0; /* le prochain voisin de v à visiter */
	    next[u]=i; /* le prochain voisin de u à visiter */
	    break;
	  } else /* v existe et a déjà été visité */
	    if((X->C[v]>=0)&&(v!=X->P[u])) /* si (u,v) est un arc de retour */
	      level[u]=MIN(level[u],X->d[v]);
	}
	if(b){ --tete; /* il n'y a plus de voisin v: on dépile u pour toujours */
	  if((v=(X->P[u]))>=0){ /* si u n'est pas une racine, il a un père v */
	    level[v]=MIN(level[v],level[u]); /* met à jour level du père de u */
	    if((v!=source)&&(level[u]>=X->d[v])&&(!X->A[v])) na++,X->A[v]=1; /* v=cut-vertex */
	  }
	}
      } /* fin du while(i<d) */

      nc++; /* ici nc=nb de composantes visitées */
    }

  X->nc=nc;
  X->na=na;
  free(pile);
  free(next);
  free(level);

  /* on réduit le tableau ->R au minimum que si dfs() l'a
     alloué. Sinon, il ne faut pas aux pointeurs de X */
  if(r) X->R=REALLOC(X->R,nc,int);

  return X;
}


typedef struct{
  int n;        // nb de sommets du graphe
  int source;   // source
  int *pere;    // tableau des parents: pere[u]
  double *dist; // tableau de distance: dist[u]
} param_bellman;


param_bellman *new_param_bellman(int n){
  param_bellman *p;
  ALLOC(p,1,param_bellman);
  p->n=n;
  p->source=0;
  if(n>0){
    ALLOC(p->pere,n,int);
    ALLOC(p->dist,n,double);
  }
  return p;
}


void free_param_bellman(param_bellman *p){
  if(p==NULL) return;
  free(p->pere);
  free(p->dist);
  free(p);
  return;
}


param_bellman *Bellman_Ford(graph *G,int source){
  /*
    Calcule les distances de source à tous les autres selon
    l'algorithme de Bellman-Ford. On retourne un tableau ->pere et
    ->dist. Il permet d'avoir des poids négatifs sur les arcs. Il ne
    faut pas qu'il y ait de cycle absorbant.
    
    On utilise en interne 4 tableaux de taille n.

    A faire: gérer les sommets éteints.
  */
  
  if(G==NULL) return NULL;
  if(G->W==NULL) return NULL;

  int u,v,n=G->n;
  int i,i1,i2,d;
  int *pile1,*pile2,*vec1,*vec2,*tmp;
  param_bellman *p=new_param_bellman(n);
  double w;

  ALLOC(pile1,n,int);
  ALLOC(pile2,n,int);
  ALLOCZ(vec1,n,int,1); /* vec1[u]=0 ssi u est dans pile1 */
  ALLOCZ(vec2,n,int,1); /* idem mais pour vec2 */

  for(u=0;u<n;u++){
    p->dist[u]=DBL_MAX;
    p->pere[u]=-1;
  }

  p->dist[source]=0.0;
  p->pere[source]=p->source=source;

  i1=i2=0;
  pile1[i1++]=source; /* empile la source */
  vec1[source]=0; /* source est dans la pile1 */

  while(i1>0){
    while(i1>0){
      u=pile1[--i1]; /* dépile u */
      vec1[u]=1; /* u n'est plus dans la pile1 */
      d=G->d[u]; /* d=degré de u */
      for(i=0;i<d;i++){
	v=G->L[u][i];
	w=p->dist[u]+G->W[u][i];
	if(w<p->dist[v]){
	  p->pere[v]=u;
	  p->dist[v]=w;
	  if(vec2[v]){ /* si v pas dans P2, empiler v dans P2 */
	    pile2[i2++]=v;
	    vec2[v]=0;
	  }
	}
      }
    }
    SWAP(i1,i2,i);
    SWAP(pile1,pile2,tmp);
    SWAP(vec1,vec2,tmp);
  }

  free(pile1);
  free(pile2);
  free(vec1);
  free(vec2);
  return p;
}


/*
  Dijkstra(G,W,s):

  1. Init(G,s)
  2. S={}
  3. Q=V(G)
  4. while Q<>{}
  5.  do u=Extract-Min(Q)
  6.     S=S u {u}
  7.     for each v in Adj[u]: Relax(u,v,W)

  Init(G,s):
  1. for each vertex v in V(G):
  2.   do d[v]=infinity
  3.      pi[v]=NULL
  4. d[s]=0

  Relax(u,v,W):
  1. if d[v]>d[u]+W[u,v]
  2.   then d[v]=d[u]+W[u,v]
  3.        pi[v]=u

  Extract-Min(Q):
  extract vertex u with smallest d[v]
*/

int WeightGraph(graph *G){
  /*
    Calcule les longueurs (poids) des arêtes dans le cas d'un graphe
    géométrique (tout en créant le tableau les tableaux G->W). Il faut
    que les tableaux XPOS,YPOS existent. Retourne 1 ssi tout c'est
    bien passé.
  */
  if((G==NULL)||(G->xpos==NULL)) return 0;

  int u,v,i,d,n=G->n;
  double dx,dy;
  ALLOC(G->W,n,double*);

  for(u=0;u<n;u++){
    d=G->d[u];
    ALLOC(G->W[u],d,double);
    for(i=0;i<d;i++){
      v=G->L[u][i];
      dx=G->xpos[u]-G->xpos[v];
      dy=G->ypos[u]-G->ypos[v];
      G->W[u][i]=sqrt(dx*dx+dy*dy);
    }
  }
  return 1;
}

/***********************************

       ISOMORPHISM, SUBGRAPH,
       MINOR, INDUCEDSUBGRAPH,
       PATHS, PS1, TREEWIDTH ...

***********************************/


int *Isomorphism(graph *G,graph *H)
/*
  Renvoie un tableau P <> NULL ssi G est isomorphe à H. Si tel est le
  cas, le tableau P indique le morphisme de H vers G. Après l'appel,
  le graphe H est modifié: ses listes sont triées si H->sort est faux
  (sauf si G=H - même pointeur). Le graphe G n'est par contre pas
  modifié. Dans H->int1 est retourné le nombre de tests effectués.
  Moins le graphe possède de symétrie, plus faible est le nombre de
  tests (et rapide est la fonction).

  On applique l'algorithme suivant. Pour chacun des deux graphes et
  chaque sommet u, on calcule son "profile" un tableau noté
  profile[u]: profile[u][i+2] = le nombre de sommets à distance i de
  u, en commençant à partir de i=1. Donc, profile[u][3] est le degré
  de u. Ceci est calculé par un simple BFS, les indices 0,1,2 étant
  réservés. On utilise profile[u][0] pour coder la taille du tableau
  profile[u], profile[u][1] pour stocker le nom de u (plutôt que le
  nombre de sommet à distance 0 qui est toujours 1) et profile[u][2]
  pour stocker la longueur du plus petit cycle passant par u. Cette
  dernière information étant calculée "gratuitement" par le BFS.

  On ordonne ensuite les sommets des graphes suivant les profiles des
  sommets avec qsort(). On ne renumérote pas les sommets dans le
  graphe, mais plutôt on donne un ordre: c'est possible avec qsort()
  car le nom original u est dans profile[u][1] le sommet u. Si bien
  que profile[i][1] sera le sommet u de rang i. On priviligie les
  profiles le grande taille (que l'on classe en premier) ce qui est
  plus discriminant. Les isomorphismes (=permutations) à tester ne
  concernent que les sommets de même profile. On construit les
  contraintes dans un tableau C, C[j] indiquant que les sommets de
  rang C[j-1] à C[j] (exclu) ont même profile, et sur lequel se base
  NextPermutation().

  Sur le même principe, on pourrait imaginer un profile plus complexe,
  comme suit: à chaque distance i et sommet u, on calcule le graphe
  G[u][i] induit par les sommets à distance i de u. On peut alors
  calculer le profile de chaque sommet de G[u][i] et ordonner les
  sommets selon celui-ci.
 */
{
  H->int1=0; /* par défaut, 0 tests */
  if((G==NULL)||(H==NULL)) return NULL;
  if(G->n!=H->n) return NULL;

  int *P; /* isomorphisme final */
  int n=G->n;

  if(G==H){ /* isomorphisme trivial si même emplacement mémoire */
    ALLOCZ(P,n,int,_i);
    return P;
  }

  param_bfs *param=new_param_bfs(n); /* pour le BFS */
  int **profile,**profileG=NULL,**profileH;
  int *R,*C; /* permutation et contraintes (sur les rangs) */
  int u,v,t,r,i;
  graph *M;

  for(M=G;;){ /* on fait la même chose pour M=G puis M=H */
    ALLOC(profile,n,int*); /* profile[u] */
    for(u=0;u<n;u++){ /* faire un BFS pour tous les sommets u */
      bfs(M,u,param); /* le premier BFS va alouer param->D */
      t=3+param->rad; /* taille du tableau profile[u] */
      ALLOCZ(profile[u],t,int,0); /* initialise le profile */
      for(v=0;v<n;v++){
	i=param->D[v];
	if(i>0) profile[u][i+2]++; /* compte les sommets à distance i>0 de v */
	param->D[v]=-1; /* réinitialise les distances pour les BFS suivants */
      }
      profile[u][0]=t; /* taille du profile */
      profile[u][1]=u; /* nom du sommet, pour qsort() */
      profile[u][2]=param->girth; /* maille */
    }
    qsort(profile,n,sizeof(int*),fcmp_profile); /* trie les profiles */

    if(M==H){ profileH=profile; break; } /* on s'arête si M=H */
    profileG=profile; /* on refait la boucle pour H */
    M=H;
  }
  free_param_bfs(param);

  /* on verifie que profileG "=" profileH */
  for(u=0;u<n;u++)
    if(fcmp_profile(profileG+u,profileH+u)){
      P=NULL;
      goto fin_noniso;
    }

  /* calcule les contraintes */
  /* ici les profiles de G et H sont identiques */
  ALLOC(C,n,int);
  R=profile[0]; /* R=profile du premier sommet. NB: profile=profileH. */
  for(u=t=0;u<n;u++){
    if(fcmp_profile(&R,profile+u)){ /* si profiles différent */
      R=profile[u];
      C[t++]=u;
    }
  }
  C[t]=n;

  ALLOC(P,n,int);
  ALLOCZ(R,n,int,_i); /* initialise l'isomorphisme sur les rangs des profiles */
  if(!H->sort) SortGraph(H); /* on trie H pour la recherche dichotomique */
  H->int1=0; /* compte le nb de tests */

  /* vérifie, pour chaque permutation P, que P(G)=H */

  do{
    H->int1++;

    /* calcule P en fonction de R */
    for(r=0;r<n;r++) P[profileG[r][1]]=profileH[R[r]][1];
    for(r=0;r<n;r++){ /* on commence par les profiles longs: r=0,1,2, ... */
      u=profileG[r][1]; /* v=P[u] est le sommet correspondant à u dans H */
      for(i=0,t=G->d[u];i<t;i++) /* on regarde si chaque voisin de u est dans H->L[v] */
	if(bsearch(P+(G->L[u][i]),H->L[P[u]],t,sizeof(int),fcmp_int)==NULL){
	  /* alors élément non trouvé */
	  i=t;r=n; /* prochaine permutation à tester */
	}
    } /* ici r=n (trouvé) ou n+1 (pas encore trouvé) */
    if(r==n) goto fin_iso; /* on a trouvé un isomorphisme P */
  }
  while(!NextPermutation(R,n,C));

  /* si on arrive ici, c'est qu'on a pas trouvé l'isomorphisme */
  free(P);
  P=NULL;

 fin_iso:
  free(R);
  free(C);

 fin_noniso:
  FREE2(profileG,n);
  FREE2(profileH,n);

  return P;
}


edge *ListEdges(graph *G){
  /*
    Construit la liste des arêtes de G, chaque arête uv ne figure
    qu'une seule fois. On a, pour tout i, E[i].u < E[i].v. Le champs
    G->m est mis à jour.
  */
  edge *E;
  int u,v,i,j,d;
  int m=NbEdges(G);
  int n=G->n;

  ALLOC(E,m,edge);
  for(u=j=0;u<n;u++){
    for(i=0,d=G->d[u];i<d;i++){
      v=G->L[u][i];
      if(u<v){ E[j].u=u; E[j++].v=v; }
    }
  }
  return E;
}


graph *Subgraph(graph *G,graph *H){
  /*
    Détermine si G est un sous-graphe de H s'ils ont même nombre de
    sommets. On renvoie un sous-graphe S de H isomorphe à G, et on
    renvoie NULL si H ne possède pas G comme sous-graphe.

    Effets de bord:
    - les listes de G sont triées et donc G->sort=1,
    - H->int1 contient le nombre total de tests effectués,
    - S->pint1 contient l'isomorphisme de S vers G, et donc de H vers G.

    L'algorithme est le suivant: on teste d'abord si la séquence des
    degrés de H est bien supérieure à celle de G (ceci prend un temps
    O(n)). Si c'est le cas, on effectue, pour tous les sous-graphes S
    de H qui ont autant d'arêtes que G, un test d'isomorphisme entre S
    et G grâce à isomorphisme(S,G).
  */
  int n=H->n;
  H->int1=0; /* nb de tests = 0 par défaut */
  if(n!=G->n) return NULL; /* pas le même nombre de sommets */

  /* on trie en O(n) les deux listes */
  int *Eh=SortInt(H->d,NULL,n,0,NULL,SORT_INC);
  int *Eg=SortInt(G->d,NULL,n,0,NULL,SORT_INC);

  int i;
  /* on s'arrête si, pour un rang i donné, degH(i)<degG(i) */
  for(i=0;i<n;i++) if(Eh[i]<Eg[i]) break;
  free(Eh);
  free(Eg);
  if(i<n) return NULL; /* G ne peut pas être un sous-graphe de H */

  int mg=NbEdges(G);
  int mh=NbEdges(H);
  graph *S=new_graph(n); /* sous-graphe de H, alloue S->d et S->L */
  edge *E=ListEdges(H); /* liste des arêtes de H: e_j=(u,v) -> E[j].u=u et E[j].v=v */
  int *B; /* les arêtes de S, sous-ensemble d'indices d'arêtes de H */
  int *P; /* isomorphisme S->G */
  int u,v,j,d;

  /* réserve l'espace pour S, sous-graphe de H */
  for(i=0;i<n;i++) ALLOC(S->L[i],H->d[i],int);

  /* initialise le sous-ensemble d'arêtes B de H avec mg arêtes */
  ALLOC(B,mg,int);
  NextSet(B,-1,mg);
  d=0; /* d=compteur pour le nombre total de tests */

  do{

    /* remplit S avec les arêtes de B */
    for(u=0;u<n;u++) S->d[u]=0; /* position libre pour le sommet u de S */
    for(i=0;i<mg;i++){ j=B[i]; /* j=numéro de la i-ème arête de B */
      u=E[j].u; v=E[j].v; /* l'arête j de E est (u,v) */
      S->L[u][S->d[u]++]=v; /* ajoute v comme voisin de u et incrémente deg(u) */
      S->L[v][S->d[v]++]=u; /* ajoute u comme voisin de v et incrémente deg(v) */
    }
    
    /* il vaut mieux que G soit le 2e paramètre, car il va être trié
       la première fois par Isomorphism(), puis plus jamais grâce au
       test de G->sort, alors que S serait trié à chaque appel */
    P=Isomorphism(S,G);
    d += 1+G->int1; /* on ajoute 1 pour donner le nombre d'ensembles testés */
  } while((P==NULL)&&(!NextSet(B,mh,mg)));

  H->int1=d; /* nombre total de tests */

  if(P==NULL){ /* on a pas trouvé de sous-graphe de H isomorphe à G */
    free_graph(S);
    S=NULL;
  }
  else S->pint1=P; /* S isomorphe à G, sous-graphe de H */

  free(E);
  free(B);
  return S;
}


graph *MatrixToGraph(int **M,int n){
  /*
    Renvoie le graphe correspondant à la matrice d'adjacence n x n
    symétrique où seule la partie inférieure est utilisée. Les listes
    du graphe de retour sont triées et les champs ->m et ->sort sont
    mise à jour.
  */
  if((M==NULL)||(n<=0)) return NULL;
  int u,v,m;
  graph *G=new_fullgraph(n);

  for(u=m=0;u<n;u++)
    for(v=0;v<u;v++)
      if(M[u][v]){
	ADD_EDGE(G,u,v); /* ajoute u-v et v-u */
	m++; /* une arête de plus */
      }
  
  /* réduit les listes */
  GraphRealloc(G,G->d);

  G->m=m;
  G->sort=1;
  return G;
}


graph *GraphOfColor(graph *G,int *col,int k){
  /*
    Renvoie un graphe C dont les sommets sont les entiers de [0,k[
    (les valeurs du tableau col[] qui doit avoir une taille G->n) et
    les arêtes les paires uv telle qu'il existe une arête xy de G avec
    col[x]=u et col[y]=v. La valeur C->m est mise à jour, et les
    listes de C sont triées (C->sort=1).
  */
  if((k<0)||(col==NULL)||(G==NULL)) return NULL;

  int u,v,cu,cv,i,d,n=G->n;
  int **M; /* matrice d'adjacence du graphe des couleurs */
  graph *C; /* le graphe des couleurs renvoyé */

  /* matrice d'adjacence inférieure à 0 */
  ALLOCMAT(M,k,k-1,int);
  for(u=0;u<k;u++)
    for(v=0;v<u;M[u][v++]=0);

  for(u=0;u<n;u++) /* parcourt G et remplit la partie inférieure de M */
    for(i=0,d=G->d[u];i<d;i++){
      v=G->L[u][i];
      if(u<v){ /* si cu=cv, on ne fait rien */
	cu=col[u];
	cv=col[v];
	if(cu>cv) M[cu][cv]=1;
	if(cv>cu) M[cv][cu]=1;
      }
    }
  
  C=MatrixToGraph(M,k);
  FREE2(M,k);
  return C;
}


int FindSet(int x,int *p)
/* routine pour UNION-FIND pour Minor() */
{
  if(x!=p[x]) p[x]=FindSet(p[x],p);
  return p[x];
}


int *Minor(graph *H,graph *G)
/*
  Détermine si H est un mineur de G. Si c'est le cas, un tableau T est
  renvoyé, sinon NULL est renvoyé. Le tableau T code un modèle du
  mineur de H dans G. Plus précisément, T[u]=c, où u est un sommet de
  G, est le nom du sommet c de H si bien que l'ensemble des sommets u
  de G tel que T[u]=c forme le super-noeud c.

  L'algorithme est le suivant: on effectue, pour tous les ensembles de
  contractions d'arêtes de G produisant un mineur avec autant de
  sommets que H, un test de sous-graphe grâce à Subgraph().
*/
{
  graph *C; /* graphe des couleurs = graphe contracté */
  graph *S=NULL; /* sous-graphe éventuellement isomorphe à C */
  int *B; /* sous-ensemble (d'indices) d'arêtes de G à contracter */
  int *couleur; /* les sommets de même couleurs seront contractés */
  int *rang; /* pour union-find rapide */
  edge e;
  
  int nh=H->n;
  int ng=G->n;
  int c=ng-nh; /* c=nb de contractions à effectuer */
  int t;
  H->int1=t=0; /* initialise le nb de tests */
  if(c<0) return NULL; /* pas de mineur */

  edge *E=ListEdges(G); /* E=liste des arêtes de G, met à jour G->m */
  int mg=G->m;
  if(c>mg){ /* fini s'il faut contracter plus d'arêtes qu'il y a dans G */
    free(E);
    return NULL;
  }

  int i,u,v,x,y;
  int test=((c<<1)<ng); /* vrai ssi on liste suivant les arêtes ou suivant les sommets */
  ALLOC(B,c,int); NextSet(B,-1,c); /* B=premier sous-ensemble de c arêtes de E */
  ALLOC(couleur,ng,int); /* couleur des sommets */
  ALLOC(rang,ng,int); /* rang des sommets, sert pour UNION-FIND */

  /*
    On pourrait générer des sous-ensembles acycliques d'arêtes
    directement, en combinant NextSet() et le test d'acyclicité.
   */

  do{

    t++; /* on compte 1 test par ensemble B testés */

    /* initialise la couleur et le rang des sommets */
    for(u=0;u<ng;u++){ couleur[u]=u; rang[u]=0; }

    /* on teste rapidement (avec UNION-FIND) si B contient un cycle */
    for(i=0;i<c;i++){ e=E[B[i]]; /* e=i-ème arête de B */
      u=e.u; couleur[u]=x=FindSet(u,couleur);
      v=e.v; couleur[v]=y=FindSet(v,couleur);
      if(x==y) break; /* y'a un cycle, sinon on fait UNION */
      if(rang[x]>rang[y]) couleur[y]=x;
      else{ couleur[x]=y; if(rang[x]==rang[y]) rang[y]++; }
    }

    if(i==c){ /* si B est acyclique, on fait un test de sous-graphe */
      if(test)
	/* on met à jour la couleur de chaque sommet. Suivant les
	   valeurs respectives de c et ng (test) on met à jour soit
	   suivant les arêtes ou suivant les sommets. */
	for(i=0;i<c;i++){
	  e=E[B[i]]; /* e=i-ème arête de B */
	  u=e.u; couleur[u]=FindSet(u,couleur);
	  v=e.v; couleur[v]=FindSet(v,couleur);
	}
      else
	for(u=0;u<ng;u++) couleur[u]=FindSet(u,couleur);

      /* on recadre les couleurs dans [0,c[. Complexité: 4ng */
      for(i=0;i<ng;rang[i++]=0);
      for(i=0;i<ng;i++) rang[couleur[i]]++; /* rang=fréquence */
      for(i=u=0;i<ng;i++) /* repère les couleurs manquantes */ 
	if(rang[i]==0) u++; else rang[i]=u;
      /* ici rang[i]=nb de zéros (=valeurs manquantes) dans rang[0..i[ */
      for(i=0;i<ng;i++) couleur[i] -= rang[couleur[i]];

      C=GraphOfColor(G,couleur,nh);
      S=Subgraph(H,C); /* avant l'appel, S=NULL nécessairement */
      t += C->int1;
      free_graph(C); /* on a plus besoin du graphe des couleurs */
    }

  } while((S==NULL)&&(!NextSet(B,mg,c)));

  H->int1=t;
  free(B);
  free(E);

  /* on a rien trouvé */
  if(S==NULL){
    free(couleur);
    free(rang);
    return NULL;
  }
  
  /* on a trouvé un mineur, on construit le modèle dans rang[] */
  for(u=0;u<ng;u++) rang[u]=S->pint1[couleur[u]];
  free_graph(S);
  free(couleur);
  return rang;
}


int *InducedSubgraph(graph *H,graph *G)
/*
  Indique si H est un sous-graphe induit de G.  La fonction renvoie un
  ensemble X de sommets tel que G[X] est ismomorphe à H. Evidemment
  |X|=H->n. On renvoie dans G->int1 le nombre de tests, et dans
  G->pint1 l'isomorphisme entre H et G[X].

  L'algorithme consiste à générer tous les ensembles X possibles de
  |V(H)| sommets à tester l'isomorphisme entre G[X] et H.
 */
{
  if((G==NULL)||(H==NULL)) return NULL;
  int ng=G->n,nh=H->n;
  if(nh>ng) return NULL;

  graph *S;
  int *X,*P,t=0;
  ALLOC(X,nh,int);
  NextSet(X,-1,nh); /* premier sous-ensemble */

  do{
    t++;
    S=ExtractSubgraph(G,X,nh,1);
    P=Isomorphism(S,H);
    t += H->int1;
    free_graph(S);
  } while((P==NULL)&&(!NextSet(X,ng,nh)));

  G->int1=t;
  free(G->pint1); /* pour éviter les fuites mémoires */
  G->pint1=P;
  if(P==NULL){ free(X); X=NULL; }
  return X;
}


int NextPath(graph *G,path *P,int j)
/*
  Cette fonction (récursive) permet de trouver tous les chemins
  simples entre deux sommets. Si le code 1 est renvoyé, c'est que tout
  c'est bien passé, sinon on renvoie le code 0. On l'utilise comme
  ceci:

   path *P=new_path(G,NULL,G->n); // crée un chemin vide P mais avec G->n sommets possibles
   P->P[0]=x; // origine du chemin
   P->P[1]=y; // destination du chemin, y=x possible
   r=NextPath(G,P,-1); // crée le premier chemin, r=0 s'il n'existe pas, sinon r=1
   r=NextPath(G,P,0);  // calcule le chemin suivant, r=0 s'il n'existe pas, sinon r=1
   r=NextPath(G,P,0);  // calcule le chemin suivant, r=0 s'il n'existe pas, sinon r=1
   ...
   free_path(P); // libère le chemin P

  Plus précisément, étant donnés un chemin P=x-...-v_j-...-y du graphe
  G et un sommet v_j du chemin (v_j=j-ème sommet du chemin), la
  fonction tente de compléter P par un autre chemin de v_j à y évitant
  x-...-v_(j-1). Si ce chemin à été trouvé, alors la partie de v_j à y
  de P est mise à jour et on renvoie 1. Sinon, le chemin est coupé
  après v_j et on renvoie 0. Dans tous les cas P est un chemin à jour
  de G. Si j<0, alors on initialise P par un chemin allant de
  x=P->P[0] à y=P->P[1].

  Algorithme: on essaye d'améliorer en premier le sous-chemin de
  v_(j+1) à y (récursivement). Si cela n'est pas possible, on calcule
  un nouveau chemin de v_j à y passant par un autre voisin v de v_j
  (autre que v_(j+1)) et évitant le chemin x-...-v_j. On passe en
  revue ainsi tous les voisins v de v_j qui ne sont pas dans
  P[0]...P[j]. Si aucun des voisins n'a un tel chemin, on retourne 0.

  Comme il faut tester les voisins v de v_j qu'une seule fois (pour un
  chemin P[0]...P[j] donné), on utilise le champs aux[v_j][i] de P qui
  donne le i-ème voisin libre de v_j avec la convention que
  aux[v_j][0] est le nombre de voisins encore possibles.

  Effet de bord: P est mis à jour.
*/
{
  if((P==NULL)||(G==NULL)) Erreur(-1); /* ne devrait jamais arriver */
  param_bfs *p;
  int i,x,y,u,v,n;

  if(j<0){ /* initialisation du premier chemin */
    n=G->n;
    if(P->aux==NULL) ALLOCMAT(P->aux,n,n,int);
    for(u=0;u<n;P->V[u++]=-1); /* vide le chemin */
    x=P->P[0];
    y=P->P[1];
    if((x<0)||(y<0)||(x>=n)||(y>=n)){ P->n=0; p=NULL; goto fin_0; } /* sommets inexistant */
    p=bfs(G,x,NULL); /* calcule le chemin */
    i=p->D[y];
    if(i<0){
      P->V[x]=0; /* x est en première position dans P */
      P->n=1;
      goto fin_0;
    }
    /* on initialise aux[x] et aux[y] qu'une seule fois */
    P->aux[y][0]=0;
    j=-1; /* pour que la longueur soit correcte */
    goto fin_1;
  }

  n=P->n;

  if(j+1==n) return 0; /* si x=y, alors pas de prochain chemin */
  if(NextPath(G,P,j+1)) return 1; /* c'est le NextPath à partir du voisin de v_j */

  /* Ici on ne peut pas augmenter v_(j+1)...y. Donc, il faut calculer
     un premier chemin depuis le prochain voisin v disponible de u=v_j
     et y qui évite x=P[0]-...-P[j]. Si cela ne marche pas avec le
     voisin v, il faut essayer le suivant, etc. */

  /* efface depuis P la fin du chemin P[j+1]...P[n-1] */
  /* pour ne garder que P[0]...P[j] */
  for(i=j+1;i<n;i++) P->V[P->P[i]]=-1;

  /* rem: ici t<d */
  p=new_param_bfs(G->n);
  ALLOC(p->D,G->n,int);
  y=P->P[n-1];
  u=P->P[j];
  i=-1;

  while((P->aux[u][0])&&(i<0)){ /* tant qu'il y a encore des voisins de u non testés */
    v=P->aux[u][(P->aux[u][0])--]; /* lit et enlève le dernier voisin dispo */
    if(P->V[v]>=0) continue; /* si le voisin est dans P[0] ... P[j] on le saute */
  
    /* initialise p->D: on doit le faire à chaque appel */
    for(i=0;i<G->n;i++) p->D[i]=-1;
    /* on enlève P[0]...P[j] du graphe pour le BFS */
    for(i=0;i<=j;i++) p->D[P->P[i]]=-2;

    /* calcule un chemin de v à y dans G\(P[0]-...-P[j]) */
    bfs(G,v,p);

    /* a-t-on trouvé un chemin ? */
    i=p->D[y]; /* si i>=0, c'est oui */
  }

  /* ici i=distance de u=v_j à y */
  if(i<0){ /* on a pas trouvé de chemin */
  fin_0:
    free_param_bfs(p);
    return 0;
  }

  /* on a trouvé un chemin, on met à jour P */
 fin_1:
  P->n=i+j+2; /* nouvelle longueur */
  /* ajoute à P[0]...P[j] le chemin de P[j+1] à y en partant de y */
  while(i>=0){
    P->P[i+j+1]=y;
    P->V[y]=i+j+1;
    y=p->P[y];
    i--;
  }

  /* initialise aux[u] pour tous les sommets u de P[j+1] à y non
     compris. Pour P[j] on le fait au moment de la lecture de v dans
     aux[u], et pour y on le fait une seule fois à la création
     (avec aux[y][0]=0) */

  for(i=j+1,n=P->n-1;i<n;i++){ /* si j=-1, alors on fait pour x aussi */
    u=P->P[i];
    P->aux[u][0]=0;
    for(v=0;v<G->d[u];v++){
      j=G->L[u][v]; /* j=voisin de u */
      /* si pas dans P on l'ajoute comme voisin possible */
      if(P->V[j]<0) P->aux[u][++P->aux[u][0]]=j;
    }
  }

  free_param_bfs(p);
  return 1;
}


int NextPath2(graph *G,path *P,int j)
/*
  Comme NextPath(G,P,j) sauf que si le nouveau chemin P' renvoyé a le
  même ensemble de sommets sur les positions allant de j à n-1, on
  passe au chemin suivant. Donc on renvoie le prochain chemin de P
  (s'il existe) qui a un ensemble de sommets différents de P->P[j] à
  P->P[n-1].
*/
{
  if(j<0) return NextPath(G,P,j);
  if(P==NULL) Erreur(-1);

  int *T,i,n=P->n,m=n-j;
  ALLOC(T,m,int);
  for(i=j;i<n;i++) T[i-j]=P->P[i]; /* copie les n-j derniers sommets de P */

  if(!NextPath(G,P,j)){
    free(T);
    return 0;
  }

  if(n==P->n){
    for(i=0;i<m;i++)
      if(P->V[T[i]]==-1){ /* ici on a un chemin différent */
	free(T);
	return 1;
      } /* ici on a le même chemin qu'au départ */
    free(T);
    return NextPath2(G,P,j);
  }

  free(T); /* ici on a un chemin différent */
  return 1;
}


#define LCONF 9  /* nb de bits pour coder un sommet dans le graphe des conflits */
#define CONFMAX (1<<LCONF) /* = 2^LCONF = nb max de sommets du graphe des conflits */
#define CONFMASK (CONFMAX-1) /* = CONFMAX-1 = mask pour un sommet */
#define CONFC 2 /* constante pour modifier le code des noeuds à 0. Il faut CONFC>1 */
#define NCMAX 512 /* taille maximum de la liste de composantes maximales. NCMAX<CONFMAX */

/* ensemble de variables pour gérer le graphe des conflits */
typedef struct {
  int n;   /* nb de sommets du graphe d'origine, sert pour la règle 5 */
  int nbi; /* nb de noeuds avec code=-1 (indéterminée) */
  int nbzi;/* nb de noeuds de code 0 indépendants */
  int outmem; /* 1 ssi il y a eut un dépassement de CONFMAX ou NCMAX */
  int paire[CONFMAX]; /* 1er noeud de la paire xy */
  int path[CONFMAX];  /* 1er noeud du chemin P, ne sert que pour PrintConflit() */
  int code[CONFMAX];  /* code du noeud i. Valeurs possibles: -1,0,1 */
  int nbc[CONFMAX];   /* nb de noeuds de la paire i dont le code est <1 */
  int *comp[CONFMAX];  /* liste des noeuds de la composante de i */
  int *cmax[NCMAX+1]; /* liste des composantes maximales */
  int tmax[NCMAX+1]; /* taille des composantes maximales */
  int ncmax; /* taille de la liste cmax */
  int nodemax; /* noeud correspondant à la composante maximale de la liste cmax */
  int valmax; /* valeur (VRAI/FAUX) des composantes maximales */
  int tcomp[CONFMAX]; /* taille de comp[i] */
  int x[CONFMAX],y[CONFMAX]; /* paire du noeud i, ne sert que pour PrintConflit() */
  graph *G; /* graphe des conflits */
  /* Attention! les noeuds des listes d'adjacence de G peuvent être >
     CONFMAX et donc aussi > G->n. On s'en sert pour coder un type
     d'arête. Donc si u est un noeud de G et v=G->L[u][i], alors le
     i-ème voisin de u est (v&CONFMASK) et le type de l'arête uv est
     (v>>LCONF). */
} conflit;


void PrintConflit(conflit *c)
/*
  Affiche le graphe des conflits, et rien si c->G->n=0.
*/
{
  if(c->G->n==0) return;

  int u,v,i,t,d;
  char T1[]="|=><";
  char T2[]="-01*";
  char T3[]="_/.";

  // UTF-8:
  // char T3[]="━┛o";
  // \xe2\x94\x80: ─
  // \xe2\x94\x81: ━
  // \xe2\x94\x98: ┘
  // \xe2\x94\x99: ┙
  // \xe2\x94\x9a: ┚
  // \xe2\x94\x9b: ┛

  printf("code (x,y) [comp] node-path-paire: node[type]\n");
  for(u=0;u<c->G->n;u++){
    t=c->code[u];
    if(c->nbzi>0){ /* si on a des zéros indépendants */
      if(t==0) t=2; /* c'est un zéro indépendant */
      else if(t==CONFC) t=0; /* c'est un zéro marqué -> remet sa valeur d'origine */
    }
    printf(" %c ",T2[t+1]);
    if(c->paire[u]==u)
      printf("(%i,%i) [",c->x[u],c->y[u]);
    else printf("\t [");
    for(v=0;v<c->tcomp[u];v++){
      printf("%i",c->comp[u][v]);
      if((c->n>9)&&(v<c->tcomp[u]-1)) printf(",");
    }
    printf("]\t%s%i",(v<5)?"\t":"",u);
    if(u>c->path[u]) printf("%c \t:",T3[1]); else{
      printf("%c",T3[0]);
      if(u>c->paire[u]) printf("%c\t:",T3[1]); else{
	printf("%c%c\t:",T3[0],T3[2]);
      }
    }
    for(i=0,d=MIN(c->G->d[u],WIDTH);i<d;i++){
      v=c->G->L[u][i];
      t=(v>>LCONF);
      printf(" %i%c",v&CONFMASK,T1[t]);
    }
    if(c->G->d[u]>WIDTH) printf("...");
    printf("\n");
  }
  printf("#nodes in conflict graph: %i\n",c->G->n);
  printf("#heavy indep. components: %i\n",c->nbzi);
  printf("#unspecified values: %i\n",c->nbi);
  if(c->outmem){
    printf("!!! Out of Memory !!!\n");
    if(c->ncmax>=NCMAX)
      printf("#nodes in conflit graph exceeded (%i)\n",CONFMAX);
    else
      printf("#maximal components exceeded (%i)\n",NCMAX);
  }
  return;
}


void ps1_delxy(conflit *c,int w)
/*
  Supprime du graphe des conflits c la dernière paire créee, et met
  ainsi à jour c. Ici w est l'indice du premier noeud de la dernière
  paire dans le graphe des conflits. Il faut supprimer tous les noeuds
  u >= w et tous les arcs entre les noeuds u et v<w.

  La liste des composantes maximales de la dernière paire sera effacée
  à la fin de la paire courante (voir le code après nextxy:).
*/
{
  int u,i,v,j;

  for(u=w;u<c->G->n;u++){
    for(i=0;i<c->G->d[u];i++){ /* pour tous les arcs v->u */
      v=c->G->L[u][i]&CONFMASK; /* v est le i-ème voisin de u */
      if(v<w) /* seulement pour les noeuds v avant w */
	for(j=0;j<c->G->d[v];j++)
	  /* on cherche et supprime u de la liste de v */
	  if((c->G->L[v][j]&CONFMASK)==u)
	    c->G->L[v][j]=c->G->L[v][--(c->G->d[v])]; /* supprime u */
    }
    free(c->G->L[u]); /* supprime la liste de u */
    free(c->comp[u]); /* supprime la composante de u */
  }

  c->G->n=w; /* met à jour le nombre de noeuds du graphe des conflits */
  return;
}


int ps1_addmax(int *C,int t,conflit *c)
/*
  Ajoute une composante C (de taille t) à la liste des composantes
  maximales déjà rencontrées, et maintient cette propriété. Il faut
  que la liste (c->cmax) soit de taille suffisante pour accueillir une
  nouvelle composante. La fonction renvoie VRAI ssi la composante C a
  été ajoutée à la liste.

  Algorithme:
    1. Pour toute composante X de L (=la liste):
       1.1. Si C est inclue ou égale à X alors FIN
       1.2. Si C contient X alors supprimer X de L
    2. Ajouter C à L

  On remarque aussi qu'il n'est pas possible de rencontrer plusieurs
  fois la même composante C avec des valeurs différentes. Si cela
  arrive, c'est pour deux chemins différents, disons Q1 et Q2. Soit X
  les voisins de C qui sont à la fois dans Q1 et Q2.  X se décompose
  en segments, chacun étant un sous-chemin de Q1 inter Q2. Tout sommet
  voisin de C doit être dans X sinon C aurait un sommet de trop. Donc
  tout chemin de G contenant X tel que C est une composante de G\P,
  doit être parallèle à Q1 et Q2. En particulier Q1 et Q2 sont
  parallèles ... et alors [A FINIR] ? Bon, bah en fait c'est
  possible. Q1 peut se réduire à une arête xy et Q2 à xzy (un sommet
  de plus). Et avec Q1 c'est vrai, mais avec Q2 cela devient faux. On
  a des contre-exemple avec des sommets (z) de degré deux.
*/
{
  int **L=c->cmax,n=c->ncmax; /* L=liste et n=|L|*/
  int *T=c->tmax; /* T=taille de chaque composante */
  int i,r;

  /* on passe en revue chaque composante la liste L */
  for(i=0;i<n;i++){
    r=SetCmp(C,L[i],t,T[i]); /* compare C et L[i] */
    if(r&6) return 0; /* C est strictement contenu (&4) ou égale (&2) à L[i] */
    if((r&8)&&(n>0)){ /* L[i] est contenu (strictement) dans C et L non vide */
      free(L[i]); /* libère ce tableau de sommets précédemment aloué par un ps1_addmax() */
      /* si L[i] dernier élément de L (i=n-1), alors il n'y a plus rien à faire */
      n--;
      if(i<n){ /* si c->cmax[i] pas dernier élément, il faut déplacer le dernier */
	L[i]=L[n]; L[n]=NULL; /* pour éviter plus tard un double free. NB: i<>n */ 
	T[i]=T[n]; /* nombre de sommets dans L[i] */
	i--; /* pour recommencer avec ce nouveau L[i] */
      }
    }
  }

  /*ajoute C à la fin de L */
  ALLOCZ(L[n],t,int,C[_i]); /* aloue et copie C dans L[i] */
  T[n]=t; /* taille de C */
  c->ncmax=n+1; /* nouvelle taille de L */
  return 1; /* on a ajouté C à L */
}


int ps1_push(int x,int v,conflit *c)
/*
  Affecte la valeur v (=0 ou 1) dans le noeud x et propage, en
  appliquant les règles décrites ci-après, à tous ses voisins et
  récursivement à tout le graphe. Renvoie 1 si une contradiction
  apparaît, ou bien si tous les noeuds de la paire de x ont pour code
  1. Sinon, on renvoie 0 (terminaison standard).

  Attention! Il faut appliquer cette fonction que si la paire de x est
  complète. Donc il ne faut pas l'appliquer si on vient de créer le
  noeud x, car on n'est pas sûr de ne pas faire un "Out of Memory" un
  peu plus tard sur la même paire.

  Effet de bord: met à jour plusieurs champs de la variable c, mais ne
  modifie pas le graphe c->G.

  Règles pour l'arc x->y:

  x=noeud x
  y=noeud y
  v=valeur 0 ou 1 qui vient d'être écrite dans x
  t=type de l'arête: 0=(Tx|Ty), 1=(|Tx|=|Ty|), 2=(Tx<Ty), 3=(Tx>Ty)
  c=graphe des conflits

  (Attention! "Tx|Ty" signifie que les composantes Tx et Ty sont
  disjointes.)

  règle 1: si Tx|Ty (disjoint) et v=0, alors écrire 1 dans y
  règle 2: si Tx=Ty, alors écrire v dans y
  règle 3: si Tx<Ty et v=0, alors écrire 0 dans y
  règle 4: si Tx>Ty et v=1, alors écrire 1 dans y
  règle 5: si Tx|Ty, v=1 et |Tx|+|Ty|=n, alors écrire 0 dans y

  On applique également la "règle du dernier -1", à savoir si la paire
  de x, après avoir écrit v, possède exactement 1 seul noeud de valeur
  -1, alors on écrit 0 dans ce noeud.

  La "règle du max" et la "règle de l'influence des voisins" sont
  appliquées plus directement par la fonction ps1() lors de la
  création d'une paire. Elles n'ont pas lieu d'être lors de la
  propagation.
*/
{
  int i,d,y,t;

  if(c->code[x]==v) return 0; /* rien à faire */
  if(c->code[x]==1-v) return 1; /* contradiction */
  /* ici on a code[x]==-1 */
  c->code[x]=v; /* écrit la valeur v */
  c->nbi--; /* et une valeur indéterminée en moins ! */
  t=(c->nbc[c->paire[x]] -= v); /* diminue seulement si on a écrit 1 */
  if(t==0) return 1; /* la paire de x est bonne, elle contient que des 1 ! */

  /* applique les règles 1-5 à tous les arcs sortant de x */
  for(i=0,d=c->G->d[x];i<d;i++){ /* pour tous les voisins de x */
    y=c->G->L[x][i]; /* y=i-ème voisin de x */
    t=(y>>LCONF); /* t=type de l'arc x->y: 0,1,2,3 */ 
    y &= CONFMASK; /* y=numéro du noeud voisin de x */

    /* applique les règles */
    switch(t){
    case 0:
      if((v==0)&&(ps1_push(y,1,c))) return 1; /* règle 1 */
      if((v==1)&&(c->tcomp[x]+c->tcomp[y]==c->n)&&(ps1_push(y,0,c))) return 1; /* règle 5 */
      break;
    case 1: if(ps1_push(y,v,c)) return 1; break; /* règle 2 */
    case 2: if((v==0)&&(ps1_push(y,0,c))) return 1; break; /* règle 3 */
    case 3: if((v==1)&&(ps1_push(y,1,c))) return 1; break; /* règle 4 */
    }
  }

  /* règle du dernier -1 ? */
  if(c->nbc[c->paire[x]]==1){
    /* on cherche alors l'unique noeud x de la paire courante qui est
       de code < 1.  NB: ce noeud existe forcément */
    x=c->paire[x]; /* x=premier noeud de la paire de x */
    while(c->code[x]==1) x++; /* passe au suivant si le code est 1 */
    return ps1_push(x,0,c); /* écrit 0 dans le noeud x */
  }

  return 0;
}

/* pour le débugage de PS1() */
/* N=niveau de récursion, POS=numéro de ligne */
#define PRINTS do{				\
    int _i;					\
    printf("%03i:%02i  ",++POS,N);		\
    for(_i=0;_i<3*N;_i++) printf(" ");		\
  }while(0)
#define DEBUG(I) //if(1){ I }

int PS1(graph *G,path *P,int version)
/*
  P est un chemin d'un graphe G, et G\P est formé d'une seule
  composante connexe. Il faut G et P <> NULL (initialisés avec
  new_graph() et new_path()). Renvoie 1 si P peut "séparer" G (voir
  explications ci-dessous). Il y a une optimisation avec le graphe des
  conflits si version>0. On utilise cette fonction comme ceci:

  path *P=new_path(G,NULL,G->n); // P=chemin vide, sans sommet
  int r=PS1(G,P); // r=0 ou 1
  free_path(P);
  
  Effet de bord: G->int1 retourne le nombre de tests (nombre de
  paires, nombre de chemins testés, et nombre de passes dans le graphe
  des conflits). Les autres champs de G et de P ne sont pas modifiés.

  Le paramètre "version" indique la variante du test:
  - version=0: sans le graphe des conflits
  - version=1: avec le graphe des conflits
  - version=2: comme version=1 mais sans le graphe des conflits lors de la récursivité
  - version=3: comme version=1 mais avec l'écriture de valeurs dans le graphe des conflits

  Améliorations possibles:

  - Si G n'est pas biconnexe, alors on pourrait tester si toutes ses
    composantes biconnexes sont bien évaluée à vraie. Si P
    n'intersecte pas une composante biconnexe B de G, alors il faut
    évaluer PS1(B,{}).

  - G\P étant connexe, on pourrait déjà supprimer les sommets de P qui
    forment un segment de P contenant une extrémité de P et qui n'ont
    pas de voisin dans G\P. En particulier, si une des extrémités de P
    est de degré 1, on peut la supprimer.

  - Privilégier les paires de sommets qui ne sont pas adjacents (pour
    diminuer la taille les composantes et avoir des chemins plus
    longs). Plus généralement, on pourrait privilégier les paires de
    sommets les plus distants. Ceci dit, on ne gagne probablement pas
    grand chose, et une renumérotation aléatoire des sommets devrait
    suffir pour ne pas traité les paires de sommets voisins en
    priorité.

  - On pourrait tester des cas simples pour G: arbre (tester si m=n-1,
    on sait que G est connexe), clique (tester si m=n(n-1)/2: si n<=4
    sommets alors vraie, sinon faux). (Ces tests sont déjà
    implémentés). Plus dur: G est outerplanar. En fait, si G est un
    arbre de cycles (chaque composante connexe est un cycle ou un K4),
    alors le test est vrai. C'est donc vrai en particulier si
    m=n. Pour tester si G est un arbre de cycle, il suffit de faire un
    DFS, puis de vérifier que si (u,x) et (u,y) sont deux arêtes qui
    ne sont pas dans l'arbre du DFS, alors x et y ne sont pas ancêtres
    l'un de l'autre (??? Pourquoi ???).

  - Pour tous les chemins possibles testés récursivement pour G, ne
    tester effectivement que ceux qui correspondent à des
    sous-ensembles de sommets différents puisque le résultat sera le
    même (mêmes composantes connexes). Pour cela, il faut gérer une
    table pour mémoriser les sous-ensembles testés. Notons que si un
    chemin est induit alors cela ne sert à rien de le mémoriser, il ne
    pourra jamais être rencontré de nouveau. On pourrait coder un
    sous-ensemble de n sommets par un entier sur n bits (n < 32 ou 64
    donc). La recherche/insertion pourrait être une recherche dans un
    arbre binaire de recherche.

 EXPLICATIONS:

  Soit P et Q deux chemins d'un graphe G. On dit que Q est parallèle à
  P, noté Q//P, s'il n'y a pas d'arête entre P\Q et G\(Q u P).

  Quelques propriétés:

  - La relation // n'est pas symétrique.
  - Tout chemin est parallèle au chemin vide.
  - Tout chemin est parallèle à lui-même.
  - La relation // n'est pas transitive.

  Pour la dernière proposition, on peut vérifier en fait que si R//Q
  et Q//P, alors on a R//P sauf s'il existe une arête uv telle que u
  in (P inter Q)\R et v in Q\(R u P).

  Soit P et Q deux chemins de G. On dit que Q dérive de P, noté Q///P,
  s'il existe une suite P_0,P_1,...,P_n de chemins de G avec P_0=Q et
  P_n=P tels que P_{i-1}//P_i pour chaque i=1..n.

  Quelques propriétés:

  - Si Q//P, alors Q///P. En particulier, Q///{} puisque Q//{}.
  - On peut avoir R//Q//P, soit R///P, sans avoir R//P (cf. ci-dessus).
  - Si R///Q et Q///P, alors R///P.

  On dit que Q///P dans un graphe valué (G,w) si tous les chemins
  P_0,...,P_n (en particulier Q et P) sont des plus courts chemins
  selon w.

  Soit P un chemin de G et w une valuation de G. On définit le
  potentiel pour P selon w comme score(P,w) := max_C { w(C)*|V(G)|+|C|
  } où le maximum est pris sur toute composante connexe C de G\P.

  Lemme 1. Supposons que G\P a une seule composante connexe, et soit Q
  un chemin de G parallèle à P différent de P. Alors, pour chaque
  valuation w de G, soit P est un demi-séparateur de G ou bien
  score(Q,w) < score(P,w).

  Preuve. Supposons que P ne soit pas un demi-séparateur de G pour la
  valuation w. Soit C la composante de G\P, et posons n = |V(G)|. Par
  définition, score(P,w) = w(C)*n + |C|. On a w(C) > w(G)/2, et donc
  w(P) < w(G)/2. Il suit que w(P) < w(C). Soit C' une composante de
  G\Q telle que w(C')*n+|C'| = score(Q,w). Comme Q est parallèle à P,
  soit C' est contenue dans C soit C' est contenue dans P. En effet,
  si C' intersecte C et P, alors C' contient une arête uv avec u in C
  et v in P. Bien sûr uv not in Q. Cela contredit le fait qu'il existe
  pas d'arête entre P\Q et G\(QuP).

  Si C' est contenue dans P, alors w(C') <= w(P) < w(C). Il suit que
  w(C') <= w(C)-1, soit w(C')*n <= w(C)*n - n. Clairement |C'| < n +
  |C|. D'où w(C')*n+|C'| < w(C)*n+|C|, soit score(Q,w) < score(P,w).

  Si C' est contenue dans C, alors w(C') <= w(C) et |C'| < |C| car
  Q<>P. Il suit que w(C')*n + |C'| < w(C)*n + |C|, soit score(Q,w) <
  score(P,w).

  Dans les deux cas nous avons prouvé que score(Q,w) < score(P,w).
  QED

  Soit G un graphe et P un chemin de G tel que G\P est composé d'une
  seule composante connexe (en particulier G est connexe). On définit
  PS1(G,P) le prédicat qui est VRAI ssi pour toute pondération w de G
  telle que P est un plus court chemin il existe dans (G,w) un chemin
  demi-séparateur qui dérive de P.

  Lemme 2. G est dans PS1 ssi PS1(G,{}) = VRAI.

  Preuve. En effet, en réécrivant la définition de PS1(G,{}) on déduit
  que PS1(G,{}) est VRAI ssi pour toute pondération w de G il existe
  dans (G,w) un chemin demi-séparateur qui dérive du chemin vide (tout
  chemin dérive du chemin vide). Notons que c'est nécessairement un
  plus court chemin de (G,w). C'est précisemment la définition de la
  classe PS1. QED

  L'objectif est d'avoir un test noté ps1(G,P) qui implémente
  PS1(G,P), disons qu'il s'en approche. La propriété souhaitée est que
  si ps1(G,P) est VRAI, alors PS1(G,P) aussi. En particulier, si
  ps1(G,{}) est VRAI, alors G est dans PS1.

  Algorithme pour le test ps1(G,P):

  On renvoie VRAI s'il existe une paire x,y de sommets où y n'est pas
  dans P telle que tout chemin Q de x à y:
  1. Q est parallèle à P, et
  2. pour toute composante C de G\(QuP), ps1(CuQ,Q)=VRAI.

  Lemme 3. Si ps1(G,P)=VRAI, alors PS(G,P)=VRAI.

  Preuve [A FINIR]. Par induction sur le nombre de sommets hors de
  P. Soit C est l'unique composante G\P. Si |C|=0, alors P est un
  demi-séparateur de G et donc PS(G,P) est VRAI. Supposons le lemme
  vrai pour tout entier < |C|.

  On suppose donc que ps1(G,P) est VRAI. Soit x,y la paire de sommets
  telle que y n'est pas dans P et où tous les chemins de x à y sont
  parallèles à P. En particulier, pour chaque valuation w, où P est un
  plus court chemin, tout plus court chemin Q selon w entre x et y est
  parallèle à P. Comme Q est différent de P (à cause du choix de y),
  on peut appliquer le lemme 1, et donc soit P est un demi-séparateur,
  soit score(Q,w) < score(P,w). On peut supposer qu'on est pas dans le
  premier cas, c'est-à-dire que P n'est pas un demi-séparateur de G,
  puisque sinon PS(G,P)=VRAI et le lemme est prouvé.

  Si Q est un demi-séparateur pour G, alors PS(G,P) est VRAI puisque Q
  est parallèle à P. Supposons que Q n'est pas un demi-séparateur pour
  G, et soit C' la composante de G\Q telle que w(C')>w(G)/2.

  On peut appliquer l'induction sur (C'uQ,Q) car comme Q<>P,
  |C'|<|C|. Posons G'=C'uQ. D'après le test ps1(G,P), ps1(G',Q) est
  VRAI. Donc par induction PS(G',Q)=VRAI et G' contient un chemin
  demi-séparateur pour la valuation w, disons P', parallèle à
  Q. Montrons d'abord que dans G, P' est parallèle à P.

  ...

  w(C')<w(G)/2 ...

  QED

  Lemme 4. Si ps1(G,P)=VRAI, alors soit il existe une paire x,y de
  sommets de G telle que tout chemin de x à y contient P, ou bien il
  n'existe pas de sommet de P ayant trois voisins inclus dans un cycle
  de G\P. [PAS SÛR, A VÉRIFIER]

  Preuve. [A FINIR] Soit x,y une paire de sommets de G avec y pas dans
  P telle que tous les chemins de x à y soient parallèles à P. Soient
  Q un tel chemin. Supposons que Q ne contient pas P. ...  QED

  Dit autrement, si on n'a pas cette propriété, alors ps1(G,P)=FAUX et
  il est inutile de tester tous les chemins Q possibles.  Remarquons
  que si G est 2-connexe, alors il ne peut avoir de paire x,y de
  sommets (avec y pas dans P) où tout chemin de x et y contient P. A
  montrer: ps1(G,P)=VRAI ssi tout les composantes 2-connexes G' de G
  on a ps1(G',P inter G)=VRAI ...

  [A VOIR]

  - u := ps1(CuQ,Q)
  - ajoute (C,u) à la liste L des composantes maximales (voir ps1_addmax)
  - si u = FAUX, ajouter un nouveau noeud au graphe des conflits (GC)
  - recommencer avec le chemin Q suivant de mêmes extrémités xy (s'il existe)
  - s'il n'y a plus de tel chemin:

    On essaye d'appliquer les trois règles (max, influence, dernier):
    - la règle du max en tenant compte de la liste L.
      si |L|<>1, alors on ne peut pas appliquer la règle du max
      sinon, L={(C,u)}
        si u = VRAI, on peut supprimer la paire xy de GC
        sinon, alors on peut appliquer la règle du max
    - la règle d'influence des voisins
    - la règle du dernier -1

    Si l'application d'une de ces règles (avec ps1_push) produit une
    contradiction dans GC, alors on a trouvé une bonne paire xy, et on
    renvoie VRAI.  Sinon, s'il existe une autre paire xy on recommence
    avec cette pnouvelle paire.

  A la fin du traitement de toutes les paires:

  - soit il reste des indéterminées (-1), et il faut les éliminer. On
    les force d'abord à 0. S'il y a une contradiction (en faisant des
    ps1_push), on en déduit que la valeur doit être 1 (et on fait
    ps1_push). Sinon, on force la valeur à 1. Si y a une
    contradiction, on déduit que la valeur doit être 0 (et on fait
    ps1_push). Sinon, s'il y a une valeur initialement indéterminée
    qui passe à la même valeur pour le forcage à 0 et à 1, on élimine
    cette indéterminée (et on fait ps1_push). On fait ainsi pour
    chaque indéterminée. Chaque fois qu'une indéterminée est éliminée,
    on recommence la recherche sur l'ensemble des indéterminées (et
    pas seulement sur celles qui restent). Si on arrive ainsi à
    éliminer toutes les indéterminées on peut passer à la
    suite. Sinon, on ne peut pas faire grand chose à part essayer tous
    les système possibles ... Voir MINISAT+ (système pseudo booléens)
    et Sugar qui transforme du système linéaire ou CSP en SAT.

  - soit il n'y a plus d'interminées. Dans ce cas on peut déduire un
    système d'équations linéaire indépendantes où les inconnues sont
    les poids des sommets. Si le système n'a pas de solution, alors
    renvoyer VRAI. Sinon, la solution trouvée peut renseigner sur une
    valuation possible pour prouver que G est éventuellement pas dans
    PS1(G,P).

 */

{ 
  G->int1=1; /* compte les tests */

  DEBUG(
	N++;
	int u;int v;
	if(G==GF) N=POS=0;
	printf("\n");
	PRINTS;printf("version=%i G=%p n=%i ",version,G,G->n);PRINTT(P->P,P->n);
	PRINTS;printf("G=");
	for(u=0;u<G->n;u++){
	  printf("%i:[",u);
	  for(v=0;v<G->d[u];v++){
	    printf("%i",G->L[u][v]);
	    if(v<(G->d[u])-1) printf(" ");
	  } printf("] ");
	} printf("\n");
	);
  
  int n=G->n;

  /* Ici on élimine un certain nombre de cas faciles à tester.
     Attention ! vérifier que G est dans PS1 dans ces cas là ne suffit
     pas (ce qui revient à vérifier que PS1(G,{})=VRAI). Il faut être
     certain qu'on a en fait PS1(G,P)=VRAI. */

  if(n-(P->n)<3){
    DEBUG(PRINTS;printf("-> n-|P|<3, moins de 3 sommets hors P\n"););
    DEBUG(N--;);
    return 1;
  }
  /* Par hypothèse P est léger, donc la composante hors de P est
     lourde. Il suffit de prendre un chemin entre ces au plus deux
     sommets hors de P. */

  /* lors d'un appel récursif, le nombre d'arêtes G->m est déjà mis à
     jour car G provient alors du résultat de ExtractSubgraph(). Donc
     NbEdges(G) est calculé en temps constant dès le 2e appel. */

  if((!P->n)&&(n<6)&&(NbEdges(G)<10)){
    DEBUG(PRINTS;printf("-> |P|=0 et n<6 et pas K_5\n"););
    DEBUG(N--;);
    return 1;
  }

  /* ici on a au moins 3 sommets de G qui sont hors de P, et P est non
     vide. Alors, comme P contient au moins deux somemts, cela fait
     que G possède au moins 5 sommets. */

  if(NbEdges(G)==((n*(n-1))>>1)){
    DEBUG(PRINTS;printf("-> clique avec > 2 sommets hors P\n"););
    DEBUG(N--;);
    return 0;
  }
  /* Ici G est clique avec n>4 et n-|P|>2. Dans le cas où tous les
     poids sont à 1 (sommets et arêtes) alors, il n'existe aucun
     chemin Q parallèle P permettant de progresser. Notons qu'une
     clique G avec n sommets donne PS1(G,P)=VRAI s'il y a 0, 1 ou 2
     sommets dans G\P. */

  if(NbEdges(G)<=n){
    DEBUG(PRINTS;printf("-> m<=n\n"););
    DEBUG(N--;);
    return 1;
  }
  /* Dans ce cas, il s'agit d'un arbre avec un cycle. Soit C la
     composante lourde de G\P, et x une des extrémités de P. La
     composante C est connectée à P par au plus deux sommets, disons u
     et v (u=v possible) et u le plus près de x. Soit y le voisin de v
     dans C. On définit alors P' comme le plus court chemin de G
     allant de x à y. Il est facile de voir que P' longe P depuis x
     puis entre dans C soit par u soit par v. Dans les deux cas les
     sommets de C ne peuvent être connectés à aucun sommet de P\P',
     les deux seules arêtes connectant C à P étant détruite par P'. */

  /* ici G possède au moins 5 sommets et 6 arêtes. */

  int x,y,i,u,v,w,d;
  path *Q=new_path(G,NULL,n);
  path *R=new_path(G,NULL,n);
  param_dfs *p=new_param_dfs(n); /* p->C n'est pas alloué */
  int *T,*M;
  graph *C;

  ALLOC(p->C,n,int); /* pour le DFS avec sommets supprimés */
  ALLOC(T,n,int); /* pour la composante C de G */
  ALLOC(M,n,int); /* pour la compatiblité des chemins P et Q */

  conflit c; /* ensembles de variables pour gérér le graphe des conflits */
  int npaire,npath,goodxy;
  /* npaire = 1er noeud dans le graphe des conflits de la paire courante */
  /* npath = 1er noeud dans GC du chemin courant pour la paire courante */
  /* goodxy = 1 ssi la paire xy est bonne, ie. tous les chemins et comp. sont ok */

  if(version>0){
    c.n=n; /* nombre de sommets du graphe G */
    c.G=new_graph(CONFMAX); /* graphe des conflits, alloue G->d et G->L */
    c.G->n=c.nbi=c.nbzi=c.ncmax=c.outmem=0; /* c.G->n=nb de noeuds déjà crées */
  }

  /* pour toutes les paires de sommets x,y de G avec y pas dans P */

  for(x=0;x<n;x++)
    for(y=0;y<n;y++){

      if(P->V[y]>=0) continue; /* y ne doit pas être dans P */
      if((P->V[x]<0)&&(y<=x)) continue; /* si x et y pas dans P, ne tester que le cas x<y */

      /* ici on a soit:
	 1) x dans P et y pas dans P, ou bien
	 2) x et y pas dans P et x<y
      */

      (G->int1)++; /* +1 pour chaque paire testée */
      goodxy=1; /* par défaut on suppose la paire xy comme bonne */

      /* calcule le 1er chemin entre x et y */
      Q->P[0]=x; Q->P[1]=y; /* initialise les extrémités du chemin Q */
      if(!NextPath(G,Q,-1)) goto fin_ps1; /* fin si pas de chemin x->y, impossible si G connexe */
      if((version>0)&&(!c.outmem)){
	npaire=c.G->n; /* initialise le 1er noeud de la paire courante */
	c.nbc[npaire]=0; /* nombre de valeurs < 1 pour la paire courante */
	/* on efface la liste des composantes maximales, s'il y en avait */
	for(u=0;u<c.ncmax;u++) free(c.cmax[u]);
	c.ncmax=0;
      }

      DEBUG(PRINTS;printf("Paire (%i,%i) G=%p n=%i\n",x,y,G,n););

      do{ /* pour tous les chemins Q entre x et y */
	(G->int1)++; /* +1 pour chaque sommet testé */

	DEBUG(PRINTS;printf("Essai du chemin ");PRINTT(Q->P,Q->n););

	/* On vérifie que Q est parallèle avec P. Il faut qu'aucun
	   sommet de P\Q n'ait de voisin en dehors de P ou de Q. */

	for(i=0;i<P->n;i++){
	  if(Q->V[u=P->P[i]]>=0) continue; /* on est aussi dans Q */
	  /* ici on est dans P\Q */
	  for(v=0;v<G->d[u];v++){ /* vérifie chaque voisin v de u */
	    if(P->V[G->L[u][v]]>=0) continue; /* si v est dans P */
	    if(Q->V[G->L[u][v]]>=0) continue; /* si v est dans Q */
	    DEBUG(PRINTS;printf("-> chemin Q non parallèle à P\n",u););
	    goodxy=0; /* cette paire n'est pas bonne */
	    goto nextxy; /* aller à la prochaine paire */
	  }
	}

	/* ici Q est parallèle à P */

	DEBUG(PRINTS;printf("-> chemin Q parallèle à ");PRINTT(P->P,P->n););

	/* on vérifie que pour chaque composante C de G\Q,
	   PS1(CuQ,Q)=VRAI. Notons que les sommets de P\Q sont
	   léger. Il ne faut donc pas les considérer. Il faut donc
	   prendre les composantes de G\(QuP).*/

	/* on enlève de G les sommets de QuP pour calculer les
	   composantes de G\(QuP) */
	for(u=0;u<n;u++) /* initialise le dfs */
	  if((P->V[u]<0)&&(Q->V[u]<0)) p->C[u]=-1; else p->C[u]=-2;
	dfs(G,0,p); /* calcule les composantes de G\(QuP) */
	DEBUG(PRINTS;printf("#composantes dans G\\(QuP) à tester: %i\n",p->nc););
	/* si aucune composante inutile de tester récursivement ce
	   chemin, G\(QuP) est vide. On peut passer au prochain chemin */
	if(p->nc==0) goto nextQ;
	/* ici, il y a au moins une composante dans G\(QuP) */

	d=R->n=Q->n; /* d=nombre de sommets du chemin Q */
	for(u=0;u<d;u++) T[u]=Q->P[u]; /* T=liste des sommets de Q */
	if((version>0)&&(!c.outmem)) npath=c.G->n; /* initialise le chemin Q au noeud courant */

	DEBUG(PRINTS;printf("Q = ");PRINTT(T,d););

	/* pour chaque composante de G\Q */

	for(i=0;i<p->nc;i++){  /* T[d..v[=i-ème composante de G\(QuP) u Q */
	  for(u=0,v=d;u<n;u++) if(p->C[u]==i) T[v++]=u;
	  /* T[0..v[=sommets de Q u C */
	  /* T[0..d[=sommets de Q, T[d..v[=sommets de la composante */
	  /* NB: les sommets de T[d..v[ sont dans l'ordre croissant */

	  DEBUG(PRINTS;printf("C\\Q=CC(%i) = ",i);PRINTT(T+d,v-d););

	  C=ExtractSubgraph(G,T,v,1); /* crée C=G[T] avec v=|T| sommets */
	  /* Attention! Q n'est pas un chemin de C, mais de G. On crée
	     donc un nouveau chemin R dans C qui est équivalent à Q */
	  for(u=0;u<v;u++) R->V[u]=-1; /* Attention! boucle avec v=C->n sommets */
	  for(u=0;u<d;u++) R->P[u]=C->pint1[Q->P[u]]-1;
	  for(u=0;u<d;u++) R->V[R->P[u]]=u;

	  DEBUG(PRINTS;printf("Q = ");PRINTT(R->P,R->n););

	  /* appel récursif avec une nouvelle version 0 ou 1:
	     - si version=0: sans le graphe des conflits -> 0
	     - si version=1: avec le graphe des conflits -> 1
	     - si version=2: comme version=1 mais sans le graphe
	       des conflits lors de la récursivité -> 0
	     - si version=3: comme version=1 mais avec l'écriture
	       de valeurs dans le graphe des conflits -> 1
	  */
	  u=PS1(C,R,version%2);

	  DEBUG(PRINTS;printf("PS1(CuQ,Q)=%i\n",u););
	  DEBUG(if(c.outmem){PRINTS;printf("PROBLEME MEMOIRE !\n");});

	  G->int1 += C->int1; /* met à jour le nombre de tests (ajoute au moins 1) */
	  free_graph(C); /* libère C qui ne sert plus à rien */

	  /* à faire que si graphe des conflits et pas eut de problème mémoire */
	  if((version>0)&&(!c.outmem)){
	    
	    /* on vérifie si la même composante n'existe pas déjà dans
	       la paire de c.G->n. Si c'est le cas, on passe à la
	       prochaine composante. NB: la composante de c.G->n est
	       dans T[d...v[ et sa taille vaut v-d. */

	    for(w=npaire;w<c.G->n;w++)
	      if(SetCmp(c.comp[w],T+d,c.tcomp[w],v-d)&2)
		goto nextC; /* si même composante, alors prochaine composante */
	    /* ici w=c.G->n */
	    /* si u=0, w sera le nouveau noeud du graphe des conflits */

	    /* ajoute T[d..v[ à la liste des composantes maximales (c.cmax) */
	    if(ps1_addmax(T+d,v-d,&c)){ c.nodemax=w; c.valmax=u; }
	    /* On stocke dans c->nodemax le noeud de la dernière
            composante ajoutée à la liste des composantes maximales.
            Si à la fin du traitement de la paire xy la liste a
            exactement une seule composante, alors c->nodemax sera
            forcément le noeud de la dernière composante ajoutée et
            c->valmax sa valeur (VRAIE/FAUSSE). */
	    if(c.ncmax>=NCMAX){ /* dépassement du nb de composantes maximales */
	      c.outmem=1; /* Ouf of Memory */
	      /* la paire xy n'est pas complète. On l'enlève, sinon
	         l'analyse des conflits pourrait ne va pas être correcte */
	      ps1_delxy(&c,npaire); /* supprime la dernière paire créee */
	      goodxy=0; /* cette paire n'est pas bonne */
	      goto nextxy; /* change de paire */
	    }
	  }
	  /* ici outmem=0 */

	if(u) goto nextC; /* si u=VRAI, on a finit le traitement de la composante */
	goodxy=0; /* une composante n'est pas bonne pour la paire xy */
	if(version==0) goto nextxy; /* si pas graphe de conflits, alors changer de paire */

	/* ici u=FAUX et version>0 */
	/* on va donc essayer d'ajouter le noeud w au graphe des conflits */
	/* ici pas de problème de mémoire, donc on ajoute le noeud w */

	(c.G->n)++; /* un noeud de plus */
	if(c.G->n==CONFMAX){ /* Out of Memory */
	  c.outmem=1;
	  ps1_delxy(&c,npaire); /* supprime la dernière paire créee, qui n'est pas bonne */
	  goto nextxy; /* ici goodxy=0 */
	}
	/* ici v=|T|, d=|Q|, w=nouveau noeud à créer */
	
	/* Attention! ne pas utiliser ps1_push(w,...) juste après la
	   création de w, car on est pas certain que la paire de w
	   sera complète ... On ne peut faire ps1_push(w,...)
	   seulement après le while(NextPath...). Idem pour
	   l'application de la règle du max ! */

	/* initialise le nouveau noeud w */

	DEBUG(PRINTS;printf("Ajoût du noeud %i dans le graphe des conflits\n",w););

	c.x[w]=x; c.y[w]=y; /* mémorise la paire x,y (pour l'affichage) */
	c.path[w]=npath; c.paire[w]=npaire; /* w rattaché à npath et à npaire */
	c.code[w]=-1; c.nbi++; /* un noeud de plus à valeur = -1 */
	(c.nbc[npaire])++; /* un noeud de plus à valeur < 1 pour cette paire */
	c.G->d[w]=0; /* initialise le degré de w */
	ALLOC(c.G->L[w],CONFMAX,int); /* crée la liste des voisins de w */
	ALLOCZ(c.comp[w],v-d,int,T[d+(_i)]); /* crée & copie la composante de w */
	c.tcomp[w]=v-d; /* taille de la composante de w */
	
	/* y'a-t'il des arcs vers les noeuds précédant w ? */
	/* calcule les arêtes sortantes de w et de leurs types */
	
	for(u=0;u<w;u++){ /* pour chaque noeud < w, v=type de l'arc */
	  v=-1; /* sentinelle: si v<0, alors pas d'arête */
	  if(u>=npath) v=0; /* test rapide: si u>=napth, alors clique */
	  else{ /* sinon calcule l'intersection des composantes */
	    v=SetCmp(c.comp[u],c.comp[w],c.tcomp[u],c.tcomp[w]);
	    /* ici les valeurs possibles pour SetCmp: 0,1,3,5,9 car T1 et T2 != {} */
	    if(v==1) v=-2; /* si v=1, T1 intersecte T2, et donc pas d'arête u-w */
	    /* avant: v: 0=disjoint, 3=égalité, 5=T1 sub T2, 9=T2 sub T1 */
	    v >>= 1; v -= (v==4); /* rem: si avant v=-2, alors après v=-1 */
	    /* après: v: 0=disjoint, 1=égalité, 2=T1 sub T2, 3=T2 sub T1 */
	  } /* si v<0, alors pas d'arête */
	  if(v>=0){ /* ajoute les arcs u->w et w->u */
	    c.G->L[u][c.G->d[u]++]=(v<<LCONF)|w; /* u->w */
	    if(v>1) v ^= 1; /* si v=2, alors v->3, et si v=3, alors v->2 */
	    c.G->L[w][c.G->d[w]++]=(v<<LCONF)|u; /* w->u (asymétrie pour v=2,3) */
	  }
	} /* fin du for(u=...) */
	
	nextC:; /* prochaine composante (next i) */
	} /* fin du for(i=0...) */

      nextQ:; /* prochain chemin Q */
      } while(NextPath(G,Q,0));
      
      /* si ici goodxy=1, c'est que pour la paire xy, tous les chemins
	 entre xy sont parallèles à P et qu'on a jamais vue de
	 composante à FAUX. Dans ce cas on a trouvé une bonne paire et
	 on a fini. */

      if(goodxy) goto fin_ps1; /* termine l'algo. */
      if(version==0) goto nextxy; /* si pas graphe des conflits */
      /* ici version>0 et goodxy=0 */
      
      /* Ici on a examiné tous les chemins Q pour la paire xy et on
      crée au moins un noeud dans le graphe des conflits. Ici tous les
      noeuds crées ont comme valeur -1. Il reste à essayer d'appliquer
      trois règles:

	 1) la règle du max: on écrit 0 s'il existe une composante de
	 xy contenant toutes les autres (y compris celle évaluées
	 récursivement à VRAI). Pour cela il faut avoir exactement une
	 seule composante maximale dans c.cmax qui doit être à
	 FAUX. Si elle est à VRAI il faut supprimer la paire xy (car
	 la plus lourde est VRAI, donc toutes les autres sont légères
	 !) et passer à la suivante.

	 2) la règle d'influence des voisins: vérifier si des voisins
	 v de chaque noeud u (de la paire xy) ne peuvent pas
	 influencer u. Par exemple, si la composante de v est la même
	 que celle de u il faut alors écrire cette valeur dans le code
	 de u.

	 3) la règle du dernier -1: si le nb de valeurs < 1 est
	 exactement 1, alors il faut écrire 0 dans ce noeud là. Il
	 faut, comme pour la règle d'influence des voisins, balayer
	 tous les sommets u de la paire xy.

	 NB: on peut traiter dans un même parcours les deux dernières
	 règles.
      */

      v=c.paire[c.G->n-1]; /* v=1er noeud de la dernière paire */
      /* NB: c.outmem=0 et c.G->n > 0 */

      /* règle du max */
      if(c.ncmax==1){ /* sinon pas de règle du max */
	if(c.valmax){ /* supprime la paire xy */
	  ps1_delxy(&c,v);
	  goto nextxy;
	}
	/* applique la règle du max: on écrit 0 dans la composante maximale */
	if(ps1_push(c.nodemax,0,&c)){ goodxy=1; goto fin_ps1; } /* fin si contradiction */
      }

      /* règle d'influence des voisins + règle du dernier -1 */
      for(u=v;u<c.G->n;u++){ /* pour tout noeud u de la dernière paire */
	if(c.code[u]>=0) continue; /* plus rien à faire */
	/* NB: si code[u]=0 ou 1, c'est qu'on a forcément déjà fait
	   un ps1_push() sur u. Et donc la règle d'influence des
	   voisins n'a pas a être testée sur u */
	else /* code=-1 */
	  if(c.nbc[c.paire[v]]==1){ /* règle du dernier -1 */
	    /* tous les sommets sont à 1, sauf u qui est ici a -1 */
	    if(ps1_push(u,0,&c)){ goodxy=1; goto fin_ps1; } /* fin si contradiction */
	    else break; /* on peut sortir du for(u...), on a tout testé */
	  }
	
	for(i=0,d=c.G->d[u];i<d;i++){ /* scan tous les voisins v de u */
	  v=c.G->L[u][i]&CONFMASK; /* v=i-ème voisin de u */
	  w=c.code[v]; if(w<0) continue; /* on ne déduit rien */
	  /* efface le code de v pour pouvoir le réécrire avec
	     ps1_push().  Il faut prendre des précaution pour que c
	     soit cohérent (il faut mettre à jour .nbi et .nbc). NB:
	     l'ancien code de v est w=0 ou 1, celui de u est aussi 0 ou 1 */
	  c.code[v]=-1;
	  c.nbi++; /* met à jour le nombre d'indéterminées */
	  c.nbc[c.paire[v]] += w; /* augmente s'il y avait 1 */
	  /* réécrit le code pour v */
	  if(ps1_push(v,w,&c)){ goodxy=1; goto fin_ps1; } /* fin si contradiction */
	  /* NB: la propagation de cette écriture va tester l'arc
	     v->u et donc modifier éventuellement le code de u */
	}
      }

    nextxy:;
    } /* fin du for(x,y=...) */
  
  /* Ici on a testé toutes paires xy et aucune n'est bonne */
  /* NB: goodxy=0 */

  if((version>0)&&(!c.outmem)){

    /* Traitement final du graphe des conflits. On pourrait ici
       essayer d'éliminer les derniers -1. Seulement, il semble
       qu'aucune des règles ne peut plus être appliquées. */

  loop_ps1:
    do{
      if(c.nbi==0) break; /* fini: il ne reste plus de -1 à modifier */
      x=c.nbi; /* mémorise le nb total de valeurs à -1 */
      //
      // que faire ? essayer toutes les valeurs possibles pour les -1 ?
      //
      // G->int1++;
    }
    while(c.nbi<x); /* tant qu'on a enlevé au moins un -1, on recommence */
    
    /* ps1x */
    if(version==3){ /* NB: si appel récursif, alors version!=3 */
      i=MEM(CPARAM,0,int);
      if(i>0){ /* lit les valeurs à l'envers */
	u=MEM(CPARAM,(2*i-1)*sizeof(int),int);
	v=MEM(CPARAM,(2*i)*sizeof(int),int);
	MEM(CPARAM,0,int)--;
	if(ps1_push(u,v,&c)){ goodxy=1; goto fin_ps1; } /* fin si contradiction */
      goto loop_ps1;
      }
    }
    
    /*
      on cherche les noeuds "0" indépendants, correspondant à des
      composantes lourdes (donc à des inégalités).
      
      Algorithme: on ne sélectionne que les noeuds de code=0 et qui
      n'ont pas de voisins v de code aussi 0 avec v<u (inclus). Ils
      doivent donc être de code 0 et minimal par rapport aux autres
      voisins de code 0. Si un tel noeud existe, on marque tous ces
      voisins de code 0 qui ne pourront plus être sélectionnés.
    */

    c.nbzi=0; /* =nb de zéros indépendants */
    for(u=0;u<c.G->n;u++){ /* pour chaque noeud du graphe des conflits */
      if(c.code[u]) continue; /* saute le noeud si pas 0 */
      for(i=0,d=c.G->d[u];i<d;i++){ /* scan tous les voisins u (qui est forcément à 0) */
	v=c.G->L[u][i]; w=c.code[v&CONFMASK];
	if(!((w==0)||(w==CONFC))) continue; /* on ne regarde que les voisins de code=0 */
	if((v>>LCONF)==3) break; /* ici i<d */
      }
      if(i==d){ /* on a trouvé un noeud u de code 0 et sans aucun arc u->v avec v<u */
	c.nbzi++; /* un de plus */
	for(i=0,d=c.G->d[u];i<d;i++){ /* modifie le code de tous les voisins à 0 de u */
	  v=c.G->L[u][i]&CONFMASK; /* v=voisin de u */
	  if(c.code[v]==0) c.code[v]=CONFC; /* on ne modifie pas deux fois un voisin */
	}
      }else c.code[u]=CONFC; /* u n'est pas bon, on ne laisse pas la valeur 0 */
    }

  }
  /* ici on a fini le traitement du graphe des conflits */

 fin_ps1: /* termine l'algo avec goodxy=0 ou 1 */

  if(version>0){ /* affichage et libération du graphe des conflits */
    if((!goodxy)&&(G==GF)) PrintConflit(&c); /* G=GF => affiche que le 1er niveau de récurrence */
    /* efface la liste des composantes maximales */
    for(u=0;u<c.ncmax;u++) free(c.cmax[u]);
    /* efface le graphe des conflits */
    for(u=0;u<c.G->n;u++) free(c.comp[u]);
    free_graph(c.G); /* c.G->L après c.G->n n'ont pas été alloués ou sont déjà libérés */
  }

  /* efface les tableaux aloués */
  free_path(Q);
  free_path(R);
  free_param_dfs(p);
  free(T);
  free(M);

  DEBUG(N--;);
  return goodxy;
}


void RemoveVertex(graph *G,int u)
/*
  Supprime toutes les occurences du sommet u dans le graphe G, sans
  renuméroter les sommets et sans réalouées les listes. Après cet
  appel, les listes de G ne sont plus forcément triées, mais la liste
  de G->L[u] existe toujours. Simplement G->d[u] est à zéro.

  Effet de bord: modifie G->sort, G->d[.], G->L[.]. Attention !  G->n
  n'est pas modifié.
*/
{
  int i,j,v,du=G->d[u],dv;
  G->d[u]=0;
  for(i=0;i<du;i++){
    v=G->L[u][i];
    dv=G->d[v];
    for(j=0;j<dv;j++)
      if(G->L[v][j]==u)	G->L[v][j]=G->L[v][--dv];
    G->d[v]=dv;
  }
  G->sort=0;
  return;
}


int AdjGraph(graph *G,int u,int v)
/*
  Return 1 ssi v apparaît dans la liste de u.
*/
{
  /* recherche dichotomique si le graphe est trié */
  if(G->sort)
    return (bsearch(&v,G->L[u],LOAD->d[u],sizeof(int),fcmp_int)!=NULL);

  /* sinon, recherche linéaire */
  int i;
  for(i=G->d[u];i>0;)
    if(G->L[u][--i]==v) return 1; /* on a trouvé v dans la liste de u */
  return 0;
}


int Treewidth(graph *H,int code)
/*
  Calcul approché ou excate de la treewidth de H. Si code=0, on
  calcule une borne supérieure (heuristique degmin). Si code=1 on
  calcule la valeur exacte. H n'est pas modifié. Dans H->int1 on
  renvoie le nb de tests effectués.
*/
{
  if(H==NULL) return 0;
  int *P,*D;
  int n,j,t,tw,l,v,n1;
  int d,r,i,u,k;
  int d0,r0,i0,u0;
  graph *G;

  H->int1=0;
  n=H->n;
  n1=n-1;
  tw=(code==1)? Treewidth(H,0) : MIN(n1,NbEdges(H));

  /* tw=upper bound sur tw. On a intérêt d'avoir la valeur la plus
     faible possible, car on va alors éliminer les mauvais ordres
     d'éliminations plus rapidement.

    tw max en fonction du nb m d'arêtes:
    m=0 => tw=0
    m=1 => tw=1
    m=2 => tw=1
    m=3 => tw<=2
    m=4 => tw<=2
    m=5 => tw<=2?
    m=6 => tw<=3
    ...
   */

  ALLOCZ(P,n,int,_i); /* permutation initiales des sommets */
  ALLOCZ(D,n,int,n1);

  do{
    G=ExtractSubgraph(H,NULL,0,0); /* on copie H */
    GraphRealloc(G,D); /* plonge G dans le graphe complet */
    k=0; /* tw par défaut si G sans d'arêtes */

    for(j=0;j<n1;j++){ /* n-1 fois */

      H->int1++;
      u0=P[i0=j];
      d0=r0=G->d[u0];
      if(code==0){
	for(i=j;i<n1;i++){
	  u=P[i]; d=G->d[u];
	  if(d<=d0){
	    /* calcule r=nb d'arêtes dans N(u) */
	    for(r=l=0;l<d;l++)
	      for(t=l+1;t<d;t++)
		r += AdjGraph(G,G->L[u][l],G->L[u][t]);
	    if((d<d0)||(r<r0)){ d0=d;r0=r;i0=i;u0=u; }
	  }
	} /* fin for(i=...) */
	P[i0]=P[j]; /* décale P[i], que si code=0 */
      }
      k=MAX(k,d0); /* met à jour tw */
      if(k>=tw) goto nextP; /* on ne fera pas moins */
      RemoveVertex(G,u0); /* supprime u */
      /* remplace N(u) par une clique */
      for(i=0;i<d0;i++){
	u=G->L[u0][i];
	for(t=i+1;t<d0;t++){
	  v=G->L[u0][t];
	  if(!AdjGraph(G,u,v)) ADD_EDGE(G,u,v);
	}
      }
      
    } /* fin for(j=...) */

    tw=MIN(tw,k);
  nextP:
    free_graph(G);
    if((code==0)||(tw<2)) break; /* si tw(G)=0 ou 1, alors on ne
				    trouvera pas plus petit */
  } while(!NextPermutation(P,n,NULL));

  free(D);
  free(P);
  return tw;
}


int *Prune(graph *G,int *z)
/*
  Algorithme permettant de supprimer les sommets du graphe G par
  dégrés minimum. Un ordre d'élémination des sommets est renvoyé sous
  la forme d'un tableau de taille n. Plus précisément, on renvoie un
  tableau de sommets T de sorte que u=T[i] est un sommet de degré
  minimum du graphe G\(T[0]...T[i-1]). La complexité est linéaire,
  i.e. O(n+m). Si *z<>NULL, on retourne dans z le degré maximum
  rencontré donc la dégénérescence du graphe.
*/
{
  int i,j,u,v,d,k,p,r,t,c,n1,n=G->n;
  int *T,*R,*D,*P;

  /*
    1. On construit les tableaux suivants:

    T[0..n[ = tableau de sommets triés par degré croissant (le résultat)
    R[0..n[ = tableau de positions dans T des sommets
    D[0..n[ = tableau des degrés des sommets
    P[0..n[ = tableau des positions dans T du premier sommet de degré donné

    2. On traite les sommets dans l'ordre T[0],T[1],... Supposons que
    u=T[i]. Alors, pour chaque voisin v dans G, encore existant (donc
    situé après i dans le tableau T), on met à jour la structure
    T,R,D,P.
   */

  /* initialise T,R,D,P */
  if(n<1) return NULL;
  ALLOCZ(D,n,int,G->d[_i]);
  R=SortInt(D,NULL,n,0,NULL,SORT_INDEXe);
  ALLOC(T,n,int); for(u=0;u<n;u++) T[R[u]]=u;
  ALLOCZ(P,n,int,-1);
  for(i=n1=n-1;i>=0;i--) P[D[T[i]]]=i; /* i=n-1,...,0 */

  for(c=i=0;i<n1;i++){ /* pour chaque sommet, pas besoin de faire le dernier */
    u=T[i]; /* u = sommet de degré (D[u]) minimum */
    if(D[u]>c) c=D[u]; /* mémorise le plus grand degré rencontré */
    d=G->d[u]; /* d = deg(u) */
    for(j=0;j<d;j++){ /* pour chaque voisin */
      v=G->L[u][j]; /* v=voisin de u */
      r=R[v]; /* v=T[r]; */
      if(r>i){ /* mettre à jour que si v existe encore */
	k=D[v]--; /* le degré de v diminue, k=D[v] juste avant */
	p=P[k--]++; /* on va échanger v et T[p]. Les sommets de degré k commencent juste après */
	if(P[k]<0) P[k]=p; /* y'a un sommet de degré k en plus */
	if(p>i){ /* si p est avant i, ne rien faire. Cel arrive que si k=0 */
	  t=T[p]; /* t = 1er sommet de de degré k */
	  T[p]=v; /* dans T, on avance v en position p (à la place de t donc) */
	  T[r]=t; /* on met t à la place de v */
	  R[v]=p; /* on met à jour la nouvelle position de v dans T */
	  R[t]=r; /* on met à jour la nouvelle position de t dans T */
	}
      }
    }
  }

  if(z!=NULL) *z=c; /* retourne la dégénérescence */
  free(R);
  free(D);
  free(P);
  return T;
}


int *GreedyColor(graph *G,int *R)
/*
  Algorithme permettant de colorier de manière gloutonne un graphe G à
  n sommets. La complexité en temps est linéaire, i.e. O(n+m). Il faut
  G non NULL. On renvoie un tableau C[0..n[ où C[u] est la couleur du
  sommet u, un entier entre 0 et n-1. Le tableau R[0..n[ donne un
  ordre (inverse) de visite des sommets: On colorie le sommet u avec
  la plus petite couleur possible dans le graphe induit par les
  sommets situé après u dans R (on commence donc avec R[n-1]). Si
  R=NULL, alors l'ordre R[i]=i est utilisé. On renvoie dans G->int1 la
  couleur maximum utilisée (donc le nb de couleurs-1). Cette valeur
  vaut -1 si n<1.  */
{
  int i,j,u,d,l,c,n=G->n;
  int *C,*M,*L;

  /*
    On utilise 3 tableaux:

    C[0..n[ = tableau final des couleurs, au départ =-1
    L[0..n-1[ = liste de couleurs utilisées par le sommet courant
    M[0..n-1[, M[i]=1 ssi la couleur i est utilisée par un voisin du
    sommet courant, au départ =0. On met une sentiennelle à M si bien
    que toujours on M[n-1]=0.

  */

  G->int1=-1;
  if(n<1) return NULL;
  ALLOCZ(C,n,int,-1);
  ALLOCZ(M,n,int,0);
  i=n-1;
  ALLOC(L,i,int);

  for(;i>=0;i--){ /* en partant de la fin */
    u=(R==NULL)? i : R[i];
    d=G->d[u]; /* d = deg(u) */

    /* on liste dans L et M les couleurs rencontrées */
    for(j=l=0;j<d;j++){ /* pour chaque voisin v de u */
      c=C[G->L[u][j]]; /* c=couleur du voisin v */
      if(c>=0){ /* voisin déjà colorié ? */
	L[l++]=c; /* on ajoute la couleur c à la liste */
	M[c]=1; /* la couleur c n'est pas à prendre */
      }
    }
    
    /* on cherche la 1ère couleur à 1 (=non-rencontrée) */
    j=0; while(M[j]) j++; /* s'arrête toujours à cause de la sentiennelle */
    C[u]=j; /* j est la couleur recherchée pour u */
    G->int1=MAX(G->int1,j); /* couleur maximum rencontrée */

    /* il faut ré-initialiser rapidement M */
    for(j=0;j<l;j++) M[L[j]]=0; /* on efface les 1 qu'on à mis dans M */
  }
  
  free(L);
  free(M);
  return C;
}


void HalfGraph(graph *G){
/*
  Modifie un graphe symétrique et le transforme en graphe asymétrique
  en ne gardant que les arcs u->v tels que v<u. En particulier les
  boucles sont supprimés. Les listes d'adjacence sont aussi
  triées. Cela permet de faire des parcours de graphe deux fois plus
  rapidement, par exemple, pour vérifier qu'une coloration est propre.
*/
  if(G==NULL) return;
  if(!G->sym) return;

  int *D,n=G->n,i,u,d;
  ALLOC(D,n,int); /* tableau des nouveaux degrés */

  SortGraph(G);
  for(u=0;u<n;u++){
    d=G->d[u];
    for(i=0;i<d;i++)
      if(G->L[u][i]>=u) break;
    D[u]=i; /* coupe la liste à i */
  }
  GraphRealloc(G,D); /* modifie toutes les listes d'adjacence */
  free(D);
  return;
}

void kColorSat(graph *G,int k){
/*
  Affiche les contraintes SAT pour la k-coloration de G au format
  Dimacs. Attention ! G est modifié (manque la moitié de ses arêtes).

  Le nombre de variables est n*k.
  Le nombre de clause est n+m*k.
*/

  if(G==NULL) return;
  int n,c,u,i,d,m;
  
  n=G->n;
  m=NbEdges(G);
  printf("p cnf %i %i\n",n*k,n+m*k);

  /*
    Variables: chaque sommet i a k variables numérotés x(i,c) avec
    i=0..n-1 et c=0..k-1. Il faut x(i,c)>0. On pose x(i,c)=1+k*i+c.

    Contraintes sommets: il faut x(i,0) v ... v x(i,k-1), et la
    conjonction pour tous les sommets.

    Containtes arêtes: pour chaque arête {i,j} et couleur c il ne faut
    pas que x(i,c)=x(j,c). Autrement dit il faut -x(i,c) v -x(j,c). Il
    faut la conjonction de toutes les couleurs c et toutes les arêtes
    {i,j}.
  */

  /* liste chaque sommet */
  for(u=0;u<n;u++){
    for(c=0;c<k;c++) printf("%i ",1+u*k+c);
    printf("0\n");
  }

  HalfGraph(G); /* enlève la moitié des arêtes */

  /* liste chaque arête */
  for(u=0;u<n;u++){
    d=G->d[u];
    for(i=0;i<d;i++)
      for(c=0;c<k;c++)
	printf("-%i -%i 0\n",1+u*k+c,1+G->L[u][i]*k+c);
  }
  
  return;
}


int *kColor(graph *G,int k)
/*
  Algorithme permettant de colorier en au plus k couleurs un graphe
  G. La complexité est évidemment exponentielle, c'est au moins du
  k^{n-1}.  On renvoie un tableau C[0..n[ où C[u] est la couleur du
  sommet u, un entier entre 0 et n-1. On renvoie NULL si n'est pas
  possible de colorier G en k couleurs, si G=NULL ou k<1. On renvoie
  dans G->int1 la couleur maximum utilisée. On symétrise le graphe
  (qui est donc modifié) afin d'enlever la moitié des arêtes à
  vérifier.

  On teste toujours en priorité la dernière arête qui a posé problème
  avant de vérifier tout le graphe.

  Optimisation possible:
  
  1. Réduction de données. On peut supprimer récursivement les sommets
  de degré < k. On pourra toujours les ajouter à la fin si la
  coloration à réussi. On peut aussi décomposer le graphe en
  composantes connexes.

  2. On peut numéroter les sommets selon un parcours BFS. De cette
  façon la vérification pourrait être accélérée lorsqu'on change une
  seule couleur.

*/
{
  int n,c,i,d,u,v,b,*C,*T;
  if((G==NULL)||(k<1)) return NULL;
  HalfGraph(G); /* enlève la moitié des arêtes */
  n=G->n;

  ALLOCZ(C,n,int,0); /* C[u]=couleur du sommet u */
  C[n-1]=k-1; /* on peut supposer qu'un sommet a une couleur fixée */
  if(n<2) goto fin_kcolor; /* s'il y a un seul sommet */
  b=1; /* b=vrai au départ */

  do{
    /* vérifie si C est une coloration propre */
    if(b){ /* on le fait que si b est vrai */
      for(u=0;u<n;u++){ /* pour chaque sommet */
	c=C[u]; d=G->d[u]; /* degré et couleur de u */
	for(i=0;i<d;i++) /* pour chaque voisin de i */
	  if(c==C[G->L[u][i]]) break; /* coloration pas propre */
	if(i<d) break; /* coloration pas propre */
      }
      if(u==n) goto fin_kcolor; /* la coloration est propre */
    }
    /* ici l'arête (u,i) n'est pas correctement colorié */
    
    /* on change C en incrémentant le compteur */
    v=0;
  loop_kcolor:
    C[v]++; 
    if(C[v]==k){
      C[v++]=0;
      if(v==n) break;
      goto loop_kcolor;
    }
    
    /* est-ce que l'arête (u,i) est toujours mal coloriée ?
       si oui, pas la peine de vérifier tout le graphe */
    b=(C[u]!=C[G->L[u][i]]); /* b=vrai si (u,i) est propre */

  }while(v<n);

  /* aucune coloration trouvée */
  free(C);
  return NULL;

 fin_kcolor:
  /* on a trouvé une coloration propre */
  /* on réduit l'espace de couleurs pour ne pas avoir de trouvé dans
     l'espace de couleur. NB: on a toujours C[n-1]=k-1 */

  ALLOCZ(T,k,int,-1);
  /* si T[c]=i, alors la couleur c se renumérote en i. Si c n'a jamais
     été rencontrée, alors i=-1 */

  for(i=u=0;u<n;u++){
    if(T[C[u]]<0) T[C[u]]=i++; /* la couleur C[u] n'a pas jamais été vue */ 
    C[u]=T[C[u]]; /* ici la couleur C[u] a déjà été vue */
  }

  free(T); /* plus besoin de T */
  G->int1=i-1; /* couleur max utilisé */
  return C;
}


/***********************************

           GRAPHES DE BASE

***********************************/


/*
  Les fonctions d'adjacences adj(i,j) doivent avoir les fonctionalités
  suivantes :

  1. adj(-1,j): initialise l'adjacence (appelée avant la génération)
  2. adj(i,-1): finalise l'adjacence (appelée après la génération)
  3. adj(i,j) pour i,j>=0, retourne 1 ssi i est adjacent à j
  4. adj(i,-2): construit l'étiquette du sommet i (via la variable NAME).

  Rem: adj(-1,j) doit toujours retourner le nb de sommets du graphe et
  fixer la variable N à cette valeur. Certaines fonctions adj()
  suppose que i<j.

 */


int load(int i,int j)
/*
  Graphe défini par un fichier et chargé en mémoire dans une variable
  LOAD de type "graph". Suivant la valeur ->sort, le test est linéaire
  ou logarithmique en min{deg(i),deg(j)}.
*/
{
  if(j<0){
    if(j==-1) free_graph(LOAD);
    return 0;
  }

  if(i<0){
    LOAD=File2Graph(SPARAM,2); /* remplit LOAD à partir du fichier */
    if(LOAD->f>0){ /* si c'est une famille, on sélectionne le premier graphe */
      graph *G=ExtractSubgraph(LOAD->G[0],NULL,0,0); /* copie le premier graphe */
      free_graph(LOAD); /* libère complètement la famille LOAD */
      LOAD=G; /* LOAD=premier graphe */
    }
    if(!LOAD->sym) DIRECTED=1; /* si le graphe est asymétrique, affichage DIRECTED */
    return N=LOAD->n;
  }

  int k; 
  /* pour avoir du min{deg(i),deg(j)} en non-orienté */
  if((!DIRECTED)&&(LOAD->d[i]>LOAD->d[j])) SWAP(i,j,k);

  return AdjGraph(LOAD,i,j);
}


int prime(int i,int j){
  if(j<0) return 0;
  if(i<0){ N=PARAM[0]; return N; }
  return (i>1)? ((j%i)==0) : 0;
}


int paley(int i,int j){
  int d,k,p,q;

  if(j<0) return 0;
  p=PARAM[0];
  if(i<0){ N=p; return N; }

  d=abs(i-j);
  q=p/2;
  for(k=1;k<=q;k++)
    if(d==((k*k)%p)) return 1;
  return 0;
}


int mycielski(int i,int j){   // suppose i<j
  int ki,kj,b,k;

  if(j<0) return 0;
  if(i<0){
    k=PARAM[0];
    if(k<2) return N=0;
    return N=3*(1<<(k-2))-1;
  }

  ki=ceil(log2((double)(i+2)/3.));
  kj=ceil(log2((double)(j+2)/3.));
  k=3*(1<<kj)-2; /* rem: k est pair */
  b=(j==k);
  if(ki==kj) return b;
  if(b) return 0;
  j -= (k>>1);
  if(j==i) return 0;
  if(i<j) return mycielski(i,j);
  return mycielski(j,i);
}


int wheel(int i,int j){
  if(j<0) return 0;
  if(i<0) return N=PARAM[0]+1;
  return (abs(j-i)==1)||(i==0)||((i==1)&&(j==N-1));
}


int windmill(int i,int j){
  if(j<0) return 0;
  if(i<0) return N=(PARAM[0]<<1)+1;
  return (i==0)||((i&1)&&(j==i+1));
}


int ring(int i,int j){
  if(j<0) return 0;
  if(i<0) return N=PARAM[0];

  int k;
  for(k=0;k<PARAM[1];k++)
    if((j==((i+PARAM[2+k])%N))||(i==((j+PARAM[2+k])%N))) return 1;

  return 0;
}


int cage(int i,int j){
  if(j<0) return 0;
  if(i<0) return N=PARAM[0];

  if( (j==(i+1)%N)||(i==(j+1)%N) ) return 1;

  int k=PARAM[1];
  if( (j==(i+PARAM[(i%k)+2])%N)||(i==(j+PARAM[(j%k)+2])%N) ) return 1;

  return 0;
}


int crown(int i,int j){
  if(j<0) return 0;
  int k=PARAM[0];
  if(i<0) return N=(k<<1);
  return ((i<k)&&(j>=k)&&(i!=(j-k))); /* utilise i<j */
}


int grid(int i,int j){
  int x,y,k,z,d,p,h,b;

  if(j<0) return 0;
  d=PARAM[0];
  if(i<0){
    N=1;
    for(k=0;k<d;k++){
      p=PARAM[k+1];
      WRAP[k]=(p<0);
      p=abs(p);
      PARAM[k+1]=p;
      N *= p;
    }
    return N;
    }

  z=h=k=b=0;

  while((k<d)&&(b<2)&&(h<2)){
    p=PARAM[k+1];
    x=i%p;
    y=j%p;
    h=abs(x-y);
    if(h==0) z++;
    if((h>1)&&(WRAP[k])&&(((x==0)&&(y==p-1))||((y==0)&&(x==p-1)))) h=1;
    if(h==1) b++;
    i /= p;
    j /= p;
    k++;
  }

  return (b==1)&&(z==(d-1));
}


int ggosset(int i,int j){
  /*
    ggosset p k d_1 v_1 ... d_k v_k
    mais PARAM = p k d d_1 v_1 ... d_k v_k

    Sommet: permutations du vecteur (v_1...v_1, ..., v_k...v_k) (ou de
    son opposé) telles que le nombre de valeurs entières v_t est
    d_t. On pose REP[i] = vecteur du sommet i qui est de taille
    d=d_1+...+d_k. On a l'arête i-j ssi le produit scalaire entre
    REP[i] et REP[j] vaut p.

    Pour calculer tous les vecteurs de taille d (les sommets) à partir
    de (v_1...v_1,...,v_k...v_k) on procède comme suit (codage d'un
    multi-ensemble à k valeurs):

    On choisit les positions de v_1 (il y en a Binom(d,d_1) possibles,
    puis on choisit les positions de v_2 parmi les d-d_1 restantes (il
    y en a Binom(d-d_1,d_2)), puis les positions de v_3 parmi les
    d-d_1-d_2 retantes, etc. Pour chaque t, les positions des d_t
    valeurs v_t sont codées par un sous-ensemble S[t] de [0,d-1] de
    taille d_t.
   */

  int t,p,d=PARAM[2];

  if(j<0){
    if(j==-1) FREE2(REP,N);
    if(j==-2) NAME_Vector(REP[i],d,",","[]",1,"%i");
    return 0;
  }

  if(i<0){
    int m,u,v,c,**S,*P,k=PARAM[1];

    /* Calcule N */
    m=d; N=2;
    for(t=3;m>0;t += 2){
      p = PARAM[t]; /* p=d_i */
      N *= Binom(m,p);
      m -= p;
    }
    ALLOCMAT(REP,N,d,int); /* vecteur de taille d représentant les sommets */

    ALLOC(P,d,int); /* tableau intermédiaire */
    ALLOCMAT(S,k,d,int); /* on réserve k tableaux (sous-ensembles) de taille <= d */
    for(t=0;t<k;t++) NextSet(S[t],-1,d); /* initialise les sous-ensembles */
    /* Note: taille |S[t]|=PARAM[(t<<1)+3] et v_t=PARAM[(t<<1)+4] */

    for(u=0;u<N;u+=2){
      /* Pour chaque sommet u on fait:

	 1. on remplit REP[u] et REP[u+1] à partir des sous-ensembles S[0]...S[k-1]
	 2. on incrémente les sous-ensembles S[0]...S[k-1]
	 
	 Pour l'étape 1, il faut passer par un tableau intermédiaire P
	 puis remplir REP[u] et REP[u+1] à partir de P. Supposons d=5,
	 k=3, et S[0]={1,3} S[1]={1}, S[2]={0,1}.  On met dans P les
	 indices t des S[t] aux bons emplacements comme suit:

           - au départ P est vide: P={-,-,-,-,-}
	   - puis on ajoute S[0]:  P={-,0,-,0,-}
	   - puis on ajoute S[1]:  P={-,0,1,0,-}
	   - puis on ajoute S[2]:  P={2,0,1,0,2}
      */

      /* Calcule P */
      for(t=0;t<d;t++) P[t]=-1; /* tableau vide au départ */
      for(t=0;t<k;t++){ /* pour chaque sous-ensemble S[t] */
	m=-1;
	for(p=v=0;p<PARAM[(t<<1)+3];p++){ /* on parcoure S[t] */
	  /* mettre t dans la S[t][p]-ème case vide de P à partir de l'indice v */
	  /* utilise le fait que dans S[t] les nombres sont croissant */
	  /* v=position courante de P où l'on va essayer d'écrire t */
	  /* c=combien de cases vides de P à partir de v faut-il encore sauter ? */
	  /* si P[v]=-1 et c=0 alors on écrit P[v]=t */
	  c=S[t][p]-m;
	  m=S[t][p]; /* mémorise l'ancienne valeur de S[t][p] */
	  while(c>0){
	    if(P[v]<0){
	      c--;
	      if(c==0) P[v]=t; /* écrit t et passe à la casse suivante */
	    }
	    v++;
	  }
	}
      }

      /* Remplit REP[u] et REP[u+1] grâce au tableau P (et incrémenter u) */
      for(t=0;t<d;t++){
	v=PARAM[(P[t]<<1)+4]; /* valeur v_t à écrire */
	REP[u][t]=v;
	REP[u+1][t]=-v;
      }

      //PRINTT(P,d);PRINTT(REP[u],d);

      /* Incrémente S[0]...S[k-1] grâce à NextSet() */
      t=0; /* on commence avec S[0] */
      m=d; /* S[t] dans {0...d-2} */
      v=3; /* PARAM[v] = taille de S[t] */
      while((NextSet(S[t],m,PARAM[v]))&(t<k)){
	t++; /* si S[t] fini on passe à S[t+1] */
	m -= PARAM[v];
	v += 2;
      }
      /* si t==k alors on a atteint le dernier sommet */
    }

    free(P);
    FREE2(S,k);
    return N;
  }

  /* Calcule le produit scalaire de REP[i] et REP[j] */
  for(p=t=0;t<d;t++) p += REP[i][t]*REP[j][t]; 
  return (p==PARAM[0]);
}


int rplg(int i,int j){
  /*
    PARAM[0]=n
    DPARAM[0]=t

    DPARAM[1]=sum_i w_i
    DREP[i][0]=degré moyen du sommet i = (N/(i+1))^(1/(t-1))
   */

  if(j<0){
    if(j==-1) FREE2(DREP,N);
    return 0;
  }

  if(i<0){
    int k;
    double c,s,n;

    N=PARAM[0];
    n=(double)N;
    ALLOCMAT(DREP,N,1,double);

    s=0.0;
    c=1.0/(DPARAM[0]-1.0);
    for(k=0;k<N;k++) s += (DREP[k][0]=pow(n/((double)k+1.0),c));
    DPARAM[1]=s;

    return N;
  }

  return (RAND01 < (DREP[i][0]*DREP[j][0]/DPARAM[1]));
}


int butterfly(int i,int j){
  int d,b,x,y;

  if(j<0) return 0;
  b=PARAM[0];
  if(i<0){
    N=1;
    for(d=0;d<b;d++) N <<= 1;
    N *= b+1;
    return N;
  }

  d=N/(b+1);   /* d=nb sommets d'un niveau */
  x=i/d;i=i%d; /* i -> (mot binaire,niveau) */
  y=j/d;j=j%d; /* j -> (mot binaire,niveau) */
  return (abs(x-y)==1) && ( (i==j) || ((i^j)==(1<<(b-x-1))) );
}


int barbell(int i,int j){
  int n,p;

  if(j<0) return 0;

  n=PARAM[0];
  p=PARAM[2];

  if(i<0){
    N=n+PARAM[1]+p-1; /* utilise n2 seulement ici */
    return N;
  }

  /* utilise le fait que i<j */

  if(j<n) return 1; /* clique 1 */
  if(n-1+p<=i) return 1; /* clique 2 */
  if((n-1<=i)&&(j<n+p)) return (j-i==1); /* chemin */
  return 0;
}


int debruijn(int i,int j){
  int d,b,k,x,y;

  if(j<0) return 0;
  b=PARAM[1];
  if(i<0){
    N=1;
    d=PARAM[0];
    for(k=0;k<d;k++) N *= b;
    return N;
  }

  x=j-(i*b)%N;
  y=i-(j*b)%N;
  return ((0<=x)&&(x<b))||((0<=y)&&(y<b));
}


int kautz(int i,int j){
  /*
    A chaque sommet i qui est entier de [0,b*(b-1)^(d-1)[, on associe
    une représentation (s1,...,sd) codée sous la forme d'un entier de
    [0,b^d[. Alors i adjacent à j ssi i et j sont adjacents dans le De
    Bruijn.
   */
  int d,b,k,l,q,r,s,t,x;

  if(j<0){
    if(j==-1) FREE2(REP,N);
    return 0;
  }
  if(i<0){
    d=PARAM[0];
    N=x=b=PARAM[1];
    t=b-1;
    for(k=1;k<d;k++) {
      N *= t;
      x *= b;
    }
    PARAM[2]=x; /* nb de sommets de De Bruijn */
    PARAM[3]=N; /* nb sommets de Kautz */
    ALLOCMAT(REP,N,1,int);

    for(l=0;l<N;l++){ /* pour tous les sommets faire .. */
      /* On voit un sommet l comme (r,s2...sd) avec r dans [0,b[ et
	 s_i de [0,b-1[. On le converti en (x1,...,xd) avec xd=r et
	 x_(d-1)=s2 ou s2+1 suivant si s2<r ou pas, etc. */
      r=x=l%b;
      q=l/b;
      for(k=1;k<d;k++){
	s=q%t;
	s+=(s>=r);
	r=s;
	q=q/t;
	x=x*b+r;
      }
      REP[l][0]=x;
    }
    return N;
  }

  N=PARAM[2]; /* modifie le nb de sommets */
  x=debruijn(REP[i][0],REP[j][0]);
  N=PARAM[3]; /* rétablit le nb de sommets */
  return x;
}


int gpstar(int i,int j)
/*
  REP[i][0...n-1] = représentation de la permutation du sommet i.
*/
{
  int n,k,c;

  n=PARAM[0];
  if(j<0){
    if(j==-1) FREE2(REP,N);
    if(j==-2){
      if(n<10) NAME_Vector(REP[i],n,"","",1,"%i");
      else NAME_Vector(REP[i],n,",","()",1,"%i");
    }
    return 0;
  }
  if(i<0){
    int *P;
    for(N=1,k=2;k<=n;k++) N *= k;
    ALLOCMAT(REP,N,n,int);
    ALLOCZ(P,n,int,_i); /* initialise P */

    /* génère toutes les permutations */
    for(c=0;c<N;c++){
      for(k=0;k<n;k++) REP[c][k]=P[k]+1; /* copie P dans REP */
      NextPermutation(P,n,NULL);
    }

    free(P);
    return N;
  }

  /* On compte les différences entre REP[i] et REP[j] */
  for(k=c=0;k<n;k++) c += (REP[i][k]!=REP[j][k]);
  return (c==PARAM[1]);
}


int linial(int i,int j){
/*
  REP[i][0...t-1] = représentation le nom du sommet i (sa vue).
*/
  int t,k,u;

  t=PARAM[1];

  if(j<0){
    if(j==-1){
      FREE2(REP,N);
    }
    if(j==-2){
      if(PARAM[0]<10) NAME_Vector(REP[i],t,"","",1,"%i");
      else NAME_Vector(REP[i],t,",","()",1,"%i");
    }
    return 0;
  }

  if(i<0){
    int *S,*P,m=PARAM[0];
    if((m<t)||(t<1)) return N=0; /* graphe vide si n<t */
    for(N=1,k=m-t+1;k<=m;k++) N *= k; /* calcul N */
    ALLOCMAT(REP,N,t,int);
    ALLOC(S,t,int);
    ALLOC(P,t,int);
    NextArrangement(S,P,-1,t); /* initialisation de S et P */
    for(u=0;u<N;u++){ /* génère tous les arrangements */
      for(k=0;k<t;k++) REP[u][k]=S[P[k]];
      NextArrangement(S,P,m,t);
    }
    free(P);
    free(S);
    return N;
  }

  if((REP[i][0]!=REP[j][t-1])||(PARAM[0]==t)){
    for(u=1,k=0;u<t;u++,k++) if(REP[i][u]!=REP[j][k]) break;
    if(u==t) return 1;
  }
  if(i<j) return linial(j,i);
  return 0;
}


int linialc(int i,int j){
  if(j<0) return linial(i,j);
  if(i<0){
    int m,t,u,k,v,m1,x,y;
    m=PARAM[0];
    t=PARAM[1];
    N=m; m1=m-1;
    for(N=m,u=m1,k=1;k<t;k++) N *= u; /* calcule N=m*(m-1)^t */
    ALLOCMAT(REP,N,t,int);
    /* on transforme u en (x0,x1,...x_t) avec x0 in [0,m[ et x_i in [0,m-1[ */
    for(u=0;u<N;u++){
      x=REP[u][0]=(u%m);
      for(v=u/m,k=1;k<t;k++){
	y = v%m1; /* y in [0,m-1[ */
	v /= m1;
	x=REP[u][k]=y+(y>=x); /* si x=y, on incrémente y */
      }
    }
    return N;
  }

  return linial(i,j);
}


int pancake(int i,int j){
/*
  REP[i][0...n-1] = représentation de la permutation du sommet i.
 */
  int k,l;

  if((j<0)||(i<0)) return gpstar(i,j);

  /* Test d'adjacence à partir de REP[i] et REP[j] */
  k=PARAM[l=0];
  do k--; while(REP[i][k]==REP[j][k]); /* i<>j, donc on s'arrête toujours */
  while((k>=0)&&(REP[i][k]==REP[j][l])) k--,l++; /* teste le "reversal" */
  return (k<0); /* adjacent si préfixe=reversal */
}


int bpancake(int i,int j){
/*
  REP[i][0...n-1] = représentation de la permutation signée du sommet
  i. Il s'agit d'une valeur de {1,2,...,n,-1,-2,...,-n} (on évite
  soigneusement 0, car -0=+0.

*/
  int k,l;

  if(j<0){
    if(j==-1) FREE2(REP,N);
    if(j==-2) NAME_Vector(REP[i],PARAM[0],"","",1,"%+i");
    return 0;
  }
  if(i<0){
    int *P,p,q,t,c,u,n=PARAM[0];
    for(t=1,k=p=2;k<=n;k++){ t *= k; p <<= 1; } /* calcule t=n! et p=2^n */
    N=p*t; /* nb de sommets */
    ALLOCMAT(REP,N,n,int); /* permutations signées représentant les sommets */
    ALLOCZ(P,n,int,_i); /* initialise une permutation P (non signée) */

    /* Génère toutes les permutations signées. On représente les
       signes par les bits de l'entier q=0...2^n-1. Si bit à 1 -> +1,
       et si bit à 0 -> -1 */
    for(c=u=0;c<t;c++){ /* répète n! fois */
      for(q=0;q<p;q++,u++) /* répète 2^n fois */
	for(k=0,l=1;k<n;k++,l<<=1){ /* l=mask=bit-0,bit-1,bit-2...,bit-(n-1) */
	  REP[u][k]=P[k]+1; /* copie P dans REP avec le signe +/-1 */
	  if(q&l) REP[u][k]=-REP[u][k]; /* copie P dans REP avec le signe +/-1 */
	}
      NextPermutation(P,n,NULL);
    }

    free(P);
    return N;
  }

  /* Test d'adjacence à partir de REP[i] et REP[j] */
  k=PARAM[l=0];
  do k--; while(REP[i][k]==REP[j][k]); /* i<>j, donc on s'arrête toujours */
  while((k>=0)&&(REP[i][k]==-REP[j][l])) k--,l++; /* teste le "reversal" */
  return (k<0); /* adjacent si préfixe=-reversal */
}


int pstar(int i,int j){
/*
  REP[i][0...n-1] = représentation de la permutation du sommet i.
 */

  if((j<0)||(i<0)) return gpstar(i,j); /* c'est comme gpstar() */

  /* Il faut deux différences dont le premier chiffre */
  return (REP[i][0]!=REP[j][0]) && (gpstar(i,j));
}


int thetagone(int i,int j){
/*
  L'adjacence est en O(k*N).  On teste aussi les sommets supprimés
  (par -delv), sinon le résultat n'est pas un sous-graphe.
*/
  int t,z,p,k;
  double d,w;

  if(j<0){
    if(j==-1) {
      free(XPOS);
      free(YPOS);
      if(XYtype==2){
	free(XSEED);
	free(YSEED);
      }
    }
    return 0;
  }

  if(i<0){
    N=PARAM[0];
    InitXY();
    return N;
  }

  p=PARAM[1];
  k=PARAM[2];
  w=DPARAM[0];

  /*
    Adjacence: pour tous les axes t, on cacule P_t(i,j)=distgone(),
    puis on détermine s'il existe un autre sommet z avec P_t(i,z) plus
    petit. Si c'est non (et que P_t(i,j) est finie), alors i est
    adjacent à j, sinon ils ne le sont pas (mais j peut être adjacent
    à i !).
   */

  for(t=0;t<k;t++){ /* pour tous les axes t */
    d=distgone(i,j,t,p,k,w); /* calcule P_t(i,j) */
    if(d<DBL_MAX){ /* distance infinie */
      for(z=0;z<N;z++){ /* pour tous les autres sommets z, même supprimés ! */
	if((z!=i)&&(distgone(i,z,t,p,k,w)<d)) z=N; /* z plus proche ? */
	  /* si oui, on arrête: P_t(i,j) contient z */
      }
      if(z==N) return 1; /* on a pas trouvé de sommet z plus proche que j */
    }
  }

  /*
    A priori pas d'arête entre i et j. Il faut alors aussi tester
    adj(j,i) car la distance P_t(i,j) n'est pas symétrique.
  */
  if(i>j) return 0;
  return thetagone(j,i);
}


int udg(int i,int j){
/*
  Deux points peuvent avoir les même coordonnées.
*/
  if(j<0) return thetagone(0,-1);

  if(i<0){
    if(NORM==2) DPARAM[0] *= DPARAM[0]; /* si norme L_2, alors r=r^2 */
    return thetagone(-1,0);
  }

  return (Norme(i,j)<=DPARAM[0]);
}


int gabriel(int i,int j){
/*
  Adjacence en O(N).
*/
  int z;
  double r,xi,yi;

  if((j<0)||(i<0)) return thetagone(i,j);

  xi=XPOS[i]; yi=YPOS[i]; /* on sauvegarde les positions de i */
  XPOS[i]=(xi+XPOS[j])/2.0; /* on change les positions de i */
  YPOS[i]=(yi+YPOS[j])/2.0; /* i <- milieu du segment i-j */
  r=Norme(i,j); /* rayon du disque centré en c */

  for(z=0;z<N;z++) /* teste même les sommets supprimés */
    if((Norme(i,z)<r)&&(z!=i)) z=N; /* ici i=centre(i,j) */
  /* ici z=N ou z=N+1 */

  XPOS[i]=xi; /* on remet la position de i */
  YPOS[i]=yi;
  return (z==N);
}


int rng(int i,int j){
/*
  Adjacence en O(N).
*/
  int z;
  double r;

  if((j<0)||(i<0)) return thetagone(i,j);

  r=Norme(i,j);
  for(z=0;z<N;z++) /* teste même les sommets supprimés */
      if(MAX(Norme(i,z),Norme(j,z))<r) return 0;

  return 1; /* fonction de distance symétrique */
}


int nng(int i,int j){
/*
  Adjacence en O(N).
*/
  int z;
  double r;

  if((j<0)||(i<0)) return thetagone(i,j);

  r=Norme(i,j);
  for(z=0;z<N;z++) /* teste même les sommets supprimés */
      if((Norme(i,z)<r)&&(z!=i)) z=N; /* alors d(z,i)<d(i,j) */

  if(z==N) return 1;

  /* Avant de dire que adj(i,j)=0, il faut tester adj(j,i), car le
     test n'est pas symétrique. */
  if(i>j) return 0;
  return nng(j,i);
}


int hexagon(int i,int j){
/*
  Utilise le fait que i<j.
    
  On voit le graphe comme une grille de p+1 lignes de 2q+2 colonnes
  allant du coin en bas à droite (0,0) au coin (p,2q+1) (en notation
  (ligne,colonne)), et dans laquelle deux sommets ont été supprimés: le
  coin (0,2q+1) et le coin (p,2q+1) si p est impair, le coin (p,0) si
  p est pair. Les numéros sont consécutifs sur une ligne, de haut en
  bas.

 Ex:

 hexagon 3 2   hexagon 2 3

 o-o-o-o-o x   o-o-o-o-o-o-o x
 |   |   |     |   |   |   |
 o-o-o-o-o-o   o-o-o-o-o-o-o-o
   |   |   |     |   |   |   |
 o-o-o-o-o-o   x o-o-o-o-o-o-o
 |   |   |
 o-o-o-o-o x

*/
  int p,q,t,li,lj,ci,cj;

  if(j<0) return 0;

  p=PARAM[0];
  q=PARAM[1];
  t=(q<<1)+2; /* longueur d'une ligne */

  if(i<0) return N=(p+1)*t-2;

  if(i>=t-1) i++; /* on insère virtuellement le coin (0,2q+1) */
  if(j>=t-1) j++;
  if((p&1)==0){ /* si p est impair, on a rien à faire */
    if(i>=p*t) i++; /* on insère virtuellement le coin (p,0) si p est pair */
    if(j>=p*t) j++;
  }

  /* on calcule les coordonnées de i et j placés sur cette
     grille (avec les coins manquant) */

  li=i/t;ci=i%t;
  lj=j/t;cj=j%t;
  
  /* utilise le fait que i<j: dans le dernier cas lj=li+1 */
  return ((li==lj)&&(abs(ci-cj)==1)) || ((ci==cj)&&(lj==(li+1))&&((ci&1)==(li&1)));
}


int whexagon(int i,int j){
/* Utilise le fait que i<j et hexagon(i,j) */
  int p,q,t,li,ci,lj,cj;

  if(j<0) return 0;

  p=PARAM[0];
  q=PARAM[1];

  if(i<0) return N=hexagon(-1,0)+p*q;

  t=N-p*q; /* nb de sommets de l'hexagone */

  /* teste si i et j sont dans l'hexagone */
  if((i<t)&&(j<t)) return hexagon(i,j);

  /* teste si i et j sont hors l'hexagone */
  if((i>=t)&&(j>=t)) return 0;

  /* on a i dans l'hexagone et j en dehors */
  lj=(j-t)/q;cj=(j-t)%q; /* j est le centre de l'hexagone (lj,cj) */
  t=(q<<1)+2; /* longueur d'une ligne de l'hexagone */

  /* on calcule les coordonnées de i dans l'hexagone */
  if(i>=t-1) i++; /* on corrige */
  if(((p&1)==0)&&(i>=p*t)) i++; /* on recorrige */
  li=i/t;ci=i%t; /* (li,ci) coordonnées de i */

  return ( ((li==lj)||(li==(lj+1))) && (abs(2*cj+1+(lj&1)-ci)<2) );
}


int hanoi(int i,int j){
/*
  Adjacence: on écrit i (et j) en base b, mot de n lettres. i et j
  sont adjacents ssi i=Puv...v et j=Pvu...u où P est un préfixe
  commun, et u,v des lettres qui se suivent (modulo b).
*/
  int n,b,ri,rj,u,v,k;

  n=PARAM[0];
  b=PARAM[1];

  if(j<0){
    if(j==-2){
      if(b<10) NAME_Base(i,b,n,"","",1);
      else NAME_Base(i,b,n,",","",1);
    }
    return 0;
  }

  if(i<0){
    if(b<2) return N=0;
    for(N=k=1;k<=n;k++) N *= b; /* calcule le nombre de sommets N=b^n */
    return N;
  }

  /* on égraine les chiffres en base b */
  
  for(ri=rj=k=0;k<n;k++){
    if(ri!=rj) break; /* on s'arrête dès qu'on diffère, on garde k */
    ri=i%b; i/=b; /* ri=dernier chiffre, i=i sans dernier chiffre */
    rj=j%b; j/=b; /* rj=dernier chiffre, j=j sans dernier chiffre */
  }
  if((((ri+1)%b)!=rj)&&(((rj+1)%b)!=ri)) return 0; /* alors pas voisin */

  u=ri; v=rj; /* ici u et v sont consécutifs (mod b) */
  for(;k<n;k++){
    ri=i%b; i/=b;
    rj=j%b; j/=b;
    if(ri!=v) return 0; /* pas bon */
    if(rj!=u) return 0; /* pas bon */
  }

  return 1;
}


int sierpinski(int i,int j){
/*
  On utilise REP[i][k] pour représenter le sommet i. C'est un mot d'au
  plus n lettres de [0,b[. On pose REP[i][n]=L où L est la longueur du
  mot.

  Le mot du sommet i représente la suite des cycles auxquels il
  appartient, sauf s'il est l'un des b sommets initiaux. Prennons
  b=3. Les 3 sommets du triangle sont 0,1,2. Si n>1, les autres
  sommets commenceront tous par 0. On a alors 3 autres triangles,
  numérotés 0,1,2, le triangle i à pour sommet le sommet i. Les 3
  sommets internes sont 00,01,02. Le sommet 00 partage les triangles 0
  et 1, 01 les triangles 1 et 2, 02 les triangles 2 et 0. Si n>2, tous
  les autres sommets commenceront par 00, 01 ou 02 suivant le
  sous-triangle auquel ils appartiennent. Dans le sous-triangle 0, les
  3 sommets internes seront 000, 001, 002. Etc.

  Ex: n=2 b=3         0 
                     /  \
                   00 -- 02
                  /  \  /  \
                 1 -- 01 -- 2

  Adjacence:

  CAS 1: extrémité (|i|=1 et |j|=n)
  Si i=x et n>1, alors il est voisin de:
  - 0 x^{n-2} x
  - 0 x^{n-2} (x-1)
  Si i=x et n=1, alors c'est le CAS 2.
  
  Soit P=le plus préfixe commun entre i et j, et k=|P|

  CAS 2: sommet du triangle le plus interne (|i|=|j|=n)
  Si i=Px, k=n-1, et n>0, alors il est voisin de:
  - P (x+1)
  - P (x-1)

  CAS 3: sommet entre deux triangles (1<|i|<n et |j|=n)
  Si i=Px, alors:

  CAS 3.1: i=P=Qx est préfixe de j (k=|i|).
  Alors il est voisin de:
  - P (x+1)^{n-k-1} x
  - P (x+1)^{n-k-1} (x+1)

  CAS 3.2: i=Px.
  Alors il est voisin de:
  - P (x+1) x^{n-p-2} x
  - P (x+1) x^{n-p-2} (x-1)

*/
  int n,b,k,t,r,x,c,li,lj;

  n=PARAM[0];
  b=PARAM[1];

  if(j<0){
    if(j==-1) FREE2(REP,N);
    if(j==-2){
      if(b<10) NAME_Vector(REP[i],REP[i][n],"","",1,"%i");
      else NAME_Vector(REP[i],REP[i][n],",","()",1,"%i");
    }
    return 0;
  }

  if(i<0){
    if((b<=2)||(n==0)) return N=0; /* graphe vide, graphe non défini */
    for(N=b,k=2;k<=n;k++) N=b*N-b;
    ALLOCMAT(REP,N,n+1,int); /* REP[t][k]=k-ème lettre du sommet t */

    for(t=0;t<N;t++){ /* calcule les noms */
      x=t;
      k=r=0;
      if(x<b) REP[t][0]=x; /* un des b sommets du 1er cycle? */
      else{
	x -= b;	
	while(1){
	  REP[t][k++]=r;
	  if(x<b) { REP[t][k]=x; break; }
	  x -= b; r=x%b; x /= b;
	}
      }
      REP[t][n]=k+1-(n==0); /* longueur du mot, corrige si n=0 */
    }

    return N;
  }

  li=REP[i][n]; /* longueur de i */
  lj=REP[j][n]; /* longueur de j */
  /* Propriété: si i<j, alors li<=lj */
  if(lj<n) return 0;

  /* CAS 1 */
  if((li==1)&&(n>1)&&(REP[j][0]==0)){
    x=REP[i][0];
    for(c=t=1;t<=n-2;t++) if(REP[j][t]!=x) { c=0; break; }
    if(c){ /* c=vrai ssi j=0x^(n-2) */
      if(REP[j][n-1]==x) return 1;
      if(REP[j][n-1]==((x-1+b)%b)) return 1;
    }
  }

  /* calcule k=longueur du préfixe commun */
  for(k=0;(k<li)&&(k<lj)&&(k<n);k++)
    if(REP[j][k]!=REP[i][k]) break;

  /* CAS 2 */
  if((li==n)&&(k==n-1)){
    x=REP[i][k];
    if(REP[j][k]==((x+1)%b)) return 1;
    if(REP[j][k]==((x-1+b)%b)) return 1;
  }

  /* CAS 3 */
  if((li==1)||(li==n)) return 0;
  x=REP[i][li-1];
  /* ici on a 1<|i|<n, |j|=n, et x=dernière lettre de i */

  /* CAS 3.1 */
  if(k==li){
    for(c=1,t=k;t<=n-2;t++) if(REP[j][t]!=((x+1)%b)) { c=0; break; }
    if(c){
      if(REP[j][n-1]==x) return 1;
      if(REP[j][n-1]==((x+1)%b)) return 1;
    }
  }
  
  /* CAS 3.2 */
  if((k==li-1)&&(REP[j][k]==((x+1)%b))){
    for(c=1,t=k+1;t<=n-2;t++) if(REP[j][t]!=x) { c=0; break; }
    if(c){
      if(REP[j][n-1]==x) return 1;
      if(REP[j][n-1]==((x-1+b)%b)) return 1;
    }
  }

  return 0; /* si i>j, alors on ne fait rien */
}


int rpartite(int i,int j)
/*
  On se sert du fait que i<j pour le test d'adjacence. Les sommets
  sont numérotés consécutivement dans chacune des parts. Utilise le
  tableau WRAP en interne: WRAP[k]=a_1+...+a_k est la somme partielle,
  WRAP[0]=0. Donc les sommets de la part i sont numérotés de WRAP[i-1]
  à WRAP[i] (exclu).
*/
{
  int k,r,s;

  if(j<0) return 0;

  r=PARAM[0];

  if(i<0){
    N=WRAP[0]=0;
    for(k=1;k<=r;k++){
      N += PARAM[k];
      WRAP[k]=N;
    }
    return N;
  }

  /*
    Pour calculer adj(i,j), avec i<j, on calcule les numéros de la
    part de i et de j: adj(i,j)=1 ssi part(i)<>part(j). Pour celà, on
    fait une recherche dichotomique de la part de i (c'est-à-dire d'un
    k tq WRAP[k]<=i<WRAP[k+1]). La complexité est O(logr), r=nb de
    parts.
  */

  /* Cherche la part de i dans [s,r[: au départ c'est dans [0,r[ */
  s=0;
  k=r/2;
  while(s<r){
    if(i<WRAP[k]){
      if(j>=WRAP[k]) return 1; /* i et j sont dans des parts différentes */
      r=k; /* ici i<j<WRAP[k]: on cherche dans [s,k[ */
      k=(s+k)>>1;
    }else{ /* ici WRAP[k]<=i<j */
      if(j<WRAP[k+1]) return 0; /* i et j sont dans la part k */
      s=k; /* ici WRAP[k]<=i<j: on cherche dans [k,r] */
      k=(k+r)>>1; 
    }
  }
  return (j>=WRAP[k+1]); /* ici i est dans la part k. On vérifie si j aussi */
}


int aqua(int i,int j)
/*
  On se sert de REP[i][0..n], n=PARAM[0].
*/
{
  int n=PARAM[0]; /* n=nb de paramètres */
  if(j<0){
    if(j==-1) FREE2(REP,N);
    if(j==-2) NAME_Vector(REP[i],n,",","",1,"%i");
    return 0;
  }

  int k,x,y;
  int *C=PARAM+1; /* C=tableau de contraintes */

  if(i<0){
    x=*C; /* s=PARAM[1]=premier terme=la somme */
    int *S=NextPart(NULL,n,x,C);
    N=0; /* calcule d'abord N pour ALLOCREP() */
    do N++; while(NextPart(S,n,x,C)!=NULL);
    ALLOCMAT(REP,N,n,int);
    N=0; /* calcule les sommets */
    do{
      for(k=0;k<n;k++) REP[N][k]=S[k];
      N++;
    } while(NextPart(S,n,x,C)!=NULL);

    return N;
  }

  if(i==j) return 0;

  /* compte et mémorise les différences entre REP[i] et REP[j]: il y en a au moins 2 */
  for(k=0,x=y=-1;k<n;k++)
    if(REP[i][k]!=REP[j][k]){
      if((x>=0)&&(y>=0)) return 0; /* si plus de 2 différences, alors pas d'arc */
      if(x<0) x=k; else if(y<0) y=k;
    }

  /* soit on a versé x vers y */
  /* k=quantité que l'on peut verser de x à y */
  k=MIN(C[y],REP[i][x]+REP[i][y])-REP[i][y];
  if((REP[j][y]==REP[i][y]+k)&&(REP[j][x]==REP[i][x]-k)) return 1;

  /* soit on a versé y vers x */
  /* k=quantité que l'on peut verser de y à x */
  k=MIN(C[x],REP[i][y]+REP[i][x])-REP[i][x];
  if((REP[j][x]==REP[i][x]+k)&&(REP[j][y]==REP[i][y]-k)) return 1;

  return 0;
}


int theta0(int i,int j){
  int T[]={0,-3,5,5,0,6,3,-2};
  if(j<0) return 0;
  if(i<0) return N=7;
  return GraphFromArray(i,j,T);
}


int icosa(int i,int j){
  int T[]={
    5,0,1,2,3,4,5,6,7,8,
    9,10,11,6,4,7,3,8,2,
    9,1,10,5,1,2,0,3,4,0,
    5,6,10,9,11,9,11,8,11,7,-2};
  if(j<0) return 0;
  if(i<0) return N=12;
  return GraphFromArray(i,j,T);
}


int rdodeca(int i,int j){
  int T[]={
    0,1,2,3,4,5,0,6,
    7,1,7,8,2,8,9,3,9,
    10,4,10,11,5,11,6,
    7,12,9,12,11,-1,
    0,13,2,13,4,-2};
  if(j<0) return 0;
  if(i<0) return N=14;
  return GraphFromArray(i,j,T);
}


int cubocta(int i,int j){
  int T[]={
    0,1,2,3,4,5,0,6,7,2,
    8,9,5,6,10,7,1,11,0,-1,
    3,10,4,9,11,8,3,-2};
  if(j<0) return 0;
  if(i<0) return N=12;
  return GraphFromArray(i,j,T);
}


int tutte(int i,int j){
  int T[]={
    0,-3,8,
    4,8,9,3,9,10,11,2,
    11,12,1,12,13,14,10,
    14,7,6,15,13,-1,
    0,16,-3,7,
    23,19,23,24,18,24,25,26,17,
    26,27,16,27,28,29,25,
    29,22,21,30,28,-1,
    0,31,-3,7,
    38,34,38,39,33,39,40,41,32,
    41,42,31,42,43,44,40,
    44,37,36,45,43,-1,
    15,20,-1,
    30,35,-1,45,5,-2};
  if(j<0) return 0;
  if(i<0) return N=46;
  return GraphFromArray(i,j,T);
}


int fritsch(int i,int j){
  int T[]={-4,0,4,6,3,7,2,4,-1,1,8,1,7,6,8,5,0,2,-2};
  if(j<0) return 0;
  if(i<0) return N=9;
  return GraphFromArray(i,j,T);
}


int soifer(int i,int j){
  int T[]={-4,0,6,4,2,6,0,7,5,8,1,3,8,5,3,-2};
  if(j<0) return 0;
  if(i<0) return N=9;
  return GraphFromArray(i,j,T);
}


int poussin(int i,int j){
  int T[]={
    -4,0,2,4,6,8,10,12,14,2,
    0,13,11,9,5,8,12,7,3,14,7,-1,
    4,1,9,0,11,-1,1,5,-1,3,6,-2};
  if(j<0) return 0;
  if(i<0) return N=15;
  return GraphFromArray(i,j,T);
}


int errara(int i,int j){
  int T[]={
    0,-5,10,9,5,15,11,16,-1,
    1,-5,6,16,12,13,4,7,-1,
    2,-6,6,7,8,9,10,-1,
    3,-6,11,12,13,14,15,11,-1,
    4,-5,7,8,5,14,13,-1,
    5,-5,8,9,14,15,-1,
    16,-5,6,11,12,10,-2};
  if(j<0) return 0;
  if(i<0) return N=17;
  return GraphFromArray(i,j,T);
}


int kittell(int i,int j){
  int T[]={
    0,-3,22,
    10,12,14,5,13,11,3,10,2,9,1,6,0,
    5,16,6,17,7,1,8,22,20,18,7,-1,
    5,15,21,14,22,9,12,-1,
    9,14,-1,18,8,20,-1,
    19,-5,15,16,17,21,-1,
    2,0,3,0,4,13,4,11,-2};
  if(j<0) return 0;
  if(i<0) return N=23;
  return GraphFromArray(i,j,T);
}


int frucht(int i,int j){
  int T[]={-4,0,4,5,3,2,7,6,8,9,11,10,1,-2};
  if(j<0) return 0;
  if(i<0) return N=12;
  return GraphFromArray(i,j,T);
}


int moser(int i,int j){
  int T[]={0,1,2,3,0,4,5,6,0,-1,1,3,2,5,4,6,-2};
  if(j<0) return 0;
  if(i<0) return N=7;
  return GraphFromArray(i,j,T);
}


int markstrom(int i,int j){
  int T[]={
    8,0,-3,12,5,13,-3,7,
    3,21,22,23,0,23,1,2,21,22,18,-1,
    15,20,14,13,4,-1,
    6,12,7,-1,17,19,-1,
    9,11,10,16,-2};
  if(j<0) return 0;
  if(i<0) return N=24;
  return GraphFromArray(i,j,T);
}


int robertson(int i,int j){
  int T[]={-4,0,4,8,13,17,2,6,10,14,0,
    1,9,16,5,12,1,-1,3,11,18,7,15,3,-2};
  if(j<0) return 0;
  if(i<0) return N=19;
  return GraphFromArray(i,j,T);
}


int headwood4(int i,int j){
  int T[]={
    0,-6,1,5,4,3,13,12,2,-1,
    1,-6,5,6,7,8,2,-1,
    2,-6,8,9,10,11,-1,
    16,-6,4,15,17,19,5,-1,
    12,-6,11,20,19,18,-1,
    14,15,3,14,13,18,17,14,18,-1,
    22,-6,6,21,24,23,7,-1,
    5,20,6,11,21,10,24,9,23,8,-2};
  if(j<0) return 0;
  if(i<0) return N=25;
  return GraphFromArray(i,j,T);
}


int wiener_araya(int i,int j){
  int T[]={
    0,-5,1,4,12,15,-1,
    1,-3,39,
    41,-5,20,23,36,-1,
    18,36,35,16,17,1,2,19,-1,
    3,21,22,5,4,8,7,26,27,9,10,29,28,39,25,24,6,-1,
    8,12,11,31,32,13,14,34,33,37,38,23,-1,
    30,40,33,-2};
  if(j<0) return 0;
  if(i<0) return N=42;
  return GraphFromArray(i,j,T);
}


int flower_snark(int i,int j){
  if(j<0){
    if(j==-2) sprintf(NAME,"%c%i",((i&3)==0)? 'c' : 't'+(i&3),i>>2);
    return 0;
  }
  int k=PARAM[0];
  if(i<0) return N=(k<<2);

  int u=(j>>2)-(i>>2); /* i<j, donc u>=0 */
  i &= 3;
  j &= 3;
  if(u==0) return (i==0);
  if((u==1)&&(i==j)) return (i>0);
  if(u!=(k-1)) return 0;
  i*=j;
  return ((i==1)||(i==6));
}


int clebsch(int i,int j){
  if((i<0)||(j<0)) return grid(i,j);
  if(((i|j)==N-1)&&((i&j)==0)) return 1; /* sommets opposés */
  return grid(i,j);
}


int arboricity(int i,int j)
/*
  Utilise REP pour la représentation implicite. REP[i][0...k-1] sont
  les k pères du sommet i.
*/
{
  if(j<0) return kautz(0,-1);

  int k=PARAM[1],t;

  if(i<0){ /* calcule N et REP[i][0..k-1] */
    int v,*X,**T;
    N=PARAM[0];
    ALLOCMAT(REP,N,k,int); /* REP est la représentation finale */
    ALLOCMAT(T,N,1,int); /* T=arbre aléatoire */
    ALLOCZ(X,N,int,_i);   /* permutation aléatoire */

    for(t=0;t<k;){
      RandomTree(N,T,1); /* calcule un arbre aléatoire T */
      T[0][0]=0; /* racine = 0 plutôt que -1, car X[-1] n'existe pas ! */
      for(v=0;v<N;v++) REP[X[v]][t]=X[T[v][0]]; /* copie et permute le père */
      REP[X[0]][t]=-1; /* père de la racine = -1 */
      if(++t<k) Permute(X,N); /* si pas fini, on permute aléatoirement X */
    }

    FREE2(T,N);
    free(X);
    return N;
    }

  for(t=0;t<k;t++) /* j est un père de i ou le contraire */
    if((REP[i][t]==j)||(REP[j][t]==i)) return 1;
  return 0;
}


int kpage(int i,int j)
/*
  Utilise REP pour la représentation implicite. Chaque sommet i
  possède 2k pères: REP[i][2j,2j+1] sont les 2 pères du sommet i dans
  le j-ème outerplanar, j=0...k-1. Pour le générer, on fait l'union de
  k outerplanars connexes enracinés et plan (outerplan en fait). Ils
  sont étiquetés aléatoirement, sauf le premier. Pour l'adjacence on
  fait comme pour un graphe d'arboricité 2k.

  Le tirage de chaque outerplanar n'est pas complètement uniforme
  (parmi les outerplans connexes à n sommets). Pour l'être il
  faudrait: 1) bi-colorier complètement l'arbre, et 2) si la dernière
  branche n'est pas toute à 0, on rejette et on recommence. En moyenne
  il y a O(1) rejets, la dernière branche étant constante en moyenne.
*/
{
  if(j<0) return kautz(0,-1);
  if(i<0){
    int u,p,q,r,s,t,z,k,c,**T;
    N=PARAM[0];
    k=(PARAM[1]<<=1);
    ALLOCMAT(REP,N,k,int); /* test d'adjacence = arboricity n 2k */
    ALLOCMAT(T,N,1,int); /* T=arbre aléatoire */

    /*
      On veut, pour chaque page p:
      REP[u][2p+0]=père de u dans l'arbre.
      REP[u][2p+1]=père2 de u.
    */

    for(p=q=0;p<k;p++,q+=2){ /* pour chaque page p=0...k-1 */
      RandomTree(N,T,0); /* calcule un arbre aléatoire T et le tableau TREE */
      
    /*
      Pour le calcul de REP[u][2p+1], on parcourt TREE puis:
      - si on descend on repère z, l'indice de la feuille.
      - si on remonte en u, on met à jour les pères2 des sommets de la
        branche précédente: ce sont les sommets de TREE dont l'indice
        va de z à u-2 (u et u-1 sont impossibles).
    */
      
      /* c=permutation circulaire aléatoire */
      if(p>0) c=random()%N; else c=0;
      T[0][0]=0; /* père de racine = 0 plutôt que -1, car X[-1] n'existe pas ! */
      for(t=0;t<N;t++) REP[(t+c)%N][q]=(T[t][0]+c)%N;
      REP[c][q]=-1; /* père de la racine = -1 */

      for(u=0;u<N;u++) REP[u][q+1]=-1; /* par défaut, pas de père2 */
      r=u=z=t=0;
      /* r-s sont deux valeurs consécutives dans TREE */
      /* t=1 ssi montée */

      while(r<N-1){ /* le dernier sommet est la dernière feuille */
	s=TREE[++u];
	if((t==1)&&(s<r)){ /* on commence à descendre */
	  t=1-t;
	  z=u-1;
	}
	if((t==0)&&(s>r)){ /* on commence à monter */
	  t=1-t;
	  while(z<u-1) REP[(TREE[z++]+c)%N][q+1]=(random()&1)?(s+c)%N:-1;
	}
	r=s;
      }
      /* supprime quelques arêtes */
      //for(u=0;u<N;u++) if(RAND01<x) REP[u][q]=-1;
    } /* fin du for(p=...) */

    FREE2(T,1);
    free(TREE);
    return N;
  }

  return arboricity(i,j);
}


int linegraph(int i,int j)
/*
  Chaque sommet i possède 2 couleurs prises dans 1...k. Les sommets i
  et j sont adjacents si une couleur de l'un est une couleur de
  l'autre.  Utilise REP pour la représentation des couleurs.
*/
{
  if(j<0) return kautz(0,-1);
  if(i<0){
    int u,k;
    N=PARAM[0];
    k=PARAM[1];
    ALLOCMAT(REP,N,2,int);
    for(u=0;u<N;u++){
      REP[u][0]=random()%k;
      REP[u][1]=random()%k;
    }
    return N;
  }

  return ((REP[i][0]==REP[j][0])||(REP[i][0]==REP[j][1])||
	  (REP[i][1]==REP[j][0])||(REP[i][1]==REP[j][1]));
}


int arytree(int i,int j)
/*
  Utilise REP. Les sommets sont numérotés selon un DFS.
*/
{
  if(j<0) return kautz(0,-1);
  if(i<0){
    int h,k,r,t;
    /* Initialise N et le tableau des père REP[i][0].  La racine est
       le sommet i=0 et n'a pas de père (-1). */
    h=PARAM[0];k=PARAM[1];r=PARAM[2];
    if((h==0)||(r==0)) N=k=1;
    if(k==0) h=(h>0); /* si k=0, alors la hauteur est au plus 1 */
    if(k<=1) N=1+r*h;
    else{ /* on alors k>1, h>0 et r>0 */
      N=1; for(t=0;t<h;t++) N *= k; /* après cette boucle, N=r^h */
      N=r*(N-1)/(k-1)+1; /* N=racine + r x (taille sous-arbre régulier) */
    }
    ALLOCMAT(REP,N,1,int); /* tableau des pères */
    N=0; REP[0][0]=-1; /* père vide pour la racine */
    TraverseRegTree(h,k,r); /* fixe REP et recalcule N */
    PARAM[1]=1;
    return N;
    }

  return arboricity(i,j);
}


int rarytree(int i,int j)
/*
  Utilise REP. Les sommets sont numérotés selon un DFS modifié: on
  pose les fils avant la récursivité.
*/
{

  if(j<0) return kautz(0,-1);
  if(i<0){
    int r,n,b,*B,p,u,x,y,z,m;

    n=PARAM[0];
    if(n<=0){ REP=NULL; return N=0;}
    b=PARAM[1];
    N=b*n+1;
    ALLOC(B,n,int); r=Dick(B,n); /* mot de Dick aléatoire */
    ALLOCMAT(REP,N,1,int);
    p=0;u=1; /* p=père courant, u=prochain sommet de REP */
    REP[0][0]=-1; /* père du sommet u=0=racine */

    /*
      Pour remplir REP à partir de B, on réalise un DFS, mais en
      posant b fils avant l'appel récursif. Plus précisément, on lit
      les valeurs de B en unaire. Disons que n=3 et
      B=[2,1,0]="110100".  A chaque "1" lu dans B, on pose, à la suite
      de REP, b nouveaux sommets qui auront tous p comme père. Le père
      suivant est alors p+1. Lorsqu'on lit "0", on ne pose aucun
      nouveau sommet. On passe au père suivant, p+1. Dans l'exemple
      (b=2), cela donne: REP=[-1,0,0,1,1,3,3], soit l'arbre.

                0
               / \
              1   2
             / \
            3   4
               / \
              5   6
     */
    for(x=0;x<n;x++){ /* pour toutes les valeurs de B, à partir de r */
      m=B[(x+r)%n];
      if(m==0) p++; /* ici p (avant l'incrémentation) est une feuille */
      else
	for(y=0;y<m;y++){
	  for(z=0;z<b;z++) REP[u++][0]=p;
	  p++;
	}
    }

    free(B);
    PARAM[1]=1; /* pour test d'aboricité k=1 */
    return N;
    }

  return arboricity(i,j);
}


int ktree(int i,int j){
/*
  Utilise REP pour la représentation implicite. REP[i][0...k-1] sont
  les k pères du sommet i. Cette fonction utilise un 3e paramètre
  caché, PARAM[2] qui vaut 0 s'il faut générer un arbre aléatoire et 1
  s'il faut un chemin.
*/
  int k,t,p,w,x,y;
  int **P;
  
  if(j<0) return kautz(0,-1);
  if(i<0){ /* calcule N et REP[i][0..k-1] */
    k=PARAM[1];
    N=PARAM[0];
    ALLOCMAT(REP,N,1,int); /* arbre ou chemin aléatoire de N-k noeuds */
    if(PARAM[2]==0) RandomTree(N-k,REP,1); /* arbre */
    else for(t=0;t<N;t++) REP[t][0]=t-1; /* chemin */

    ALLOCMAT(P,N,k,int); /* On contruit la représentation implicite dans P */

    /* Chacun des k+1 sommets de la racine (numéros de 0 à k) ont pour
       pères tous les autres sommets de la clique */

    for(t=0;t<=k;t++) /* pour les k+1 sommets */
      for(p=0;p<k;p++) /* pour les k pères 0..k-1 */
	P[t][p]=(t+p+1)%(k+1); /* il faut sauter t */

    /* On utilise le fait que les noeuds de REP forment un DFS. En
       traitant les sommets dans l'ordre on est sûr que le père est
       déjà traité */
 
    for(t=k+1;t<N;t++){ /* on démarre à k+1, les k+1 sommets de la
			   racine sont déjà traités */
      p=REP[t-k][0]; /* père du sommet t du graphe, donc du noeuds t-k */
      w=random()%(k+1); /* indice d'un des sommets du père ne sera pas choisi */
      p += k; /* p=nom réel du père dans le graphe */
      P[t][0]=p; /* remplit avec le père lui-même */
      x=0; /* x=indice des pères pour P[p] */
      y=(w>0); /* y=prochain indice des pères pour P[t]. Si w=0 on
		  saute le père */
      while(x<k){
	P[t][y++]=P[p][x++];
	if(w==x) y--;
      }
    }

    FREE2(REP,N-k);  /* libère l'arbre */
    REP=P;           /* REP vaut maintenant P */
    return N;
    }

  return arboricity(i,j); /* même fonction */
}


int halin(int i,int j){
/*
  REP[0][i] = père de i dans l'arbre. Les feuilles de cet arbre (sans
  sommet de degré deux) sont les sommets < p.
*/
  int p=PARAM[0]; /* nb de feuilles */

  if(j<0){
    if(j==-1) free(REP[0]);
    return 0;
  }
  if(i<0){
    if(p<3) return N=0;
    ALLOCMAT(REP,1,(p<<1)-1,int); /* REP[0] = 1 tableau d'au plus 2p-2 entiers */

    /* Principe du calcul de l'arbre à p feuilles et sans sommets de
       degré deux.

       On construit l'arbre à partir des feuilles. Soit A le tableau
       des sommets actifs, de taille au plus p. Au départ
       A=[0,1,...,p-1] est l'ensemble des feuilles. Tant que |A|>2 on
       répète la procédure suivante. On tire un tableau aléatoire R de
       taille |A|<=p entiers >=0 dont la somme fait |A| et possédant
       au moins une valeur > 1. Un peu comme Dick(), mais on commence
       par tirer une position dans R que l'on fixe à 2.

       Ensuite on parcoure R pour construire le prochain tableau B de
       sommet actif, le prochain A (en fait on met les nouveaux
       sommets directement au début de A). Les indices k et u sont
       ceux de R et de A respecteviement. Si R[k]=0, alors on passe
       simplement à k+1 sans rien faire d'autre. Si R[k]=1, on ajoute
       A[u] dans le tableau B et on passe au prochain sommet de A
       (u++). Le père du sommet A[u] n'est alors toujours pas fixé. Si
       enfin R[k]>1, alors on créer un nouveau sommet v que l'on
       ajoute à B, et les R[k] sommets A[u]...A[u+R[k]-1] on alors
       pour père v. On pose ensuite B=A et on met à jour la taille de
       A.

       Lorsque |A|<=2 on en déduit le nombre de sommets N. On fixe
       alors le sommet A[0] n'a pas de père (=-1). Si on termine avec
       |A|=2 alors le père de A[1] est fixé à A[0].
    */

    int *P=REP[0]; /* raccourci pour REP[0] */
    int *A,*R,t;
    ALLOCZ(A,p,int,_i); /* tableau des sommets actifs */
    ALLOC(R,p,int); /* tableau des valeurs random */
    int u; /* indice dans A du prochain sommet actif */
    int q; /* indice dans A du prochain nouveau sommet actif, q<=u */
    int k; /* indice dans R, k>=u */
    int a; /* taille de A, a<=p */
    N=a=p; /* nb courant de sommets dans le graphe, N>=p */

    while(a>2){
      for(k=0;k<a;R[k++]=0); /* tableau R à zéro */
      R[random()%a]=2; /* met un "2" quelque part */
      for(k=2;k<a;k++) R[random()%a]++; /* incrémente a-2 fois des positions de R */
      for(k=u=q=0;k<a;k++){ /* parcoure les valeurs de R */
	if(R[k]==0) continue;
	if(R[k]==1) { A[q++]=A[u++]; continue; }
	t=u+R[k]; /* ici t>=2 */
	for(;u<t;u++) P[A[u]]=N; /* père de A[u]=nouveau sommet */
	A[q++]=N++; /* un sommet de plus, et un nouveau actif de plus */
      }
      a=q; /* nouvelle taille de A=nb de nouveaux sommets actifs */
    }

    P[A[0]]=-1;
    if(a==2) P[A[1]]=A[0];

    free(A);
    free(R);
    REALLOC(P,N,int); /* recalibrage du tableau REP[0] */

    return N;
  }

  if((i<p)&&(j<p)) return ((j==((i+1)%p))||(i==((j+1)%p))); /* cycle */
  return ((REP[0][i]==j)||(REP[0][j]==i)); /* arbre */
}


int permutation(int i,int j){
/*
  REP[0][i] = permutation du sommet i.
*/
  if(j<0){
    if(j==-1) free(REP[0]);
    if(j==-2) sprintf(NAME,"(%i,%i)",i,REP[0][i]);
    return 0;
  }
  if(i<0){
    int u;
    N=PARAM[0];
    ALLOCMAT(REP,1,N,int); /* REP[0] = 1 tableau de N entiers */
    /* Génère dans une permutation aléatoire de [0,N[ */
    for(u=0;u<N;u++) REP[0][u]=u;
    Permute(REP[0],N);
    return N;
  }

  return ((i-REP[0][i])*(j-REP[0][j])<0);
}


int interval(int i,int j){
/*
  A chaque sommet i correspond un intervalle [a,b] de [0,2N[, avec
  a=REP[i][0] et b=REP[i][1].
*/
  int k,x,m;

  if(j<0){
    if(j==-1) FREE2(REP,N);
    if(j==-2) sprintf(NAME,"[%i,%i]",REP[i][0],REP[i][1]);
    return 0;
  }
  if(i<0){
    N=PARAM[0];
    m=(N<<1);

    /* génère un intervalle REP[k] pour k, [a,b] dans [0,2N[ avec a<=b */
    ALLOCMAT(REP,N,2,int);
    for(k=0;k<N;k++){
      x=random()%m;
      REP[k][0]=x;
      REP[k][1]=x+random()%(m-x);
    }
    return N;
  }

  return ( ((REP[i][0]<=REP[j][0])&&(REP[j][1]<=REP[i][1])) || ((REP[j][0]<=REP[i][0])&&(REP[i][1]<=REP[j][1])) );
}


int sat(int i,int j){
  /*
    [0,2n[: les variables positives et négatives
    [2n+i*k,2n+(i+1)*k[: clause numéro i (i=0 ... PARAM[1])
   */
  int n,k;

  if(j<0) return 0;
  if(i<0){
    n=2*PARAM[0];
    N=n+PARAM[1]*PARAM[2];
    ALLOCMAT(REP,N,1,int);

    /* chaque sommet-clause est connecté à une variable (positive ou négative) */
    for(k=n;k<N;k++) REP[k][0]=random()%n;

    return N;
  }

  if(i>j) { k=i; i=j; j=k; }
  /* maintenant i<j */
  n=2*PARAM[0];
  k=PARAM[2];

  if(j<n) return ((j==i+1)&&(j&1)); /* i-j et j impaire */
  if(i>=n) return (j-i<=k); /* dans la même clique ? */
  return (REP[j][0]==i);
}


int gpetersen(int i,int j){
/*
  u_i dans [0,n[ et v_i dans [n,2n[, N=2n.
  Utilise le fait que i<j
*/
  int n,r;

  if(j<0){
    if(j==-2) sprintf(NAME,"%c%i",(i<N/2)?'u':'v',i%(N/2));
    return 0;
  }
  n=PARAM[0];
  if(i<0){
    N=2*n;
    return N;
  }

  /* u_i-v_i, j>i */
  if(j==(i+n)) return 1;

  /* sinon, par d'arête entre u_i et v_j */
  if((i<n)&&(n<=j)) return 0;

  /* u_i-u_{i+1 mod n}, ici i<j<n */
  if(i<n) return (j==(i+1)%n)||(i==(j+1)%n);

  /* v_i-v_{i+r mod n} */
  r=PARAM[1];
  i -= n;
  j -= n;
  /* ici i,j<n mais j<i possible*/
  return (j==(i+r)%n)||(i==(j+r)%n);
}


int kneser(int i,int j){
/*
  REP[i][0...k-1] sont les ensembles représentant les sommets
*/
  int n,k,r,v,x,y;

  k=PARAM[1];

  if(j<0){
    if(j==-1) FREE2(REP,N);
    if(j==-2) NAME_Vector(REP[i],k,",","{}",1,"%i");
    return 0;
  }

  r=PARAM[2];

  if(i<0){
    int *S;
    N=Binom(n=PARAM[0],k); /* on a besoin de N pour allouer REP */
    ALLOCMAT(REP,N,k,int); /* on connaît N */
    if(k<1) return N=1; /* si k=0, alors N=1, puis return N */
    ALLOC(S,k,int);
    NextSet(S,-1,k); /* premier sous-ensemble */
    for(x=0;x<N;x++){ /* pour tous les sommets x du graphe */
      for(y=0;y<k;y++) REP[x][y]=S[y]; /* copie dans REP[x] */
      NextSet(S,n,k); /* sous-ensemble suivant */
    }
    free(S);

    return N;
  }

  /*
    Calcule si l'intersection possède au plus r éléments. L'algorithme
    ici est en O(k) en utilisant le fait que les éléments de REP sont
    rangés dans un ordre croissant.
   */

  /* v=nb d'élements commun */
  v=x=y=0; /* indices pour i et j */

  while((v<=r)&&(x<k)&&(y<k)){
    if(REP[i][x]==REP[j][y]) {
      v++; x++; y++;
    }
    else
      if(REP[i][x]<REP[j][y]) x++; else y++;
  }

  return (v<=r);
}


int rig(int i,int j){
/*
  REP[i][0]=taille t_i de l'ensemble associé au sommet i
  REP[i][1...t_i]=ensemble associé au sommet i
*/
  int x,y,t,k;
  double p;

  if(j<0){
    if(j==-1) FREE2(REP,N);
    if(j==-2) NAME_Vector(REP[i]+1,REP[i][0],",","{}",1,"%i");
    return 0;
  }

  k=PARAM[1];
  p=DPARAM[0];

  if(i<0){
    int *S;
    N=PARAM[0];
    //if((N<1)||(k<0)) return 0; /* graphe vide dans ce cas */
    ALLOC(S,k+1,int); /* ensemble S[1...k] temporaire pour un sommet */
    ALLOC(REP,N,int*);
    for(x=0;x<N;x++){ /* pour chaque sommet x */
      t=0; for(y=1;y<=k;y++) if(RAND01<p) S[++t]=y; /* t=taille de S */
      ALLOC(REP[x],t+1,int); /* espace pour le sommet x */
      REP[x][0]=t; /* écrit S dans REP[x][1...t] */
      for(y=1;y<=t;y++) REP[x][y]=S[y];
    }
    free(S);
    return N;
  }

  /*
    Détermine si l'intersection de REP[i][1...] et REP[j][1...] est
    vide ou pas.  L'algorithme utilise le fait que les éléments de REP
    sont rangés dans un ordre croissant.
   */

  x=y=1; /* indices pour les ensemble de i et j */
  k=REP[i][0]; /* taille de l'ensemble de i */
  t=REP[i][0]; /* taille de l'ensemble de j */

  while((x<=k)&&(y<=t)){
    if(REP[i][x]==REP[j][y]) return 1;
    if(REP[i][x]<REP[j][y]) x++; else y++;
  }

  return 0;
}


int kout(int i,int j)
/*
  REP[i]=tableau des voisins de i. Si REP[i][j]<0, alors c'est la fin
  de la liste.  Attention ! Si i<j, alors REP[i] ne peut pas contenir
  le sommet j (qui a été crée après i). Pour le test d'adjacence, il
  faut donc tester si i est dans REP[j], et pas le contraire !
*/
{
  if(j<0) return kautz(0,-1);

  int k=PARAM[1],x,y;

  if(i<0){
    int r,d,z;
    int *T;
    
    N=PARAM[0];
    ALLOCMAT(REP,N,k,int);
    ALLOC(T,N,int);

    REP[0][0]=-1;     /* le sommet 0 est seul !*/
    for(x=1;x<N;x++){ /* x=prochain sommet à rajouter */
      r=(x<k)?x:k;    /* le degré de x sera au plus r=min{x,k}>0 */
      d=1+random()%r; /* choisir un degré d pour x: d=[1,...,r] */
      for(y=0;y<x;y++) T[y]=y; /* tableau des voisins possibles */
      r=x;              /* r-1=index du dernier élément de T */
      for(y=0;y<d;y++){ /* choisir d voisins dans T */
	z=random()%r;   /* on choisit un voisin parmi ceux qui restent */
        REP[x][y]=T[z]; /* le y-ème voisin de x est T[z] */
        T[z]=T[--r];    /* enlève T[z], met le dernier élément de T à sa place */
      }
      if(d<k) REP[x][d]=-1; /* arrête la liste des voisins de x */
    }

    free(T);
    return N;
  }

  if(i>j) { x=i; i=j; j=x; }
  /* maintenant i<j, donc j a été crée après i */
  
  for(y=0;y<k;y++){
    if(REP[j][y]==i) return 1;
    if(REP[j][y]<0) return 0;
  }
  return 0;
}


/********************

  OPERATEURS UNAIRES

  C'est un graphe comme les autres (défini par la fonction d'adjacence
  adj), sauf qu'il prend comme paramètre un autre graphe (qui a pour
  fonction d'adjacence adj0). L'adjacence adj() se calcule donc en
  fonction de adj0(). En général, les opérateurs doivent mettre POS=0,
  sinon les positions XPOS/YPOS pourraient ne pas être définies pour
  certains indices.

  - star(i,j): ajoute des sommets de degré 1 à un graphe.

*********************/

adjacence *adj0; /* graphe d'origine pour l'opération star() */
int N0; /* nb de sommets du graphe initial */
int *tab_aux; /* tableau auxiliaire */

int simule(adjacence *f,int i,int j,int n){
/*
  Appelle la fonction d'adjacence f(i,j) sans modifier N

  N=nb de sommets du graphe courant
  f=fonction d'adjacence du graphe d'origine
  n=nb de sommet du graphe d'origine
*/
  int t,r;
  t=N; /* sauve le N courant */
  N=n; /* met le N du graphe initial, car f peut dépendre de cette valeur */
  r=f(i,j); /* calcul adj(i,j) */
  N=t; /* remet le N du graphe courant */
  return r; /* retourne la valeur */
}

int apex(int i,int j){
/*
  Les sommets de 0...N0-1 sont ceux du graphe initial. Ceux de numéro
  >= N0 sont les apices (donc de degré N0).  Se sert du fait que
  i<j. Désactive POS.
*/
  if(i<0){
    N0=simule(adj0,i,j,0); /* N0=N du graphe initial */
    N=N0+APEX; /* N=nb de sommets du graphe final, k=nb de sommets ajoutés */
    POS=0; /* il ne faut pas afficher les positions, cela n'a pas de sens */
    return N; /* valeur de retour pour star() */
  } 

  if(j<0){
    simule(adj0,i,j,N0);
    return 0;
  }

  if((i<N0)&&(j<N0)) return simule(adj0,i,j,N0);
  if((i>=N0)&&(j>=N0)) return 0; /* les sommets ajoutés ne sont pas voisins */
  return 1; /* ici i<N0<=j, j est un apex */
}


int star(int i,int j){
/*
  Les sommets de 0...N0-1 sont ceux du graphe initial. Ceux de numéro
  >= N0 sont ceux de degré 1.  Se sert du fait que i<j. Désactive POS.
*/
  if(i<0){
    int k,t;
    N0=simule(adj0,i,j,0); /* N0=N du graphe initial */
    k=STAR; if(k<0) k *= (-N0);
    N=N0+k; /* N=nb de sommets du graphe final, k=nb de sommets ajoutés */
    ALLOC(tab_aux,k,int); /* k=nb de sommets ajoutés */
    for(t=0;t<k;t++){
      if(STAR>0) tab_aux[t]=random()%N0;
      else tab_aux[t]=t/(-STAR);
    }
    POS=0; /* il ne faut pas afficher les positions, cela n'a pas de sens */
    return N; /* valeur de retour pour star() */
  } 

  if(j<0){
    simule(adj0,i,j,N0);
    if(j==-1) free(tab_aux);
    return 0; /* valeur de retour pour star() */
  }

  if((i<N0)&&(j<N0)) return simule(adj0,i,j,N0);

  if((i>=N0)&&(j>=N0)) return 0; /* les sommets ajoutés ne sont pas voisins */

  /* ici i<N0<=j */
  return (tab_aux[j-N0]==i);
}


/***********************************

           FONCTIONS DU PROGRAMME
               PRINCIPAL

***********************************/

int InitVertex(int n){
/*
  Remplit le tableau V[i] donnant l'étiquette finale du sommet i et
  supprime les sommets suivant la valeur DELV. Utilise aussi SHIFT.
  Si PERMUTE est vrai V[] est remplit d'une permutation aléatoire de
  SHIFT+[0,n[. Si V[i]=-1 cela signifie que i a été supprimé (DELV).
  La fonction retourne le nombre de sommets final du graphe,
  c'est-à-dire le nombre d'étiquettes >=0. Si k sommets ont été
  supprimés, alors les valeurs de V[] sont dans SHIFT+[0,n-k[.

  Initialise aussi le tableau VF[j], avec j=0...n-k, de sorte que
  VF[j]=i si VF[i] est le j-ème sommet non supprimé. Il est important
  que VF[] ait une taille de N au départ. Un realloc() le
  redimensionne plus tard dans la fonction.
*/

  int i,j,k,r; /* r=n-(nb de sommets supprimés) */
  long seuil;

  /* supprime les sommets */
  if(DELV<0.0){ /* ici on en supprime exactement |DELV| sommets */
    if(DELV<-(double)N) DELV=-(double)N;
    for(i=0;i<n;i++) VF[i]=i; /* on va se servir temporairement de VF */
    r=-(int)DELV; /* les r premières valeurs de VF seront les sommets à supprimer */
    for(i=0;i<r;i++){
      j=i+(random()%(n-i));
      SWAP(VF[i],VF[j],k);
    }
    for(i=0;i<r;i++) V[VF[i]]=-1; /* on supprime ces r sommets */
    for(i=r=0;i<n;i++) /* on remplit V et VF */
      if(V[i]>=0) { VF[r]=i; V[i]=r++; }
  }
  else{ /* ici on supprime chaque sommet avec proba DELV */
    seuil=(double)DELV*(double)RAND_MAX;
    for(i=r=0;i<n;i++)
      if(random()<seuil) V[i]=-1;
      else { VF[r]=i; V[i]=r++; }
  } /* dans les deux cas, r=nb de sommets restant */

  /* réajuste le tableau VF à la taille minimum */
  VF=REALLOC(VF,r,int);

  if(PERMUTE) Permute(V,n);

  /* ne rien faire si SHIFT=0 */
  if(SHIFT>0)
    for(i=0;i<r;i++)
      V[VF[i]] += SHIFT;

  return r;
}

void ScanINC(int *dmax,int *dmin,int *m){
/*
  Calcule, en fonction des tableaux INC[] et VF[], le degré max, le
  degré min et le nombre d'arêtes du graphe (final).
*/
  int d,i;
  *m=*dmax=0;
  *dmin=INT_MAX;
  for(i=0;i<NF;i++){
    d=INC[VF[i]]; /* d=degré du i-ème sommet existant */
    *m += d;
    if(d>*dmax) *dmax=d;
    if(d<*dmin) *dmin=d;
  }
  *m >>= 1;
  return;
}

void Header(int c){
/*
  Affiche et calcule le préambule (date, nom du graphe, nb de
  sommets). Suivant la valeur de c, le nb d'arêtes est donnée.  Si
  bit-0 de c=1, alors l'entête de base est affichée.  Si bit-1 de c=1,
  alors on affiche le nb d'arêtes, le degré min et max.

*/
  int i;

  /* affichage de la date, nom du graphe, ligne de commande et de n */
  if(c&1){
    system("printf '//\n// ' ; date");
    printf("// gengraph");
    for(i=1;i<ARGC;i++) printf(" %s",ARGV[i]);
    printf("\n");
    printf("// n=%i",NF);
  }

  /* affichage du nb_edges, maxdeg et mindeg */
  if(c&2){
    int maxdeg,mindeg,nbedges;
    ScanINC(&maxdeg,&mindeg,&nbedges);
    if(!(c&1)) printf("//\n//");
    printf(" m=%i",nbedges);
    printf(" maxdeg=%i",maxdeg);
    printf(" mindeg=%i",mindeg);
  }

  if(c) printf("\n//\n");
  return;
}

char *ComputeNAME(int i){
/*
  Fonction servant deux fois dans Out(i,j).  On retourne le pointeur
  NAME après avoir modifier son contenu. Dans le cas du format dot, il
  faut laisser les indices, les noms sont affichés à la fin.
*/
  sprintf(NAME,"%i",(V==NULL)?i:V[i]); /* par défaut NAME="V[i]" */
  if((FORMAT==F_standard)&&(LABEL==1)) adj(i,-2); /* NAME = nom de i */
  if(strlen(NAME)>NAMEMAX) Erreur(17);
  return NAME;
}

void Out(int i,int j){
/*
  Affiche l'arête i-j suivant le format FORMAT.

  Si i<0 et j<0, alors la fonction Out() est initialisée.
  Si i<0, alors c'est la terminaison de la fonction Out().
  Si j<0, alors affiche i seul (sommet isolé).

  Si HEADER=1, alors Out() doit se charger d'afficher l'en-tête. Si
  CHECK<>0 alors elle doit aussi se charger de créer et de remplir la
  liste d'adjacence du graphe GF et de son nombre de sommets NF.
*/
  int x,y,z;
  double w;
  char fmt[15]="%lf %lf\n"; /* format de précision par défaut pour
			       l'affichage de XPOS/YPOS dans le format
			       F_xy */

  /*-----------------------
    Initialise la fonction
  -------------------------*/
  if((i<0)&&(j<0)){
    if(CHECK) L0=L1=L2=Insert(NULL,0,0); /* initialise la liste */

    switch(FORMAT){

    case F_standard:
      if(HEADER) Header(1);
      STRsep1="";
      STRsep2=" ";
      STRedge=(DIRECTED)?"->":"-";
      return;

    case F_dot:
      if(HEADER) Header(1);
      printf("%sgraph {\n",(DIRECTED)?"di":"");
      if(VCOLOR&0x10){
	printf("\tgraph [layout=nop, splines=line];\n");
	printf("\tnode [height=1.0, width=0.4, style=filled, shape=rectangle];\n");
	return;
      }
      if(POS){
	w=PosAspect();
	/* layout=nop: pour dire à dot de ne pas re-calculer les positions */
	printf("graph [layout=nop, splines=line, bb=\"%.2lf,%.2lf,%.2lf,%.2lf\"];\n",
	       w*XMIN-2*VSIZEK*VSIZEXY,w*YMIN-2*VSIZEK*VSIZEXY,
	       w*XMAX+2*VSIZEK*VSIZEXY,w*YMAX+2*VSIZEK*VSIZEXY);
	/* si on ne met pas le "2*" on a des sommets tronqués ... */
	if(XYgrid){ /* affiche éventuellement une grille, avant le
		       graphe pour qu'elle apparaisse en arière-plan */
	  double rx,ry;
	  if(XYgrid<0){
	    if(XYtype==3) XYgrid=N; /* si -xy permutation */
	    else XYgrid=1+(int)(sqrt((double)N));
	  }
	  printf("\nsubgraph {\n");
	  printf(" node [label=\"\", height=0, width=0, color=gray];\n");
	  printf(" edge [color=gray];");
	  z=N; /* premier sommet de la grille, qui doit être après ceux de G */
	  rx=(double)(XMAX-XMIN)/(double)(XYgrid-1); /* pas de la grille en X */
	  ry=(double)(YMAX-YMIN)/(double)(XYgrid-1); /* pas de la grille en Y */
	  for(y=0;y<XYgrid;y++){
	      for(x=0;x<XYgrid;x++){
		/* affiche le sommet courant (x,y) ainsi que deux
		   arêtes incidentes vers les voisins (x+1,y) et
		   (x,y+1), s'ils existent */
		printf("\n %i [pos=\"%lf,%lf\"];",z,
		       w*(XMIN+(double)x*rx),w*(YMIN+(double)y*ry));
		if(x+1<XYgrid) printf(" %i--%i;",z,z+1); /* arête vers (x+1,y) */
		if(y+1<XYgrid) printf(" %i--%i;",z,z+XYgrid); /* arête vers (x,y+1) */
		z++; /* prochain sommet */
	      }
	    }
	  printf("\n}\n\n");
	}
      }
      printf("node [");
      if(LABEL==0) printf("label=\"\", shape=point, "); /* sans label */
      w=POS?VSIZESTD:VSIZEXY; /* taille des sommets */
      printf("height=%.2lf, width=%.2lf];\n",w,w);
      if(DOTFILTER=="neato") printf("edge [len=%.2lf]\n",LEN);
      STRsep1=";";
      STRsep2="; ";
      STRedge=(DIRECTED)?"->":"--";
    }
    return;
  }

  /*--------------------
    Termine la fonction
  ----------------------*/
  if(i<0){
    if(CHECK){
      free(L1); /* supprime le dernier élément de L0 */
      if(L0==L1) GF=new_graph(0);
      else{
	L2->next=NULL; /* coupe la liste à l'avant dernier élément */
	GF=List2Graph(L0,4); /* initialise GF et NF */
      }
      NF=GF->n;
    }

    switch(FORMAT){

    case F_standard:
    case F_dot:
      if((VCOLOR&0x10)==0){ /* court-circuite l'affichage du graphe si "-vcolor list" */
	if(LIGNE>0) printf("%s\n",STRsep1); /* newline si fini avant la fin de ligne */
	if(FORMAT==F_standard){
	  if(HEADER) Header(2);
	  return;
	}

	if(POS||(LABEL>0)){
	  w=PosAspect();
	  printf("\n");
	  for(y=0;y<NF;y++){
	    x=VF[y]; /* le sommet x existe */
	    printf("%i [",V[x]);
	    if(POS) /* Note: XPOS/YPOS existent forcément si POS=1 */
	      printf("pos=\"%lf,%lf\"",w*XPOS[x],w*YPOS[x]);
	    if(LABEL>0){
	      VIDE(NAME); /* on vide NAME avant de le calculer */
	      if(LABEL==1){
		adj(x,-2); /* calcule le nom dans NAME */
		if(strlen(NAME)>NAMEMAX) Erreur(17);
	      }
	      printf("%slabel=\"%s\"",(POS?", ":""),(NONVIDE(NAME)? NAME : "\\N"));
	    }
	    printf("];\n");
	  }
	}
	
	if(VSIZE&&(NF>0)){ /* taille en fonction du degré */
	  double alpha,smin;
	  ScanINC(&x,&z,&y); /* x=degmax, z=degmin */
	  smin=POS?VSIZESTD:VSIZEXY;
	  alpha=(x==z)? 0.0 : smin*(VSIZEK-1)/((double)(x-z));
	  printf("\n");
	  for(y=0;y<NF;y++){
	    x=VF[y]; /* le sommet x existe */
	    w=smin + alpha*(INC[x]-z);
	    printf("%i [height=%.2lf, width=%.2lf];\n",V[x],w,w);
	  }
	}
      }

      if(VCOLOR&&(NF>0)){ /* couleur des sommets */
	color c={0,0,0},*P; /* couleur noire par défaut */
	int *D;

	if(VCOLOR&0x8){ /* on initialise la PALETTE */
	  NPAL=strlen(SPARAM);
	  if(NPAL==1) { /* SPARAM=x alors SPARAM=xx */
	    SPARAM[1]=SPARAM[0];
	    SPARAM[2]='\0';
	    NPAL=2;
	  }
	  ALLOC(PALETTE,NPAL,color);
	  for(y=z=0;y<NPAL;y++){ /* z=prochaine entrée libre dans PALETTE */
	    x=index(COLORCHAR,SPARAM[y])-COLORCHAR; /* x=index de la couleur */
	    if((0<=x)&&(x<COLORNB)) PALETTE[z++]=COLORBASE[x]; /* on a trouvé la couleur */
	  }
	  if(z<2){ PALETTE[0]=PALETTE[1]=c; z=2; } /* si pas trouvé 2 couleurs */
	  NPAL=z; /* NPAL=nb de couleurs trouvées */
	}

	if(VCOLOR&0x17){ /* fonction de couleur: 1,2,3,4,5 ou "-vcolor list" */
	  if((VCOLOR&0x7)>2){ /* si "degm", "randg" ou "kcolor" */
	    if((VCOLOR&0x7)==3){
	      int *T=Prune(GF,NULL); /* si "degm" */
	      D=GreedyColor(GF,T); /* calcule les couleurs selon l'ordre T */
	      y=1+GF->int1; /* y=nb de couleurs */
	      free(T); /* on a plus besoin de T */
	    }
	    if((VCOLOR&0x7)==4){ /* si "randg" */
              int *T;
	      ALLOCZ(T,NF,int,_i);
	      Permute(T,NF); /* T=permutation aléatoire */
	      D=GreedyColor(GF,T); /* calcule les couleurs selon l'ordre T */
	      y=1+GF->int1; /* y=nb de couleurs */
	      free(T); /* on a plus besoin de T */
	    }
	    if((VCOLOR&0x7)==5){ /* si "kcolor" */
              y=MEM(CPARAM,0,int); /* y=nb de couleur */
	      D=kColor(GF,y);
              if(D==NULL){ ALLOCZ(D,NF,int,0); y=1; } /* une seule couleur */
	    }
	  }
	  else{
	    ScanINC(&x,&z,&y); /* calcule x=degmax, z=degmin */
	    y=x-z+1; /* nb a priori de couleurs nécessaires */
	    ALLOCZ(D,NF,int,INC[VF[_i]]-z);
	    if((VCOLOR&0x7)==2){ /* si "degr" */
	      int *R=SortInt(D,NULL,N,0,&y,SORT_RANKe);
	      /* après SortInt(), y=nb de degré différents */
	      free(D);
	      D=R; /* on substitue R à D */
	    }
	  }
	  /* ici D[x]=indice de la couleur du sommet x, et y nb de couleurs */
	  P=GradColor(PALETTE,NPAL,y); /* calcule un palette P de y couleurs. NB: ici NPAL>1 */
	  printf("\n");
	  if(VCOLOR&0x10){ /* si "-vcolor list" */
	    char s[6],cc;
	    for(x=0;x<y;x++){
	      c=P[x];
	      for(z=0;z<COLORNB;z++) /* on cherche c dans COLORBASE */
		if((COLORBASE[z].r==c.r)&&(COLORBASE[z].g==c.g)&&(COLORBASE[z].b==c.b)) break; 
	      cc=(z<COLORNB)? COLORCHAR[z] : ' ';
	      if((c.r+c.g+c.b)<150) strcpy(s,"white"); else strcpy(s,"black");
	      printf("\t%i [pos=\"%i,0\", color=\"#%.02x%.02x%.02x\", label=\"%c\", fontcolor=%s];\n",x,10+28*x,c.r,c.g,c.b,cc,s);
	    }
	  }else{ /* si pas "-vcolor list" */
	    for(x=0;x<NF;x++){
	      c=P[D[x]]; /* c=couleur du degré (ou du rang) de x */
	      printf("%i [style=filled, fillcolor=\"#%02x%02x%02x\"];\n",V[VF[x]],c.r,c.g,c.b);
	    }
	  }

	  free(D);
	  free(P);
	}
	
	if(VCOLOR&0x8) free(PALETTE);
      }

      printf("}\n"); /* si F_dot on ferme le "}" */
      if(HEADER) Header(2); /* affiche les arêtes */
      return;
      
    case F_no:
      if(HEADER) Header(3);
      return;

    case F_list:
      if(HEADER) Header(3);
      PrintGraphList(GF);
      return;
      
    case F_matrix:
    case F_smatrix:
      if(HEADER) Header(3);
      PrintGraphMatrix(GF);
      return;

    case F_xy:
      if(HEADER) Header(3);
      if((XPOS==NULL)||(YPOS==NULL)) Erreur(8);
      printf("%i\n",NF); /* nombre de sommets final */
      if(ROUND<10){ /* construit le nouveau format */
	strcpy(fmt,"%.00lf %.00lf\n");
	fmt[3]=fmt[10]=(char)('0'+MAX(ROUND,0)); /* si ROUND<0 -> 0 */
      }
      for(y=0;y<NF;y++){
	x=VF[y]; /* le sommet x existe */
	printf(fmt,XPOS[x],YPOS[x]);
      }
      return;

    default: Erreur(5); /* normalement sert à rien */
    }
  }

  /*-----------------------------------------
    Affichage à la volée: "i-j", "i" ou "-j"
  -------------------------------------------*/

  if(CHECK){
    L1=Insert(L2=L1,i,0);   /* on ajoute i */
    if(j>=0) L1=Insert(L2=L1,j,(DIRECTED)?4:1); /* on ajoute ->j ou -j */
    if(VCOLOR&0x10) return; /* ne rien faire d'autre si "-vcolor
			       list". NB: CHECK>0 dans ce cas */
  }

  if((FORMAT==F_standard)||(FORMAT==F_dot)){
    if(j<0) LAST=-1; /* sommet isolé, donc LAST sera différent de i */
    if(i!=LAST) printf("%s%s",(LIGNE==0)? "" : STRsep2,ComputeNAME(i));
    LAST=j;   /* si i=LAST, alors affiche ...-j. Si j<0 alors LAST<0 */
    if(j>=0) printf("%s%s",STRedge,ComputeNAME(j));
    if(++LIGNE==WIDTH){
      LIGNE=0; LAST=-1;
      printf("%s\n",STRsep1);
    }
  } /* si format matrix, smatrix etc., ne rien faire (c'est fait à la fin: i<0) */

  return;
}


double CheckProba(double v){
/*
  Retourne une probabilité entre [0,1].
*/
  if(v<0.0) return 0.0;
  if(v>1.0) return 1.0;
  return v;
}


void Grep(char *mot)
/*
  Cherche "^....mot" dans l'aide contenu dans le source du programme,
  puis termine par exit().
*/
{
  char *s;
  ALLOC(s,CMDMAX,char);
  strcat(s,"sed -n '/*[#] ###/,/### #/p' ");
  strcat(strcat(s,*ARGV),".c|");
  /* rem: sed -E n'est pas standard ... */
  strcat(strcat(strcat(s,"sed -nE '/^[.][.][.][.]"),mot),"(($)|( ))/,/^$/p'|");
  strcat(s,"sed 's/^[.][.][.][.]/    /g'");
  strcat(s,"| awk '{n++;print $0;}END{if(!n) ");
  strcat(s,"print\"Erreur : argument incorrect.\\nAide sur '");
  strcat(s,strcat(mot,"' non trouvée.\\n\";}'"));
  printf("\n");
  system(s);
  free(s);
  exit(EXIT_SUCCESS);  
}


char *NextArg(int *i)
/*
  Retourne ARGV[i] s'il existe puis incrémente i.  Si l'argument
  n'existe pas, on affiche l'aide en ligne sur le dernier argument
  demandé et on s'arrête.
*/
{
  if(((*i)==ARGC)||(strcmp(ARGV[*i],"?")==0)) Grep(ARGV[(*i)-1]);
  return ARGV[(*i)++];
}


void CheckHelp(int *i)
/*
  Incrémente i puis vérifie si le prochain argument est "?". Si tel
  est le cas, une aide est affichée. Cette fonction devrait être
  appellée lorsqu'on vérifie l'aide pour un graphe sans
  paramètre. Sinon c'est fait par NextArg().
*/
{
  (*i)++;
  if(((*i)!=ARGC)&&(strcmp(ARGV[*i],"?")==0)) Grep(ARGV[(*i)-1]);
  return;
}


void Help(int i)
/*
  Affiche l'aide complète.
*/
{
  char *s;
  ALLOC(s,CMDMAX,char);

  strcat(s,"sed -n '/*[#] ###/,/### #/p' ");
  strcat(strcat(s,*ARGV),".c | ");
  strcat(s,"sed -e 's/\\/\\*[#] ###//g' -e 's/### [#]\\*\\///g' ");
    
  if((++i==ARGC)||(ARGV[i][0]=='?')) strcat(s,"-e 's/^[.][.][.][.][.]*/    /g'|more");
  else{
    strcat(s,"|awk 'BEGIN{x=\".....\"}/^[.][.][.][.]./{x=$0} /");
    strcat(strcat(s,ARGV[i]),"/{if(x!=\".....\")print substr(x,5)}'|sort -u");
  }

  system(s);
  free(s);
  exit(EXIT_SUCCESS);
}


void ListGraph(void){
/*
  Affiche les graphes possibles et puis termine.
*/
  char *s;

  ALLOC(s,CMDMAX,char);
  strcat(s,"sed -n '/*[#] ###/,/### #/p' ");
  strcat(strcat(s,*ARGV),".c| ");
  strcat(s,"sed -e 's/\\/\\*[#] ###//g' -e 's/### [#]\\*\\///g'| ");
  strcat(s,"grep '^[.][.][.][.][^-.]'| sed 's/^[.][.][.][.]//g'");
  system(s);
  free(s);
  exit(EXIT_SUCCESS);
}


void PipeDot(int j){
/*
  Gère l'option "-format dot<xxx>"
  Ici on a: ARGV[j]="dot<xxx>"

  Rem: on pourrait utiliser popen() au lieu de réécrire la ligne de
  commande et de lancer system()
*/
  int i;
  char *s;
  char type[5];

  CheckHelp(&j);j--;
  strcpy(type,ARGV[j]+3); /* type=<xxx> */
  ARGV[j][3]='\0'; /* ARGV[j]="dot" plutôt que "dot<xxx>" */

  /*
    On réécrit la ligne de commande:
    1. en remplaçant -format dot<xxx> par -format dot
    2. puis en ajoutant à la fin: | dot -T<xxx> -K <filter> ...
  */
  
  ALLOC(s,CMDMAX,char);
  for(i=0;i<ARGC;i++) strcat(strcat(s,ARGV[i])," ");
  strcat(strcat(strcat(strcat(s,"| dot -T"),type)," -K "),DOTFILTER);
  system(s);
  free(s);
  exit(EXIT_SUCCESS);
}


void Visu(int j){
/*
  Gère l'option "-visu"
  Ici on a: ARGV[j]="-visu"
*/
  int i;
  char *s;

  CheckHelp(&j);j--;

  /* on reconstruit dans s la ligne de commande */
  ALLOC(s,CMDMAX,char);VIDE(s);
  for(i=0;i<j;i++) strcat(strcat(s,ARGV[i])," ");
  strcat(s,"-format dot ");
  i++; /* saute "-visu" */
  for(;i<ARGC;i++) strcat(strcat(s,ARGV[i])," ");
  strcat(strcat(strcat(strcat(s,"| dot -Tpdf -K "),DOTFILTER)," -o "),GRAPHPDF);
  system(s);
  free(s);
  exit(EXIT_SUCCESS);
}


void MainCC(int j){
/*
  Gère l'option "-maincc"
  Ici on a: ARGV[j]="-maincc"

  On insère dans la ligne de commande "-check maincc -format no |
  ./gengraph load -" à la place de "-maincc". Au lieu de terminer avec
  system() il faudrait mieux redécouper (Split) les arguments (ARGV et
  ARGC) et continuer l'analyse pour n'avoir qu'un seul appel système.
*/
  int i;
  char *s;

  CheckHelp(&j);j--;

  /* on reconstruit dans s la ligne de commande */
  ALLOC(s,CMDMAX,char);VIDE(s);
  for(i=0;i<j;i++) strcat(strcat(s,ARGV[i])," ");
  strcat(s,"-check maincc -format no ");
  i++; /* saute "-maincc" */
  for(;i<ARGC;i++) strcat(strcat(s,ARGV[i])," ");
  strcat(s,"| ./gengraph load -");
  system(s);
  free(s);
  exit(EXIT_SUCCESS);
}


/***********************************

           OPTIONS -ALGO

***********************************/


void PrintMorphism(char *s,int *P,int n)
/*
  Affiche le tableau P de n éléments sous la forme:
  i0->j0   i1->j1 ...
  i8->j8   i9->j8 ...
*/
{
  const int k=8; /* nb de "->" affichés par ligne */
  int i;
  printf("%s",s); /* normalement printf(s) est ok, mais Warning sur certain system */
  for(i=0;i<n;i++)
    printf("%i->%i%s",i,P[i],(((i%k)<(k-1))&&(i<n-1))?"\t":"\n");
  return;
}


/***********************************

           FONCTIONS DE TESTS
           POUR -FILTER

***********************************/


int ftest_minus(graph *G)
/*
  Retourne VRAI ssi G n'est pas dans la famille F, c'est-à-dire G
  n'est isomorphe à aucun graphe de F.
*/
{
  graph *F=MEM(FPARAM,0,graph*);
  if(F==NULL) return (G==NULL);

  int i,*P;
  for(i=0;i<F->f;i++){
    P=Isomorphism(G,F->G[i]);
    free(P);
    if(P!=NULL) return 0;
  }
  
  return 1;
}


int ftest_minus_id(graph *G)
/*
  Retourne VRAI ssi F2 ne contient aucun graphe d'identifiant égale à
  celui de G. La complexité est en O(log|F2|). Il est important que F2
  soit triée par ordre croissant des ID.
*/
{
  graph *F=MEM(FPARAM,0,graph*);
  if(F==NULL) return 0;
  return (bsearch(&G,F->G,F->f,sizeof(graph*),fcmp_graphid)==NULL);
}


int ftest_unique(graph *G)
/*
  Retourne VRAI ssi la sous-famille F allant des indices i+1 à F->f ne contient
  pas G, où i=MEM(FPARAM,0,int).

  Effet de bord: MEM(FPARAM,0,int) est incrémenté.
*/
{
  int i = (MEM(FPARAM,0,int) += 1);
  int *P;

  for(;i<FAMILY->f;i++){
    P=Isomorphism(G,FAMILY->G[i]);
    free(P);
    if(P!=NULL) return 0;
  }

  return 1;
}


int ftest_minor(graph *G)
/*
  Retourne VRAI ssi H est mineur de G.
*/
{
  int *T=Minor(MEM(FPARAM,0,graph*),G);
  if(T==NULL) return 0;
  free(T);
  return 1;
}


int ftest_minor_inv(graph *G)
{
  int *T=Minor(G,MEM(FPARAM,0,graph*));
  if(T==NULL) return 0;
  free(T);
  return 1;
}


int ftest_sub(graph *G)
/*
  Retourne VRAI ssi H est sous-graphe de G avec même nb de sommets.
*/
{
  graph *C=Subgraph(MEM(FPARAM,0,graph*),G);
  if(C==NULL) return 0;
  free_graph(C);
  return 1;
}


int ftest_sub_inv(graph *G)
{
  graph *C=Subgraph(G,MEM(FPARAM,0,graph*));
  if(C==NULL) return 0;
  free_graph(C);
  return 1;
}


int ftest_isub(graph *G)
/*
  Retourne VRAI ssi H est sous-graphe induit de G.
*/
{
  int *C=InducedSubgraph(MEM(FPARAM,0,graph*),G);
  if(C==NULL) return 0;
  free(C);
  return 1;
}


int ftest_isub_inv(graph *G)
{
  int *C=InducedSubgraph(G,MEM(FPARAM,0,graph*));
  if(C==NULL) return 0;
  free(C);
  return 1;
}


int ftest_iso(graph *G)
/*
  Retourne VRAI ssi H est isomorphe à G. Aucun intérêt de faire
  programmer iso-inv.
*/
{
  int *T=Isomorphism(MEM(FPARAM,0,graph*),G);
  if(T==NULL) return 0;
  free(T);
  return 1;
}


int ftest_id(graph *G)
{ return InRange(G->id,FPARAM); }


int ftest_vertex(graph *G)
{ return InRange(G->n,FPARAM); }


int ftest_edge(graph *G)
{ return InRange(NbEdges(G),FPARAM); }


int ftest_degmax(graph *G)
{ return InRange(Degree(G,1),FPARAM); }


int ftest_degmin(graph *G)
{ return InRange(Degree(G,0),FPARAM); }


int ftest_deg(graph *G)
{ int u,b=1,n=G->n;
  for(u=0;(u<n)&&(b);u++) b=InRange(G->d[u],FPARAM);
  return b;
}


int ftest_degenerate(graph *G)
{
  int x;
  int *T=Prune(G,&x);
  free(T);
  return InRange(x,FPARAM);
}


int ftest_gcolor(graph *G)
{
  int *T=Prune(G,NULL);
  int *C=GreedyColor(G,T);
  free(T);
  free(C);
  return InRange(1+G->int1,FPARAM);
}


int ftest_component(graph *G)
{
  param_dfs *p=dfs(G,0,NULL);
  int c=p->nc; /* nb de cc */
  free_param_dfs(p);
  return InRange(c,FPARAM);
}


int ftest_forest(graph *G)
{
  param_dfs *p=dfs(G,0,NULL);
  int c=p->nc; /* nb de cc */
  free_param_dfs(p);
  return InRange(c,FPARAM)&&(NbEdges(G)==G->n-c);
}


int ftest_cutvertex(graph *G)
{
  param_dfs *p=dfs(G,0,NULL);
  int x=p->na;
  free_param_dfs(p);
  return InRange(x,FPARAM);
}


int ftest_biconnected(graph *G)
{
  param_dfs *p=dfs(G,0,NULL);
  int b=(p->nc==1)&&(p->na==0)&&(G->n>2);
  free_param_dfs(p);
  return b;
}


int ftest_ps1xx(graph *G, int version)
{
  path *P=new_path(G,NULL,G->n);
  int v=PS1(G,P,version);
  free_path(P);
  return v;
}


int ftest_ps1(graph *G) { return ftest_ps1xx(G,0); }
int ftest_ps1b(graph *G) { return ftest_ps1xx(G,1); }
int ftest_ps1c(graph *G) { return ftest_ps1xx(G,2); }
int ftest_ps1x(graph *G) { return ftest_ps1xx(G,3); }

int ftest_radius(graph *G)
{
  param_bfs *p;
  int x=G->n;
  int u;
  for(u=0;u<G->n;u++){
    p=bfs(G,u,NULL);
    if(x>=0){
      if(p->nc<G->n) x=-1;
      else x=MIN(x,p->rad);
    }
    free_param_bfs(p);
  }
  return InRange(x,FPARAM);
}


int ftest_girth(graph *G)
{
  param_bfs *p;
  int x=1+G->n;
  int u;
  for(u=0;u<G->n;u++){
    p=bfs(G,u,NULL);
    if(p->girth>0) x=MIN(x,p->girth);
    free_param_bfs(p);
  }
  if(x>G->n) x=-1;
  return InRange(x,FPARAM);
}


int ftest_diameter(graph *G)
{
  param_bfs *p;
  int x=-1;
  int u;
  for(u=0;u<G->n;u++){
    p=bfs(G,u,NULL);
    if(p->nc==G->n) x=MAX(x,p->rad);
    free_param_bfs(p);
  }
  return InRange(x,FPARAM);
}


int ftest_hyperbol(graph *G)
{
  param_bfs *p;
  int **D;
  int n=G->n,h=0;
  int u,v,x,y,d1,d2,d3,t;
  ALLOC(D,n,int*);

  /* calcule la matrice de distances */
  for(u=0;u<n;u++){
    p=bfs(G,u,NULL);
    D[u]=p->D;
    if(p->nc<n){ /* remplace -1 par +infini */
      for(v=0;v<n;v++) if(p->D[v]<0) p->D[v]=INT_MAX;
    }
    p->D=NULL;
    free(p); /* efface p, mais pas p->D */
  }

  /* pour tous les quadruplets {u,v,x,y} */
  for(u=0;u<n;u++)
    for(v=u+1;v<n;v++)
      for(x=v+1;x<n;x++)
	for(y=x+1;y<n;y++){
	  d1=D[u][v]+D[x][y];
	  d2=D[u][x]+D[v][y];
	  d3=D[u][y]+D[v][x];
	  if(d1<d2) SWAP(d1,d2,t);
	  if(d1<d3) SWAP(d1,d3,t);
	  if(d2<d3) d2=d3; /* on se fiche de d3 */
	  if(d1-d2>h) h=d1-d2;
	}

  FREE2(D,n); /* efface la matrice de distances */
  if(h==INT_MAX) h=-1; /* cela ne peut jamais arriver */
  return InRange(h,FPARAM);
}


int ftest_tw(graph *G)
{ return InRange(Treewidth(G,1),FPARAM); }


int ftest_tw2(graph *G)
{ return (Treewidth(G,0)<=2); }


int ftest_rename(graph *G)
{
  G->id=SHIFT++;
  return 1;
}


graph *Filter(graph *F,test *f,int code){
  /*
    Etant donnée une famille de graphes et une fonction de test f,
    renvoie une sous-famille de graphes G de F telle que f(G) est
    vraie (si code=0) ou faux (si code=1). Attention! si on libère F,
    cela détruit la sous-famille renvoyée.

    Effet de bord: si PVALUE est vrai, alors dans les graphes filtrés
    G on met dans G->int1 la valeur du paramètre, CVALUE.
  */
  if((F==NULL)||(F->f==0)) return NULL;
  int i,j,n=F->f;
  graph *R;

  R=new_graph(0);
  ALLOC(R->G,n,graph*); /* a priori R est de même taille que F */

  for(i=j=0;i<n;i++){
    if(f(F->G[i])^code){
      R->G[j++]=F->G[i];
      if(PVALUE) F->G[i]->int1=CVALUE;
    }
  }

  R->G=REALLOC(R->G,j,graph*);
  R->f=j;
  return R;
}


graph *Graph2Family(graph *G){
/*
  Renvoie une famille composée d'un seul graphe G.
  Effet de bord: met G->id=0.
*/
  graph *F=new_graph(0);
  ALLOC(F->G,1,graph*);
  F->G[0]=G;
  F->f=1;
  G->id=0;
  return F;
}


void ApplyFilter(int code,int index)
/*
  Applique le filtre FTEST (avec le code=0 ou 1) à la famille de
  graphes FAMILY (voir un graphe seul), et affiche la famille
  résultante. On affiche aussi le nombre de graphes obtenus et la
  ligne de commande (sous forme de commentaire). Si index>=0, alors
  ARGV[index] donne le paramètre.

  Effet de bord: FAMILY est libérée.
*/
{
  graph *R;
  int i;

  if(FAMILY->f==0) FAMILY=Graph2Family(FAMILY); /* transforme en famille si graphe simple */
  R=Filter(FAMILY,FTEST,code); /* calcule la sous-famille */

  printf("// #graphs: %i\n// generated by:",(R==NULL)?0:R->f);
  for(i=0;i<ARGC;i++) printf(" %s",ARGV[i]);
  printf("\n");
  if((index>=0)&&(PVALUE)) /* on affiche la valeur */
    for(i=0;i<R->f;i++)
      printf("[%i] %s: %i\n",R->G[i]->id,ARGV[index],R->G[i]->int1);
  else PrintGraph(R); /* on affiche le graphe */

  /* ! aux free() de famille de graphes ! */
  free_graph(FAMILY); /* libère en premier la famille */
  free(R->G); /* libère la sous-famille */
  free(R);
  return;
}


/***********************************

               MAIN

***********************************/


int main(int argc, char *argv[]){

  int i,j,k,noredirect;
  long seuil,seuilr;

  ARGC=argc;
  ARGV=argv;

  if(ARGC==1) Help(0);   /* aide si aucun argument */

  /* valeurs par défaut */

  adj=ring;
  PARAM[0]=0;
  VIDE(NAME); /* chaîne vide */
  seuil=(long)getpid(); /* seuil par défaut */
  i=1;

  /*****************************************

           ANALYSE DE LA LIGNE DE COMMANDE

    Il faut éviter de faire des traitements trop coûteux en temps dans
    l'analyse de la ligne de commande car on peut être amené à refaire
    cette analyse une deuxième fois à cause de -visu et à cause de la
    réécriture de la ligne de commande.

  *****************************************/

  while(i<ARGC){

    if (EQUAL("-help")||EQUAL("?")) Help(i);
    if EQUAL("-list"){ CheckHelp(&i); ListGraph(); }
    j=i;

    /* les options */

    if EQUAL("-permute"){ PERMUTE=1; goto param0; }
    if EQUAL("-not"){ NOT=1-NOT; goto param0; }
    if EQUAL("-undirected"){ DIRECTED=0; LOOP=0; goto param0; }
    if EQUAL("-directed"){ DIRECTED=1; LOOP=1; goto param0; }
    if EQUAL("-noloop"){ LOOP=0; goto param0; }
    if EQUAL("-loop"){ LOOP=1; goto param0; }
    if EQUAL("-header"){ HEADER=1; goto param0; }
    if EQUAL("-visu") Visu(i);     /* se termine par exit() */
    if EQUAL("-maincc") MainCC(i); /* se termine par exit() */
    if EQUAL("-vsize"){ VSIZE=1; goto param0; }
    if EQUAL("-seed"){ i++;
	seuil=(long)STRTOI(NextArg(&i));
	srandom(seuil);
	goto fin;
      }
    if EQUAL("-width"){ i++;
	WIDTH=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("-shift"){ i++;
	SHIFT=STRTOI(NextArg(&i));
	if(SHIFT<0) Erreur(6);
	goto fin;
      }
    if EQUAL("-pos"){ i++;
	POS=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("-label"){ i++;
	LABEL=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("-dotfilter"){ i++;
	DOTFILTER=NextArg(&i); /* pointe sur le nom du filtre */
	goto fin;
      }
    if EQUAL("-len"){ i++;
	LEN=STRTOD(NextArg(&i));
	goto fin;
      }
    if EQUAL("-norm"){ i++;
	NORM=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("-delv"){ i++;
	DELV=STRTOD(NextArg(&i));
	if(DELV>1.0) DELV=1.0;
	goto fin;
      }
    if EQUAL("-dele"){ i++;
	DELE=CheckProba(STRTOD(NextArg(&i)));
	goto fin;
      }
    if EQUAL("-redirect"){ i++;
	REDIRECT=CheckProba(STRTOD(NextArg(&i)));
	goto fin;
      }
    if EQUAL("-star"){ i++;
	STAR=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("-apex"){ i++;
	APEX=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("-vcolor"){ i++;NextArg(&i);i--; /* pour l'aide en ligne */
	/* bits 0-2: codage de la fonction de couleur=1,2,3,4,5
	   bit 3: "pal"
	   bit 4: "list" */
	if EQUAL("deg"){ i++; VCOLOR=(VCOLOR|0x7)^0x7; /* efface les 3 derniers bits */
	    VCOLOR |= 1; goto fin; }
	if EQUAL("degr"){ i++; VCOLOR=(VCOLOR|0x7)^0x7;
	    VCOLOR |= 2; goto fin; }
	if EQUAL("degm"){ i++; VCOLOR=(VCOLOR|0x7)^0x7;
	    VCOLOR |= 3; CHECK |= 1; goto fin; }
	if EQUAL("randg"){ i++; VCOLOR=(VCOLOR|0x7)^0x7;
	    VCOLOR |= 4; CHECK |= 1; goto fin; }
	if EQUAL("kcolor"){ i++; VCOLOR=(VCOLOR|0x7)^0x7;
	    VCOLOR |= 5; CHECK |= 1;
	    if(CPARAM==NULL) ALLOC(CPARAM,PARAMSIZE,void*);
	    MEM(CPARAM,0,int)=STRTOI(NextArg(&i)); /* CPARAM[0]=ARGV[i] */
	    goto fin; }
	if EQUAL("pal"){ i++;
	    VCOLOR |= 0x8; /* set bit-3 */
	    strcpy(SPARAM,NextArg(&i)); /* SPARAM=ARGV[i] */
	    goto fin;
	  }
	if EQUAL("list"){ i++;
	    VCOLOR |= 0x10;
	    CHECK |= 1;
	    FORMAT = F_dot;
	    goto fin; }
	Erreur(9);
      }
    if EQUAL("-format"){ i++;NextArg(&i);i--; /* pour l'aide en ligne */
	FORMAT=-1; /* pour savoir si on a trouvé le FORMAT */
	if EQUAL("standard") FORMAT=F_standard;
	if EQUAL("matrix") { FORMAT=F_matrix; CHECK |= 1;}
	if EQUAL("smatrix"){ FORMAT=F_smatrix;CHECK |= 1;}
	if EQUAL("list")   { FORMAT=F_list; CHECK |= 1;}
	if EQUAL("xy")       FORMAT=F_xy;
	if EQUAL("no")       FORMAT=F_no;
	if PREFIX("dot"){ /* si "dot" ou "dot<xxx>" */
	    if EQUAL("dot") FORMAT=F_dot; /* si "dot" seul */
	    else PipeDot(i); /* se termine par exit() */
	  }
	if(FORMAT<0) Erreur(5); /* le format n'a pas été trouvé */
	i++;
	goto fin;
      }
    if EQUAL("-xy"){ i++;NextArg(&i);i--; /* pour l'aide en ligne */
	POS=1; /* il faut POS=1 */
	if EQUAL("load"){ i++;
	    FILEXY=NextArg(&i); /* pointe sur le nom du fichier */
	    goto fin;
	  }
	if EQUAL("noise"){ i++;
	    NOISEr=STRTOD(NextArg(&i));
	    NOISEp=STRTOD(NextArg(&i));
	    goto fin;
	  }
	if EQUAL("scale"){ i++;
	    SCALEX=STRTOD(NextArg(&i));
	    SCALEY=STRTOD(NextArg(&i));
	    goto fin;
	  }
	if EQUAL("seed"){ i++;
	    SEEDk=abs(STRTOI(NextArg(&i)));
	    SEEDp=STRTOD(NextArg(&i));
	    XYtype=2;
	    goto fin;
	  }
	if EQUAL("round"){ i++;
	    ROUND=STRTOI(NextArg(&i));
	    goto fin;
	  }
	if EQUAL("permutation"){ CheckHelp(&i);
	    ROUND=0; /* a priori coordonnées entières dans ce cas */
	    XYtype=3;
	    goto fin;
	  }
	if EQUAL("unique"){ CheckHelp(&i);
	    XYunique=1;
	    goto fin;
	  }
	if EQUAL("grid"){ CheckHelp(&i);
	    XYgrid=STRTOI(NextArg(&i));
	    goto fin;
	  }
	if EQUAL("vsize"){ CheckHelp(&i);
	    XYvsize=STRTOD(NextArg(&i));
	    goto fin;
	  }
	Erreur(1); /* l'option après -xy n'a pas été trouvée */
      }
    if EQUAL("-algo"){ i++;NextArg(&i);i--; /* pour l'aide en ligne */
	Erreur(11); /* l'option après -algo n'a pas été trouvée */
      }
    if EQUAL("-filter"){ i++;NextArg(&i);i--; /* pour l'aide en ligne */
	FAMILY=File2Graph(NextArg(&i),2); /* lit une famille ou un graphe */
	if(FPARAM==NULL) ALLOC(FPARAM,PARAMSIZE,void*);
	NextArg(&i);i--; /* vérifie s'il y a bien un autre argument */
	PVALUE=0; /* par défaut, on affiche pas "value" mais les graphes */
	if EQUAL("not"){ k=1; i++;NextArg(&i);i--; } else k=0;
	 /* vérifie s'il y a bien un autre argument */
	if EQUAL("rename"){
	    FTEST=ftest_rename;
	    i++;
	    SHIFT=STRTOI(NextArg(&i));
	    ApplyFilter(0,-1);
	    goto fin;
	  }
	if EQUAL("biconnected"){
	    FTEST=ftest_biconnected;
	  filter0:
	    i++;
	    ApplyFilter(k,-1);
	    goto fin;
	  }
	if EQUAL("id"){
	    FTEST=ftest_id;
	  filter1:
	    i++;
	    ReadRange(NextArg(&i),FPARAM);
	    ApplyFilter(k,i-2);
	    goto fin;
	  }
	int c; /* code pour File2Graph() */
	if EQUAL("minor"){
	    FTEST=ftest_minor;
	    c=34; /* détection du shift et charge toujours un graphe */
	  filter2:
	    i++;
	    MEM(FPARAM,0,graph*)=File2Graph(NextArg(&i),c);
	    ApplyFilter(k,-1);
	    free_graph(MEM(FPARAM,0,graph*));
	    goto fin;
	  }

	if EQUAL("ps1x"){ i++;
	    FTEST=ftest_ps1x;
	    MEM(CPARAM,0,int)=STRTOI(NextArg(&i));
	    for(c=MEM(CPARAM,0,int);c>=1;c--){ /* met les arguments à l'envers */
	      MEM(CPARAM,(2*c-1)*sizeof(int),int)=STRTOI(NextArg(&i));
	      MEM(CPARAM,(2*c)*sizeof(int),int)=STRTOI(NextArg(&i));
	    }
	    i--;
	    goto filter0;
	  }
	if EQUAL("ps1"){ FTEST=ftest_ps1; goto filter0; }
	if EQUAL("ps1b"){ FTEST=ftest_ps1b; goto filter0; }
	if EQUAL("ps1c"){ FTEST=ftest_ps1c; goto filter0; }
	if EQUAL("tw2"){ FTEST=ftest_tw2; goto filter0; }
	if EQUAL("unique"){ FTEST=ftest_unique; MEM(FPARAM,0,int)=0; goto filter0; }
	if EQUAL("vertex"){ FTEST=ftest_vertex; goto filter1; }
	if(EQUAL("edge")||EQUAL("edges")){ FTEST=ftest_edge; goto filter1; }
	if EQUAL("deg"){ FTEST=ftest_deg; goto filter1; }
	if EQUAL("degenerate"){ FTEST=ftest_degenerate; goto filter1; }
	if EQUAL("degmax"){ FTEST=ftest_degmax; goto filter1; }
	if EQUAL("degmin"){ FTEST=ftest_degmin; goto filter1; }
	if EQUAL("gcolor"){ FTEST=ftest_gcolor; goto filter1; }
	if EQUAL("component"){ FTEST=ftest_component; goto filter1; }
	if EQUAL("cut-vertex"){ FTEST=ftest_cutvertex; goto filter1; }
	if EQUAL("radius"){ FTEST=ftest_radius; goto filter1; }
	if EQUAL("girth"){ FTEST=ftest_girth; goto filter1; }
	if EQUAL("diameter"){ FTEST=ftest_diameter; goto filter1; }
	if EQUAL("hyperbol"){ FTEST=ftest_hyperbol; goto filter1; }
	if EQUAL("tw"){ FTEST=ftest_tw; goto filter1; }
	if EQUAL("forest"){ FTEST=ftest_forest; goto filter1; }
	if EQUAL("minor-inv"){ FTEST=ftest_minor_inv; c=34; goto filter2; }
	if EQUAL("sub"){ FTEST=ftest_sub; c=34; goto filter2; }
	if EQUAL("sub-inv"){ FTEST=ftest_sub_inv; c=34; goto filter2; }
	if EQUAL("isub"){ FTEST=ftest_isub; c=34; goto filter2; }
	if EQUAL("isub-inv"){ FTEST=ftest_isub_inv; c=34; goto filter2; }
	if EQUAL("iso"){ FTEST=ftest_iso; c=34; goto filter2; }
	if EQUAL("minus"){ FTEST=ftest_minus; c=2; goto filter2; }
	if EQUAL("minus-id"){ FTEST=ftest_minus_id; c=10; goto filter2; }
	/* alias */
	if EQUAL("connected"){ FTEST=ftest_component;
	    ReadRange("1",FPARAM); goto filter0; }
	if EQUAL("bipartite"){ FTEST=ftest_gcolor;
	    ReadRange("<3",FPARAM); goto filter0; }
	if EQUAL("isforest"){ FTEST=ftest_forest;
	    ReadRange("t",FPARAM); goto filter0; }
	if EQUAL("istree"){ FTEST=ftest_forest;
	    ReadRange("1",FPARAM); goto filter0; }
	if EQUAL("cycle"){ FTEST=ftest_forest;
	    k=1-k; ReadRange("t",FPARAM); goto filter0; }
	if EQUAL("all"){ FTEST=ftest_vertex;
	    ReadRange("t",FPARAM); goto filter0; }
	Erreur(14); /* l'option après -filter n'a pas été trouvée */
      }
    if EQUAL("-check"){ i++;NextArg(&i);i--; /* pour l'aide en ligne */
	if(CPARAM==NULL) ALLOC(CPARAM,PARAMSIZE,void*);
	if EQUAL("bfs"){
	    CHECK=CHECK_BFS;
	  check0:
	    i++;
	    MEM(CPARAM,0,int)=STRTOI(NextArg(&i));
	    goto fin;
	  }
	if EQUAL("iso"){
	    CHECK=CHECK_ISO;
	  check1:
	    MEM(CPARAM,0,int)=++i;
	    NextArg(&i); /* pour vérifier si i existe */
	    goto fin;
	  }
	if(EQUAL("deg")||EQUAL("edge")||EQUAL("edges")){
	  CHECK=CHECK_DEG;
	check2:
	  i++;
	  goto fin;
	}
	if EQUAL("paths"){
	    CHECK=CHECK_PATHS;
	  check3:
	    i++;
	    MEM(CPARAM,0,int)=STRTOI(NextArg(&i));
	    MEM(CPARAM,sizeof(int),int)=STRTOI(NextArg(&i));
	    goto fin;
	  }
	if EQUAL("ps1x"){ i++;
	    CHECK=CHECK_PS1x;
	    MEM(CPARAM,0,int)=STRTOI(NextArg(&i));
	    for(k=MEM(CPARAM,0,int);k>=1;k--){ /* met les arguments à l'envers */
	      MEM(CPARAM,(2*k-1)*sizeof(int),int)=STRTOI(NextArg(&i));
	      MEM(CPARAM,(2*k)*sizeof(int),int)=STRTOI(NextArg(&i));
	    }
	    goto fin;
	  }
	if EQUAL("dfs"){ CHECK=CHECK_DFS; goto check0; }
	if EQUAL("bellman"){ CHECK=CHECK_BELLMAN; goto check0; }
	if EQUAL("kcolor"){ CHECK=CHECK_KCOLOR; goto check0; }
	if EQUAL("kcolorsat"){ CHECK=CHECK_KCOLORSAT; FORMAT=F_no; goto check0; }
	if EQUAL("sub"){ CHECK=CHECK_SUB; goto check1; }
	if EQUAL("isub"){ CHECK=CHECK_ISUB; goto check1; }
	if EQUAL("minor"){ CHECK=CHECK_MINOR; goto check1; }
	if EQUAL("degenerate"){ CHECK=CHECK_DEGENERATE; goto check2; }
	if EQUAL("gcolor"){ CHECK=CHECK_GCOLOR; goto check2; }
	if EQUAL("ps1"){ CHECK=CHECK_PS1; goto check2; }
	if EQUAL("ps1b"){ CHECK=CHECK_PS1b; goto check2; }
	if EQUAL("ps1c"){ CHECK=CHECK_PS1c; goto check2; }
	if EQUAL("twdeg"){ CHECK=CHECK_TWDEG; goto check2; }
	if EQUAL("tw"){ CHECK=CHECK_TW; goto check2; }
	if EQUAL("girth"){ CHECK=CHECK_GIRTH; goto check2; }
	if EQUAL("maincc"){ CHECK=CHECK_MAINCC; goto check2; }
	if EQUAL("paths2"){ CHECK=CHECK_PATHS2; goto check3; }
	Erreur(12); /* l'option après -check n'a pas été trouvée */
      }

    /* graphes de base */

    if EQUAL("tutte"){
	adj=tutte;
      param0:
	CheckHelp(&i);
	goto fin;
      }
    if EQUAL("prime"){
	adj=prime;
      param1:
	i++;
	PARAM[0]=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("arboricity"){
	adj=arboricity;
      param2:
	i++;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("sat"){
	adj=sat;
      param3:
	i++;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=STRTOI(NextArg(&i));
	PARAM[2]=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("ktree"){
	PARAM[2]=0;
      gotoktree:
	i++;
	adj=ktree;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=STRTOI(NextArg(&i));
	if(PARAM[0]<=PARAM[1]) Erreur(6);
	goto fin;
      }
    if EQUAL("kpath"){ PARAM[2]=1; goto gotoktree; }
    if EQUAL("hypercube"){
	adj=grid;
      gotohypercube:
	i++;
	PARAM[0]=STRTOI(NextArg(&i));
	if(PARAM[0]+1>DIMAX) Erreur(4);
	for(k=1;k<=PARAM[0];k++) PARAM[k]=2;
	goto fin;
      }
    if EQUAL("ggosset"){
	adj=ggosset;
	i++;
	int k,d=0;
	PARAM[0]=STRTOI(NextArg(&i)); /* copie p */
	PARAM[1]=STRTOI(NextArg(&i)); /* copie k */
	for(k=0;k<(PARAM[1]<<1);k++) PARAM[k+3]=STRTOI(NextArg(&i));
	/* il est important de calculer la dimension d pour accélérer l'adjacence */
	for(k=0;k<PARAM[1];k++) d += PARAM[(k<<1)+3];
	PARAM[2]=d;
	goto fin;
      }
    if EQUAL("clebsch"){ adj=clebsch; goto gotohypercube; }
    if EQUAL("grid"){
	adj=grid;
      gotogrid:
	i++;
	PARAM[0]=STRTOI(NextArg(&i));
	if(PARAM[0]+1>DIMAX) Erreur(4);
	for(k=1;k<=PARAM[0];k++)
	  PARAM[k]=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("rpartite"){ adj=rpartite; goto gotogrid; }
    if EQUAL("aqua"){ adj=aqua; DIRECTED=1; goto gotogrid; }
    if EQUAL("ring"){
	adj=ring;
      gotoring:
	i++;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=STRTOI(NextArg(&i));
	if(PARAM[1]+2>DIMAX) Erreur(4);
	for(k=0;k<PARAM[1];k++)
	  PARAM[k+2]=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("cage"){ adj=cage; goto gotoring; }
    if EQUAL("kout"){ i++;
	adj=kout;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=STRTOI(NextArg(&i));
	if(PARAM[1]>=PARAM[0]-1) PARAM[1]=PARAM[0]-1;
	if(PARAM[1]<1) Erreur(6);
	goto fin;
      }
    if EQUAL("kneser"){ i++;
	adj=kneser;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=STRTOI(NextArg(&i));
	PARAM[2]=STRTOI(NextArg(&i));
	if(PARAM[1]>PARAM[0]) PARAM[1]=PARAM[0];
	goto fin;
      }
    if EQUAL("udg"){ i++;
	adj=udg; POS=1;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[0]=MAX(PARAM[0],1); /* n>0 */
	DPARAM[0]=STRTOD(NextArg(&i)); /* !!! lecture d'un double */
	if(DPARAM[0]<0.0) DPARAM[0]=sqrt(log((double)PARAM[0])/(double)PARAM[0]);
	/* Threshold théorique r0 (cf. [EMY07]):
	   pour n=10,    r0=0.4798
	   pour n=100,   r0=0.2145
	   pour n=1000,  r0=0.08311
	   pour n=10000, r0=0.03034
	 */
	DPARAM[0]=CheckProba(DPARAM[0]); /* proba */
	goto fin;
      }
    if EQUAL("rig"){ i++;
	adj=rig;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[0]=MAX(PARAM[0],1); /* n>0 */
	PARAM[1]=STRTOI(NextArg(&i));
	PARAM[1]=MAX(PARAM[1],1); /* k>0 */
	DPARAM[0]=STRTOD(NextArg(&i));
	if(DPARAM[0]<0.0){
	  DPARAM[0]=log((double)PARAM[0])/(double)PARAM[1];
	  if(PARAM[1]>PARAM[0]) DPARAM[0]=sqrt(DPARAM[0]/(double)PARAM[0]);
	}
	DPARAM[0]=CheckProba(DPARAM[0]); /* proba */
	goto fin;
      }
    if EQUAL("rplg"){ i++;
	adj=rplg;
	PARAM[0]=STRTOI(NextArg(&i));
	DPARAM[0]=STRTOD(NextArg(&i));
	goto fin;
      }
    if EQUAL("thetagone"){ i++;
	adj=thetagone; POS=1;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=STRTOI(NextArg(&i));
	PARAM[2]=STRTOI(NextArg(&i));
	DPARAM[0]=CheckProba(STRTOD(NextArg(&i))); /* !!! lecture d'un double */
	goto fin;
      }
    if PREFIX("load"){ i++;
	adj=load;
	strcpy(SPARAM,NextArg(&i)); /* SPARAM=ARGV[i] */
	goto fin;
      }
    if EQUAL("theta0"){ adj=theta0; goto param0; }
    if EQUAL("icosa"){ adj=icosa; goto param0; }
    if EQUAL("rdodeca"){ adj=rdodeca; goto param0; }
    if EQUAL("cubocta"){ adj=cubocta; goto param0; }
    if EQUAL("fritsch"){ adj=fritsch; goto param0; }
    if EQUAL("soifer"){ adj=soifer; goto param0; }
    if EQUAL("poussin"){ adj=poussin; goto param0; }
    if EQUAL("errara"){ adj=errara; goto param0; }
    if EQUAL("kittell"){ adj=kittell; goto param0; }
    if EQUAL("frucht"){ adj=frucht; goto param0; }
    if EQUAL("moser"){ adj=moser; goto param0; }
    if EQUAL("markstrom"){ adj=markstrom; goto param0; }
    if EQUAL("robertson"){ adj=robertson; goto param0; }
    if EQUAL("headwood4"){ adj=headwood4; goto param0; }
    if EQUAL("wiener_araya"){ adj=wiener_araya; goto param0; }
    if EQUAL("pstar"){ adj=pstar; PARAM[1]=2; goto param1; }
    if EQUAL("paley"){ adj=paley; goto param1; }
    if EQUAL("mycielski"){ adj=mycielski; goto param1; }
    if EQUAL("wheel"){ adj=wheel; goto param1; }
    if EQUAL("halin"){ adj=halin; goto param1; }
    if EQUAL("windmill"){ adj=windmill; goto param1; }
    if EQUAL("interval"){ adj=interval; goto param1; }
    if EQUAL("permutation"){ adj=permutation; goto param1; }
    if EQUAL("pancake"){ adj=pancake; goto param1; }
    if EQUAL("bpancake"){ adj=bpancake; goto param1; }
    if EQUAL("crown"){ adj=crown; goto param1; }
    if EQUAL("flower_snark"){ adj=flower_snark; goto param1; }
    if EQUAL("gabriel"){ adj=gabriel; POS=1; goto param1; }
    if EQUAL("rng"){ adj=rng; POS=1; goto param1; }
    if EQUAL("nng"){ adj=nng; POS=1; goto param1; }
    if EQUAL("gpetersen"){ adj=gpetersen; goto param2; }
    if EQUAL("butterfly"){ adj=butterfly; goto param2; }
    if EQUAL("debruijn"){ adj=debruijn; goto param2; }
    if EQUAL("kautz"){ adj=kautz; goto param2; }
    if EQUAL("gpstar"){ adj=gpstar; goto param2; }
    if EQUAL("hexagon"){ adj=hexagon; goto param2; }
    if EQUAL("whexagon"){ adj=whexagon; goto param2; }
    if EQUAL("hanoi"){ adj=hanoi; goto param2; }
    if EQUAL("sierpinski"){ adj=sierpinski; goto param2; }
    if EQUAL("kpage"){ adj=kpage; goto param2; }
    if EQUAL("rarytree"){ adj=rarytree; goto param2; }
    if EQUAL("line-graph"){ adj=linegraph; goto param2; }
    if EQUAL("linial"){ adj=linial; goto param2; }
    if EQUAL("linialc"){ adj=linialc; goto param2; }
    if EQUAL("arytree"){ adj=arytree; goto param3; }
    if EQUAL("barbell"){ adj=barbell; goto param3; }

    /* graphes composés */

    if EQUAL("tree"){ adj=arboricity; PARAM[1]=1; goto param1; }
    if EQUAL("cycle"){ adj=ring; PARAM[1]=PARAM[2]=1; goto param1; }
    if EQUAL("binary"){ adj=arytree; PARAM[1]=PARAM[2]=2; goto param1; }
    if EQUAL("rbinary"){ adj=rarytree; PARAM[1]=2; goto param1; }
    if EQUAL("stable"){ adj=ring; PARAM[1]=0; goto param1; }
    if EQUAL("clique"){ adj=ring; NOT=1-NOT; PARAM[1]=0; goto param1; }
    if EQUAL("outerplanar"){ adj=kpage; PARAM[1]=1; goto param1; }
    if EQUAL("prism"){ adj=gpetersen; PARAM[1]=1; goto param1; }
    if EQUAL("lollipop"){ adj=barbell; PARAM[2]=0; goto param2; }
    if EQUAL("td-delaunay"){
	adj=thetagone; POS=1;
	PARAM[1]=PARAM[2]=3;
	DPARAM[0]=1.0;
	goto param1;
      }
    if EQUAL("theta"){ i++;
	adj=thetagone; POS=1;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=3;
	PARAM[2]=STRTOI(NextArg(&i));
	DPARAM[0]=6.0/(double)PARAM[2];
	goto fin;
      }
    if EQUAL("dtheta"){ i++;
	adj=thetagone; POS=1;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=3;
	PARAM[2]=STRTOI(NextArg(&i))/2;
	DPARAM[0]=3.0/(double)PARAM[2];
	goto fin;
      }
    if EQUAL("yao"){ i++;
	adj=thetagone; POS=1;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=0;
	PARAM[2]=STRTOI(NextArg(&i));
	DPARAM[0]=2.0/(double)PARAM[2];
	goto fin;
      }
    if EQUAL("path"){ i++;
	adj=grid;
	PARAM[0]=1;
	PARAM[1]=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("torus"){ i++;
	adj=grid;
	PARAM[0]=2;
	PARAM[1]=-STRTOI(NextArg(&i));
	PARAM[2]=-STRTOI(NextArg(&i));
	goto fin;
    }
    if EQUAL("mesh"){ i++;
	adj=grid;
	PARAM[0]=2;
	PARAM[1]=STRTOI(NextArg(&i));
	PARAM[2]=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("mobius"){ i++;
	adj=ring;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=2;
	PARAM[2]=1;
	PARAM[3]=PARAM[0]/2;
	goto fin;
      }
    if EQUAL("star"){ i++;
	adj=rpartite;
	PARAM[0]=2;
	PARAM[1]=1;
	PARAM[2]=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("random"){ i++;
	adj=ring;
	NOT=1-NOT;
	PARAM[0]=STRTOI(NextArg(&i));
	DELE=1.0-STRTOD(NextArg(&i));
	PARAM[1]=0;
	goto fin;
      }
    if EQUAL("tw"){
	PARAM[2]=0;
      gototw:
	i++;
	adj=ktree;
	PARAM[0]=STRTOI(NextArg(&i));
	PARAM[1]=STRTOI(NextArg(&i));
	DELE=0.5;
	goto fin;
      }
    if EQUAL("pw"){
	PARAM[2]=1;
	goto gototw;
      }
    if EQUAL("bipartite"){ i++;
	adj=rpartite;
	PARAM[0]=2;
	PARAM[1]=STRTOI(NextArg(&i));
	PARAM[2]=STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("claw"){ i++;
	adj=rpartite;
	PARAM[0]=2;PARAM[1]=1;PARAM[2]=3;
	goto fin;
      }
    if EQUAL("cylinder"){ i++;
	adj=grid;
	PARAM[0]=2;
	PARAM[1]=STRTOI(NextArg(&i));
	PARAM[2]=-STRTOI(NextArg(&i));
	goto fin;
      }
    if EQUAL("caterpillar"){ i++;
	adj=grid;
	k=STRTOI(NextArg(&i)); /* nb de sommets total */
	srandom(seuil);  /* initialise le générateur aléatoire */
	STAR=random()%k; /* entre 0...k-1 sommets de deg=1. Active l'opération star() */
	PARAM[0]=1;
	PARAM[1]=k-STAR; /* PARAM[0]=nb de sommets du chemin, qui est >=1 */
	goto fin;
      }
    if EQUAL("sunlet"){ i++;
	adj=grid;
	PARAM[0]=1;
	PARAM[1]=-STRTOI(NextArg(&i));
	STAR=-1;
	goto fin;
      }

    /* graphes sans paramètres, donc commençant par CheckHelp() */

    if EQUAL("cube"){ CheckHelp(&i);
	adj=crown;
	PARAM[0]=4;
	goto fin;
      }
    if EQUAL("tietze"){ CheckHelp(&i);
	adj=flower_snark;
	PARAM[0]=3;
	goto fin;
      }
    if EQUAL("hajos"){ CheckHelp(&i);
	adj=sierpinski;
	PARAM[0]=2;
	PARAM[1]=3;
	goto fin;
      }
    if EQUAL("gosset"){ CheckHelp(&i);
	adj=ggosset;
	PARAM[0]=8;
	PARAM[1]=2;
	PARAM[2]=2;PARAM[3]=3;
	PARAM[4]=6;PARAM[5]=-1;
	goto fin;
      }
    if EQUAL("diamond"){ CheckHelp(&i);
	adj=cage;
	PARAM[0]=4;
	PARAM[1]=2;
	PARAM[2]=2;PARAM[3]=0;
	goto fin;
      }
    if EQUAL("headwood"){ CheckHelp(&i);
	adj=cage;
	PARAM[0]=14;
	PARAM[1]=2;
	PARAM[2]=5;PARAM[3]=-5;
	goto fin;
      }
    if EQUAL("franklin"){ CheckHelp(&i);
	adj=cage;
	PARAM[0]=12;
	PARAM[1]=2;
	PARAM[2]=5;PARAM[3]=-5;
	goto fin;
      }
    if EQUAL("pappus"){ CheckHelp(&i);
	adj=cage;
	PARAM[0]=18;
	PARAM[1]=6;
	PARAM[2]=5;PARAM[3]=7;PARAM[4]=-7;
	PARAM[5]=7;PARAM[6]=-7;PARAM[7]=-5;
	goto fin;
      }
    if EQUAL("mcgee"){ CheckHelp(&i);
	adj=cage;
	PARAM[0]=24;
	PARAM[1]=3;
	PARAM[2]=12;PARAM[3]=7;PARAM[4]=-7;
	goto fin;
      }
    if EQUAL("tutte-coexter"){ CheckHelp(&i);
	adj=cage;
	PARAM[0]=30;
	PARAM[1]=6;
	PARAM[2]=-7;PARAM[3]=9;PARAM[4]=13;
	PARAM[5]=-13;PARAM[6]=-9;PARAM[7]=7;
	goto fin;
      }
    if EQUAL("gray"){ CheckHelp(&i);
	adj=cage;
	PARAM[0]=54;
	PARAM[1]=6;
	PARAM[2]=7;PARAM[3]=-7;
	PARAM[4]=25;PARAM[5]=-25;
	PARAM[6]=13;PARAM[7]=-13;
	goto fin;
      }
    if EQUAL("chvatal"){ CheckHelp(&i);
	adj=cage;
	PARAM[0]=PARAM[1]=12;
	PARAM[2]=PARAM[4]=PARAM[7]=PARAM[10]=PARAM[12]=PARAM[13]=3;
	PARAM[3]=PARAM[5]=PARAM[6]=PARAM[8]=6;
	PARAM[9]=PARAM[11]=-3;
	goto fin;
      }
    if EQUAL("house"){ CheckHelp(&i);
	adj=grid;
	NOT=1-NOT;
	PARAM[0]=1;
	PARAM[1]=5;
	goto fin;
      }
    if EQUAL("wagner"){ CheckHelp(&i);
	adj=ring;
	PARAM[0]=8;
	PARAM[1]=2;
	PARAM[2]=1;
	PARAM[3]=4;
	goto fin;
      }
    if EQUAL("grotzsch"){ CheckHelp(&i);
	adj=mycielski;
	PARAM[0]=4;
	goto fin;
      }
    if EQUAL("nauru"){ CheckHelp(&i);
	adj=pstar;
	PARAM[0]=4;
	PARAM[1]=2; /* important ! sinon pstar ne marche pas */
	goto fin;
      }
    if EQUAL("petersen"){ CheckHelp(&i);
	adj=kneser;
	PARAM[0]=5;
	PARAM[1]=2;
	PARAM[2]=0;
	goto fin;
      }
    if EQUAL("desargues"){ CheckHelp(&i);
	adj=gpetersen;
	PARAM[0]=10;
	PARAM[1]=3;
	goto fin;
      }
    if EQUAL("durer"){ CheckHelp(&i);
	adj=gpetersen;
	PARAM[0]=6;
	PARAM[1]=2;
	goto fin;
      }
    if EQUAL("dodeca"){ CheckHelp(&i);
	adj=gpetersen;
	PARAM[0]=10;
	PARAM[1]=2;
	goto fin;
      }

  fin:
    if(j==i){
      if PREFIX("-") Erreur(2); /* option non trouvée */
      Erreur(10); /* graphe non trouvé */
    }
  }

  /***********************************

           COEUR DU GENERATEUR

  ***********************************/

  srandom(seuil); /* initialise le générateur aléatoire */
  if(STAR) { adj0=adj; adj=star; } /* le graphe est maintenant star() */
  if(APEX) { adj0=adj; adj=apex; } /* le graphe est maintenant apex() */
  if(STAR&&APEX) Erreur(18); /* on ne peut pas avoir les deux */
  adj(-1,0); /* initialisation du graphe, calcule N avant la suppression des sommets */
  if(N<0) N=0;

  if((FILEXY!=NULL)&&(XPOS==NULL)) InitXY(); /* il faut charger les positions */
  if((POS)&&(XPOS==NULL)) Erreur(8); /* il faut que XPOS contiennent des valeurs ! */
  if(LABEL==1) PERMUTE=0; /* si on souhaite les labels, on ne permute rien */

  ALLOC(V,N,int);    /* V[i]=étiquette du sommet i, -1 si i est supprimé */
  ALLOC(VF,N,int);   /* VF[j]=indice du j-ème sommet non supprimé */
  NF=InitVertex(N);  /* initialise V[i]/VF[i] et retourne NF=#sommets final */
  ALLOC(INC,N,int);  /* INC[i]=1 ssi i possède un voisin, 0 si sommet isolé */
  for(j=0;j<NF;j++) INC[VF[j]]=0;

  /* constantes pour accélérer les tests */
  seuil=(double)(1.0-DELE)*(double)RAND_MAX;
  seuilr=(double)(REDIRECT)*(double)RAND_MAX;
  noredirect=(REDIRECT==0.0);

  /*
    Génère les adjacences i-j en tenant compte des sommets isolés et
    des sommets supprimés. Les sommets isolés sont affichés en dernier
    à cause de l'option -redirect. On a toujours i<j lorsque l'arête
    i-j doit être sortie.
  */

  Out(-1,-1); /* initialise le format d'affichage */

  for(i=0;i<N;i++)       /* pour tous les i */
    if(V[i]>=0){         /* si i existe */
      for(j=(DIRECTED)?0:i+1-LOOP;j<N;j++) /* pour tous les j>i */
	if((V[j]>=0)&&((!DIRECTED)||(i!=j)||(LOOP))) /* si j existe ... */
	  if((random()<seuil)&&(adj(i,j)^NOT)){
	    /* ici l'arête i-j devrait être sortie */
	    if(noredirect){ /* pas de redirection d'arête */
	      INC[i]++;
	      INC[j]++;
	      Out(i,j);
	    }else{ /* on redirige l'arête i-j vers i-k */
	      k=(random()<seuilr)? random()%N : j;
	      if((V[k]>=0)&&(k!=i)){
		/* on affiche l'arête que si k existe et si k<>i */
		/* Attention ! il ne faut toucher ni à i ni à j */
		INC[i]++;
		INC[k]++;
		if(k<i) Out(k,i); else Out(i,k); /* pour avoir i<j */
	      }
	    }
	  }
    }

  /*
    Affiche les sommets isolés. Ils ne sont connus qu'à la fin à cause
    de l'option -redirect.
  */

  for(i=0;i<N;i++)
    if((V[i]>=0)&&(!INC[i])) Out(i,-1);

  Out(-1,0); /* fin de l'affichage, doit être fait AVANT adj(0,-1) */
  if((CHECK)&&(POS)){ /* mémorise XPOS,YPOS qui vont être supprimées par adj(0,-1) */
    ALLOC(GF->xpos,NF,double);
    ALLOC(GF->ypos,NF,double);
    for(i=0;i<NF;i++){
      GF->xpos[i]=XPOS[VF[i]];
      GF->ypos[i]=YPOS[VF[i]];
    }
  }
  adj(0,-1); /* libère éventuellement la mémoire utilisée pour adj() */
  adj=NULL; /* ici adj() n'est plus définie */
  free(V);
  free(VF);
  free(INC);

  /***********************************

           FIN DU GENERATEUR

           Le graphe est GF
           Le nombre de sommets est NF

  ***********************************/

  if(CHECK){
    if(CHECK==CHECK_MAINCC){
      param_dfs *p=dfs(GF,MEM(CPARAM,0,int),NULL);
      graph *C;
      int d0,d1,n,c;
      int *T;
      for(i=c=n=d0=d1=0;i<p->nc;i++){ /* détermine la plus grosse cc */
	/* d1-d0=nb de sommets de la cc numéro i */
	if(i+1<p->nc) d1=p->d[p->R[i+1]]; else d1=NF;
	if(d1-d0>n){ n=d1-d0; c=i; } /* n=nb de sommets de cc max */ 
	d0=d1;
      }
      c=p->C[p->R[c]]; /* c=couleur de la composante max */
      ALLOC(T,n,int); /* T=sommet de GF à garder */
      for(i=j=0;i<NF;i++) if(p->C[i]==c) T[j++]=i;
      free_param_dfs(p); /* p ne sert plus à rien */
      C=ExtractSubgraph(GF,T,n,1); /* construit le cc max */
      free(T); /* T ne sert plus à rien */
      CHECK=0; /* pour ne pas restocker le graphe */
      PrintGraph(C);
      free_graph(C);
    }
    if(CHECK==CHECK_BFS){
      param_bfs *p=bfs(GF,MEM(CPARAM,0,int),NULL);
      int t,c;
      printf("root=%i\n",p->root);
      printf("rad[%i]=%i\n",p->root,p->rad);
      printf("girth[%i]=%i\n",p->root,p->girth);
      printf("#vertices traversed=%i\n",p->nc);
      printf("distance:\n");
      for(k=1;k<=p->rad;k++){
	printf(" %i:",k);
	for(c=t=0;t<NF;t++) if(p->D[t]==k){ c++; printf(" %i",t); }
	printf("  (%i)\n",c);
      }
      printf("vertices not connected:");
      for(t=c=0;t<NF;t++) if(p->D[t]<0){ c++; printf(" %i",t); }
      if(c==0) printf(" none\n");
      else printf("  (%i)\n",c);
      printf("parent:");
      for(k=0;k<NF;k++) printf(" %i",p->P[k]);
      printf("\n");
      free_param_bfs(p);
    }
    if(CHECK==CHECK_DFS){
      param_dfs *p=dfs(GF,MEM(CPARAM,0,int),NULL);
      printf("source: %i\n",MEM(CPARAM,0,int));
      printf("#component: %i%s\n",p->nc,(p->nc==1)?" (connected)":"");
      printf("#cut-vertex: %i%s\n",p->na,((p->nc==1)&&(p->na==0)&&(NF>2))?" (biconnected)":"");
      printf("root:");
      for(i=0;i<p->nc;i++) printf(" %i",p->R[i]);
      if(p->na) printf("\ncut-vertex:");
      for(i=0;i<NF;i++) if(p->A[i]) printf(" %i",i);
      if(p->nc>1){
	printf("\ncomponent:");
	for(i=0;i<NF;i++) printf(" %i",p->C[i]);
      }
      printf("\nparent:");
      for(i=0;i<NF;i++)
	if(p->P[i]<0) printf(" -"); else printf(" %i",p->P[i]);
      printf("\n");
      free_param_dfs(p);
    }
    if(CHECK==CHECK_BELLMAN){
      if(!WeightGraph(GF))
	printf("Seulement possible pour les graphes géométriques !\n");
      else{
	double dx,dy,s=-1.0;
	param_bellman *p=Bellman_Ford(GF,MEM(CPARAM,0,int));
	int u=p->source;
	printf("source=%i\n",u);
	for(k=0;k<NF;k++){
	  dx=GF->xpos[p->source]-GF->xpos[k];
	  dy=GF->ypos[p->source]-GF->ypos[k];
	  dx=sqrt(dx*dx+dy*dy);
	  dy=(p->source!=k)? p->dist[k]/dx : 0.0;
	  if(dy>s){ s=dy; u=k; }
	  printf("dist[%i]=%lf parent=%i stretch=%lf (%lf)\n",k,p->dist[k],p->pere[k],dy,dx);
	}
	printf("stretch max: %lf\n",s);
	while(u!=p->source){
	  printf("%i->",u);
	  u=p->pere[u];
	}
	printf("%i\n",u);
	free_param_bellman(p);
      }
    }
    if(CHECK==CHECK_DEG){
      int *F;
      printf("#edges: %i\n",NbEdges(GF));
      printf("degmin: %i degmax: %i\n",Degree(GF,0),Degree(GF,1));
      k=NF;
      F=SortInt(GF->d,NULL,NF,0,&k,SORT_FREQv);
      printf("deg:");
      for(k=0;k<NF;k++) if(F[k]) printf(" %i (x%i) ",k,F[k]);
      printf("\n");
      free(F);
    }
    if(CHECK==CHECK_DEGENERATE){
      int *T=Prune(GF,&k);
      printf("Degenerate: %i\n",k);
      for(k=0;k<NF;k++) printf("%i ",T[k]);
      printf("\n");
      free(T);
    }
    if(CHECK==CHECK_GCOLOR){
      int *T=Prune(GF,NULL);
      int *C=GreedyColor(GF,T);
      printf("#colors: %i\n",1+GF->int1);
      PrintMorphism("Coloring (node->color):\n",C,GF->n);
      free(C);
      free(T);
    }
    if(CHECK==CHECK_KCOLOR){
      int k=MEM(CPARAM,0,int);
      int *C=kColor(GF,k);
      if(C==NULL) printf("There is no %i-coloration for this graph.\n",k);
      else{
	printf("#colors: %i\n",1+GF->int1);
	PrintMorphism("Coloring (node->color):\n",C,GF->n);
	free(C);
      }
    }
    if(CHECK==CHECK_KCOLORSAT){
      kColorSat(GF,MEM(CPARAM,0,int));
    }
    if((CHECK>=CHECK_PS1)&&(CHECK<=CHECK_PS1x)){
      int c=0;
      if(CHECK==CHECK_PS1b) c=1;
      if(CHECK==CHECK_PS1c) c=2;
      if(CHECK==CHECK_PS1x) c=3;
      path *P=new_path(GF,NULL,NF);
      int v=PS1(GF,P,c);
      printf("#tests: %i\nPS1: %s\n",GF->int1,v?"yes (PS1 for sure)":"no (probably not PS1)");
      free_path(P);
    }
    if(CHECK==CHECK_TWDEG){
      int *T=Prune(GF,&k);
      printf("treewidth <= %i\n",Treewidth(GF,0));
      printf("treewidth >= %i\n",k);
      free(T);
    }
    if(CHECK==CHECK_TW){
      k=Treewidth(GF,1);
      printf("#tests: %i\ntreewidth: %i\n",GF->int1,k);
    }
    if(CHECK==CHECK_GIRTH){
      param_bfs *p;
      int u,x=1+NF;
      for(u=0;u<NF;u++){
	p=bfs(GF,u,NULL);
	if(p->girth>0) x=MIN(x,p->girth);
	free_param_bfs(p);
      }
      if(x>NF) x=-1;
      printf("girth: %i\n",x);
    }
    if((CHECK==CHECK_PATHS)||(CHECK==CHECK_PATHS2)){/*  Sort tous les chemins de x à y */
      int (*f)(graph*,path*,int)=(CHECK==CHECK_PATHS)? NextPath : NextPath2;
      path *P=new_path(GF,NULL,NF); /* chemin vide */
      P->P[0]=MEM(CPARAM,0,int);
      P->P[1]=MEM(CPARAM,sizeof(int),int);
      int u,v=f(GF,P,-1); /* initialise le premier chemin */
      while(v){
	for(u=0;u<P->n;u++) printf("%i ",P->P[u]);
	printf("\n");
	v=f(GF,P,0);
      }
      free_path(P);
    }
    if(CHECK==CHECK_ISO){
      char *s=ARGV[MEM(CPARAM,0,int)];
      graph *H=File2Graph(s,34);
      int *P=Isomorphism(GF,H);
      printf("H: %s\n#tests: %i\n",s,H->int1);
      if(P==NULL) printf("Non-isomorphic.\n");
      else{
	PrintMorphism("Isomorphism G->H:\n",P,NF);
	free(P);
      }
      free_graph(H);
    }
    if(CHECK==CHECK_SUB){
      char *s=ARGV[MEM(CPARAM,0,int)];
      graph *H=File2Graph(s,34);
      graph *S=Subgraph(GF,H);
      printf("H: %s\n#tests: %i\n",s,H->int1);
      if(S==NULL) printf("G is not a subgraph of H.\n");
      else{
	printf("Subgraph S of H isomorphic to G:\n");
	PrintGraph(S);
	PrintMorphism("Isomorphism S->G:\n",S->pint1,S->n);
	free_graph(S);
      }
      free_graph(H);
    }
    if(CHECK==CHECK_MINOR){
      char *s=ARGV[MEM(CPARAM,0,int)];
      graph *H=File2Graph(s,34);
      int *C=Minor(H,GF);
      printf("H: %s\n#tests: %i\n",s,H->int1);
      if(C==NULL) printf("H is not a minor of G.\n");
      else{
	int c,u;
	printf("Model of H in G:\n");
	for(c=0;c<H->n;c++){ /* pour chaque couleur c */
	  printf("%i -> {",c);
	  for(u=0;u<NF;u++) /* on affiche les sommets de la couleur c */
	    if(C[u]==c) printf(" %i",u);
	  printf(" }\n");
	}
	free(C);
      }
      free_graph(H);
    }
    if(CHECK==CHECK_ISUB){
      char *s=ARGV[MEM(CPARAM,0,int)];
      graph *H=File2Graph(s,34);
      int *X=InducedSubgraph(H,GF);
      printf("H: %s\n#tests: %i\n",s,GF->int1);
      if(X==NULL) printf("H is not an induced subgraph of G.\n");
      else{
	int u;
	printf("Vertices of the induced subgraph S:");
	for(u=0;u<H->n;u++) printf(" %i",X[u]);
	for(u=0;u<H->n;u++) GF->pint1[u]=X[u];
	PrintMorphism("\nIsomorphism H->S:\n",GF->pint1,H->n);
	free(X);
      }
      free_graph(H);
    }

    free_graph(GF);
    free(CPARAM);
  } /* fin if(CHECK) */

  //if(FILTER){
  //switch(FILTER){
  //default: break;
  //}
  //free_graph(GF);
  //free(FPARAM);
  //} /* fin if(FILTER) */
  
  return 0;
}

/*
  Quelques règles pour la rédaction de l'aide:

  Le nom d'une option doit commencer par "-". Cela ne doit pas être le
  cas pour un graphe. Un paragraphe commence par "...." (les "...." ne
  sont pas affichés) et se termine par une ligne vide. Donc il faut
  ajouter "...." pour avoir un saut de ligne dans un même
  paragraphe. Le "....." (x5) devant "HISTORIQUE" est là pour stopper
  la recherche d'un mot clef ("?").

*/

/*# ###
Générateur de graphes - v3.3 - © Cyril Gavoille - Juillet 2014

USAGE

       gengraph [-option] graph_name [parameter_list]


DESCRIPTION

       Génére sur la sortie standard un graphe. Par défaut le graphe
       est non orienté et affiché selon une liste d'arêtes (en texte),
       mais d'autres formats sont possibles: liste d'adjacence, dot de
       GraphViz, xfig ou pdf par exemple. En paramètre figure le nom
       du graphe ainsi que ses paramètres éventuels (nombre de sommets
       etc.). La commande appelée seule affiche l'aide sur le
       générateur. Si les paramètres d'une option ou d'un graphe sont
       absents ou remplacés par "?", une aide spécfique est affichée.

       Ex:
          gengraph -help
	  gengraph -list | sort
	  gengraph tree ?
	  gengraph ? arbre
          gengraph tutte
          gengraph hypercube 8
          gengraph mesh 7 3 -not
	  gengraph mesh 20 20 -dele .2
	  gengraph rdodeca -visu
          gengraph tree 100 -visu
	  gengraph gabriel 50 -visu
	  gengraph gabriel 2000 -xy seed 1 0.15 -visu
	  gengraph sierpinski 7 3 -visu
	  gengraph udg 400 .1 -visu
	  gengraph udg 400 .1 -xy seed 3 1.5 -visu
	  gengraph udg 400 -1 -vsize -vcolor deg -visu
          gengraph arytree 6 3 3 -dotfilter circo -visu
	  gengraph arboricity 100 2 -vcolor degr -visu
	  gengraph prime 6 -directed -noloop -visu
	  gengraph aqua 3 3 2 1 -label 1 -directed -dotfilter dot -visu
	  echo "0->1->2->0" | gengraph load - -check bfs 0
	  gengraph tutte | gengraph -filter - diameter p
          gengraph rplg 500 2.9 -check maincc -format no |
                   gengraph load - -vcolor degr -vsize -visu
          gengraph -xy scale 15 15 -xy round 0 -xy grid 16 rng 30 -visu
	  gengraph linial 7 3 -check kcolorsat 3 | ./glucose -model

   LE FORMAT STANDARD

       Le format par défaut (ou standard) est une liste d'arêtes ou de
       chemins écrits en texte simple. Ce format minimaliste est très
       proche de celui du format "dot" de GraphViz.  D'autres formats
       de sortie sont possibles, notamment le format "dot" (voir
       l'option -format).  Les sommets sont numérotés consécutivement
       de 0 à n-1 où n est le nombre de sommets présents dans le
       graphe (en fait cela peut être changé avec l'option -shift).
       Une arête entre i et j est représentée par i-j, un arc de i
       vers j par i->j. Les sommets isolés sont simplement représentés
       par le numéro du sommet suivit d'un espace ou d'un retour de
       ligne. Dit autrement, le nombre de sommets du graphe est
       l'entier le plus grand + 1, et s'il y a i-j (ou i->j), alors il
       existe une arête (ou un arc) entre les sommets i et j.

       Pour une représentation plus compacte, les arêtes consécutives
       d'un chemin du graphe peuvent être regroupées en blocs. Par
       exemple, les deux arêtes 3-5 et 5-8 peuvent être regroupées en
       3-5-8. Mais ce n'est pas obligatoire. Les sommets isolés et les
       arêtes (ou les blocs d'arêtes) sont séparés par des espaces ou
       des sauts de ligne. Une boucle sur un sommet i est codée par
       i-i. Les arêtes multiples sont codées par la répétition d'une
       même arête, comme par exemple i-j i-j, ou encore i-j-i (même
       convention pour les arcs i->j->i).

       Quelques exemples:

       Ex1: 0 1-2-3-1

       représente un graphe à 4 sommets, composé d'un sommet isolé (0)
       et d'un cycle à trois sommets (1,2,3). En voici une
       représentation graphique possible:

              0   1
                 / \
                3---2

       Ex2: 4-2-1-0-3-2-5

       représente un graphe à 6 sommets composé d'un cycle de longueur
       4 et de deux sommets de degré 1 attaché à 2. En voici une
       représentation graphique possible:

                  1
                 / \
              4-2   0
               / \ /
              5   3

   COMMENT LE GENERATEUR FONCTIONNE ?

       Pour chaque graphe une fonction adj(i,j) est définie. Elle
       fournit l'adjacence (0 ou 1) entre les sommets i et j, entiers
       entre 0 et n-1. Le graphe est affiché en générant toutes les
       paires {i,j} possibles et en appelant adj(i,j) (ou tous les
       couples (i,j) possibles dans le cas orientés). Les graphes sont
       ainsi générés de manière implicite. Les arêtes du graphe ne
       sont pas stockées en mémoire, mais affichées à la volée.  Ceci
       permet de générer des graphes de très grande taille sans
       nécessiter O(n^2) espace de mémoire centrale. Pour certains
       graphes cependant, comme les arbres, les graphes
       d'intersections, ou les graphes géométriques, une structure de
       données en O(n) est utilisée. Pour les formats d'affichage
       liste, matrix, et smatrix une structure de données de taille
       linéaire (en O(n+m) où m est le nombre d'arêtes) est utilisée
       en interne. Ces trois derniers formats sont donc à éviter. Pour
       la génération de très grand graphe, le format standard ou dot
       doit être privilégié.

   COMMANDES EXTERNES

       Le programme fait appel, pour certaines fonctions, aux
       commandes systèmes suivantes: sed, grep, awk, more, sort, date,
       printf, dot.


OPTIONS


....-help [word] or ? [word]
....
       Affiche l'aide en ligne qui est contenue dans le fichier source
       (.c) du générateur. Pour cela, le source doit être dans le même
       répertoire que la commande. Si "word" est précisé, alors les
       options et noms de graphes contenant "word" sont affichés.  Une
       aide détaillée sur chaque option (ou graphe) ainsi affichée
       peut alors être fourni par "gengraph option ?". La forme ?
       peut ne pas fonctionner si un fichier d'un seul caractère
       existe dans le répertoire courant (à cause de l'interprétation
       du shell). Il faut alors utiliser '?' au lieu de ?.
....
       Ex: gengraph ? arbre
           gengraph ktree ?

....-list
....
       Affiche la liste des graphes et leurs paramètres qu'il est
       possibles de générer, d'abord les graphes de bases puis les
       composés. On obtient une aide sur un graphe particulier si son
       nom est suivit d'un "?" ou si ses paramètres éventuelles sont
       abscents (dans ce cas il doit être le dernier mot de la ligne
       de commande).

....-directed
....-undirected
....
       Produit le graphe orienté en testant les n(n-1) arcs possibles,
       l'option -undirected permettant de revenir à la situation par
       défaut (voir aussi -(no)loop). En format standard où en dot, un
       arc apparaît comme x->y au lieu de x-y. Tous les graphes ne
       sont pas forcément définis pour fonctionner correctement avec
       cette option. La plupart des graphes vont apparaître comme
       orientés symétriques.

....-noloop
....-loop
....
       Ne produit pas les boucles pour les graphes (-noloop). C'est
       l'option par défaut pour les graphes non-orientés. L'option
       -loop les autorise. C'est l'option par défaut pour les graphes
       orientés. Cette option doit être placée après -(un)directed.

....-not
....
       Inverse la fonction d'adjacence, et donc affiche le complément
       du graphe. Cette option est prioritaire devant l'option
       -redirect.

....-dele p
....
       Permet de supprimer chaque arête du graphe générée avec probabilité p.

....-delv p
....
       Similaire à -dele p mais concerne les sommets. Le sommet et ses
       arêtes incidentes sont alors supprimés. Si p est un entier <0,
       alors exactement -p sommets sont supprimés. Si k sommets sont
       supprimés, alors le nom des sommets restant est dans
       l'intervalle [0,n-k[ où n est le nombre de sommets initial du
       graphe. Les noms des sommets sont donc éventuellement
       renumérotés. Voir aussi les options -permute et -shift. Bien
       sûr la fonction d'adjacence adj(i,j) est appliquée sur les noms
       (i,j) originaux.

....-star n
....
       Ajoute n sommets pendant (degré 1) aux sommets du graphe. Si
       n>0, alors n représente le nombre total de sommets ajoutés,
       chacun des n sommets étant connectés aléatoirement uniforménent
       aux sommets du graphe original. Si n<0, alors -n sommets sont
       ajoutés à chacun des sommets du graphe. Cette opération est
       appliquée en priorité. Les options -star et -apex sont
       incompatibles.

....-apex n
....
       Ajoute n sommets universels, donc connectés à tous les sommets
       du graphe. Cette opération est appliquée en priorité. Les
       options -star et -apex sont incompatibles.

....-redirect p
....
       Redirige chaque arête uniformément avec probabilité p. Plus
       précisément, si {i,j} est une arête du graphe original G, alors
       avec probabilité p l'arête affichée est {i,k} au lieu de {i,j}
       où k est un sommet choisi uniformément parmis les sommets du
       graphe G. Si l'arête {i,j} est supprimée par -dele ou si le
       sommet i est supprimé par -delv, la redirection n'a pas lieu.
       Cette option est appliquée après l'option -not. Le graphe G
       tient donc compte de -not avant de rediriger ses arêtes.

....-seed s
....
       Permet d'initialiser le générateur aléatoire avec la graine s,
       permettant de générer plusieurs fois la même suite aléatoire.
       Par défaut, la graine est initialisée avec le numéro de
       processus de la commande, donc génère par défaut des suites
       différentes à chaque lancement.

....-width m
....
       Limite à m le nombre d'arêtes et de sommets isolés affichés par
       ligne. Cette option n'a pas de signification particulière en
       dehors des formats standard et dot. Par exemple, -width 1
       affiche une arrête (ou un sommet isolé) par ligne. L'option
       -width 0 affiche tout sur une seule ligne. La valeur par défaut
       est 12.

....-shift s
....
       Permet de renuméroter les sommets à partir de l'entier s
       positif. La valeur par défaut est -shift 0.  L'intérêt de cette
       option est de pouvoir réaliser des unions de graphes simplement
       en renumérotant les sommets et en concaténant les fichiers aux
       formats standard ou list. Cette option n'a pas d'effets pour
       les formats de sortie de type matrice.

....-permute
....
       Permute aléatoirement uniformément le nom des sommets
       lorsqu'ils sont affichés.  Les numéros restent dans
       l'intervalle initial qui, sauf si l'option -shift a été
       utilisée, est [0,n[ où n est le nombre de sommets du graphe
       réellement généré. Voir aussi l'option -label.

....-header
....
       Affiche un préambule donnant certaines informations sur le
       graphe, sous forme de commentaire à la C++ (//). Par défaut
       aucun préambule n'est affiché. Les informations affichées sont:
       - la date
       - la ligne de commande qui a produit la génération du graphe
       - le nombre de sommets
       - le nombre d'arêtes, le degrés maximum et minimum
       Pour les formats standard et dot, le nombre d'arêtes (et les
       degrés min et max) n'est pas déterminé avant l'affichage du
       graphe. Pour cette raison ces nombres ne sont affichés qu'après
       le graphe. Pour n'avoir que les informations sur le graphe,
       utiliser -header avec l'option -format no.

....-check [parameters]
....
       Stocke en mémoire le graphe généré sous la forme d'une liste
       d'adjacence et lui applique un algorithme. Cette option
       nécessite un espace supplémentaire en O(n+m) pour le stockage
       du graphe. Utiliser "-format no" pour ne pas afficher le graphe
       généré.
....
       -check bfs s
....
          Effectue un parcours en largeur d'abord sur le graphe généré
          depuis le sommet s. La distance depuis s est affichée, ainsi
          que l'arborescence (-1 indique que le sommet n'a pas de
          père). La longueur du plus petit cycle passant par s est
          aussi donnée. Elle vaut -1 s'il n'existe pas.
....
       -check dfs s
....
          Effectue un parcours en profondeur d'abord sur le graphe
          généré depuis le sommet s. Le nombre de composantes ainsi
          que l'arborescence (-1 indique que le sommet n'a pas de
          père) sont donnés.
....
       -check deg
       -check edge
       -check edges
....
          Affiche la distribution des degrés et le nombre d'arêtes du
          graphe.
....
       -check degenerate
....
          Donne la dégénérescence du graphe, ainsi que l'ordre
          d'élimination correspondant des sommets.
....
       -check girth
....
          Donne la maille du graphe, -1 si c'est l'infini.
....
       -check gcolor
....
          Donne une borne supérieure sur le nombre chromatique du
          graphe en utilisant l'heuristique du degré minimum.
....
       -check kcolor k
....
          Donne une k-coloration du graphe (et la couleur pour chaque
          sommet), si c'est possible. Pour cela une recherche
          exhaustive de toutes les k-colorations est effectuée. Le
          temps est raisonable si k=3 et n<20.
....
       -check kcolorsat k
....
          Donne une formulation SAT de la k-coloration du graphe. Il
          s'agit de la formulation multivaluée classique, un sommet
          pouvant avoir plusieurs couleurs sans que celà nuise à la
          validité du résultat. Les contraintes sont décrites au
          format Dimacs CNF. On peut alors envoyer le résultat à un
          solveur SAT comme MiniSat ou Glucose.
....
	  Ex: gengraph linial 6 3 -check kcolorsat 3 | ./glucose -model
....
       -check ps1
       -check ps1b
       -check ps1c
       -check ps1x n u_1 v_1 ... u_n v_n
....
          Applique le test ps1 ou l'une de ses variantes (voir -filter
          ps1 pour plus de détail sur ce test). Affiche aussi le
          nombre de tests réalisés (nombre de paires de sommets et de
          chemins testés).
....
       -check paths x y
       -check paths2 x y
....
          Liste tous les chemins simples entre les sommets x et
          y. N'affiche rien si x et y ne sont pas connectés. L'ordre
          est défini suivant le premier plus court chemins dans
          l'ordre des sommets depuis le sommet x. La variante paths2
          est similaire sauf que les paths renvoyés n'ont que des
          ensembles de sommets différents.
....
       -check iso H
....
          Teste si le graphe généré G est isomorphe à H. Si oui,
          l'isomorphisme de G à H est donné. Le nombre de tests
          affichés est le nombre de fois où les graphes sont comparés,
          la comparaison prend un temps linéaire en la taille des
          graphes). Plus les graphes sont symétriques (comme un cycle
          ou un hypercube), plus le nombre de tests sera important.
....
          Tester l'isomorphisme entre deux cycles de 8 sommets
          étiquetés aléatoirement prends environ 4 mille tests, et
          entre deux cycles de 12 sommets, 30 millions de tests soit
          9" environ. Pour deux arbres à 75 sommets (aléatoires mais
          isomorphes), moins de 20 tests suffisent.
....
       -check sub H
....
          Teste si le graphe généré G est un sous-graphe couvrant de H
          (donc avec le même nombre de sommets). S'ils ont le même
          nombre d'arêtes, le test est équivalent à l'isomorphisme. Le
          nombre de tests est le nombre total de fois où deux graphes
          sont comparés.  On peut tester si H est Hamiltonien en
          prennant pour G un cycle.
....
          Tester un cycle de longueur 12 dans une grille 3x4 prend
          jusqu'à environ 32 millions de tests (parfois bien moins),
          soit < 10" environ.
....
       -check minor H
....
          Teste si le graphe G généré contient H comme mineur. Les
	  graphes peuvent être non connexes. S'ils ont le même nombre
	  de sommets le test est équivalent à celui du sous-graphe
	  (voir -check sub). Dans le cas positif, un modèle de H dans
	  G est fourni.
....
          Le principe consiste à contracter des arêtes de G, de toutes
	  les manières possibles, et à tester si H est un sous-graphe
	  du graphe contracté. Le nombre de tests affichés est le
	  nombre de contractions plus le nombre total de tests
	  réalisés par les tests de sous-graphe. Pour H=K_4 il est
	  préférable d'utiliser -check twdeg qui donne 2 ssi le graphe
	  ne contient pas K_4 comme mineur.
....
       -check twdeg
....
          Donne une borne supérieure et inférieure sur la treewidth du
          graphe. Pour la borne supérieure, on utilise l'heuristique
          du sommet de degré minimum que l'on supprime et dont on
          complète le voisinage par une clique. En cas d'égalité (même
          degré) on sélectionne le sommet dont il faut rajouter le
          moins d'arêtes. La borne inférieure qui est donnée provient
          de la dégénérescence. La treewidth est exacte si 0,1 ou 2
          est retournée. L'algorithme est en O(n^2).
....
       -check tw
....
          Calcule la treewidth du graphe en analysant tous les ordres
          d'éliminations. La complexité est donc en n!. Il ne faut
          l'utiliser que si le nombre de sommets est < 12 (Ex:
          gengraph random 12 .5 -check tw donne 5 en environ 750
          millions de tests). Parfois, l'utilisation de -permute peut
          accélérer le traitement, car partir d'un ordre d'élimination
          déjà bon permet d'éliminer rapidement beaucoup d'ordres
          possibles.
....
       -check maincc
....
          Affiche, dans le mode standard seulement, le graphe
          correspondant à la composante connexe la plus grande en
          nombre de sommets. Les sommets sont éventuellement
          renumérotés. En principe cette option s'utilise avec -format
          no, sinon le graphe initial est aussi affiché. Avec "-format
          no | ./gengraph load -" on peut alors afficher le graphe
          dans le format souhaité, et ajouter aussi -visu.

....-filter family[:range] [not] test [parameters]
....
       Affiche les graphes d'une famille pour lesquels le test est
       vrai (ou faux si "test" est précédé de "not"). Le paramètre
       "family" est un nom de fichier ou "-" pour l'entrée standard.
       Il contient la famille de graphes (ou un graphe seul) au format
       standard.  L'affichage est influencé par l'option -width qui
       doit être placée avant -filter. La variante "family:range"
       permet de sélectionner les graphes de la famille dont les
       identifiants sont spécifiés par "range", un ensemble de valeurs
       selon le format "value" décrit ci-après. La variante "-:range"
       est possible.
....
       Dans la suite, la présence de "value" dans les paramètres d'une
       option représente un ensemble de valeurs possibles. Par
       exemple, -filter F vertex '>5' filtre les graphes de la famille
       F comportant plus de 5 sommets. De manière générale, "value"
       est une suite d'intervalles d'entiers sépararés par des ","
       chaque intervalle étant codé comme suit:
....
          <x ........ valeur inférieure à x
          >x ........ valeur supérieure à x
          x ou =x ... valeur égale à x
	  x-y ....... valeur dans l'intervalle [x,y]
	  t ......... toujours vrai (intervalle infini)
	  p ......... affiche la valeur plutôt que le graphe
....
       Ex1: -filter F vertex '5,7-13,>100'
       Ex2: -filter F vertex '5-10,p'
       Ex3: -filter F edge p
       Ex4: -filter F id 5,7
....
       L'exemple 1 filtre les graphes de la famille F ayant n sommets
       avec soit n=5, soit n dans [7,13], ou soit n>100. L'exemple 2
       affiche le nombre de sommets des graphes ayant entre 5 et 10
       sommets.  L'exemple 3 affiche le nombre d'arêtes de chaque
       graphe.  L'exemple 4 affiche les graphes d'identifant 5 et 7 de
       la famille F.
....
       Pour avoir le maximum/minimum de "value" faire:
       ... -filter ... p | grep '^\[' | sort -rnk 3 | head
       ... -filter ... p | grep '^\[' | sort -nk 3 | head
....
       Si "value" contient le symbole > ou < il est alors préférable
       de mettre des quotes ('>14' par exemple) pour que la commande
       soit correctement interprétée par le shell.
....
       La différence principale avec -check est que le résultat de
       -filter est non verbeux alors que -check, qui ne s'applique pas
       a priori sur des familles de graphes mais sur un graphe seul,
       donne des explications sur l'exécution de l'algorithme. Avec
       -check l'algorithme s'applique au graphe généré, alors qu'avec
       -filter c'est toujours à partir d'un fichier.
....
       -filter F id value
....
          Filtre les graphes de F dont l'identifiant est déterminé par
          value. Cela permet d'extraire un ou plusieurs graphes
          donnés. C'est équivalent à "-filter F:value all".
....
       -filter F rename shift
....
          Affiche tous les graphes de la famille en renumérotant les
          graphes à partir de l'entier "shift".
....
       -filter F vertex value
....
          Filtre les graphes de F d'un nombre de sommets déterminé
          par value.
....
       -filter F edge value
       -filter F edges value
....
          Filtre les graphes de F d'un nombre d'arêtes déterminé par
          value.
....
       -filter F all (= vertex t)
....
          Affiche tous les graphes de F ce qui permet en particulier
          de les compter.
....
       -filter F1 minus F2
....
          Affiche F1\F2, c'est-à-dire tous les graphes de F1 qui ne
          sont pas isomorphes F2 (si F2 est un graphe) ou à l'un des
          graphes F2 (dans le cas d'une famille).
....
       -filter F1 minus-id F2
....
          Comme "minus" mais concerne les identifiants: supprime de F1
          les graphes dont l'identifiant existe dans F2 (qui peut être
          un graphe ou une famille de graphe).  La complexité est
          environ (|F1|+|F2|)log|F2|, alors que pour "minus" elle est
          en |F1|*|F2|*T où T est le temps pour décider si deux
          graphes pris dans F1 et F2 sont isomorphes.
....
       -filter F minor[-inv] H
....
          Filtre les graphes de F contenant H comme mineur. La
          variante minor-inv filtre les graphes de F qui sont mineurs
          de H. Si H=K_4, il est préférable d'utiliser -filter tw2.
....
       -filter F sub[-inv] H
....
          Filtre les graphes de F contenant H comme sous-graphe,
          chaque graphe de F devant avoir le même nombre de sommets
          que H. La variante sub-inv filtre les graphes de F qui sont
          un sur-graphe de H.
....
       -filter F isub[-inv] H
....
          Filtre les graphes de F contenant H comme sous-graphe
          induit. La variante isub-inv filtre les graphes de F qui
          sont sous-graphes induits de H.
....
       -filter F iso H
....
          Filtre les graphes de F isomorphes à H.
....
       -filter F degenerate value
....
          Filtre les graphes de F de dégénérescence déterminée par
          value.
....
       -filter F forest value
....
          Filtre les graphes de F qui sont des forêts dont le nombre
          d'arbres est déterminé par value.
....
       -filter F isforest (= forest t)
....
          Filtre les graphes de F qui sont des forêts.
....
       -filter F istree (= forest =1)
....
          Filtre les graphes de F qui sont des arbres.
....
       -filter F cycle (= not forest t)
....
          Filtre les graphes de F contenant au moins un cycle.
....
       -filter F degmax/degmin value
....
          Filtre les graphes de F de degré maximum (ou minimum)
          déterminé par value.
....
       -filter F deg value
....
          Filtre les graphes de F où tous les sommets ont un degré
          déterminé par value. Ainsi -filter deg 4-7 filtre les
          graphes avec un degré minimum au moins 4 et un degré maximum
          au plus 7.
....
       -filter F gcolor value
....
          Filtre les graphes de F dont le nombre de couleurs obtenu
          selon l'heuristique du degré minimum est déterminé par
          value.
....
       -filter F bipartite (= gcolor <3)
....
          Filtre les graphes de F qui sont bipartis.
....
       -filter F component value
....
          Filtre les graphes de F dont le nombre de composantes
          connexes est déterminé par value.
....
       -filter F connected (= component 1)
....
          Filtre les graphes de F qui sont connexes.
....
       -filter F biconnected
....
          Filtre les graphes de F qui sont 2-connexes. Un graphe G est
          k-connexe s'il n'y a pas d'ensemble avec <k sommets qui
          déconnecte G ou laisse G avec 1 sommet. Un graphe est
          2-connexe s'il est connexe, ne possède pas de sommet
          d'articulation et a plus de 2 sommets. Les cliques de taille
          k+1 sont k-connexes.
....
       -filter F radius value
....
          Filtre les graphes de F dont le rayon est déterminé par
          value. Le rayon est la profondeur du plus petit arbre
          couvrant le graphe. Il vaut -1 si le graphe n'est pas
          connexe.
....
       -filter F girth value
....
          Filtre les graphes de F dont la maille est déterminée par
          value. La maille est la taille du plus petit cycle. Elle
          vaut -1 si le graphe n'a pas de cycle.
....
       -filter F diameter value
....
          Filtre les graphes de F dont le diamètre est déterminé par
          value. Le diamètre vaut -1 si le graphe n'est pas connexe.
....
       -filter F cut-vertex value
....
          Filtre les graphes de F dont le nombre de sommets
          d'articulations est déterminé par value. Un sommet est un
          point d'articulation si sa suppression augmente le nombre de
          composante connexe. Les sommets de degré 1 ne sont pas des
          point d'articulation. Le graphe est biconnexe ssi value<1 ou
          si le graphe est une clique avec au moins deux sommets. On
          peut tester si un graphe est une clique avec -filter degmin
          ou -filter deg.
....
       -filter F ps1
       -filter F ps1b
       -filter F ps1c
       -filter F ps1x n u_1 v_1 ... u_n v_n
....
          Filtre les graphes G de la famille F dont le test ps1 est
          vrai, c'est-à-dire si l'évaluation de la fonction f(G,{})
          décrite ci-après est vraie.
....

	  Soit P un chemin d'un graphe G tel que G\P est connexe. La
          fonction f(G,P) est vraie ssi G est vide (en pratique
          |G|-|P|<3 suffit) ou s'il existe deux sommets x,y de G où y
          n'est pas dans P tels que pour tout chemin Q entre x et y
          dans G "compatible" avec P (c'est-à-dire P et Q
          s'intersectent en exactement un segment) on a les deux
          conditions suivantes: (1) il n'y a pas d'arête entre les
          sommets de P\Q et de G\(QuP); et (2) pour toute composante
          connexe C de G\(QuP), f(CuQ,Q) est vraie. Le test est
          optimisé dans un certain nombre de cas, en particulier: les
          arbres (toujours vrai), les cliques (vrai ssi n<5).

....
	  La variante ps1b calcule et affiche de plus un graphe des
          conflits (affichage modifiable par -width), chaque noeud de
          ce graphe correspondant à un argument (CuQ,Q) évalué à faux
          par f. La valeur (ou code) d'un noeud est 0 (=lourd ou
          faux), 1 (=léger ou vrai) ou - (indéterminée). Suivant
          certaines règles, les valeurs 0 ou 1 sont propagées selon le
          type des arêtes du graphes des conflits.  Résoudre le graphe
          des conflits revient à trouver une affectation des valeurs 0
          ou 1 aux noeuds qui respecte (sans contradiction) toutes les
          règles.
....
	  La fonction f(G,{}) est évaluée à vraie si le graphe des
          conflits n'a pas de solution, c'est-à-dire si une
          contradiction a été découverte ou si pour une paire de
          sommets (x,y) tous ses noeuds sont à 1.
....
	  On affiche le code d'un noeud (0,1,-) ainsi que les sommets
          de sa composante (par ex: [237]).  Les noeuds du graphe des
          conflits sont reliées par des arêtes typées. Les voisins v
          d'un noeud u sont listés avec le type de l'arête, si l'un
          des 4 cas suivants se produit (il n'y a pas d'arête entre u
          et v dans les autres cas):
....
	     v<  (la composante de v est incluse dans celle de u)
	     v>  (la composante de v contient celle de u)
	     v=  (les composantes de u et v sont les mêmes) 
	     v|  (les composantes de u et v sont disjointes) 
....
	  Parmi les règles on trouve par exemple: si deux noeuds du
          graphe des conflits u=(CuQ,Q) et v=(C'uQ',Q') sont
          disjoints, c'est-à-dire C n'intersecte pas C', alors seule
          une des expressions f(CuQ,Q) ou f(C'uQ',Q') peut être
          fausse, pas les deux. Dit autrement, les composantes de u et
          v ne peuvent pas être "lourdes" (=0) toutes les deux en même
          temps. Et donc, si le code de u est 0, celui de v est
          1. Notons que le code de u et v égale à 1 est comptabible
          avec cette règle.
....	  
	  La variante ps1c est similaire à ps1b sauf que récursivement
          seul le test ps1 est appliqué, et pas ps1b. Le test ps1c est
          plus long que ps1 mais plus rapide que ps1b. La variante
          ps1x est similaire à ps1b sauf que les valeurs v_i sont
          écrites dans le noeuds u_i du graphe des conflits principal
          (pas ceux générés lors des appels récursifs). Plus
          précisément, v_1 (0 ou 1) est écrit dans le noeud u_1, puis
          sa valeur est propagée. Ensuite v_2 est écrit puis propagée,
          etc.
....
          Dans tous les cas, si G n'est pas connexe, le résultat n'est
          pas déterminé.
....
       -filter F tw value
....
          Filtre les graphes de F selon leur treewidth. L'algorithme
          pour le calcul de la treewidth est assez lent. Pour les
          petites valeurs de tw, des alternatives sont possibles (voir
          -check tw et -filter tw2). Pour savoir si un graphe G est de
          treewidth 3 il suffit de savoir si G contient l'un des 4
          mineurs suivants:
....
          echo "[0]"  > F; ./gengraph clique 5 >> F
	  echo "[1]" >> F; ./gengraph wagner >> F
	  echo "[2]" >> F; ./gengraph prism 5 >> F
	  echo "[3]" >> F; ./gengraph hajos >> F ; echo "0-1-2-0" >> F
	  cat G |./gengraph -filter F minor-inv - -format no
....
       -filter F tw2
....
          Affiche les graphes de treewidth <= 2. L'algorithme est en
          O(n^2). Ce test peut être utilisé pour tester (plus
          rapidement qu'avec -filter minor) les graphes sans mineur
          K_4.
....
       -filter F hyperbol value
....
          Filtre les graphes de F selon leur hyperbolicité. Il s'agit
          de la valeur (entière) maximum, surtout des quadruplets de
          sommets {u,v,x,y}, de la différence des deux plus grandes
          sommes parmi les sommes de distance: uv+xy, ux+vy et
          uy+vx. La complexité est en O(n^4).

....-format type
....
       Spécifie le format de sortie. Il est préférable d'utiliser
       cette option en dernier sur la ligne de commande. Les valeurs
       possibles pour "type" sont:
....
       - standard: format standard (liste d'arêtes), c'est le plus compact.
       - list: liste d'adjacence.
       - matrix: matrice d'adjacence.
       - smatrix: matrice supérieure, diagonale comprise.
       - dot: format de GraphViz qui est très proche du format standard.
       - dot<xxx>: dessine le graphe avec GraphViz et converti au format <xxx>.
       - xy: positions X,Y qui ont été utilisées pour le graphe géométrique.
       - no: n'affiche rien, à utiliser en combinaison avec -header ou -check.
....
       Les formats matrix/smatrix/list nécessitent de stocker le
       graphe en mémoire, donc nécessite un espace en O(n+m), alors
       que le graphe est généré à la volée pour les formats standard
       ou dot. Les formats <xxx> pour dot les plus utilisés sont: pdf,
       fig, svg, ps, jpg, gif, png (voir man dot).
....
       L'option -format dot<xxx> est équivalent à "-format dot | dot
       -T<xxx> ...". Elle doit donc être utilisée en dernier sur la
       ligne de commande. Le filtre dot utilisé pour dessiner le
       graphe peut être spécifié par l'option -dotfilter. L'affichage
       des noms de sommets est contrôlé par l'option -label.
....
       Remarque: les positions affichées dans le format dot
       ([pos="..."]) diffèrent d'un facteur proportionnel à sqrt(n)
       par rapport aux positions originales du graphe (qui peuvent
       être affichées par -format xy). Ce facteur permet de garder une
       taille raisonable pour les sommets car sous dot les sommets ont
       une taille fixe minimale.

....-vcolor option [parameters]
....
       Ces options permettent de modifier la couleur des sommets. Ces
       options n'ont d'effets qu'avec le format dot (et ses variantes
       comme -visu).  Par défaut les sommets sont des points
       noirs. Notez que les attributs par défaut des sommets
       (couleurs, formes, etc.)  peuvent être modifiés directement par
       dot (voir l'option -N de dot). Cependant l'option -vcolor
       permet d'individualiser la couleur d'un sommet, en fonction de
       son degré par exemple. Il peut avoir plusieurs options -vcolor
       sur la ligne de commande.
....
       -vcolor deg[r]
....
          La couleur dépend du degré du sommet (deg) ou du rang du
          degré du sommet (degr). Ainsi, les sommets de plus petit
          degré obtiennent la première couleur de la palette, les
          sommets de plus grand degré la dernière couleur de la
          palette, et les autres sommets une couleur intermédiaire de
          la palette. Donc une seule couleur est utilisée si le graphe
          est régulier.
....
       -vcolor degm
....
          Effectue une coloration propre (deux sommets voisins ont des
          couleurs différentes) suivant l'heuristique du degré
          minimum: récursivement, le sommet de degré minimum obtient
          la plus petite couleur qui n'est pas utilisée par ses
          voisins. Cela donne des colorations avec assez peu de
          couleurs pour les graphes de faible arboricité (planaire,
          tw, pw, kout, ...) ou de faible degré. Avec cette technique,
          les graphes bipartis (tree, crown, ...) sont coloriés avec
          deux couleurs. Cette option nécessite un espace et un temps
          en O(n+m).
....
       -vcolor randg
....
          Effectue une coloration propre en utilisant un algorithme
          glouton sur un ordre aléatoire des sommets: récursivement,
          le sommet d'indice i obtient la plus petite couleur qui
          n'est pas utilisée par ses voisins d'indice j<i. Cette
          option nécessite un espace et un temps en O(n+m).
....
       -vcolor kcolor k
....
          Effectue une k-coloration propre du graphe, si c'est
          possible. Si cela n'est pas possible, la première couleur
          est appliquée à tous les sommets. L'algorithme est le même
          que celui utilisé pour -check kcolor.
....
       -vcolor pal grad
....
          Permet de fixer la palette de couleurs utilisée par les
          sommets. Le paramètre "grad" est un mot sur l'alpabet [a-z],
          les caractères en dehors de cet alphabet sont ignorés.
          Chaque lettre correspond à une couleur de base:
....
	  a=aquamarine     h=hotpink      o=olive         v=violet
	  b=blue           i=indigo       p=purple        w=white
	  c=cyan           j=orange       q=pink          x=gray
	  d=darkorange     k=khaki        r=red           y=yellow
	  e=chocolate      l=lavender     s=salmon        z=black
	  f=forestgreen    m=magenta      t=teal
	  g=green (lime)   n=navy         u=yellowgreen
....
          La palette est calculée selon une extrapolation linéaire
          entre les points définis par le mot grad. Par exemple, si
          grad=rb, la palette sera composée d'un dégradé allant du
          rouge (r) au bleu (b). Si grad=rgbr, le dégradé ira du rouge
          au vert puis au bleu et enfin au rouge. Pour avoir une
          couleur (de base) unique sur tous les sommets, faire
          grad=x. Par exemple, pour avoir tous les sommets blancs, on
          peut faire:
....
          gengraph gabriel 30 -vcolor deg -vcolor pal w -visu
....
          La palette par défaut correspond au mot "grad" suivant:
          redjykugfocatbhsqvmpinzxlw.
....
       -vcolor list
....
          Produit l'affichage de la palette des couleurs utilisées
          pour un graphe plutôt que le graphe lui-même. Cela permet en
          particulier de savoir combien de couleur ont été utilisées.
          La palette est générée en affichant au format dot un graphe
          particulier où les sommets (représentés par un rectangle)
          sont les couleurs utilisées. Utilisez -visu pour visualiser
          la palette sous forme pdf. Le nom des sommets correspond à
          la lettre de la couleur de base comme spécifié par -vcolor
          pal.
....
	  Ex1: gengraph gabriel 50 -vcolor degm -vcolor list
	  (génère la palette utilisée pour ce graphe de Gabriel)
....
          Ex2: gengraph prime 53 -vcolor list
	  (un moyen simple de génèrer la palette par défaut)
....
          Ex3: gengraph clique 100 -vcolor degm -vcolor pal rb -vcolor list
          (génère un dégradé de 100 couleurs allant du rouge au bleu)

....-vsize
....
       La taille des sommets est proportionelle à son degré, alors que
       par défaut elle est fixe. Cette option n'a d'effet qu'avec le
       format dot (et ses variantes). Elle est combinable avec
       -vcolor.

....-visu
....
       Crée un fichier "g.pdf" permettant de visualiser le graphe. Il
       s'agit d'un raccourci de l'option "-format dotpdf > g.pdf".

....-len p
....
       Spécifie la longueur des arêtes pour le format dot et le filtre
       "neato". La valeur par défaut est 1.0, et une valeur plus
       grande (comme 2.0 ou 3.0) alonge les arêtes et permet dans
       certain cas de mieux visualiser le graphe.

....-dotfilter filter
....
       Spécifie le filtre de GraphViz, c'est-à-dire l'algorithme de
       dessin utilisé par dot. Par défaut, le filtre est "neato". Les
       filtres principaux sont: dot, neato, twopi, circo, fdp, sfdp,
       ...  Faire "dot -K ." pour afficher les filtres disponibles.

....-pos b
....
       Active (b=1) ou désactive (b=0) la génération des positions des
       sommets pour le format dot. Cela sert à indiquer à l'algorithme
       de dessin dot de respecter les coordonnées des sommets.
       L'option par défaut est -pos 0, mais cette option est activée
       pour tous les graphes géométriques (udg, gabriel, thetagone
       ...).

....-label b
....
       Active (b>0) ou désactive (b=0) l'affichage du nom des sommets
       pour les formats dot et standard. Si b=1, il s'agit du nom
       original du sommet. Cette représentation n'est pas implémentée
       pour tous les graphes. Par défaut les noms sont les entiers de
       [0,n[, n nombre de sommets du graphe généré. L'option -label 1
       -visu permet alors de dessiner le nom des sommets, car par
       défaut les noms ne sont pas dessinés (b=0). L'option -label 2
       -visu force l'affichage des noms sous-forme d'entiers. Comme
       cette option influence l'option -format dot<xxx>, l'option
       -label doit être placée avant -format. L'option -label 1 annule
       l'option -permute, mais -label 2 ne le fait pas. Pour avoir la
       correspondance entre l'indice et le nom du sommet faire
       simplement: ... -label 1 -format dot | grep label

....-norm l
....
       Fixe la norme pour l'adjacence de certains graphes géométriques
       (udg, gabriel, rng, nng). Les valeurs possibles pour l sont
       1,2,3,4 pour respectivement les normes L_1, L_2, L_max,
       L_min. La norme par défaut est L_2, la norme euclidienne.

....-xy option [parameters]
....
       Cette option contrôle la façon dont sont générées les
       coordonnées des sommets d'un graphe géométrique. Par défaut les
       positions sont tirées aléatoirement uniformément dans le carré
       [0,1[ x [0,1[, mais cela peut être changé par l'option -xy.
       Notez bien que, même si c'est très improbable, deux sommets
       peuvent avoir les mêmes positions (voir l'option -xy unique).
....
       -xy load file
....
          Charge les positions à partir du fichier "file" ou de
          l'entrée standard si file=-. Cela permet de tester les
          adjacences d'un graphe géométrique à partir de positions
          pré-déterminées. Le format est celui de -format xy.
....
          Ex: gengraph gabriel 10 -xy load file.pos
....
	  Le nombre de sommets du graphe est déterminé par le fichier
          et non pas par les paramètres du graphe. Cette option n'a
          d'effet que pour les graphes géométriques. La structure du
          fichier au format texte doit être:
....
	         n
		 x_1 y_1
		 x_2 y_2
		 ...
		 x_n y_n
....
	  où n est le nombre de positions. Les positions x_i y_i ne
	  sont pas forcément dans l'intervalle [0,1[. Notez qu'avec
	  l'option -format xy, il est possible d'effectuer la
	  transformation d'un fichier de positions. L'exemple suivant
	  normalise les coordonnées du fichier g.pos dans le carré
	  unité: gengraph -xy load g.pos -xy scale 1 1 -format xy
....
       -xy scale a b
....
          Effectue un redimensionement des positions de sorte quelles
          se situent dans le rectangle [0,a[ x [0,b[. En prenant
          a=b=1, les coordonnées seront renormalisées dans le carré
          [0,1[ x [0,1[. Cette opération est effectuée juste avant la
          génération des arêtes, en particulier après avoir effectué
          l'opération -xy noise (voir ci-après) et/ou -xy load. Les
          positions sont recadrées pour laisser une fine bande vide
          sur le bord du rectangle. Cette bande a une largeur ~
          a/sqrt(n) et une hauteur ~ b/sqrt(n) où n est le nombre de
          points.
....
       -xy grid n
....
          Ajoute une grille n x n au graphe généré, ce qui est utile
          lorsque les coordonnées des points sont entiers.
          Techniquement, on ajoute au format de sortie dot un
          sous-graphe représentant la grille où les sommets et les
          arêtes sont de couleur grise. Si n=-1, alors le paramètre
          est initialisé à 1+int(sqrt(N)) ou bien à N si l'option "-xy
          permutation" est présente.
....
       -xy vsize f
....
          Facteur de grossissement des sommets pour le format dot. Par
          défaut f=1.0.
....
       -xy noise r p
....
          Effectue une pertubation aléatoire sur les positions des
	  sommets. Le déplacement de chaque sommet est effectué dans
	  sa boule de rayon r (pour p>0) selon une loi en puissance de
	  paramètre p. Prendre p=1 pour une pertubation uniforme dans
	  cette boule, p>1 pour une concentration des valeurs vers le
	  centre et p<1 pour un écartement du centre. Les valeurs <0
	  de p donne des écartements au delà du rayon r.
....
          Plus précisément, une direction (angle de 0 à 2pi) est
	  choisie aléatoirement uniformément, puis, selon cette
	  direction, un décalage aléatoire est effectué selon une loi
	  en puissance: si x est uniforme dans [0,1[, le décalage sera
	  d(x)=r*x^p.  Après cette opération, il est possible que les
	  points ne soient plus dans le rectangle d'origine, ce qui
	  peut bien sûr être corrigé par -xy scale.
....
       -xy seed k p
....
          Fixe k graines pour la génération aléatoire des coordonnées.
	  Les graines sont choisies uniformément dans le carré [0,1[ x
	  [0,1[ puis centrées par rapport à leur barycentre. Chaque
	  point est alors tiré aléatoirement autour d'une des graînes
	  et à une distance variant selon une loi en puissance de
	  paramètre p avec un rayon r ~ sqrt(ln(k)/k) (voir "noise").
....
       -xy permutation
....
          Génère les points correspondant à une permutation P
          aléatoire uniforme. Le point i à aura pour position
          (i,P(i)).
....
       -xy round p
....
          Arrondi les coordonnées à 10^-p près. Il faut p entier
          <10. Donc p=0 arrondi à l'entier le plus proche. Cet
          opérateur est appliqué après "scale". Il sert aussi à
          préciser le nombre de décimales à afficher pour l'option
          "-format xy" (par défaut p=6). Par exemple, la combinaison
          -xy scale 100 100 -xy round -1 permet d'avoir des
          coordonnées multiples de 10.
....
       -xy unique
....
          Supprime les sommets en double, correspondant aux mêmes
          positions. Cela peut être utile lorsqu'on utilise -xy round
          par exemple. Cette opération est appliquée après toutes les
          autres, notamment après scale et round. Ceci est réalisé à
          l'aide d'un tri rapide en temps O(nlogn).


   GRAPHES

       Deux types de graphes sont possibles : les graphes de base et
       les graphes composés. Ces derniers sont obtenus en paramétrant
       un graphe de base. Une catégorie importante de graphes sont les
       graphes géométriques (qui peuvent être composé ou de
       base). L'adjacence est déterminée par les coordonnées associées
       aux sommets. De nombreuses options s'y refèrent.  Ils activent
       tous par défaut l'option -pos. Les graphes orientés activent
       quant à eux tous l'option -directed.
       


   GRAPHES DE BASE :

....grid d n_1 ... n_d
....
       Grille à d dimensions de taille n_1 x ... x n_d. Si la
       dimension n_i est négative, alors cette dimension est cyclique.
       Par exemple, "grid 1 -10" donnera un cycle à 10 sommets.

....ring n k c_1 ... c_k
....
       Anneaux de cordes à n sommets avec k cordes de longueur
       c_1,...,c_k.

....cage n k c_1 ... c_k
....
       Graphe cubic pouvant servir à la construction de graphes
       n-cage, c'est-à-dire aux plus petits graphes cubic à n sommets
       de maille donnée. Ils sont toujours Hamiltoniens. Ils peuvent
       être vus comme des anneaux de cordes irréguliers. Ils sont
       construits à partir d'un cycle de longueur n découpé en n/k
       intervalles de k sommets. Le i-ème sommet de chaque intervalle,
       disons le sommet numéro j du cycle, est adjacent au sommet
       numéro j+c_i du cycle (modulo n). Les valeurs c_i peuvent être
       positives ou négatives. Cette fonction permet aussi de
       construire des graphes avec des sommets de degré 4 comme "cage
       8 2 0 2" (voir aussi le graphe de Chvátal) ou avec des sommets
       de degré 2 comme "cage 4 2 2 0".

....arboricity n k
....
       Graphe d'arboricité k aléatoire à n sommets. Ce graphe est
       composé de l'union de k arbres aléatoires. Il est donc toujours
       connexe.  Chacun des arbres est un arbre plan enraciné dont les
       sommets sont permutés aléatoirement, sauf le premier arbre dont
       les sommets forment un DFS, la racine étant numérotée 0. Ces
       graphes possèdent au plus k(n-1) arêtes, et pour k=1 il s'agit
       d'un arbre aléatoire.

....rarytree n b
....
       Arbre b-aire plan aléatoire uniforme à n noeuds internes. Il
       possède bn+1 sommets. La racine est de degré b, les autres
       sommets sont de degré b+1 (soit b fils) ou 1 (=feuille). Si
       b=1, le graphe est une union de chemin. Si n=1, alors le graphe
       est une étoile à b feuilles.

....arytree h k r
....
       Arbre de hauteur h où chaque noeud interne à exactement k fils,
       le degré de la racine étant de degré r. Notez que "arytree h 1
       r" génère une étoile de degré r où chaque branche est de
       longueur h.

....kpage n k
....
       Graphe k-pages aléatoire. Par définition on peut positionner
       les sommets en cercle, dessiner les arêtes comme des segments
       de droites, et colorier les arêtes en k couleurs de façon à ce
       que les arêtes de chaque couleur induisent un dessin de graphe
       planaire-extérieur. Les graphes 1-page sont les graphes
       planaires-extérieurs, et les 2-pages sont les sous-graphes de
       graphes planaires hamiltoniens.
....
       Ces graphes sont construits par le processus aléatoire suivant.
       On génère k graphes planaires-extérieurs aléatoires uniformes
       connexes à n sommets (plan et enraciné) grâce à une bijection
       avec les arbres plans enracinés dont tous les sommets, sauf
       ceux de la dernière branche, sont bicolorés. On fait ensuite
       l'union de ces k graphes en choisissant aléatoirement la racine
       des arbres, sauf celui du premier planaire-extérieur (ce qui
       correspond à une permutation circulaire des sommets sur la face
       extérieure).

....ktree n k
....
       k-arbre aléatoire à n sommets. Il faut n>k, mais k=0 est
       possible. Il est généré à partir d'un arbre enraciné aléatoire
       à n-k noeuds grâce à "tree n-k". Cela constitue des "sacs" que
       l'on remplit avec les n sommets suivant un parcours en
       profondeur de l'arbre. Plus précisément, on met k+1 sommets
       dans le sac racine, puis un sommet différent pour chacun des
       autre sac. Ensuite, le sommet unique de ces sacs ajoute des
       arêtes vers exactement k sommets choisis aléatoirement dans le
       sac de son père, et ces k sommets sont ajoutés à son sac.

....kpath n k
....
       k-chemin aléatoire à n sommets. La construction est similaire à
       celle utilisée pour ktree. L'arbre aléatoire est remplacé par
       un chemin. Ces graphes sont des graphes d'intervalles
       particuliers (voir "interval n").

....rig n k p
....
       Graphe d'intersections aléatoire (Uniform Random Intersection
       Graph).  Il possède n sommets, chaque sommet u étant représenté
       par un sous-ensemble S(u) aléatoires de {1,...,k} tel que i
       appartient à S(u) avec probabilité p. Si p<0, alors p est fixée
       au seuil théorique de connectivité, à savoir p=sqrt(ln(n)/(nk))
       si k>n et p=ln(n)/k sinon. Il y a une arête entre u et v ssi
       S(u) et S(v) s'intersectent. La probabilité d'avoir une arête
       entre u et v est donc P_e=1-(1-p^2)^m, mais les arêtes ne sont
       pas indépendantes (Pr(uv|uw)>Pr(uv)). En général, pour ne pas
       avoir P_e qui tend vers 1, on choisit les paramètres de façon à
       ce que kp^2<cste. Lorsque k>=n^3, ce modèle est équivalent a
       modèle des graphes aléatoires d'Erdös-Reny.

....kneser n k r
....
       Graphe de Kneser généralisé. Le graphe de Kneser K(n,k) classic
       est obtenu avec r=0. Les sommets sont tous les sous-ensembles à
       k éléments de [0,n[ (il faut donc 0<=k<=n). Les sommets i et j
       sont adjacents ssi leurs ensembles correspondant ont au plus r
       éléments en commun. Le nombre chromatique de K(n,k), établit
       par Lovasz, vaut n-2k+2 pour tout n>=2k-1>0. Le graphe de
       Petersen est le graphe K(5,2).

....gpetersen n r
....
       Graphe de Petersen généralisé P(n,r), 0<=r<n/2. Ce graphe cubic
       possède 2n sommets qui sont u_1, ..., u_n, v_1, ..., v_n. Les
       arêtes sont, pour tout i, u_i-u_{i+1}, u_i-v_i, et v_i,v_{i+r}
       (indice modulo n). Il peut être dessiné tel que toute ses
       arêtes sont de même longueur (unit distance graph). Ce graphe
       est biparti ssi n est pair et r est impair. C'est un graphe de
       Cayley ssi r^2 = 1 (modulo n). P(n,r) est Hamiltonien ssi r<>2
       ou n<>5 (modulo 6). P(n,r) est isomorphe à P(n,(n-2r+3)/2)).
       P(4,1) est le cube, P(5,2) le graphe de Petersen, P(6,2) le
       graphe de Dürer, P(8,2) le graphe de Möbius-Kantor, P(10,2) le
       dodécahèdre, P(10,3) le graphe de Desargues, P(12,5) le graphe
       de Nauru, P(n,1) un prisme.

....rpartite r a_1 ... a_d
....
       Graphe r-parti complet K_{a_1,...,a_r}. Ce graphe possède
       n=a_1+...+a_r sommets. Les sommets sont partitionnés en r parts
       comme suit: part 1 = [0,a_1[, part 2 = [a_1,a_1+a_2[, ...  part
       r = [a_1+...+a_{r-1}, n[. Les sommets i et j sont adjacents ssi
       i et j appartiennent à des parts différentes.

....ggosset p k d_1 v_1 ... d_k v_k
....
       Graphe de Gosset généralisé. Les sommets sont tous les vecteurs
       entiers de dimension d = d_1 + ... + d_k dont les coordonnées
       comprennent, pour i=1...k, exactement d_i fois la valeur
       v_i. Il existe une arête entre les vecteurs u et v si et
       seulement le produit scalaire entre u et v vaut l'entier p. Des
       valeurs intéressantes sont par exemple: 1 2 2 -1 2 0, 8 2 2 3 6
       -1, ...

....crown n
....
       Graphe biparti à 2n sommets où le i-ème sommet de la première
       partie est voisin au j-ème sommet de la seconde partie ssi
       i<>j. Pour n=3, il s'agit du cycle à 6 sommets, pour n=4, il
       s'agit du cube (à 8 sommets).

....interval n
....
       Graphe d'intersection de n intervalles d'entiers aléatoires
       uniformes pris dans [0,2n[. Des graphes d'intervalles peuvent
       aussi être générés par "kpath n k".

....permutation n
....
       Graphe de permutation sur une permutation aléatoire uniforme
       des entiers de [0,n[.

....prime n
....
       Graphe à n sommets tel que i est adjacent à j ssi i>1 et j
       divisible par i.

....paley n
....
       Graphe de Paley à n sommets. Deux sommets sont adjacents ssi
       leur différence est un carré modulo n. Il faut que n soit une
       puissance d'un nombre premier et que n=1 modulo 4. Les
       premières valeurs possibles pour n sont: 5, 9, 13, 17, 25, 29,
       37, 41, 49, ... A noter que le complément d'un graphe de Paley
       est le graphe lui même.

....mycielski k
....
       Graphe de Mycielski de paramètre (nombre chromatique) k. C'est
       un graphe sans triangle, k-1 (sommets) connexe, de nombre
       chromatique k. Le premier graphe de la série est M2 = K2, puis
       on trouve M3=C5, M4 est le graphe de Grötzsch à 11 sommmets.

....wheel n
....
       Roue à n sommets. Graphe composé d'un cycle à n sommets et d'un
       sommet universel, donc connecté à tous les autres.

....windmill n
....
       Graphe composé de n cycles de longueur trois ayant un sommet commun.

....barbell n1 n2 p
....
       Graphe des haltères (Barbell Graph) composé de deux cliques de
       n1 et n2 sommets reliées par un chemin de longueur p. Il
       possède n1+n2+p-1 sommets. Si p=0 (p=-1), le graphe est composé
       de deux cliques ayant un sommet (une arête) en commun. Plus
       généralement, si p<=0, le graphe est composé de deux cliques
       s'intersectant sur 1-p sommets.

....sat n m k
....
       Graphe issu de la réduction du problème k-SAT à Vertex
       Cover. Le calcul d'un Vertex Cover de taille minimum pour ce
       graphe est donc difficile pour k>2. Soit F une formule de k-SAT
       avec n variables x_i et m clauses de k termes. Le graphe généré
       par "sat n m k" possède un Vertex Cover de taille n+(k-1)m si
       et seulement si F est satisfiable. Ce graphe est composé d'une
       union de n arêtes et de m cliques de k sommets, plus des arêtes
       connectant certains sommets des cliques aux n arêtes. Les n
       arêtes représentent les n variables, une extrémité pour x_i,
       l'autre pour non(x_i). Ces sommets ont des numéros dans [0,2n[,
       x_i correspond au sommet 2(i-1) et non(x_i) au sommet 2i-1,
       i=1...n. Les sommets des cliques ont des numéros consécutifs >=
       2n. Le j-ème sommet de la i-ème clique est connecté à l'une des
       extrémités de l'arête k ssi la j-ème variable de la i-ème
       clause est x_k (ou non(x_i)). Ces connexions sont aléatoires
       uniformes.

....kout n k
....
       Graphe à n sommets, au plus kn arêtes et k+1 coloriable, crée
       par le processus aléatoire suivant: les sommets sont ajoutés
       dans l'ordre croissant de leur numéro, i=0,1,...n-1. Le sommet
       i est connecté à d voisins qui sont pris aléatoirement
       uniformément parmi les sommets dont le numéro est < i. La
       valeur d est choisie aléatoirement uniformément entre 1 et
       min{i,k}. Il faut k>0. Le graphe est connexe, et pour k=1, il
       s'agit d'un arbre.

....icosa
....
      Isocahèdre: graphe panaire 5-régulier à 12 sommets. Il possède
      30 arêtes et 20 faces qui sont des triangles. C'est le dual du
      dodécahèdre.

....cubocta
....
       Cuboctahèdre: graphe planaire 4-régulier à 12 sommets. Il
       possède 24 arêtes et 14 faces qui sont des triangles ou des
       carrés. C'est le dual du rhombicdodécahèdre.

....rdodeca
....
       Rhombic-dodécahèdre: graphe planaire à 14 sommets avec des
       sommets de degré 3 ou 4. Il possède 21 arêtes et 12 faces qui
       sont des carrés. C'est le dual du cuboctahèdron.

....tutte
....
       Graphe de Tutte. C'est un graphe planaire cubic à 46 sommets
       qui n'est pas Hamiltonien.

....theta0
....
       Graphe Theta_0. C'est un graphe à 7 sommets série-parallèle
       obtenu à partir d'un cycle de longueur 6 et en connectant deux
       sommets antipodaux par un chemin de longueur 2. C'est un "unit
       distance graph".

....fritsch
....
       Graphe de Fritsch. Il est planaire maximal à 9 sommets qui peut
       être vu comme un graphe de Hajós dans un triangle. C'est, avec
       le graphe de Soifer, le plus petit contre-exemple à la
       procédure de Kempe.

....soifer
....
       Graphe de Soifer. Il est planaire maximal à 9 sommets. C'est,
       avec le graphe de Fritsch, le plus petit contre-exemple à la
       procédure de Kempe.

....poussin
....
       Graphe de Poussin. Il est planaire maximal à 15 sommets. C'est
       un contre-exemple à la procédure de Kempe.

....headwood4
....
       Graphe de Headwood pour la conjecture des 4 couleurs,
       contre-exemple de la preuve de Kempe. Il est planaire maximal
       avec 25 sommets, est de nombre chromatique 4, de diamètre 5, de
       rayon 3 et Hamiltonien.

....errara
....
       Graphe d'Errara. Il est planaire maximal à 17 sommets. C'est un
       contre-exemple à la procédure de Kempe.

....kittell
....
       Graphe de Kittell. Il est planaire maximal à 23 sommets. C'est
       un contre-exemple à la procédure de Kempe.

....frucht
....
       Graphe de Frucht. Il est planaire cubic à 12 sommets. Il n'a
       pas de symétrie non-triviale. C'est un graphe de Halin de
       nombre chromatique 3, de diamètre 4 et de rayon 3.

....halin p
....
       Graphe de Halin aléatoire basé sur un arbre à p>2 feuilles. Il
       possède au plus 2p-2 sommets. Il est constitué d'un arbre sans
       sommets de degré deux dont les p feuilles sont connectés par un
       cycle (de p arêtes). Ces graphes de degré minimum au moins deux
       sont planaires, arête-minimale 3-connexes, Hamiltonien (et le
       reste après la suppression de n'importe quel sommet), de
       treewidth exactement 3 (ils contiennent K_4 comme mineur). Ils
       contiennent toujours au moins un triangle et sont de nombre
       chromatique 3 ou 4.

....butterfly d
....
       Graphe Butterfly de dimension d. Les sommets sont les paires
       (w,p) où w est un mot binaire de d bits et p un entier de
       [0,d]. Le sommet (w,p) est adjacent à (w',p+1) ssi les bits de
       w sont identiques à ceux de w' sauf pour celui de numéro p à
       partir de la gauche, bit qui peut donc être éventuellement
       différent. Il possède (d+1)*2^d sommets, tous de degré au plus
       4.

....line-graph n k
....
       Line-graphe aléatoire à n sommets et de paramètre k, entier
       >0. Plus k est petit, plus le graphe est dense, le nombre
       d'arêtes étant proportionnel à (n/k)^2. Si k=1, il s'agit d'une
       clique à n sommets. Ces graphes sont obtenus en choisissant,
       pour chaque sommet, deux couleurs parmi {1,...,k}. Deux sommets
       sont alors adjacents ssi ils possèdent la même couleur. Ces
       graphes sont claw-free (sans K_{1,3} induit). Comme les graphes
       claw-free, les line-graphes connexes avec un nombre pair de
       sommets possède toujours un couplage parfait. On rappel qu'un
       graphe G est le line-graphe d'un graphe H si les sommets de G
       correspondent aux arêtes de H et où deux sommets de G sont
       adjacents ssi les arêtes correspondantes dans H sont
       incidentes.  Pour parle parfois de graphe adjoint.

....debruijn d b
....
       Graphe de De Bruijn de dimension d et de base b. Il a b^d
       sommets qui sont tous les mots de d lettres sur un alphabet de
       b lettres. Le sommet (x_1,...,x_d) est voisin des sommets
       (x_2,...,x_d,*). Ce graphe est Hamiltonien, de diamètre d et le
       degré de chaque sommet est 2b, 2b-1 ou 2b-2. Pour d=3 et b=2,
       le graphe est planaire.

....kautz d b
....
       Graphe de Kautz de dimension d et de base b. Il a b*(b-1)^(d-1)
       sommets qui sont tous les mots de d lettres sur un alphabet de
       b lettres avec la contrainte que deux lettres consécutives
       doivent être différentes. L'adjacence est celle du graphe de De
       Bruijn. C'est donc un sous-graphe induit de De Bruijn (debruijn
       d b). Il est Hamiltonien, de diamètre d et le degré de chaque
       sommet est 2b-2 ou 2b-3. Pour d=b=3 le graphe est planaire.

....linial n t
....
       Neighborhood graph des cycles introduit par Linial. C'est le
       graphe de voisinage des vues de taille t d'un cycle orienté
       symétrique à n sommets ayant des identifiants uniques de
       [0,n[. Il faut n>=t>0. Le nombre chromatique de ce graphe est k
       ssi il existe un algorithme distribué qui en temps t-1
       (resp. en temps (t-1)/2 avec t impair) peut colorier en k
       couleurs tout cycle orienté (resp. orienté symétrique) à n
       sommets ayant des identifiants uniques et entiers de [0,n[. Les
       sommets sont les t-uplets d'entiers distincts de [0,n[. Le
       sommet (x_1,...,x_t) est voisin des sommets (x_2,...,x_t,y) où
       y<>x_1 si n>t et y=x_1 si n=t.  C'est un sous-graphe induit de
       linialc n t, et donc du graphe de Kautz (kautz t n) et de De
       Bruijn (debruijn t). Le nombre de sommets est m(m-1)...(m-t+1).
       Certaines propriétés se déduisent du graphe linialc n t.

....linialc m t
....
       Neighborhood graph des cycles colorés.  Il s'agit d'une
       variante du graphe linial n t. La différence est que les
       sommets du cycle n'ont plus forcément des identitiés uniques,
       mais seulement une m-coloration, m<=n. L'adjacence est
       identique, mais les sommets sont les (2t+1)-uplets
       (x_1,...,x_{2t+1}) d'entiers de [0,m[ tels que x_i<>x_{i+1}. Il
       s'agit donc d'un sous-graphe induit de linialc m t, lui-même
       sous-graphe induit du graphe de Kautz (kautz 2t+1 m) et donc de
       De Bruijn (debruijn 2t+1 m). Le nombre de sommets est
       m(m-1)^{2t} et son degré maximum est 2(m-1). La taille de la
       clique maximum est 3 si m>2 et t>1. Le nombre chromatique de ce
       graphe est 3 pour m=4, 4 pour 5<=m<=24. Pour 25<=m<=70 c'est au
       moins 4 et au plus 5, la valeur exacte n'étant pas connue.

....pancake n
....
       Graphe "pancake" de dimension n. Il a n! sommets qui sont les
       permutations de {1,...,n}. Les sommets x=(x_1,...,x_n) et
       y=(y_1,...,y_n) sont adjacents s'il existe un indice k tel que
       y_i=x_i pour tout i>k et y_i=x_{k-i} sinon. Dit autrement la
       permutation de y doit être obtenue en retournant un préfixe de
       x. Le graphe est (n-1)-régulier. Son diamètre qui est linéaire
       en n n'est pas précisément connu. Les premières valeurs
       connues, pour n=1...17, sont: 0, 1, 3, 4, 5, 7, 8, 9, 10, 11,
       13, 14, 15, 16, 17, 18, 19. Donc les diamètres 2, 6, 12
       n'existent pas.

....bpancake n
....
       Graphe "burn pancake" de dimension n. Il a n!*2^n sommets qui
       sont les permutations signées de {1,...,n}. Les sommets
       x=(x_1,...,x_n) et y=(y_1,...,y_n) sont adjacents s'il existe
       un indice k tel que y_i=x_i pour tout i>k et y_i=-x_{k-i}
       sinon. Dit autrement la permutation de y doit être obtenue en
       retournant un préfixe de x et en inversant les signes. Par
       exemple, le sommet (+2,-1,-5,+4) et voisin du sommet
       (+5,+1,-2,+4). Comme le graphe pancake, c'est un graphe
       (n-1)-régulier de diamètre également linéaire en n.

....gpstar n d
....
       Graphe "permutation star" généralisé de dimension n. Il a n!
       sommets qui sont les permutations de {1,...,n}. Deux sommets
       sont adjacents si leurs permutations diffèrent par d
       positions. C'est un graphe régulier.

....pstar n
....
       Graphe "permutation star" de dimension n. Il a n! sommets qui
       sont les permutations de {1,...,n}. Deux sommets sont adjacents
       si une permutation est obtenue en échangeant le premier élément
       avec un autre. Le graphe est (n-1)-régulier. Le graphe est
       biparti et de diamètre floor(3(n-1)/2). C'est un sous-graphe
       induit d'un gpstar n 1.

....hexagon p q
....
       Graphe planaire composé de p rangées de q hexagones, le tout
       arrangé comme un nid d'abeille. Ce graphe peut aussi être vu
       comme un mur de p rangées de q briques, chaque brique étant
       représentée par un cycle de longueur 6. Il possède
       (p+1)*(2p+2)-2 sommets et est de degré maximum 3. Sont dual est
       le graphe whexagon.

....whexagon p q
....
       Comme le graphe hexagon p q sauf que chaque hexagone est
       remplacé par une roue de taille 6 (chaque hexagone possède un
       sommet connecté à ses 6 sommets). C'est le dual de
       l'hexagone. Il possède pq sommets de plus que l'hexagone p q.

....hanoi n b
....
       Graphe de Hanoï généralisé, le graphe classique est obtenu avec
       b=3. Il est planaire avec b^n sommets et est défini de manière
       récursive. Le niveau n est obtenu en faisant b copies du niveau
       n-1 qui sont connectés comme un cycle par une arête, le niveau
       0 étant le graphe à un sommet. Il faut b>1. Lorsque n=2, on
       obtient un sorte de soleil.

....sierpinski n b
....
       Graphe de Sierpiński généralisé, le graphe classique, le
       triangle Sierpiński qui est planaire, est obtenu avec b=3. Il a
       ((b-2)*b^n+b)/(b-1) sommets et est défini de manière récursive.
       Le niveau n est obtenu en faisant b copies du niveau n-1 qui
       sont connectés comme un cycle, le niveau 1 étant un cycle de b
       sommets. Il faut b>2 et n>0. A la différence du graphe d'Hanoï,
       les arêtes du cycle sont contractées. Le graphe de Hajós est
       obtenu avec n=2 et b=3.

....moser
....
       Graphe "Moser spindle" découvert par les frères Moser. C'est un
       "unit distance graph" du plan (deux points sont adjacents s'ils
       sont à distance exactement 1) de nombre chromatique 4. Il est
       planaire et possède 7 sommets. On ne connaît pas d'unit
       distance graphe avec un nombre chromatique supérieur.

....markstrom
....
       Graphe de Markström. Il est cubic planaire à 24 sommets. Il n'a
       pas de cycle de longueur 4 et 8.

....robertson
....
       Graphe de Robertson. C'est le plus petit graphe 4-régulier de
       maille 5. Il a 19 sommets, est 3-coloriable et de diamètre 3.

....wiener_araya
....
       Graphe de Wiener-Araya. C'est le plus petit graphe
       hypo-hamiltonien connu (découvert en 2009), c'est-à-dire qu'il
       n'est pas Hamiltonien, alors la suppression de n'importe quel
       sommet le rend Hamiltonien. Il est planaire, possède 42 sommets
       et 67 arêtes.

....clebsch n
....
       Graphe de Clebsch d'ordre n. Il est construit à partir d'un
       hypercube de dimension n en ajoutant des arêtes entres les
       sommets opposés, c'est-à-dire à distance n. Le graphe classique
       de Clebsch est réalisé pour n=4 dont le diamètre est deux.

....flower_snark n
....
       Graphe cubic à 4n sommets construit de la manière suivante: 1)
       on part de n étoiles disjointes à 3 feuilles, la i-ème ayant
       pour feuilles les sommets notés u_i,v_i,w_i, i=1..n; 2) pour
       chaque x=u,v ou w, x_1-x_2-...-x_n induit un chemin; et 2) sont
       adjacents: u_0-u_n, v_0-w_n et w_0-v_n. Pour n>1, ces graphes
       sont non planaires, non hamiltoniens, 3-coloriables et de
       maille au plus 6.

....udg n r
....
       Graphe géométrique aléatoire (random geometric graph) sur n
       points du carré [0,1[ x [0,1[. Deux sommets sont adjacents si
       leurs points sont à distance <= r.  Il s'agit de la distance
       selon la norme L_2 (par défaut), mais cela peut être changée
       par l'option -norm. Le graphe devient connexe avec grande
       probabilité lorsque r=rc ~ sqrt(ln(n)/n). Si r<0, alors le
       rayon est initialisé à rc. Un UDG (unit disk graph) est
       normalement un graphe d'intersection de disques fermés de rayon
       1.

....gabriel n
....
       Graphe de Gabriel. Graphe géométrique défini à partir d'un
       ensemble de n points du carré [0,1[ x [0,1[.  Les points i et j
       sont adjacents ssi le plus petit disque (voir -norm) passant
       par i et j ne contient aucun autre point. Ce graphe est
       planaire et connexe. C'est un sous-graphe du graphe de
       Delaunay. Son étirement est non borné.

....rng n
....
       Graphe du proche voisinage (Relative Neighborhood
       Graph). Graphe géométrique défini à partir d'un ensemble de n
       points du carré [0,1[ x [0,1[.  Les points i et j sont
       adjacents ssi il n'existe aucun point k tel que
       max(d(k,i),d(k,j)) < d(i,j) où d est la distance (L_2 par
       défaut, voir -norm).  Dit autrement, la "lune" définie par i et
       j doit être vide. Ce graphe est planaire et connexe. C'est un
       sous-graphe du graphe de Gabriel.

....nng n
....
       Graphe du plus proche voisin (Nearest Neighbor Graph). Graphe
       géométrique défini à partir d'un ensemble de n points du carré
       [0,1[ x [0,1[.  Le point i est connecté au plus proche autre
       point (par défaut selon la norme L_2, voir -norm). Ce graphe
       est une forêt couvrante du graphe rng de degré au plus 6 (si la
       norme est L_2).

....thetagone n p k v
....
       Graphe géométrique défini à partir d'un ensemble de n points du
       carré [0,1[ x [0,1[. En général le graphe est planaire et
       connexe avec des faces internes de longueur au plus p (pour k
       diviseur de p et v=1). On peut interpréter les paramètres comme
       suit: p le nombre de cotés d'un polygone régulier, k le nombre
       d'axes (ou de direction), et v le cône de visibilité décrit par
       un réel entre 0 et 1. Toute valeur de p<3 est interprétée comme
       une valeur infinie, et le polygone régulier correspondant
       interprété comme un cercle. L'adjacence entre une paire de
       sommets est déterminée en temps O(k*n).
....
       Plus formellement, pour tout point u et v, et entier i, on
       note P_i(u,v) le plus petit p-gone (polygone régulier à p
       cotés) passant par u et v dont u est un sommet, et dont le
       vecteur allant de u vers son centre forme un angle de i*2pi/k
       avec l'axe des abscisses, intersecté avec un cône de sommet u
       et d'angle v*(p-2)*pi/p (v*pi si p est infini) et dont la
       bissectrice passe par le centre du p-gone. Alors, u est voisin
       de v s'il un existe au moins un entier i dans [0,k[ tel que
       l'intérieur de P_i(u,v) est vide. La distance entre u et le
       centre du p-gone définit alors une de distance (non symétrique)
       de u à v.
....
       Si v=1 (visibilité maximale), P_i est précisément un p-gone. Si
       v=0 (visibilité minimale), P_i est l'axe d'angle i*2pi/k pour
       un entier i. Si v=.5, P_i est un cône formant un angle égale à
       50% de l'angle défini par deux cotés consécutifs du p-gone,
       angle vallant (p-2)pi/p. Si v=2p/((p-2)k) (ou simplement 2/k si
       p est infini) alors la visibilité correspond à un cône d'angle
       2pi/k, l'angle entre deux axes. Comme il faut v<=1, cela
       implique que k>=2p/(p-2) (k>=2 si p infini). On retrouve le
       Theta_k-Graph pour chaque k>=6 en prenant p=3 et v=6/k, le
       demi-Theta-Graph pour tout k>=3 en prenant p=3 et v=3/k, le
       Yao_k-Graph pour chaque k>=2 en prenant p=0 (infini) et v=2/k,
       et la triangulation de Delaunay si p=0 (infini), k très grand
       et v=1. En fait, ce n'est pas tout-à-fait le graph Yao_k, pour
       cela il faudrait que u soit le centre du polygone (c'est-à-dire
       du cercle).

....rplg n t
....
       Random Power-Law Graph. Graphe à n sommets où les degrés des
       sommets suivent une loi de puissance. L'espérance du degré du
       sommet i=0...n-1 est w_i=(n/(i+1))^(1/(t-1)). Il s'agit d'une
       distribution particulière d'un Fixed Degree Random Graph où la
       probabilité d'avoir l'arête i-j est w_i*w_j/sum_k w_k. En
       général il faut prendre le paramètre t comme un réel de ]2,3[,
       la valeur communément observée pour le réseau Internet étant
       t=2.1.

....load file[:range]
....

       Graphe défini à partir du fichier "file" ou de l'entrée
       standard si file vaut "-". Si "file" est une famille de
       graphes, alors il est possible d'utiliser la variante
       "file:range" permettant de préciser l'identifiant du graphe
       (sinon seul le premier le graphe est chargé). Le graphe (ou la
       famille) doit être au format standard, les sommets numérotés
       par des entiers positifs. Les caractères qui ne sont pas des
       chiffres, des arêtes ("-") ou des arcs ("->") sont ignorés.
       Ceux situés sur une ligne après // sont ignorés, ce qui permet
       de mettre des commentaires. Le temps de chargement et l'espace
       de stockage sont en O(n+m).  Notez que la suite d'options "load
       file -format ..."  permet de convertir "file" au format
       souhaité. Ce graphe active l'option -directed si "file"
       correspond à un graphe est orienté, et donc -undirected n'a pas
       d'effet dans ce cas.
....
       Pour savoir comment le fichier est chargé en mémoire, lisez les
       quelques lignes de la fonction File2List du code source avec:
       sed -n '/^list [*]File2List(/,/^}$/p' gengraph.c


   GRAPHES ORIENTES :

....aqua n c_1 ... c_n
....
       Graphe orienté dont les sommets sont les suites de n entiers
       positifs dont la somme fait c_1 et dont le i-ème élément est au
       plus c_i. Cela peut être interprété comme les façons de
       répartir un liquide (en quantité c_1) dans n récipients de
       capacité c_1 ... c_n. Il y a un arc entre u->v s'il existe i et
       j tels que le récipient c_i de u a été versé dans le récipient
       c_j de v (ou inversement de c_j vers c_i).


   GRAPHES COMPOSES :

....mesh p q (= grid 2 p q)
....
       Grille 2D de p x q sommets.

....hypercube d (= grid d 2 ... 2)
....
       Hypercube de dimension d.

....path n (= grid 1 n)
....
       Chemin à n sommets.

....cycle n (= ring n 1 1)
....
       Cycle à n sommets.

....torus p q (= grid 2 -p -q)
....
       Tore à p x q sommets.

....stable n (= ring n 0)
....
       Stable à n sommets.

....clique n (= -not ring n 0)
....
       Graphe complet à n sommets.

....bipartite p q (= rpartite 2 p q)
....
       Graphe biparti complet K_{p,q}.

....claw (= rpartite 2 1 3)
....
       Graphe biparti complet K_{1,3}.

....star n (= rpartite 2 1 n)
....
       Arbre (étoile) à n feuilles et de profondeur 1.

....tree n (= arboricity n 1)
....
       Arbre plan enraciné aléatoire à n sommets. Les sommets sont
       numérotés selon un parcours DFS (et plan) à partir de la racine.

....caterpillar n (= grid 1 n-r -star r, r=random()%n)
....
       Arbre à n sommets dont les sommets internes (de degré > 1)
       induisent un chemin. Il est obtenu à partir d'un chemin de
       longueur n-r (où r est un nombre aléatoire entre 0 et n-1) et
       en appliquant l'option -star r. Pour des raisons techniques,
       l'option -seed, si elle est utilisée, doit être placée avant
       caterpillar.

....outerplanar n (= kpage n 1)
....
       Graphe planaire extérieur aléatoire connexe à n sommets (plan
       et enraciné). Ils sont en bijection avec les arbres plans
       enracinés dont tous les sommets, sauf ceux de la dernière
       branche, sont bicolorés.

....random n p (= -not ring n 0 -dele 1-p)
....
       Graphe aléatoire à n sommets et dont la probabilité d'avoir une
       arête entre chaque pair de sommets est p. L'option -dele étant
       déjà présente, il n'est pas conseillé de la réutiliser pour ce
       graphe.

....sunlet n (= grid 1 -n -star -1)
....
       Cycle à n sommets avec un sommet pendant à chacun d'eux. Un
       sunlet 3 est parfois appelé "net graph".

....lollipop n p (= barbell n p 0)
....
       Graphe "tapette à mouches" (Lollipop Graph) composé d'une
       clique à sommets relié à un chemin de longueur p. Il a n+p
       sommets.

....petersen (= kneser 5 2 0)
....
       Graphe de Kneser particulier. Il est cubic et possède 10
       sommets. Il n'est pas Hamiltonien et c'est le plus petit graphe
       dont le nombre de croisements (crossing number) est 2.

....tietze (= flower_snark 3)
....
       Graphe de Tietze. Il est cubic avec 12 sommets. Il possède un
       chemin hamiltonien, mais pas de cycle. Il peut être plongé sur
       un ruban de Möbius, a un diamètre et une maille de 3. Il peut
       être obtenu à partir du graphe de Petersen en appliquant une
       opération Y-Delta.

....mobius-kantor (= gpetersen 8 2)
....
       Graphe de Möbius-Kantor. Graphe cubic à 16 sommets de genre
       1. Il est Hamiltonien, de diamètre 4 et de maille 6.

....dodeca (= gpetersen 10 2)
....
       Dodécahèdre: graphe planaire cubic à 20 sommets. Il possède 30
       arêtes et 12 faces qui sont des pentagones. C'est le dual de
       l'icosahèdre.

....desargues (= gpetersen 10 3)
....
       Graphe de Desargues. Il est cubic à 20 sommets. Il est
       Hamiltonien, de diamètre 5 et de maille 6.

....durer (= gpetersen 6 2)
....
       Graphe de Dürer. Graphe cubic planaire à 12 sommets de diamètre
       4 et de maille 3. Il peut être vu comme un cube avec deux
       sommets opposés tronqués.

....prism n (= gpetersen n 1)
....
       Prisme, c'est-à-dire le produit cartésien d'un cycle à n
       sommets et d'un chemin à deux sommets.

....cylinder p q (= grid p -q)
....
       Produit cartésien d'un chemin à p sommets et d'un cycle à q
       sommets. Cela généralise le prisme (prism n = cylinder n 3). Un
       cube est un cylinder 2 4.

....nauru (= pstar 4)
....
       Graphe de Nauru. C'est un graphe cubic à 24 sommets. Il s'agit
       d'un graphe "permutation star" de dimension 4. C'est aussi un
       graphe de Petersen généralisé P(12,5).

....headwood (= cage 14 2 5 -5)
....
       Graphe de Headwood. C'est un graphe cubic à 14 sommets, de
       maille 6 et de diamètre 3. C'est le plus petit graphe dont le
       nombre de croisements (crossing number) est 3.

....franklin (= cage 12 2 5 -5)
....
       Graphe de Franklin. C'est un graphe cubic à 12 sommets, de
       maille 4 et de diamètre 3.

....pappus (= cage 18 6 5 7 -7 7 -7 5)
....
       Graphe de Pappus. C'est un graphe cubic à 18 sommets, de maille
       6 et de diamètre 4.

....mcgee (= cage 24 3 12 7 -7)
....
       Graphe de McGee. C'est un graphe cubic à 24 sommets, de maille
       7 et de diamètre 4.

....tutte-coexter (= cage 30 6 -7 9 13 -13 -9 7)
....
       Graphe de Tutte-Coexter appelé aussi 8-cage de Tutte. C'est un
       graphe cubic à 30 sommets, de maille 8 et de diamètre 4. C'est
       un graphe de Levi mais surtout un graphe de Moore, c'est-à-dire
       un graphe d-régulier de diamètre k dont le nombre de sommets
       est 1+d*S(d,k) (d=impair) ou 2*S(d,k) (d=pair) avec
       S(d,k)=sum_{i=0}^(k-1) (d-1)^i.

....gray (= cage 54 6 7 -7 25 -25 13 -13)
....
       Graphe de Gray. C'est un graphe cubic à 54 sommets qui peut
       être vu comme le graphe d'incidence entre les sommets d'une
       grille 3x3x3 et les 27 lignes droites de la grille. Il est
       Hamiltonien, de diamètre 6, de maille 8, et de genre 7. Il est
       arête-transitif et régulier sans être sommet-transitif.

....grotzsch (= mycielski 4)
....
       Graphe de Grötzsch. C'est le plus petit graphe sans triangle de
       nombre chromatique 4. Il possède 11 sommets et 20 arêtes. Comme
       le graphe de Chvátal, il est non-planaire de diamètre 2, de
       maille 4 et Hamiltonien. C'est le graphe de Mycielskian du
       cycle à 5 sommets.

....hajos (= sierpinski 2 3)
....
       Graphe de Hajós. Il est composé de trois triangles deux à deux
       partageant deux sommets. On peut le voir aussi comme un
       triangle dans un triangle. Il est planaire et possède 6
       sommets. C'est un graphe de Sierpinski ou encore le
       complémentaire d'un "sunlet 3".

....house (= -not grid 1 5)
....
       Graphe planaire à 5 sommets en forme de maison. C'est le
       complémentaire d'un chemin à 5 sommets.

....wagner (= ring 8 2 1 4)
....
       Graphe de Wagner appelé aussi graphe W_8. C'est un graphe cubic
       à 8 sommets qui n'est pas planaire mais sans K_5. C'est aussi
       un graphe de Möbius.

....mobius n (= ring n 2 1 n/2)
....
       Echelle de Möbius. Il s'agit d'une échelle (grille 2xn) dont
       les bords de la première et la dernière ligne sont connectés
       avec un croisement d'arêtes.

....cube (= crown 4)
....
       Hypercube de dimension 3.

....diamond (= cage 4 2 2 0)
....
       Clique à quatre sommets moins une arête.

....gosset (= ggosset 8 2 2 3 6 -1)
....
       Graphe de Gosset. Il est 27-régulier avec 56 sommets et 756
       arêtes, de diamètre, de rayon et de maille 3. Il est
       27-arête-connexe et 27-sommet-connexe. C'est localement un
       graphe de Schläfli, c'est-à-dire que pour tout sommet le
       sous-graphe induit par ses voisins est isomorphe au graphe de
       Schläfli, qui lui-même localement un graphe de Clebsch.

....binary h (= arytree h 2 2)
....
       Arbre binaire de profondeur h possédant 2^(h+1)-1 sommets. La
       racine est de degré deux.

....rbinary n (= rarytree 2 n)
....
       Arbre binaire plan aléatoire uniforme à n noeuds internes. Il
       possède 2n-1 sommets.

....tw n k (= ktree n k -dele .5)
....
       Graphe de largeur arborescente au plus k aléatoire à n
       sommets. Il s'agit d'un k-arbre partiel aléatoire dont la
       probabilité d'avoir une arête est 1/2. L'option -dele étant
       déjà présente, il n'est pas conseillé de la réutiliser pour ce
       graphe.

....pw n k (= kpath n k -dele .5)
....
       Graphe de pathwidth au plus k, aléatoire et avec n sommets.

....td-delaunay n (= thetagone n 3 3 1)
....
       Triangulation de Delaunay utilisant la distance triangulaire
       (TD=Triangular Distance). Il s'agit d'un graphe planaire défini
       à partir d'un ensemble de n points aléatoires du carré [0,1[ x
       [0,1[. Ce graphe a un étirement de 2 par rapport à la distance
       euclidienne entre deux sommets du graphe. Ce graphe, introduit
       par Chew en 1986, est le même que le graphe "demi-theta_6", qui
       est un "theta-graph" utilisant 3 des 6 cônes. La dissymétrie
       qui peut apparaître entre le bord droit et gauche du dessin est
       lié au fait que chaque sommet n'a qu'un seul axe dirigé vers la
       droite, alors qu'il y en a deux vers la gauche.

....theta n k (= thetagone n 3 k 6/k)
....
       Theta-graphe à k secteurs réguliers défini à partir d'un
       ensemble de n points du carré [0,1[ x [0,1[. Les sommets u et v
       sont adjacents si le projeté de v sur la bissectrice de son
       secteur est le sommet le plus proche de u. Ce graphe n'est pas
       planaire en général, mais c'est un spanner du graphe complet
       euclidien. Le résultat est valide seulement si k>=6.

....dtheta n k (= thetagone n 3 k/2 6/k)
....
       Demi-Theta-graphe à k secteurs réguliers défini à partir d'un
       ensemble de n points du carré [0,1[ x [0,1[. La définition est
       similaire au Theta-graphe excepté que seul 1 secteur sur 2 est
       considéré. Il faut k pair. Le résultat est valide seulement si
       k>=6. Pour k=6, ce graphe coïncide avec le graphe td-delaunay.

....yao n k (= thetagone n 0 k 2/k)
....
       Graphe de Yao à k secteurs réguliers défini à partir d'un
       ensemble de n points du carré [0,1[ x [0,1[. Les sommets u et v
       sont adjacents si v est le sommet le plus proche de u (selon la
       distance euclidienne) de son secteur. Ce graphe n'est pas
       planaire en général, mais c'est un spanner du graphe complet
       euclidien. Le résultat est valide seulement si k>=2. En fait,
       ce n'est pas tout à fait le graphe de Yao (voir thetagone).

.....
HISTORIQUE

       v1.2 octobre 2007:
            - première version

       v1.3 octobre 2008:
            - options: -shift, -width
            - correction d'un bug pour les graphes de permutation
	    - accélération du test d'ajacence pour les arbres, de O(n) à O(1),
              grâce à la représentation implicite
	    - nouveau graphes: outerplanar, sat

       v1.4 novembre 2008:
            - format de sortie: matrix, smatrix, list
            - nouveau graphe: kout
            - correction d'un bug dans l'option -width
	    - correction d'un bug dans la combinaison -format/shift/delv

       v1.5 décembre 2008:
            - correction d'un bug dans tree lorsque n=1

       v1.6 décembre 2009:
            - nouveaux graphes: rpartite, bipartite

       v1.7 janvier 2010:
            - nouveaux graphes: icosa, dodeca, rdodeca, cubocta, geo,
	      wheel, cage, headwood, pappus, mcgee, levi, butterfly,
	      hexagon, whexagone, arytree, binary, ktree, tw, kpath,
	      pw, arboricity, wagner, mobius, tutte-coexter, paley
            - nouveau format de sortie: -format dot
	    - nouvelles options: -header, -h, -redirect, -dotpdf
            - correction d'un bug dans kout, et dans tree lorsque n=0
	    - tree devient un cas particulier d'arboricity.
	    - aide en ligne pour les paramètres des graphes.

       v1.8 juillet 2010:
            - nouveaux graphes: chvatal, grotzsch, debruijn, kautz
	      gpstar, pstar, pancake, nauru, star, udg, gpetersen,
              mobius-kantor, desargues, durer, prism, franklin,
	      gabriel, thetagone, td-delaunay, yao, theta, dtheta
            - suppression du graphe geo (remplacé par udg)
            - nouvelles options: -pos, -norm, -label, -dotfilter
	    - nouvelle famille d'options: -xy file/noise/scale/seed
	    - définition plus compacte dodeca (non explicite)
	    - utilisation du générateur random() plutôt que rand().
	    - correction d'un bug dans "-format standard" qui provoquait une erreur.
	    - correction d'un bug dans kneser pour k=0, n=0 ou k>n/2.
	    - nouveaux formats: -format dot<xxx>, -format xy
	    - suppression de -dotpdf (qui est maintenant: -format dotpdf)
	    - labeling pour: gpetersen, gpstar, pstar, pancake, interval,
	      permutation

       v1.9 août 2010:
            - renome -h en -list
	    - renome -xy file en -xy load
	    - centrage des positions sur le barycentre des graines (-xy seed)
	    - nouvelles options: -star, -visu, -xy round
	    - les graphes peuvent être stockés en mémoire, sous la forme d'une liste
	      d'adjacence grâce à l'option -check.
	    - généralisation de -delv p avec p<0
	    - nouveaux graphes: caterpillar, hajos, hanoi, sierpinski
	      sunlet, load file
	    - labeling pour: hanoi, sierpinski
	    - aide sur toutes les options (nécessitant au moins un paramètre)
              et non plus seulement pour les graphes
	    - nouvelle famille d'options: -vcolor deg/degr/pal
	    - correction d'un bug pour l'aide dans le cas de commande
	      préfixe (ex: pal & paley)

       v2.0 septembre 2010:
	    - nouvelles options: -vcolor degm/list/randg, -xy unique/permutation,
	      -check bfs, -algo iso/sub
	    - l'option -xy round p admet des valeurs négatives pour p.
	    - les options "load file" et "-xy load file" permettent la
              lecture à partir de l'entrée standard en mettant
              file="-", la lecture de famille de graphes, et supporte les commentaires.
	    - les formats list/matrix/smatrix utilisent un espace
	      linéaire O(n+m) contre O(n^2) auparavant.
	    - les sommets sur le bord (graphes géométriques) ne sont plus coupés
	      (bounding-box (bb) plus grandes).
	    - nouveaux graphes: kpage, outerplanar n (=kpage n 1), rng, nng
	      fritsch, soifer, gray, hajos (qui avait été définit mais non
	      implémenté !), crown, moser, tietze, flower_snark, markstrom,
	      clebsch, robertson, kittell, rarytree, rbinary, poussin, errara
	    - les graphes de gabriel (et rng,nng) dépendent maintenant de -norm.
	    - "wheel n" a maintenant n+1 sommets, et non plus n.
	    - aide en ligne améliorée avec "?". Ex: gengraph tutte ? / -visu ?
	    - les options -help et ? permettent la recherche d'un mot clef.
	      Ex: gengraph -help planaire / ? arbre
	    - description plus compacte de tutte (et des graphes à partir d'un tableau)
	    - correction d'un bug pour rpartite (qui ne marchait pas)

       v2.1 octobre 2010:
	    - nouvelles options:
	      -check degenerate/gcolor/edge/dfs/ps1/paths/paths2/iso/sub/minor/isub
	      -filter minor/sub/iso/vertex/edge/degenerate/ps1
	      -filter degmax/degmin/deg/gcolor/component/radius/girth/diameter
	      -filter cut-vertex/biconnected/isub/all/minor-inv/isub-inv/sub-inv
            - suppression de -algo iso/sub: l'option -algo est réservée à la mis
	      au point de -check
	    - extension de -label b à b=2 qui force l'affiche des noms
              sous forme d'entiers même avec -permute.
	    - correction d'un bug pour house (qui ne marchait pas)
	    - nouveau graphe: windmill

       v2.2 novembre 2010:
            - gestion des graphes orientés: lecture d'un fichier de
              graphe (ou d'une famille avec arcs et arêtes)
	    - nouvelles options: -(un)directed, -(no)loop, -check twdeg/tw,
	      -filter tw/id/hyperbol/rename
	    - permet l'affichage de la "value" (p) dans l'option -filter
	    - nouveau graphe: aqua
	    - correction du graphe tutte-coexter et suppression du
              graphe levi (qui en fait était le graphe de tutte-coexter).
	    - généralisation de l'option "load" à load:id family

       v2.3 décembre 2010:
            - nouvelles options: -check ps1bis/edge, -filter ps1bis/tw2
	      -filter minus/minus-id/unique/connected/bipartite/forest
	      -check ps1ter
	    - remplacement de atof()/atoi() par strtod()/strtol() qui
	      sont plus standards.
	    - remplacement de LONG_MAX par RAND_MAX dans les
              expressions faisant intervenir random() qui est de type
              long mais qui est toujours dans [0,2^31[, même si
              sizeof(long)>4. Il y avait un bug pour les architectures
              avec sizeof(long)=8.
	    - nouveau graphe: cylinder
	    - suppression de la variante "load:id" au profit de la
              forme plus générale "file:range" valable pour load, -filter, etc.

       v2.4 janvier 2011:
            - correction d'un bug dans -filter minus-id
	    - correction d'un bug dans rpartite (incorrect à partir de r>5 parts)
	    - correction d'un bug dans whexagon (nb de sommets incorrects)
	    - nouvelles options: -check ps1x/girth, -filter ps1c/ps1x
	    - renomage: ps1bis -> ps1b, ps1ter -> ps1c
	    - nouveau graphe: mycielski
	    - la graphe grotzsch est maintenant défini à partir du graphe
	      mycielski (la définition précédante était fausse)
	    - bug détecté: td-delaunay 500 -check gcolor -format no -seed
              7 | grep '>6' qui donne jusqu'à 7 couleurs; le nb de
              couleurs affiché dans -check gcolor est erroné

       v2.5 mars 2011:
	    - nouveaux graphes: line-graph, claw

       v2.6 juin 2011:
	    - amélioration du test -filter ps1: détection de cliques et d'arbres

       v2.7 octobre 2011:
	    - nouvelle option: -check bellman (pour les géométriques seulement)
	    - ajoût des champs xpos,ypos à la structure "graph".
	    - nouveaux graphes: linial, linialc, cube, diamond, theta0,

       v2.8 novembre 2011:
	    - nouveaux graphes: ggosset, gosset, rplg, wiener_araya, headwood4
	    - correction d'un bug pour "-xy seed k n" lorsque k=1.
	    - nouvelles options: -check maincc, -maincc (non documentée)

       v2.9 février 2013:
	    - nouveau graphe: frucht, halin
	    - correction d'un bug pour "-check gcolor" qui ne
	      retrounait pas le nombre correct de couleurs, et qui de
	      plus n'utilisait pas l'heuristique du degré minimum.
	    - correction d'un bug dans le graphe permutation avec
	      l'option -label 1.

       v3.0 octobre 2013:
	    - nouveaux graphes: rig, barbell, lollipop
	    - généralisation de l'option -filter forest
	    - nouvelles options: -apex, -filter isforest, -filter istree, -filter cycle
	    - correction d'un bug dans -filter vertex
	    - amélioration de l'aide lors d'erreurs de paramètre comme:
              "-filter F vertex" au lieu de "-filter F vertex n"
	    - amélioration de l'option -header

       v3.1 décembre 2013:
	    - nouveaux graphes: bpancake
	    - légère modification des labels des sommets des graphes pancake, 
	      gpstar et pstar
	    - nouvelles options: -xy grid, -xy vsize
	    - modification de la taille des sommets pour dot permettant de tenir
	      compte de -xy scale.

       v3.2 mai 2014:
            - amélioration du test ps1b (ajoût de règle et réduction
	      du nombre d'indéterminées dans graphes des conflits)

       v3.3 juillet 2014:
            - modification importante du code pour -check ps1
            - modification des graphes linial et linialc
	    - nouvelles options: -check kcolor, -vcolor kcolor, -len, -check kcolorsat
### #*/
