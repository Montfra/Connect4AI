/*
	Canvas pour algorithmes de jeux à 2 joueurs
	
	joueur 0 : humain
	joueur 1 : ordinateur
			
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Paramètres du jeu
#define LARGEUR_MAX 8 		// nb max de fils pour un noeud (= nb max de coups possibles)

#define TEMPS 5		// temps de calcul pour un coup avec MCTS (en secondes)

#define rows 8
#define cols 7
// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

// Critères de fin de partie
typedef enum {NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {

	int joueur; // à qui de jouer ?

	char plateau[cols][rows];

} Etat;

// Definition du type Coup
typedef struct {
	int colonne;

} Coup;

// Copier un état 
Etat * copieEtat( Etat * src ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	etat->joueur = src->joueur;
	
	int i,j;
	for (i=0; i< cols; i++)
		for ( j=0; j<rows; j++)
			etat->plateau[i][j] = src->plateau[i][j];
	

	
	return etat;
}

// Etat initial 
Etat * etat_initial( void ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));


	int i,j;	
	for (i=0; i< cols; i++)
		for ( j=0; j<rows; j++)
			etat->plateau[i][j] = ' ';
	
	return etat;
}


void afficheJeu(Etat * etat) {

	int i,j;
	printf("   |");
	for ( j = 0; j < rows; j++)
		printf(" %d |", j);
	printf("\n");
	printf("------------------------------------");
	printf("\n");
	
	for(i=0; i < cols; i++) {
		printf(" %d |", i);
		for ( j = 0; j < rows; j++)
			printf(" %c |", etat->plateau[i][j]);
		printf("\n");
		printf("------------------------------------");
		printf("\n");
	}
}


// Nouveau coup 
Coup * nouveauCoup( int j ) {
	Coup * coup = (Coup *)malloc(sizeof(Coup));
	
	coup->colonne = j;
	
	return coup;
}

// Demander à l'humain quel coup jouer 
Coup * demanderCoup () {
    int j;
	printf(" quelle colonne ? ") ;
	scanf("%d",&j); 
	
	return nouveauCoup(j);
}

// Modifier l'état en jouant un coup 
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {

	if (coup->colonne > rows) return 0;

    for (int i = cols - 1; i >= 0; --i) {
        if ( etat->plateau[i][coup->colonne] == ' ' ) {
            etat->plateau[i][coup->colonne] = etat->joueur ? 'O' : 'X';

            // à l'autre joueur de jouer
            etat->joueur = AUTRE_JOUEUR(etat->joueur);

            return 1;
        }
    }
    return 0;
}

// Retourne une liste de coups possibles à partir d'un etat 
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {
	
	Coup ** coups = (Coup **) malloc((1+LARGEUR_MAX) * sizeof(Coup *) );
	
	int k = 0;
	
    for (int i = 0; i < rows; ++i) {
        for (int j = cols - 1; j >= 0 ; --j) {
            if ( etat->plateau[j][i] == ' ' ) {
                coups[k] = nouveauCoup(i);
                k++;
                break;
            }
        }
    }
	
	coups[k] = NULL;

	return coups;
}


// Definition du type Noeud 
typedef struct NoeudSt {
		
	int joueur; // joueur qui a joué pour arriver ici
	Coup * coup;   // coup joué par ce joueur pour arriver ici
	
	Etat * etat; // etat du jeu
			
	struct NoeudSt * parent; 
	struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
	int nb_enfants;	// nb d'enfants présents dans la liste
	
	// POUR MCTS:
	int nb_victoires;
	int nb_simus;
	
} Noeud;


// Créer un nouveau noeud en jouant un coup à partir d'un parent 
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {
	Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));

    for (int i = 0; i < LARGEUR_MAX; ++i) {
        noeud->enfants[i] = NULL;
    }
	
	if ( parent != NULL && coup != NULL ) {
		noeud->etat = copieEtat ( parent->etat );
		jouerCoup ( noeud->etat, coup );
		noeud->coup = coup;			
		noeud->joueur = AUTRE_JOUEUR(parent->joueur);		
	}
	else {
		noeud->etat = NULL;
		noeud->coup = NULL;
		noeud->joueur = 0; 
	}
	noeud->parent = parent; 
	noeud->nb_enfants = 0; 
	
	// POUR MCTS:
	noeud->nb_victoires = 0;
	noeud->nb_simus = 0;
	

	return noeud; 	
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
	Noeud * enfant = nouveauNoeud (parent, coup ) ;
	parent->enfants[parent->nb_enfants] = enfant;
	parent->nb_enfants++;
	return enfant;
}

void freeNoeud ( Noeud * noeud) {
	if ( noeud->etat != NULL)
		free (noeud->etat);

	int i = 0;
	while (noeud->enfants[i] != NULL && i < LARGEUR_MAX) {
        freeNoeud(noeud->enfants[i]);
        i++;
	}
    noeud->nb_enfants = 0;
	
	if ( noeud->coup != NULL)
		free(noeud->coup); 

	free(noeud);
}
	

// Test si l'état est un état terminal 
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {

    // tester si un joueur a gagné
	int i,j,k,n = 0;
	for ( i=0;i < cols; i++) {
		for(j=0; j < rows; j++) {
			if ( etat->plateau[i][j] != ' ') {
				n++;	// nb coups joués

				// lignes
				k=0;
				while ( k < 4 && i-k >= 0 && etat->plateau[i-k][j] == etat->plateau[i][j] )
                    k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// colonnes
				k=0;
				while ( k < 4 && j-k >= 0 && etat->plateau[i][j-k] == etat->plateau[i][j] )
				    k++;

				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// diagonales
				k=0;
				while ( k < 4 && i-k >= 0 && j+k < rows && etat->plateau[i-k][j+k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				k=0;
				while ( k < 4 && i-k >= 0 && j-k >= 0 && etat->plateau[i-k][j-k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;		
			}
		}
	}

	// printf("nnbCoup %i", n);
	// et sinon tester le match nul	
	if ( n == 55 )
		return MATCHNUL;
		
	return NON;
}

float mu(Noeud * i) {
    if (i->nb_simus == 0) {
        return 0;
    }

    if(i->parent->etat->joueur == 0) {
        return (float)((float)i->nb_simus - (float)i->nb_victoires) / (float)i->nb_simus;
    }

    return (float)i->nb_victoires / (float)i->nb_simus;
}

float B(Noeud * i) {
    float N = i->nb_simus;
    float NParent = i->parent->nb_simus;
    float C = sqrt(2);
    float u = mu(i);

    float B = u + C * sqrt( log(NParent) / N );

    return B;
}


Noeud * selection(Noeud * racine, int * bes) {
    int nbfils;

    // Calcul le nombre de fils
    Coup ** coups = coups_possibles(racine->etat);
    int k = 0;
    while ( coups[k] != NULL) {
        k++;
    }

    nbfils = k;

    if (nbfils == racine->nb_enfants && nbfils != 0) {
        // Tous les fils sont déjà developé

        // Choix du meilleur fils
        float actualRatio;
        float bestRatio = B(racine->enfants[0]);
        int best = 0;


        for (int i = 0; i < racine->nb_enfants; i++) {

            actualRatio = B(racine->enfants[i]);

            if (bestRatio < actualRatio) {
                bestRatio = actualRatio;
                best = i;
            }

            if(bestRatio == actualRatio) {
                int ran = rand() % 2;
                if (ran == 0) {
                    bestRatio = actualRatio;
                    best = i;
                }
            }
        }

        if (bes != NULL) {
            *bes = best;
        }

        return selection(racine->enfants[best], NULL);

    }
    else {
        // Tous les fils ne sont pas encore developé
        return racine;
    }

}

int choixRandomDev(Noeud * noeud) {

    Coup ** coups = coups_possibles(noeud->etat);
    int k = 0;
    while ( coups[k] != NULL) {
        k++;
    }

    if (k == 0) {return -1;}

    int choix = -1;
    while (choix == -1) {
        choix = rand() % k;

        for (int i = 0; i < noeud->nb_enfants; ++i) {
            if (noeud->enfants[i]->coup->colonne == choix) {
                choix = -1;
            }
        }
    }

    return choix;
}

int choixRandom(Noeud * noeud) {
    Coup ** coups = coups_possibles(noeud->etat);
    int k = 0;
    while ( coups[k] != NULL) {
        k++;
    }

    if (noeud->nb_enfants == k || k == 0) { return -1;}

    return rand() % noeud->nb_enfants;
}

void developper(Noeud * noeud) {
    // Choisit un fils aléatoirement
    Coup ** coups = coups_possibles(noeud->etat);

    int choix = choixRandomDev(noeud);

    if (choix == -1) return;

    Coup * meilleur_coup = coups[ choix ];

    // Developper ce fils
    ajouterEnfant(noeud, meilleur_coup);
}

FinDePartie simuler(Noeud * noeud) {
    FinDePartie fin;
    int res = 0;
    fin = testFin( noeud->etat );
    while ( fin == NON ) {
        int choix = choixRandom(noeud);

        if (choix == -1) {
            fin = MATCHNUL;
            break;
        }

        noeud = noeud->enfants[choix];

        if (noeud->parent->joueur == 1) {
            noeud->joueur = 0;
        }
        else {
            noeud->joueur = 1;
        }

        developper(noeud);
        res++;
        fin = testFin( noeud->etat );
    }

    if (fin == ORDI_GAGNE) {
        fin = ORDI_GAGNE;
    }
    else {
        fin = MATCHNUL;
    }

    return fin;
}

void update(Noeud * noeud, FinDePartie resultat) {

    while (noeud->parent != NULL) {
        noeud->nb_simus++;

        if ( resultat == ORDI_GAGNE ) {
            noeud->nb_victoires++;
        }

        noeud = noeud->parent;
    }

    noeud->nb_simus++;

    if ( resultat == ORDI_GAGNE ) {
        noeud->nb_victoires++;
    }
}

// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat * etat, int tempsmax) {

	clock_t tic, toc;
	tic = clock();
	int temps;

	Coup ** coups;
	
	// Créer l'arbre de recherche
	Noeud * racine = nouveauNoeud(NULL, NULL);
	racine->etat = copieEtat(etat);

	// créer les premiers noeuds:
	coups = coups_possibles(racine->etat);
	int k = 0;
	Noeud * enfants;
	while ( coups[k] != NULL) {
		enfants = ajouterEnfant(racine, coups[k]);
		k++;
	}


	int iter = 0;
	int win = 0;
    int best;
	do {


	    // AMELIORATION
	    if (iter == 0) {
	        int test = 0;
            for (int i = 0; i < racine->nb_enfants; ++i) {
                if (testFin(racine->enfants[i]->etat) == ORDI_GAGNE) {
                    best = i;
                    test = 1;
                }
            }

            if (test == 1) {
                break;
            }
	    }
	    // FIN AMELIORATION


	    // MCTS
	    Noeud * actualRacine = selection(racine, &best);

        developper(actualRacine);

        FinDePartie res = simuler(actualRacine);


        if (res == ORDI_GAGNE) {
            win++;
        }

        update(actualRacine, res);

		toc = clock(); 
		temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
		iter ++;
	} while ( temps < tempsmax);

	printf("Nombre de simulation %i\n", racine->nb_simus);
	printf("Pourcentage de chance de ganger %f %%", ((float)racine->nb_victoires / (float)racine->nb_simus) * 100.0f);
	// fin de l'algorithme
	
	// Jouer le meilleur premier coup
	jouerCoup(etat, coups[best] );
	// Penser à libérer la mémoire :
	freeNoeud(racine);
	free (coups);
}

int main(void) {

	Coup * coup;
	FinDePartie fin;
	
	// initialisation
	Etat * etat = etat_initial(); 
	
	// Choisir qui commence : 
	printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
	scanf("%d", &(etat->joueur) );
	
	// boucle de jeu
	do {
		printf("\n");
		afficheJeu(etat);
		
		if ( etat->joueur == 0 ) {
			// tour de l'humain
			
			do {
				coup = demanderCoup();
			} while ( !jouerCoup(etat, coup) );
									
		}
		else {
			// tour de l'Ordinateur
			
			ordijoue_mcts( etat, TEMPS );
			
		}
		
		fin = testFin( etat );
	}	while ( fin == NON ) ;

	printf("\n");
	afficheJeu(etat);
		
	if ( fin == ORDI_GAGNE )
		printf( "** L'ordinateur a gagné **\n");
	else if ( fin == MATCHNUL )
		printf(" Match nul !  \n");
	else
		printf( "** BRAVO, l'ordinateur a perdu  **\n");
	return 0;
}
