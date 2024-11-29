#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define MAX_LEN_STR 20
#define NUM_BUCKET_MAGAZZINO 3121
#define NUM_BUCKET_RICETTARIO 3121 
#define MAX_LEN_COMANDO 17
#define DIM_INIT_ARRAY_LOTTI 1

typedef struct __attribute__((__packed__)){
    unsigned int peso;
    unsigned int scadenza;
} lotto_t; 

typedef lotto_t * ptr_lotti; 

typedef struct __attribute__((__packed__)) nodiMagazzino {
    char nome[MAX_LEN_STR];
    ptr_lotti * array_lotti; 
    struct nodiMagazzino * next;
    unsigned int sum_peso_lotti;
    unsigned int heap_size;
} nodo_lista_magazzino_t; /*NODI DI LISTA DEGLI INGREDIENTI, PARTE DALLA TABELLA HASH MAGAZZINO*/

typedef struct __attribute__((__packed__)) nodiIngredienti {
    struct nodiIngredienti * next;
    unsigned int peso;
    nodo_lista_magazzino_t * ptr_ingrediente_magazzino;
} nodo_lista_ingredienti_t; /*LISTA DEGLI INGREDIENTI DI OGNI RICETTA, PARTE DALLA RICETTA (NODO DI LISTA CONCATENATA CHE PARTE DAL BUCKET DEL RICETTARIO)*/

typedef struct __attribute__((__packed__)) nodiRicette {
    char nome[MAX_LEN_STR];
    nodo_lista_ingredienti_t * testa_lista_ingredienti;
    struct nodiRicette * next;
    short int presente_in_ordini; /*VA INCREMENTATO SOLO IN CASO DI ORDINE NON RIFIUTATO E DECREMENTATO SOLO CON CORRIERE*/
} nodo_lista_ricettario_t; /*NODI DI LISTA DELLE RICETTE, PARTE DALLA TABELLA HASH RICETTARIO*/

typedef struct nodiOrdiniAttesa {
    unsigned int data_arrivo;
    char nome_ricetta[MAX_LEN_STR];
    short int quantita;
    nodo_lista_ingredienti_t * OrdAtt_testa_lista_ingredienti;
    struct nodiOrdiniAttesa *next;
} nodo_lista_ordini_attesa_t; /*STRUCT LISTA ORDINI ATTESA: CODA FIFO*/

typedef struct {
    nodo_lista_ordini_attesa_t * testa_lista_ordini_attesa;
    nodo_lista_ordini_attesa_t * coda_lista_ordini_attesa;
} lista_ordini_attesa_t;

typedef struct __attribute__((__packed__)) nodiOrdiniPronti {
    struct nodiOrdiniPronti * next;
    unsigned int data_arrivo;
    char nome_ricetta[MAX_LEN_STR];
    unsigned int peso; /*TOTALE ORDINE, OSSIA PESO ELEMENTO * QUANTITA*/
    short int num_elementi;
} nodo_lista_ordini_pronti_t;

typedef struct {
    nodo_lista_ordini_pronti_t * testa_lista_ordini_pronti;
    nodo_lista_ordini_pronti_t * coda_lista_ordini_pronti;
} lista_ordini_pronti_t;

/*****************************************************************************/
/*                             PROTOTIPI                                     */
/*****************************************************************************/

unsigned long hash_lotto(char str[]);

unsigned long hash_ricetta(char str[]);

void rifornimento_un_lotto(nodo_lista_magazzino_t * magazzino[], char nome[], unsigned int, unsigned int);

void scorri_ordini_attesa_prepara(nodo_lista_magazzino_t * magazzino[], lista_ordini_attesa_t * , lista_ordini_pronti_t *, unsigned int);

void aggiungi_ricetta(nodo_lista_ricettario_t * ricettario[], nodo_lista_magazzino_t * magazzino[]);

void rimuovi_ricetta(nodo_lista_ricettario_t *ricettario[], char nome_ricetta[]);

nodo_lista_magazzino_t * cerca_ptr_ingrediente_magazzino(nodo_lista_magazzino_t * magazzino[], char nome_ingrediente[]);

void ordine(nodo_lista_ricettario_t *ricettario[], nodo_lista_magazzino_t * magazzino[], lista_ordini_pronti_t * , lista_ordini_attesa_t * , unsigned int);

nodo_lista_ordini_pronti_t * unisci(nodo_lista_ordini_pronti_t * , nodo_lista_ordini_pronti_t * );

void corriere(lista_ordini_pronti_t * , nodo_lista_ricettario_t * ricettario[], unsigned int capienza_camioncino, unsigned int tempo);

void merge_sort_list(nodo_lista_ordini_pronti_t ** );

void cancella_lista_ingredienti(nodo_lista_ingredienti_t * );

/*FUNZIONI PER GESTIRE HEAP*/
void min_heapify(ptr_lotti A[], unsigned int i, nodo_lista_magazzino_t *);

ptr_lotti min_heap_minimo(ptr_lotti A[], nodo_lista_magazzino_t *);

ptr_lotti min_heap_estrai_minimo(ptr_lotti A[], nodo_lista_magazzino_t * );

/* TODO: OTTIMIZZAZIONE INSERISCI LOTTO: SOMMARE IL PESO A UN LOTTO CON SCADENZA UGUALE*/
ptr_lotti * min_heap_inserisci(ptr_lotti * A, nodo_lista_magazzino_t * , ptr_lotti);

/*****************************************************************************/
/*                                MAIN                                       */
/*****************************************************************************/

int main(void){
    unsigned int tempo = 0;
    unsigned int periodo, capienza;
    char comando[MAX_LEN_COMANDO];
    char c_controllo = 'a';

    nodo_lista_magazzino_t *magazzino[NUM_BUCKET_MAGAZZINO];
    nodo_lista_ricettario_t *ricettario[NUM_BUCKET_RICETTARIO];
    nodo_lista_ordini_pronti_t * tmp_pronti;
    nodo_lista_ordini_attesa_t * tmp_attesa;
    nodo_lista_ordini_attesa_t * del_attesa;
    nodo_lista_ordini_pronti_t * del_pronti;
    nodo_lista_ricettario_t * tmp_ric;
    nodo_lista_ricettario_t * del_ric;
    nodo_lista_magazzino_t * tmp_mag;
    nodo_lista_magazzino_t * del_mag;
    
    /*INIZIALIZZA A NULL PER AVERE UNA CONDIZIONE DI "BUCKET NON RIEMPITO"*/
    for (int j = 0; j < NUM_BUCKET_MAGAZZINO; j++){
        magazzino[j] = NULL;
    }
    for (int j = 0; j < NUM_BUCKET_RICETTARIO; j++){
        ricettario[j] = NULL;
    }

    char nome_ingr_lotto_curr_rifornimento[MAX_LEN_STR];
    unsigned int scadenza_lotto_curr_rifornimento;
    unsigned int peso_lotto_curr_rifornimento;
    char nome_ricetta_da_rimuovere[MAX_LEN_STR];

    /*INIZIALIZZA ORDINI ATTESA*/
    lista_ordini_attesa_t lista_ordini_attesa;
    lista_ordini_attesa.coda_lista_ordini_attesa = NULL;
    lista_ordini_attesa.testa_lista_ordini_attesa = NULL;
    /*INIZIALIZZA ORDINI PRONTI*/
    lista_ordini_pronti_t lista_ordini_pronti;
    lista_ordini_pronti.testa_lista_ordini_pronti = NULL;
    lista_ordini_pronti.coda_lista_ordini_pronti = NULL;

    /* LEGGI CAPIENZA E PERIODO E INSERISCI IN VAR */
    if (scanf("%u", &periodo) == 1);
    if (scanf("%u", &capienza) == 1);
    /* COMINCIO IL CICLO */
    while (scanf("%s", comando) != EOF){
        if ((tempo % periodo) == 0 && tempo != 0){ /*CHIAMA CORRIERE*/
            corriere(&lista_ordini_pronti, ricettario, capienza, tempo);
        }
        if (strcmp(comando, "ordine") == 0){
            ordine(ricettario, magazzino, &lista_ordini_pronti, &lista_ordini_attesa, tempo);
        }
        else if (strcmp(comando, "rifornimento") == 0){
            /*CICLO WHILE CHE LEGGE FINCHE NON TROVI UN NUOVO COMANDO*/
            while (c_controllo != '\n' && c_controllo != EOF){
                if (scanf("%s", nome_ingr_lotto_curr_rifornimento) == 1); /*QUI PUOI OTTIMIZZARE METTENDO STA ROBA IN RIFORNIMENTO_UN_LOTTO*/
                if (scanf("%d", &peso_lotto_curr_rifornimento) == 1);
                if (scanf("%d", &scadenza_lotto_curr_rifornimento) == 1);
                rifornimento_un_lotto(magazzino, nome_ingr_lotto_curr_rifornimento, scadenza_lotto_curr_rifornimento, peso_lotto_curr_rifornimento);
                c_controllo = (char)getchar_unlocked();
            }
            /*SCORRI TUTTO IL DATABASE DEGLI ORDINI IN ATTESA E PREPARA GLI ORDINI PREPARABILI*/
            scorri_ordini_attesa_prepara(magazzino, &lista_ordini_attesa, &lista_ordini_pronti, tempo);
            printf("rifornito\n");
            c_controllo = 'a';
        }
        else if (strcmp(comando, "aggiungi_ricetta") == 0){ 
            aggiungi_ricetta(ricettario, magazzino);
        }
        else if (strcmp(comando, "rimuovi_ricetta") == 0){ 
            if (scanf("%s", nome_ricetta_da_rimuovere) == 1);
            rimuovi_ricetta(ricettario, nome_ricetta_da_rimuovere);
        }

        tempo++;
    }
    if (tempo % periodo == 0){ /*ULTIMA CHIAMATA CORRIERE*/
        corriere(&lista_ordini_pronti, ricettario, capienza, tempo);
    }
    tmp_attesa = lista_ordini_attesa.testa_lista_ordini_attesa;
    while(tmp_attesa != NULL){
        del_attesa = tmp_attesa;
        tmp_attesa = tmp_attesa->next;
        free(del_attesa);
    }
    tmp_pronti = lista_ordini_pronti.testa_lista_ordini_pronti;
    while(tmp_pronti != NULL){
        del_pronti = tmp_pronti;
        tmp_pronti = tmp_pronti->next;
        free(del_pronti);
    }
    for (int j = 0; j < NUM_BUCKET_RICETTARIO; j++){
        tmp_ric = ricettario[j];
        while (tmp_ric!=NULL){
            cancella_lista_ingredienti(tmp_ric->testa_lista_ingredienti);
            del_ric=tmp_ric;
            tmp_ric=tmp_ric->next;
            free(del_ric);
        }
    }
    for (int j = 0; j < NUM_BUCKET_MAGAZZINO; j++){
        tmp_mag = magazzino[j];
        while(tmp_mag!= NULL){
            del_mag=tmp_mag;
            tmp_mag = tmp_mag->next;
            free(del_mag->array_lotti);
            free(del_mag);
        }
    }
}

/*****************************************************************************/
/*                                RIFORNIMENTO                               */
/*****************************************************************************/
/*RIFORNIMENTO UN LOTTO:
1. SE BUCKET INGREDIENTE E' VUOTO, ALLOCA UN NUOVO NODO TESTA DELLA LISTA INGREDIENTE (SETTA NOME E PESO) E UN HEAP DI HEAP SIZE 1 IN 
    CUI METTI IL LOTTO (ANCHE LUI DA CREARE E INIZIALIZZARE PESO E SCADENZA)
2. SE BUCKET NON E' VUOTO, SCORRI LISTA E SE NON TROVI NODO ALLOCA UN NUOVO NODO IN TESTA ALLA LISTA  (SETTA NOME E PESO) E UN HEAP 
    DI HEAP SIZE 1 IN CUI METTI IL LOTTO (ANCHE LUI DA CREARE E INIZIALIZZARE PESO E SCADENZA). ALTRIMENTI VAI A 3.
3. SE IL BUCKET NON E' VUOTO, CREA UN NUOVO LOTTO INSERISCI_MIN_HEAP E AGGIORNA SUM_PESO
NB: INGREDIENTE HA SOMMA_PESO_LOTTI_CORRENTI E NOME (E HEAP SIZE E DIM MALLOC PER L'HEAP MA DI ESSI SI OCCUPANO LE FUNZIONI PER GLI
    HEAP), LOTTO HA SCADENZA E PESO
*/

void rifornimento_un_lotto(nodo_lista_magazzino_t * magazzino[], char nome_ingr_lotto_curr[], unsigned int scadenza_lotto_curr, unsigned int peso_lotto_curr){
    unsigned long hash_curr = hash_lotto(nome_ingr_lotto_curr);
    nodo_lista_magazzino_t * ptr_testa_lista_ingr_magazzino;
    lotto_t * ptr_lotto_curr;
    if (magazzino[hash_curr] ==  NULL){ /*SE BUCKET E' VUOTO*/
        /*ALLORA DEVO ALLOCARE UNA NUOVA LISTA INGREDIENTI, CON IL NOSTRO IN TESTA*/ 
        magazzino[hash_curr] = (nodo_lista_magazzino_t *)malloc(sizeof(nodo_lista_magazzino_t));
        ptr_testa_lista_ingr_magazzino = magazzino[hash_curr];
        if (ptr_testa_lista_ingr_magazzino == NULL){/*CHECK MALLOC*/
            printf("allocazione andata male\n");
        }
        ptr_testa_lista_ingr_magazzino->next = NULL;
        strcpy(ptr_testa_lista_ingr_magazzino->nome, nome_ingr_lotto_curr);
        ptr_testa_lista_ingr_magazzino->sum_peso_lotti = peso_lotto_curr;
        /*ALLOCA ARRAY LOTTI ASSOCIATO ALL'INGREDIENTE IN INPUT, ASSEGNA PRIMO ELEMENTO AL LOTTO INPUT*/
        ptr_testa_lista_ingr_magazzino->array_lotti = (ptr_lotti *)malloc(sizeof(ptr_lotti)*DIM_INIT_ARRAY_LOTTI);
        if (ptr_testa_lista_ingr_magazzino == NULL){/*CHECK MALLOC*/
            printf("allocazione andata male\n");
        }
        /*ALLOCA STRUCT LOTTO ASSEGNA PRIMO ELEMENTO AL LOTTO INPUT*/
        ptr_lotto_curr = (ptr_lotti)malloc(sizeof(lotto_t));
        if (ptr_lotto_curr == NULL){/*CHECK MALLOC*/
            printf("allocazione andata male\n");
        }
        ptr_lotto_curr->peso = peso_lotto_curr;
        ptr_lotto_curr->scadenza = scadenza_lotto_curr;
        ((ptr_testa_lista_ingr_magazzino->array_lotti)[0]) = ptr_lotto_curr; /*INSERISCI LOTTO*/
        ptr_testa_lista_ingr_magazzino->heap_size = 1;
    }
    else { /*SE BUCKET NON E' VUOTO*/
        /*ALLORA DEVO SCORRERE LA LISTA E CERCARE SE C'E' L'INGREDIENTE*/
        nodo_lista_magazzino_t * ptr_lista_nodo_ingr_curr = magazzino[hash_curr]; /*SETTA ALLA TESTA DELLA LISTA E CERCA INGR NOME*/
        while (ptr_lista_nodo_ingr_curr != NULL && strcmp(ptr_lista_nodo_ingr_curr->nome, nome_ingr_lotto_curr) != 0){
            ptr_lista_nodo_ingr_curr = ptr_lista_nodo_ingr_curr->next;
        }
        /*TERMINATO IL CICLO IN ptr_lista_nodo_ingr_curr HO IL NODO DELL'INGR CERCATO O NULL SE NON C'E'*/
        if (ptr_lista_nodo_ingr_curr != NULL){ /*HO TROVATO L'INGREDIENTE*/
            /*DEVO INSERIRE IL LOTTO NELLA CODA DI PRIORITA, E AGGIORNARE CONTATORE PESO ALLOCA LOTTO*/
            ptr_lotto_curr = (lotto_t *)malloc(sizeof(lotto_t));
            if (ptr_lotto_curr == NULL){/*CHECK MALLOC*/
                printf("allocazione andata male\n");
            }
            ptr_lotto_curr->peso = peso_lotto_curr;
            ptr_lotto_curr->scadenza = scadenza_lotto_curr;
            /*INSERISCI LOTTO*/
            ptr_lista_nodo_ingr_curr->array_lotti = min_heap_inserisci(ptr_lista_nodo_ingr_curr->array_lotti, ptr_lista_nodo_ingr_curr, ptr_lotto_curr);
            ptr_lista_nodo_ingr_curr->sum_peso_lotti = (ptr_lista_nodo_ingr_curr->sum_peso_lotti) + peso_lotto_curr;
        }
        else{ /*L'INGREDIENTE NON E' NELLA LISTA (ptr_lista_nodo_ingr_curr==NULL)*/
            /*CREA NODO PER L'INGREDIENTE E INSERISCI IN TESTA ALLA LISTA*/
            ptr_lista_nodo_ingr_curr = (nodo_lista_magazzino_t *)malloc(sizeof(nodo_lista_magazzino_t)); /*CREA NODO INGREDIENTE*/
            if (ptr_lista_nodo_ingr_curr == NULL){/*CHECK MALLOC*/
                printf("allocazione andata male\n");
            }
            strcpy(ptr_lista_nodo_ingr_curr->nome, nome_ingr_lotto_curr);
            magazzino[hash_curr]->sum_peso_lotti = peso_lotto_curr;
            ptr_lista_nodo_ingr_curr->next = magazzino[hash_curr]; /*INSERISCI IN TESTA*/
            magazzino[hash_curr] = ptr_lista_nodo_ingr_curr;
            /*CREA HEAP DI LOTTI ASSOCIATO E LOTTO*/
            ptr_testa_lista_ingr_magazzino = magazzino[hash_curr];
            ptr_testa_lista_ingr_magazzino->array_lotti = (ptr_lotti *)malloc(sizeof(ptr_lotti)*DIM_INIT_ARRAY_LOTTI);
            /*CREA LOTTO E INSERISCI IN PRIMA POSIZIONE*/
            ptr_lotto_curr = (lotto_t *)malloc(sizeof(lotto_t));
            if (ptr_lotto_curr == NULL){/*CHECK MALLOC*/
                printf("allocazione andata male\n");
            }
            ptr_lotto_curr->peso = peso_lotto_curr;
            ptr_lotto_curr->scadenza = scadenza_lotto_curr;
            (ptr_testa_lista_ingr_magazzino->array_lotti)[0] = ptr_lotto_curr;
            ptr_testa_lista_ingr_magazzino->heap_size = 1;
        }
    }
}

/*
1. SCORRI LISTA ATTESA, PER OGNUNO:
    1. CONTROLLA SE HA L'INGREDIENTE DEL RIFORNIMENTO: SE SI VEDI SE SI PUO' PREPARARE E IN CASO PREPARA (VAI A 2.), SE NO TERMINA
    2. COPIA LA PARTE DI "ORDINE" IN CUI CONTROLLI SE VAI PUOI PREPARARE: 
       1. CONTROLLA SE L'ORDINE E' PREPARABILE (TOGLI ORDINI SCADUTI E GUARDA SE GLI ALTRI SONO ABBASTANZA PER PREPARARE)
       2. SE E' PREPARABILE, ELIMINA I LOTTI E VAI A 1.3., ALTRIMENTI NON FARE NIENTE
    3. INSERISCI ORDINATO (A DIFFERENZA DI "ORDINE") IN ORDINI PRONTI E ELIMINA L'ORDINE DALLA LISTA IN ATTESA*/

void scorri_ordini_attesa_prepara(nodo_lista_magazzino_t * magazzino[], lista_ordini_attesa_t * ptr_lista_ordini_attesa, lista_ordini_pronti_t * ptr_lista_ordiniPronti, unsigned int tempo_curr){
    nodo_lista_ordini_attesa_t * ptr_ordine_attesa_curr = ptr_lista_ordini_attesa->testa_lista_ordini_attesa;
    nodo_lista_ordini_attesa_t * ptr_ordine_attesa_prec = NULL;
    nodo_lista_ingredienti_t * ptr_ingr_curr_ordineCurr; /*DELLA LISTA INGREDIENTI ASSOCIATA ALL'ORDINE*/
    nodo_lista_magazzino_t * tmp_ingr;
    unsigned int tmp_peso;
    unsigned int resta_in_attesa = 0;
    unsigned int peso_richiesto_ingr_curr, peso_ordine_tot;
    unsigned int num_elementi;
    nodo_lista_magazzino_t * ptr_ingr_magazzino_curr; /*DEL MAGAZZINO PER I LOTTI*/
    ptr_lotti ptr_lotto_curr;
    ptr_ordine_attesa_curr= ptr_lista_ordini_attesa->testa_lista_ordini_attesa;
    /*SCORRI LISTA ORDINI ATTESA*/
    while (ptr_ordine_attesa_curr != NULL && resta_in_attesa == 0){ /*SE LISTA ATTESA E' VUOTA, CORRETTAMENTE, NON ENTRA*/
        peso_ordine_tot = 0;
        num_elementi = ptr_ordine_attesa_curr->quantita;
        ptr_ingr_curr_ordineCurr = ptr_ordine_attesa_curr->OrdAtt_testa_lista_ingredienti;
        /*SCORRI LISTA INGREDIENTI PER CALCOLARE SE E' PREPARABILE*/
        while (ptr_ingr_curr_ordineCurr != NULL){ 
            /*CALCOLA PESO RICHIESTO*/
            peso_richiesto_ingr_curr = (ptr_ingr_curr_ordineCurr->peso) * num_elementi; 
            ptr_ingr_magazzino_curr = ptr_ingr_curr_ordineCurr->ptr_ingrediente_magazzino;
            if (ptr_ingr_magazzino_curr != NULL && ptr_ingr_magazzino_curr->sum_peso_lotti < peso_richiesto_ingr_curr){
                resta_in_attesa = 1;
                break;
            }
            if (ptr_ingr_magazzino_curr != NULL){ /*SE L'INGREDIENTE C'E', #####POTREBBE ESSERE RIDONDANTE STO CHECK MA AMEN*/
                if (ptr_ingr_magazzino_curr->heap_size > 0){ /*EVITA SEGFAULT IN CASO ARRAY SIA VUOTO*/
                    /*ELIMINA LOTTI SCADUTI: SONO TUTTI QUELLI CON DATA DI SCADENZA MINORE QUINDI USA ESTRAI MINIMO*/
                    ptr_lotto_curr = (ptr_ingr_magazzino_curr->array_lotti)[0];
                    while (ptr_lotto_curr->scadenza < tempo_curr){
                        ptr_ingr_magazzino_curr->sum_peso_lotti = (ptr_ingr_magazzino_curr->sum_peso_lotti) - (ptr_lotto_curr->peso);
                        min_heap_estrai_minimo(ptr_ingr_magazzino_curr->array_lotti, ptr_ingr_magazzino_curr);
                        if (ptr_ingr_magazzino_curr->heap_size > 0){
                            ptr_lotto_curr = (ptr_ingr_magazzino_curr->array_lotti)[0];
                        }
                        else {
                            resta_in_attesa = 1;
                            break;
                        }
                    }

                    /*VEDI SE NON HO SUFFICIENTI INGREDIENTI, IN CASO METTI IN ATTESA, ALTRIMENTI NON FARE NULLA (CONTINUA)*/
                    if (peso_richiesto_ingr_curr > (ptr_ingr_magazzino_curr->sum_peso_lotti)){
                        resta_in_attesa = 1;
                        break;
                    }
                }
                else{
                    resta_in_attesa = 1;
                    break;
                }
            }

            else{ /*SE L'INGREDIENTE NON C'E', METTI IN ATTESA #####POTREBBE ESSERE RIDONDANTE STO CHECK MA AMEN*/
                resta_in_attesa = 1;
                break;
            }
            ptr_ingr_curr_ordineCurr = ptr_ingr_curr_ordineCurr->next;
        }
        if (resta_in_attesa == 1 && ptr_ingr_curr_ordineCurr!=ptr_ordine_attesa_curr->OrdAtt_testa_lista_ingredienti){
            tmp_ingr = ptr_ingr_curr_ordineCurr->ptr_ingrediente_magazzino;
            tmp_peso = ptr_ingr_curr_ordineCurr->peso;
            ptr_ingr_curr_ordineCurr->peso = ptr_ordine_attesa_curr->OrdAtt_testa_lista_ingredienti->peso;
            ptr_ingr_curr_ordineCurr->ptr_ingrediente_magazzino = ptr_ordine_attesa_curr->OrdAtt_testa_lista_ingredienti->ptr_ingrediente_magazzino;
            ptr_ordine_attesa_curr->OrdAtt_testa_lista_ingredienti->peso = tmp_peso;
            ptr_ordine_attesa_curr->OrdAtt_testa_lista_ingredienti->ptr_ingrediente_magazzino = tmp_ingr;
        }
        /*SE E' PREPARABILE, PREPARA*/
        ptr_ingr_curr_ordineCurr = ptr_ordine_attesa_curr->OrdAtt_testa_lista_ingredienti; /*SALVA TESTA LISTA INGREEDIENTI*/
        if (resta_in_attesa == 0){ 
            /*PRIMA PREPARA, CIOE' DEALLOCA LOTTI, POI CREA ORDINE PRONTO, DEALLOCA ORDINE IN ATTESA*/
            while (ptr_ingr_curr_ordineCurr != NULL){
                ptr_ingr_magazzino_curr = ptr_ingr_curr_ordineCurr->ptr_ingrediente_magazzino; /*RICAVA PTR A INGR IN MAGAZZINO*/
                /*SCORRI I LOTTI E PREPARALI*/
                peso_richiesto_ingr_curr = (ptr_ingr_curr_ordineCurr->peso) * num_elementi; /*CALCOLA PESO RICHIESTO DI INGREDIENTE CURR*/
                peso_ordine_tot = peso_ordine_tot + peso_richiesto_ingr_curr;
                ptr_ingr_magazzino_curr->sum_peso_lotti = ptr_ingr_magazzino_curr->sum_peso_lotti - peso_richiesto_ingr_curr; /*AGGIORNA (DECREMENTA) SUM PESO LOTTI*/
                while(peso_richiesto_ingr_curr > 0){
                    ptr_lotto_curr = (ptr_ingr_magazzino_curr->array_lotti)[0]; /*NON SERVE ITERARE SE ELIMINA MINIMO (PRIMO ELEMENTO) OGNI VOLTA*/
                    if (peso_richiesto_ingr_curr >= (ptr_lotto_curr->peso)){ /*SE E' COSI DEVO ELIMINARE LOTTO*/
                        peso_richiesto_ingr_curr = peso_richiesto_ingr_curr - ptr_lotto_curr->peso;
                        ptr_lotto_curr->peso = 0;
                        min_heap_estrai_minimo(ptr_ingr_magazzino_curr->array_lotti, ptr_ingr_magazzino_curr); /*NON SERVONO CONTROLLI SU SE IL LOTTO NON E' VUOTO PERCHE IL CONTROLLO AVVIENE COL PESO*/
                    }
                    else{ /* SE NON E' COSI *NON* DEVO ELIMINARE LOTTO, SOLO CONSUMARLO PARZIALMENTE */
                        ptr_lotto_curr->peso =  ptr_lotto_curr->peso - peso_richiesto_ingr_curr;
                        peso_richiesto_ingr_curr = 0;
                    }
                }

                ptr_ingr_curr_ordineCurr = ptr_ingr_curr_ordineCurr->next;
            }
            /* INSERISCI ORDINATO IN ORDINI PRONTI */
            nodo_lista_ordini_pronti_t * nuovo_nodo_ordine_pronto; 
            nodo_lista_ordini_pronti_t * nodo_ordinep_tmp;
            nuovo_nodo_ordine_pronto = (nodo_lista_ordini_pronti_t *)malloc(sizeof(nodo_lista_ordini_pronti_t));
            if (nuovo_nodo_ordine_pronto == NULL){ /* SETTO STRUTTURA */
                printf("allocazione andata male\n");
            }
            nuovo_nodo_ordine_pronto->data_arrivo = ptr_ordine_attesa_curr->data_arrivo;
            strcpy((nuovo_nodo_ordine_pronto->nome_ricetta), (ptr_ordine_attesa_curr->nome_ricetta));
            nuovo_nodo_ordine_pronto->num_elementi = num_elementi;
            nuovo_nodo_ordine_pronto->peso = peso_ordine_tot;
            /*INSERIMENTO ORDINATO*/
            nodo_ordinep_tmp = ptr_lista_ordiniPronti->testa_lista_ordini_pronti;
            if (nodo_ordinep_tmp == NULL){ /*LISTA ORDINI PRONTI VUOTA: INSERISCI IN TESTA*/
                ptr_lista_ordiniPronti->testa_lista_ordini_pronti = nuovo_nodo_ordine_pronto;
                nuovo_nodo_ordine_pronto->next = NULL;
            }
            else{ /*LISTA ORDINI PRONTI NON VUOTA, INSERIMENTO ORDINATO*/
                if (nuovo_nodo_ordine_pronto->data_arrivo <= nodo_ordinep_tmp->data_arrivo){ /*INSERISCI IN TESTA*/
                    ptr_lista_ordiniPronti->testa_lista_ordini_pronti = nuovo_nodo_ordine_pronto;
                    nuovo_nodo_ordine_pronto->next = nodo_ordinep_tmp;
                }
                else{ /*NON IN TESTA, VALIDO SIA PER IN MEZZO SIA PER CODA*/
                    while (nodo_ordinep_tmp->next != NULL && nuovo_nodo_ordine_pronto->data_arrivo > nodo_ordinep_tmp->next->data_arrivo){
                        nodo_ordinep_tmp = nodo_ordinep_tmp->next;
                    }/*FINITO STO CICLO DOVREI AVERE IL NODO SUCCESSIVO ALLA POSIZIONE GIUSTA IN nodo_ordinep_tmp->next, QUINDI
                    IL MIO NUOVO ORDINE PRONTO VA INSERITO TRA TMP E TMP->NEXT*/
                    if (nodo_ordinep_tmp->next == NULL){ /*NODO IN CODA*/
                        ptr_lista_ordiniPronti->coda_lista_ordini_pronti = nuovo_nodo_ordine_pronto;
                        nodo_ordinep_tmp->next = nuovo_nodo_ordine_pronto;
                        nuovo_nodo_ordine_pronto->next = NULL;
                    }
                    else{/*NODO NON IN CODA*/
                        nuovo_nodo_ordine_pronto->next = nodo_ordinep_tmp->next;
                        nodo_ordinep_tmp->next = nuovo_nodo_ordine_pronto;
                    }
                }
            }
            /*ELIMINA DA LISTA ATTESA FAI LA FREE E NON INCREMENTARE NIENTE*/
            if (ptr_ordine_attesa_curr == ptr_lista_ordini_attesa->coda_lista_ordini_attesa && ptr_ordine_attesa_curr == ptr_lista_ordini_attesa->testa_lista_ordini_attesa && ptr_lista_ordini_attesa->testa_lista_ordini_attesa != NULL){ //ELIMINO 1 ELEMENTO
                nodo_lista_ordini_attesa_t * tmp_per_eliminare_attesa;
                tmp_per_eliminare_attesa = ptr_ordine_attesa_curr;
                ptr_lista_ordini_attesa->testa_lista_ordini_attesa = NULL;
                ptr_lista_ordini_attesa->coda_lista_ordini_attesa = NULL;
                ptr_ordine_attesa_curr = NULL;
                free(tmp_per_eliminare_attesa);
            }
            else if (ptr_ordine_attesa_curr == ptr_lista_ordini_attesa->testa_lista_ordini_attesa){ /*ELIMINO IN TESTA*/
                nodo_lista_ordini_attesa_t * tmp_per_eliminare_attesa;  
                tmp_per_eliminare_attesa = ptr_ordine_attesa_curr;
                ptr_lista_ordini_attesa->testa_lista_ordini_attesa = ptr_lista_ordini_attesa->testa_lista_ordini_attesa->next;
                ptr_ordine_attesa_curr = ptr_lista_ordini_attesa->testa_lista_ordini_attesa;
                free(tmp_per_eliminare_attesa);
            }
            else if (ptr_ordine_attesa_curr == ptr_lista_ordini_attesa->coda_lista_ordini_attesa){ //ELIMINO IN CODA
                nodo_lista_ordini_attesa_t * tmp_per_eliminare_attesa;
                tmp_per_eliminare_attesa = ptr_ordine_attesa_curr;
                ptr_lista_ordini_attesa->coda_lista_ordini_attesa = ptr_ordine_attesa_prec;
                ptr_lista_ordini_attesa->coda_lista_ordini_attesa->next = NULL;
                ptr_ordine_attesa_curr = ptr_ordine_attesa_curr->next;
                free(tmp_per_eliminare_attesa);
            }
            else{/*ELIMINO IN MEZZO*/
                nodo_lista_ordini_attesa_t * tmp_per_eliminare_attesa;
                ptr_ordine_attesa_prec->next = ptr_ordine_attesa_curr->next;
                tmp_per_eliminare_attesa = ptr_ordine_attesa_curr;
                ptr_ordine_attesa_curr = ptr_ordine_attesa_curr->next;
                free(tmp_per_eliminare_attesa);
            }   
        }
        else{/*ALTRIMENTI, SE RESTA_IN_ATTESA == 1  NON DEVI FARE NIENTE: DEVI INCREMENTARE*/
            ptr_ordine_attesa_prec = ptr_ordine_attesa_curr;
            ptr_ordine_attesa_curr = ptr_ordine_attesa_curr->next;
            resta_in_attesa = 0;
        }
    }
}

/*****************************************************************************/
/*                                AGGIUNGI RICETTA                           */
/*****************************************************************************/
/*
1. SE IL BUCKET E' PROPRIO VUOTO, ALLOCA LA TESTA DELLA LISTA CON LA TUA RICETTA E ALLOCA LA LISTA DEGLI INGREDIENTI ASSOCIATA E INIZIALIZZA "PRESENTE"
   INOLTRE SALVA, IN OGNI NODO DELLA LISTA INGREDIENTI ASSOCIATA ALLA RICETTA, IL NODO DEL MAGAZZINO IN CUI E' L'INGR
2. SE IL BUCKET NON E' VUOTO, O TROVI LA RICETTA E TERMINA (NON FAI NIENTE) OPPURE NON LA TROVI (VAI A 3.)
3. INSERISCI UN NUOVO NODO CON LA TUA RICETTA IN TESTA, ALLOCA LISTA INGREDIENTI E INIZIALIZZA "PRESENTE".
    INOLTRE SALVA, IN OGNI NODO DELLA LISTA INGREDIENTI ASSOCIATA ALLA RICETTA, IL NODO DEL MAGAZZINO IN CUI E' L'INGR
NOTA: OGNI INGREDIENTE HA NOME E PESO E PUNT A NODO MAGAZZINO
*/

/*PUOI OTTIMIZZARE PARTE INIZIALE: testa_liste_ricette e' inutile)*/
void aggiungi_ricetta(nodo_lista_ricettario_t * ricettario[], nodo_lista_magazzino_t *magazzino[]){ /*POSSO FARE TUTTO IN FUNZIONE TANTO I DATI CHE DEVONO RESTARE SONO ALLOCATI CON LA MALLOC*/
    char nome_ricetta[MAX_LEN_STR];
    char trash[MAX_LEN_STR];
    char c ='a';
    if(scanf("%s", nome_ricetta) == 1);  /*LEGGI NOME RICETTA*/

    unsigned long hash_curr = hash_ricetta(nome_ricetta);
    nodo_lista_ricettario_t * testa_lista_ricette = ricettario[hash_curr];

    if (testa_lista_ricette ==  NULL){ /*RICETTA NON PRESENTE NELLA LISTA DEGLI INGREDIENTI*/
        /* DEVO ALLOCARE UNA NUOVA LISTA E INSERIRE, nel bucket, IL PUNT ALLA TESTA*/
        ricettario[hash_curr] = (nodo_lista_ricettario_t *)malloc(sizeof(nodo_lista_ricettario_t)); 
        if (ricettario[hash_curr] == NULL){
            printf("allocazione andata male\n");
        }
        testa_lista_ricette = ricettario[hash_curr];
        testa_lista_ricette->next = NULL;
        strcpy(testa_lista_ricette->nome, nome_ricetta);
        /* ORA LEGGI IL PRIMO INGREDIENTE E ALLOCA LA LISTA DI INGREDIENTI APPENDENDOLA AL NODO DELLA RICETTA*/
        char nome_ingrediente_curr[MAX_LEN_STR];
        int peso_ingrediente_curr;
        nodo_lista_ingredienti_t * nodo_lista_ingredienti_curr;

        testa_lista_ricette->testa_lista_ingredienti = (nodo_lista_ingredienti_t *)malloc(sizeof(nodo_lista_ingredienti_t));
        if (testa_lista_ricette->testa_lista_ingredienti == NULL){/*CHECK MALLOC*/
            printf("allocazione andata male\n");
        }
        nodo_lista_ingredienti_curr = testa_lista_ricette->testa_lista_ingredienti;

        /*INIZIALIZZA I CAMPI DELL'INGREDIENTE*/
        if (scanf("%s", nome_ingrediente_curr) == 1);
        nodo_lista_ingredienti_curr->next = NULL;
        if (scanf("%d", &peso_ingrediente_curr) == 1);
        nodo_lista_ingredienti_curr->peso = peso_ingrediente_curr;
        nodo_lista_ingredienti_curr->ptr_ingrediente_magazzino = cerca_ptr_ingrediente_magazzino(magazzino, nome_ingrediente_curr);
        c = (char)getchar_unlocked();
        while (c != '\n'){ /*FINCHE NON FINISCO DI LEGGERE INGREDIENTI DA INPUT, ALLOCA QUELLI RICEVUTI IN CODA*/
            nodo_lista_ingredienti_curr->next = (nodo_lista_ingredienti_t *)malloc(sizeof(nodo_lista_ingredienti_t));/*NODO DOPO*/
            if (nodo_lista_ingredienti_curr->next == NULL){/*CHECK MALLOC*/
                printf("allocazione andata male\n");
            }
            nodo_lista_ingredienti_curr = nodo_lista_ingredienti_curr->next;
            if (scanf("%s", nome_ingrediente_curr) == 1);

            if (scanf("%d", &peso_ingrediente_curr) == 1);
            nodo_lista_ingredienti_curr->peso = peso_ingrediente_curr;

            nodo_lista_ingredienti_curr->ptr_ingrediente_magazzino = cerca_ptr_ingrediente_magazzino(magazzino, nome_ingrediente_curr);

            c = (char)getchar_unlocked();
        }
        /* IL NODO IN CURR QUANDO ESCO DAL CICLO E' L'ULTIMO PRESO */
        nodo_lista_ingredienti_curr->next = NULL;
        /*INIZIALIZZO PRESENTE IN ORDINI*/
        testa_lista_ricette->presente_in_ordini = 0;
        printf("aggiunta\n");
    }
    else { /*SE IL BUCKET DELLE RICETTE NON E' VUOTO */
        /*DEVO SCORRERE LA LISTA DELLE RICETTE*/
        nodo_lista_ricettario_t * nodo_ricetta_lista_curr = ricettario[hash_curr]; /*SETTA ALLA TESTA DELLA LISTA E CERCA INGR NOME*/
        while (nodo_ricetta_lista_curr != NULL && strcmp(nodo_ricetta_lista_curr->nome, nome_ricetta) != 0){
            nodo_ricetta_lista_curr = nodo_ricetta_lista_curr->next;
        }
        /*IL NODO IN CURR QUANDO ESCO DAL CICLO E' NULL SE SONO ALLA FINE O QUELLO CON IL NOME CERCATO*/
        if (nodo_ricetta_lista_curr == NULL){ /*RICETTA NON PRESENTE*/
            /*ALLOCA NUOVO NODO IN TESTA ALLA LISTA DELLE RICETTE*/
            nodo_ricetta_lista_curr = (nodo_lista_ricettario_t *)malloc(sizeof(nodo_lista_ricettario_t));
            if (nodo_ricetta_lista_curr == NULL){
                printf("allocazione andata male\n");
            }
            nodo_ricetta_lista_curr->next = ricettario[hash_curr]; /*METTI IN TESTA*/
            ricettario[hash_curr] = nodo_ricetta_lista_curr;
            strcpy(nodo_ricetta_lista_curr->nome, nome_ricetta);
            /*ORA LEGGI GLI INGREDIENTI E ALLOCA LA LISTA DI INGREDIENTI*/
            /*PRIMO INGREDIENTE*/
            char nome_ingrediente_curr[MAX_LEN_STR];
            int peso_ingrediente_curr;
            nodo_lista_ingredienti_t * nodo_lista_ingredienti_curr;

            nodo_ricetta_lista_curr->testa_lista_ingredienti = (nodo_lista_ingredienti_t *)malloc(sizeof(nodo_lista_ingredienti_t));
            if (nodo_ricetta_lista_curr->testa_lista_ingredienti == NULL){
                printf("allocazione andata male\n");
            }
            nodo_lista_ingredienti_curr = nodo_ricetta_lista_curr->testa_lista_ingredienti;

            if (scanf("%s", nome_ingrediente_curr) == 1);
            nodo_lista_ingredienti_curr->next = NULL;
            if (scanf("%d", &peso_ingrediente_curr) == 1);
            nodo_lista_ingredienti_curr->peso = peso_ingrediente_curr;
            nodo_lista_ingredienti_curr->ptr_ingrediente_magazzino = cerca_ptr_ingrediente_magazzino(magazzino, nome_ingrediente_curr);
            c = (char)getchar_unlocked();
            while (c != '\n'){ /*FINCHE NON FINISCO DI LEGGERE INGREDIENTI DA INPUT, ALLOCA QUELLI RICEVUTI IN CODA*/
                nodo_lista_ingredienti_curr->next = (nodo_lista_ingredienti_t *)malloc(sizeof(nodo_lista_ingredienti_t));
                if (nodo_lista_ingredienti_curr->next == NULL){
                    printf("allocazione andata male\n");
                }
                nodo_lista_ingredienti_curr = nodo_lista_ingredienti_curr->next;
                if (scanf("%s", nome_ingrediente_curr) == 1);
                if (scanf("%d", &peso_ingrediente_curr) == 1);
                nodo_lista_ingredienti_curr->peso = peso_ingrediente_curr;
                nodo_lista_ingredienti_curr->ptr_ingrediente_magazzino = cerca_ptr_ingrediente_magazzino(magazzino, nome_ingrediente_curr);

                c = (char)getchar_unlocked();
            }
            nodo_lista_ingredienti_curr->next = NULL;
            /*INIZIALIZZA PRESENTE IN ORDINI*/
            nodo_ricetta_lista_curr->presente_in_ordini = 0;
            printf("aggiunta\n");
        }
        else { /*RICETTA GIA PRESENTE*/
            printf("ignorato\n");
            while (c != '\n'){
                if (scanf("%s", trash)==1);
                c = (char)getchar_unlocked();
            }
        }
    }
}

/*CERCA_PTR_INGREDIENTE_MAGAZZINO
1. CERCA NODO INGREDIENTE
2. SE NON E' PRESENTE, INIZIALIZZA UN NUOVO NODO DELLA LISTA DI INGREDIENTI ASSOCIATA AL BUCKET DEL MAGAZZINO, CON UN LOTTO FANTOCCIO
   CHE HA PESO 0 E HA SCADENZA 0 COSI VIENE ELIMINATO SUBITO LA PRIMA VOLTA.
3. IN OGNI CASO RESTITUISCI UN PTR
*/
nodo_lista_magazzino_t * cerca_ptr_ingrediente_magazzino(nodo_lista_magazzino_t * magazzino[], char nome_ingrediente[]){
    nodo_lista_magazzino_t * ptr_nodo_ingr_magazzino_curr;
    unsigned long hash_ingr = hash_lotto(nome_ingrediente);

    ptr_nodo_ingr_magazzino_curr = magazzino[hash_ingr];

    if (ptr_nodo_ingr_magazzino_curr == NULL){ /*SE NON HO NIENTE NEL BUCKET*/
        magazzino[hash_ingr] = (nodo_lista_magazzino_t *)malloc(sizeof(nodo_lista_magazzino_t));
        if (magazzino[hash_ingr] == NULL){/*CHECK MALLOC*/
            printf("allocazione andata male\n");
        }
        ptr_nodo_ingr_magazzino_curr = magazzino[hash_ingr];
        /* INIZIALIZZO IL NODO INGREDIENTE E ALLOCO UN LOTTO "FANTOCCIO" CHE VERRA ELIMINATO SUBITO GRAZIE ALLA SCADENZA 0, DI PESO 0,
        COSI NON HO PROBLEMI */
        ptr_nodo_ingr_magazzino_curr->next = NULL; /*E' LA TESTA DI UNA LISTA DI DIM 1*/
        strcpy(ptr_nodo_ingr_magazzino_curr->nome, nome_ingrediente);
        ptr_nodo_ingr_magazzino_curr->sum_peso_lotti = 0;
        ptr_nodo_ingr_magazzino_curr->array_lotti = NULL;
        ptr_nodo_ingr_magazzino_curr->heap_size = 0;
    }
    else{
        /*SCORRI LISTA BUCKET MAGAZZINO PER CERCARE L'INGREDIENTE CURR*/
        while (ptr_nodo_ingr_magazzino_curr != NULL && strcmp(ptr_nodo_ingr_magazzino_curr->nome, nome_ingrediente) != 0){/*SCORRI FINCHE NON TROVI*/
            ptr_nodo_ingr_magazzino_curr = ptr_nodo_ingr_magazzino_curr->next;
        }/*A FINE CICLO HO NULL O L'INGREDIENTE VOLUTO*/
        if (ptr_nodo_ingr_magazzino_curr ==  NULL){/*SE NON C'E' INGREDIENTE MA BUCKET NON VUOTO*/
            /*ALLOCA E INIZIALIZZA*/
            ptr_nodo_ingr_magazzino_curr = (nodo_lista_magazzino_t *)malloc(sizeof(nodo_lista_magazzino_t));
            if (ptr_nodo_ingr_magazzino_curr == NULL){/*CHECK MALLOC*/
                printf("allocazione andata male\n");
            }
            strcpy(ptr_nodo_ingr_magazzino_curr->nome, nome_ingrediente);
            ptr_nodo_ingr_magazzino_curr->sum_peso_lotti = 0;
            ptr_nodo_ingr_magazzino_curr->array_lotti = NULL;
            ptr_nodo_ingr_magazzino_curr->heap_size = 0;
            /*METTI IN TESTA*/
            ptr_nodo_ingr_magazzino_curr->next = magazzino[hash_ingr];
            magazzino[hash_ingr] = ptr_nodo_ingr_magazzino_curr;
        }
    }
    return ptr_nodo_ingr_magazzino_curr;
}

/*****************************************************************************/
/*                                RIMUOVI RICETTA                            */
/*****************************************************************************/

/*
1. SE BUCKET VUOTO, NON FARE NIENTE
2. SE BUCKET NON VUOTO, SCORRI LISTA ASSOCIATA: SE NON TROVI NIENTE TERMINA, ALTRIMENTI VAI A 3.
3. SE TROVI RICETTA, SE PRESENTE == 0 ALLORA NON E' PRESENTE IN ORDINI E ALTRO E PUOI RIMUOVERE, ALTRIMENTI "ORDINI IN SOSPESO"
*/

void rimuovi_ricetta(nodo_lista_ricettario_t *ricettario[], char nome_ricetta_da_rimuovere[]){
    nodo_lista_ricettario_t * nodo_ricetta_lista_curr = ricettario[hash_ricetta(nome_ricetta_da_rimuovere)];
    nodo_lista_ricettario_t * nodo_ricetta_lista_prev = NULL; /*SERVE PER LA RIMOZIONE*/
    nodo_lista_ricettario_t * tmp_elimino;

    if (nodo_ricetta_lista_curr == NULL){ /*RICETTA NON PRESENTE: NON HO NIENTE ALLOCATO NEL BUCKET*/
        printf("non presente\n");
    }
    else{ /*RICETTA FORSE PRESENTE, IL BUCKET NON E' VUOTO*/
        while (nodo_ricetta_lista_curr != NULL && strcmp(nodo_ricetta_lista_curr->nome, nome_ricetta_da_rimuovere) != 0){/*SCORRI FINCHE NON TROVI*/
            nodo_ricetta_lista_prev = nodo_ricetta_lista_curr;
            nodo_ricetta_lista_curr = nodo_ricetta_lista_curr->next;
        }
        /*IL NODO IN CURR QUANDO ESCO DAL CICLO E' NULL SE SONO ALLA FINE O QUELLO CON IL NOME CERCATO*/
        if (nodo_ricetta_lista_curr == NULL){ /*SE NON HO TROVATO LA RICETTA*/
            printf("non presente\n");
        }
        else{ /*SE HO TROVATO LA RICETTA*/
            if (nodo_ricetta_lista_curr->presente_in_ordini == 0){/*SE NON HO ORDINI CON LA RICETTA*/
                /*RIMUOVI LA RICETTA*/
                if (nodo_ricetta_lista_prev == NULL && nodo_ricetta_lista_curr->next == NULL){
                    tmp_elimino = ricettario[hash_ricetta(nome_ricetta_da_rimuovere)];
                    ricettario[hash_ricetta(nome_ricetta_da_rimuovere)] = NULL;
                    cancella_lista_ingredienti(tmp_elimino->testa_lista_ingredienti);
                    free(tmp_elimino);
                    tmp_elimino = NULL;
                }
                else if (nodo_ricetta_lista_prev == NULL){ /*NON SONO MAI ENTRATO NEL CICLO SOPRA ==> LA RICETTA DA RIMUOVERE E' IN TESTA*/
                    tmp_elimino = ricettario[hash_ricetta(nome_ricetta_da_rimuovere)];
                    ricettario[hash_ricetta(nome_ricetta_da_rimuovere)] = nodo_ricetta_lista_curr->next;
                    cancella_lista_ingredienti(tmp_elimino->testa_lista_ingredienti);
                    free(tmp_elimino);
                    tmp_elimino = NULL;
                }
                else{ /*FUNZIONA ANCHE SE IL NODO CURR E' L'ULTIMO: IN CASO LO ELIMINA E PONE IL NEXT DI QUELLO PRIMA A NULL*/
                    nodo_ricetta_lista_prev->next = nodo_ricetta_lista_curr->next;
                    tmp_elimino = nodo_ricetta_lista_curr;
                    cancella_lista_ingredienti(tmp_elimino->testa_lista_ingredienti);
                    free(tmp_elimino);
                    tmp_elimino = NULL;
                }
                printf("rimossa\n");
            }
            else{/*SE HO ORDINI CON LA RICETTA*/
                printf("ordini in sospeso\n");
            }
        }
    }
}

/*****************************************************************************/
/*                                ORDINE                                     */
/*****************************************************************************/
/*
1. LEGGI DA INPUT RICETTA, NUMERO ELEMENTI (QUANTITA')
2. CERCA RICETTA DELL'ORDINE -> SE NON C'E' TERMINA, SE C'E' PROSEGUI
3. SCORRI LISTA INGREDIENTI ASSOCIATO ALLA RICETTA, PER OGNUNO
    1. ACCEDI A MAGAZZINO E SCORRI LISTA CONCATENATA FINCHE' NON LO TROVI
    2. SE LO TROVI BENE, VAI A 3.3, ALTRIMENTI METTI IN ATTESA E VAI A 4.
    3. SCORRI LISTA LOTTI FINCHE NON ELIMINI TUTTI I LOTTI SCADUTI (TUTTI IN CIMA POICHE SCADENZA MINORE GRAZIE A ORDINE) E AGGIORNA 
        CONTATORE PESO MAGAZZINO INGREDIENTE
    4. SCORRI LISTA LOTTI FINCHE NON VEDI CHE NON HAI ABBASTANZA INGREDIENTI (E IN CASO METTI IN ATTESA) O IL PESO RICHIESTO E' 0
4. SE METTI IN ATTESA E' VERO: METTI IN CODA ALLA LISTA ATTESA
5. ALTRIMENTI INSERISCI ORDINATAMENTE IN LISTA ORDINE: SAI CHE E' L'ULTIMO ARRIVATO QUINDI IN CODA
*/

void ordine(nodo_lista_ricettario_t *ricettario[], nodo_lista_magazzino_t * magazzino[], lista_ordini_pronti_t * lista_ordini_pronti, lista_ordini_attesa_t * lista_ordini_attesa, unsigned int tempo_curr){
    unsigned int metti_in_attesa = 0;
    char nome_ricetta_ordine[MAX_LEN_STR];
    unsigned int num_elementi;
    nodo_lista_ricettario_t * ptr_ricetta;
    nodo_lista_ingredienti_t * ptr_ingrediente_curr;
    //nodo_lista_ingredienti_t * ptr_ingrediente_prec = NULL;
    unsigned int peso_richiesto_ingr_curr;
    unsigned int peso_ordine_tot = 0; /*RILEVANTE E CORRETTO SOLO PER ORDINI PRONTI (PREPARATI TOTALMENTE, SENNO MI FERMO PRIMA DELL'ULTIMO INGREDIENTE)*/
    nodo_lista_magazzino_t * ptr_nodo_lista_magazzino_ingr;
    ptr_lotti ptr_lotto_curr;

    /*LEGGI DA INPUT*/
    if (scanf("%s", nome_ricetta_ordine) == 1);
    if (scanf("%u", &num_elementi) == 1);
    /*CERCA RICETTA NEL RICETTARIO*/
    ptr_ricetta = ricettario[hash_ricetta(nome_ricetta_ordine)];
    while (ptr_ricetta != NULL && strcmp(ptr_ricetta->nome, nome_ricetta_ordine) != 0){/*SCORRI FINCHE NON TROVI*/
        ptr_ricetta = ptr_ricetta->next;
    }
    /*USCITO DAL CICLO PTR_RICETTA E' NULL O LA RICETTA VOLUTA*/
    if (ptr_ricetta != NULL){ /*SE LA RICETTA E' NEL RICETTARIO*/
        printf("accettato\n");
        ptr_ingrediente_curr = ptr_ricetta->testa_lista_ingredienti; /*SALVA TESTA LISTA INGREEDIENTI*/
        ptr_ricetta->presente_in_ordini = ptr_ricetta->presente_in_ordini + 1; /*POSSO GIA INCREMENTARE, NON IMPORTA DOVE FINISCE ORDINE*/

        /*SCORRI LISTA INGREDIENTI PER CALCOLARE SE E' PREPARABILE*/
        while (ptr_ingrediente_curr != NULL && metti_in_attesa == 0){ 
            peso_richiesto_ingr_curr = (ptr_ingrediente_curr->peso) * num_elementi; /*CALCOLA PESO RICHIESTO*/
            ptr_nodo_lista_magazzino_ingr = ptr_ingrediente_curr->ptr_ingrediente_magazzino;
            if (ptr_nodo_lista_magazzino_ingr != NULL){ /*SE L'INGREDIENTE C'E', #####POTREBBE ESSERE RIDONDANTE STO CHECK MA AMEN*/
                /*ELIMINA LOTTI SCADUTI: SONO TUTTI QUELLI CON DATA DI SCADENZA MINORE QUINDI USA ESTRAI MINIMO*/
                if (ptr_nodo_lista_magazzino_ingr->heap_size > 0){ /*EVITO SEG FAULT IN CASO SIA VUOTO L'ARRAY*/
                    ptr_lotto_curr = (ptr_nodo_lista_magazzino_ingr->array_lotti)[0];
                    while (ptr_lotto_curr->scadenza < tempo_curr){
                        ptr_nodo_lista_magazzino_ingr->sum_peso_lotti = ptr_nodo_lista_magazzino_ingr->sum_peso_lotti - ptr_lotto_curr->peso;
                        min_heap_estrai_minimo(ptr_nodo_lista_magazzino_ingr->array_lotti, ptr_nodo_lista_magazzino_ingr);
                        if (ptr_nodo_lista_magazzino_ingr->heap_size > 0){
                            ptr_lotto_curr = (ptr_nodo_lista_magazzino_ingr->array_lotti)[0];
                        }
                        else{
                            metti_in_attesa = 1;
                            break;
                        }
                    }
                    /*VEDI SE NON HO SUFFICIENTI INGREDIENTI, IN CASO METTI IN ATTESA, ALTRIMENTI NON FARE NULLA (CONTINUA)*/
                    if (peso_richiesto_ingr_curr > (ptr_nodo_lista_magazzino_ingr->sum_peso_lotti)){
                        metti_in_attesa = 1;
                        break;
                    }
                }
                else{
                    metti_in_attesa = 1;
                    break;
                }
            }
            else{ /*SE L'INGREDIENTE NON C'E', METTI IN ATTESA #####POTREBBE ESSERE RIDONDANTE STO CHECK MA AMEN*/
                metti_in_attesa = 1;
                break;
            }
            //ptr_ingrediente_prec = ptr_ingrediente_curr;
            ptr_ingrediente_curr = ptr_ingrediente_curr->next;
        }
        /*if (metti_in_attesa == 1 && ptr_ingrediente_prec != NULL){
            ptr_ingrediente_prec->next = ptr_ingrediente_curr->next;
            ptr_ingrediente_curr->next = ptr_ricetta->testa_lista_ingredienti;
            ptr_ricetta->testa_lista_ingredienti = ptr_ingrediente_curr;
        }*/
        /*PREPARA INGREDIENTI PREPARABILI: NON DEVO EVITARE SEG FAULT SU ARRAY PERCHE SE SON QUI IL PRIMO ELEMENTO CE L'HO SEMPRE*/
        ptr_ingrediente_curr = ptr_ricetta->testa_lista_ingredienti; /*SALVA TESTA LISTA INGREEDIENTI*/
        if (metti_in_attesa == 0){ 
            while (ptr_ingrediente_curr != NULL){
                ptr_nodo_lista_magazzino_ingr = ptr_ingrediente_curr->ptr_ingrediente_magazzino; /*RICAVA PTR A INGR IN MAGAZZINO*/
                /*SCORRI I LOTTI E PREPARALI*/
                peso_richiesto_ingr_curr = (ptr_ingrediente_curr->peso) * num_elementi; /*CALCOLA PESO RICHIESTO DI INGREDIENTE CURR*/
                ptr_nodo_lista_magazzino_ingr->sum_peso_lotti = ptr_nodo_lista_magazzino_ingr->sum_peso_lotti - peso_richiesto_ingr_curr; /*POSSO FARLO SENZA TROPPI PROBLEMI PERCHE TANTO HO GIA CONTROLLATO SE E' PREPARABILE*/
                peso_ordine_tot = peso_ordine_tot + peso_richiesto_ingr_curr;
                while (peso_richiesto_ingr_curr > 0){
                    ptr_lotto_curr = (ptr_nodo_lista_magazzino_ingr->array_lotti)[0]; /*NON SERVE ITERARE SE ELIMINA MINIMO (PRIMO ELEMENTO) OGNI VOLTA*/
                    if (peso_richiesto_ingr_curr >= (ptr_lotto_curr->peso)){ /*SE E' COSI DEVO ELIMINARE LOTTO*/
                        peso_richiesto_ingr_curr = peso_richiesto_ingr_curr - ptr_lotto_curr->peso;
                        ptr_lotto_curr->peso = 0;
                        min_heap_estrai_minimo(ptr_nodo_lista_magazzino_ingr->array_lotti, ptr_nodo_lista_magazzino_ingr); /*NON SERVONO CONTROLLI SU SE IL LOTTO NON E' VUOTO PERCHE IL CONTROLLO AVVIENE COL PESO*/
                    }
                    else{ /* SE NON E' COSI *NON* DEVO ELIMINARE LOTTO, SOLO CONSUMARLO PARZIALMENTE */
                        ptr_lotto_curr->peso =  ptr_lotto_curr->peso - peso_richiesto_ingr_curr;
                        peso_richiesto_ingr_curr = 0;
                    }
                }
                ptr_ingrediente_curr = ptr_ingrediente_curr->next;
            }
            nodo_lista_ordini_pronti_t * nuovo_nodo_ordine_pronto;
            /*INSERISCI IN CODA IN ORDINI PRONTI*/
            nuovo_nodo_ordine_pronto = (nodo_lista_ordini_pronti_t *)malloc(sizeof(nodo_lista_ordini_pronti_t));
            if (nuovo_nodo_ordine_pronto == NULL){ /*CHECK MALLOC*/
                printf("allocazione andata male\n");
            }
            /* SETTO STRUTTURA */
            nuovo_nodo_ordine_pronto->data_arrivo = tempo_curr;
            strcpy(nuovo_nodo_ordine_pronto->nome_ricetta, nome_ricetta_ordine);
            nuovo_nodo_ordine_pronto->peso = peso_ordine_tot;
            nuovo_nodo_ordine_pronto->num_elementi = num_elementi;
            /*INSERIMENTO IN CODA: INFATTI SAI E' PER FORZA L'ULTIMO PREPARATO. SE LA LISTA E' VUOTA DEVI INSERIRE IN TESTA PERO'*/
            nuovo_nodo_ordine_pronto->next = NULL;
            if (lista_ordini_pronti->testa_lista_ordini_pronti == NULL){/*SE TESTA VUOTA*/
                lista_ordini_pronti->testa_lista_ordini_pronti = nuovo_nodo_ordine_pronto;
                lista_ordini_pronti->coda_lista_ordini_pronti = lista_ordini_pronti->testa_lista_ordini_pronti;
            }
            else{/*ALTRIMENTI: TESTA NON VUOTA (LISTA NON VUOTA)*/
                lista_ordini_pronti->coda_lista_ordini_pronti->next = nuovo_nodo_ordine_pronto;
                lista_ordini_pronti->coda_lista_ordini_pronti = nuovo_nodo_ordine_pronto;
            }
        }
        else{/*SE E' MESSO IN ATTESA*/
            /*INSERISCI IN ATTESA (IN CODA)*/
            nodo_lista_ordini_attesa_t * nuovo_nodo_ordine_attesa = (nodo_lista_ordini_attesa_t *)malloc(sizeof(nodo_lista_ordini_attesa_t));
            if (nuovo_nodo_ordine_attesa == NULL){/*CHECK MALLOC*/
                printf("allocazione andata male\n");
            }
            /*SETTO STRUTTURA*/
            nuovo_nodo_ordine_attesa->data_arrivo = tempo_curr;
            strcpy(nuovo_nodo_ordine_attesa->nome_ricetta, nome_ricetta_ordine);
            nuovo_nodo_ordine_attesa->quantita = num_elementi;
            nuovo_nodo_ordine_attesa->OrdAtt_testa_lista_ingredienti = ptr_ricetta->testa_lista_ingredienti;
            /*INSERIMENTO IN CODA O IN TESTA SE LISTA VUOTA*/
            nuovo_nodo_ordine_attesa->next = NULL;
            if (lista_ordini_attesa->testa_lista_ordini_attesa == NULL){/*CASO LISTA VUOTA*/
                lista_ordini_attesa->testa_lista_ordini_attesa = nuovo_nodo_ordine_attesa;
                lista_ordini_attesa->coda_lista_ordini_attesa = lista_ordini_attesa->testa_lista_ordini_attesa;
            }
            else{ /*ALTRI CASI*/
                lista_ordini_attesa->coda_lista_ordini_attesa->next = nuovo_nodo_ordine_attesa;
                lista_ordini_attesa->coda_lista_ordini_attesa = nuovo_nodo_ordine_attesa;
            }
        }
    }
    else{ /*SE LA RICETTA NON E' NEL RICETTARIO*/
        printf("rifiutato\n");
    }
}

/*****************************************************************************/
/*                                CORRIERE                                   */
/*****************************************************************************/

/*CORRIERE:
1. SCORRI LISTA ORDINI PRONTI (GIA ORDINATI PER DATA DI ARRIVO), NEL FRATTEMPO ACCEDI A RICETTA E DECREMENTA "PRESENTI", E PER OGNUNO 
    RIMUOVI E INSERISCI (IN LISTA SEPARATA "SELEZIONATI")
2. ORDINA LISTA PESO
3. STAMPA PROPRIAMENTE LA LISTA DEGLI ORDINI CARICATI
4. DEALLOCA LISTA SELEZIONATI
*/

void corriere(lista_ordini_pronti_t * lista_ordini_pronti, nodo_lista_ricettario_t * ricettario[], unsigned int capienza_camioncino, unsigned int tempo_curr){
    nodo_lista_ordini_pronti_t * ptr_curr_lista_ordiniPronti = lista_ordini_pronti->testa_lista_ordini_pronti;
    nodo_lista_ordini_pronti_t * ptr_curr_lista_ordiniProntiSel = NULL;
    nodo_lista_ordini_pronti_t * ptr_testa_lista_ordiniProntiSel = NULL;
    nodo_lista_ordini_pronti_t * tmp_selez_da_eliminare;
    nodo_lista_ricettario_t * tmp_nodo_ricettario;
    unsigned int sum_pesi_ordiniSel = 0;
    char nome_ricetta_ordinep_curr[MAX_LEN_STR]; 

    if (ptr_curr_lista_ordiniPronti == NULL){/*SE NON HAI ORDINI PRONTI*/
        printf("camioncino vuoto\n");
    }
    else{/*SE HAI ORDINI PRONTI*/
        if (ptr_curr_lista_ordiniPronti->peso > capienza_camioncino){ /*SE IL PRIMO ORDINE E' TROPPO GRANDE PER IL CAMION*/
            printf("camioncino vuoto\n");
        }
        else{
            while (ptr_curr_lista_ordiniPronti != NULL && (sum_pesi_ordiniSel + ptr_curr_lista_ordiniPronti->peso) <= capienza_camioncino){
                /*"SELEZIONA" (AGGIUNGI A LISTA SELEZIONATI) E TIENI CONTO DEL PESO*/
                sum_pesi_ordiniSel = sum_pesi_ordiniSel + ptr_curr_lista_ordiniPronti->peso;
                if (ptr_curr_lista_ordiniProntiSel == NULL){ /*SE LISTA SELEZIONATI VUOTA*/
                    ptr_curr_lista_ordiniProntiSel = ptr_curr_lista_ordiniPronti;
                    ptr_testa_lista_ordiniProntiSel = ptr_curr_lista_ordiniPronti;
                }
                else{/*SE NON SONO IN TESTA (LISTA GIA PIENA)*/
                    ptr_curr_lista_ordiniProntiSel->next = ptr_curr_lista_ordiniPronti;
                    ptr_curr_lista_ordiniProntiSel = ptr_curr_lista_ordiniProntiSel->next;
                }
                /*CERCA RICETTA E DECREMENTA PRESENTE*/
                strcpy(nome_ricetta_ordinep_curr, ptr_curr_lista_ordiniPronti->nome_ricetta);
                tmp_nodo_ricettario = ricettario[hash_ricetta(nome_ricetta_ordinep_curr)];
                while (strcmp(tmp_nodo_ricettario->nome, nome_ricetta_ordinep_curr) != 0){
                    tmp_nodo_ricettario = tmp_nodo_ricettario->next;
                }/*FINE CICLO HO TMP_NODO_RICETTARIO NELLA VARIABILE TMP, POSSO DECREMENTARE*/
                tmp_nodo_ricettario->presente_in_ordini = tmp_nodo_ricettario->presente_in_ordini - 1;
                ptr_curr_lista_ordiniPronti = ptr_curr_lista_ordiniPronti->next;
            }
            ptr_curr_lista_ordiniProntiSel->next = NULL;/*ULTIMO ELEMENTO VA INIZIALIZZATO A NULL ESSENDO LA CODA*/
            /*TAGLIA VIA DA PRONTI TUTTO CIO' CHE HAI MESSO IN SEL, OSSIA TAGLIA FINO ALL'ELEMENTO IN CUI SEI USCITO DAL CICLO*/
            lista_ordini_pronti->testa_lista_ordini_pronti = ptr_curr_lista_ordiniPronti;
            /*RIORDINA SELEZIONATI*/
            merge_sort_list(&ptr_testa_lista_ordiniProntiSel);
            /*STAMPA SELEZIONATI E DEALLOCA*/
            ptr_curr_lista_ordiniPronti = ptr_testa_lista_ordiniProntiSel;
            while (ptr_curr_lista_ordiniPronti != NULL){ /*FINCHE NON INCONTRA L'ULTIMO ELEMENTO*/
                printf("%d %s %d\n", ptr_curr_lista_ordiniPronti->data_arrivo, ptr_curr_lista_ordiniPronti->nome_ricetta, ptr_curr_lista_ordiniPronti->num_elementi);fflush(stdout);
                tmp_selez_da_eliminare = ptr_curr_lista_ordiniPronti;
                ptr_curr_lista_ordiniPronti = ptr_curr_lista_ordiniPronti->next;
                free(tmp_selez_da_eliminare);
            }
        }
    }
}

void merge_sort_list(nodo_lista_ordini_pronti_t ** double_ptr_testa_lista){
    nodo_lista_ordini_pronti_t * ptr_testa_lista = *double_ptr_testa_lista;
    nodo_lista_ordini_pronti_t * primo, * secondo;
    nodo_lista_ordini_pronti_t * veloce, *lento;

    if ((ptr_testa_lista == NULL) || (ptr_testa_lista->next == NULL)){
        return ;
    }
    lento = ptr_testa_lista;
    veloce = lento->next;
    while (veloce != NULL){
        veloce = veloce->next;
        if (veloce != NULL){
            lento = lento->next;
            veloce = veloce->next;
        }
    }
    primo = ptr_testa_lista;
    secondo = lento->next;
    lento->next = NULL;
    /*MERGE SORT CHIAMATA RICORSIVA SPEZZA*/
    merge_sort_list(&primo);
    merge_sort_list(&secondo);
    /*UNISCI*/
    *double_ptr_testa_lista = unisci(primo, secondo);
}

nodo_lista_ordini_pronti_t * unisci(nodo_lista_ordini_pronti_t * primo, nodo_lista_ordini_pronti_t * secondo){
    nodo_lista_ordini_pronti_t * risultato = NULL;
    if (primo == NULL){
        return secondo;
    }
    else if (secondo == NULL){
        return primo;
    }
    if (primo->peso >= secondo->peso){
        risultato = primo;
        risultato->next = unisci(primo->next, secondo);
    }
    else{
        risultato = secondo;
        risultato->next = unisci(primo, secondo->next);
    }
    return risultato;
}

/*****************************************************************************/
/*                                FUNZIONI HASH                              */
/*****************************************************************************/

unsigned long hash_lotto(char stringa[]){
    unsigned long hash = 5381;
    int caratt;
    int i = 0;
    while ((caratt = stringa[i])){
        hash = ((hash << 5) + hash) + caratt;
        i++;
    }
    return hash % NUM_BUCKET_MAGAZZINO;
}

unsigned long hash_ricetta(char stringa[]){
    unsigned long hash = 5381;
    int caratt;
    int i = 0;

    while ((caratt = stringa[i])){
        hash = ((hash << 5) + hash) + caratt;
        i++;
    }
    return hash % NUM_BUCKET_RICETTARIO;
}

/*****************************************************************************/
/*                            LOTTI MIN HEAP FUNZIONI                        */
/*****************************************************************************/

void min_heapify(ptr_lotti A[], unsigned int i, nodo_lista_magazzino_t * ptr_nodo_ingrediente){ /*A e' un array di pointer a struct lotti*/
    unsigned int piuPiccolo;
    ptr_lotti tmp;

    if (((2*i+1) <= ptr_nodo_ingrediente->heap_size) && (A[2*i+1]->scadenza < A[i]->scadenza)){
        piuPiccolo = 2*i+1;
    }
    else{
        piuPiccolo = i;
    }
    if (((2*i+2) <= ptr_nodo_ingrediente->heap_size) && (A[2*i+2]->scadenza < A[piuPiccolo]->scadenza)){
        piuPiccolo = 2*i+2;
    }
    if (piuPiccolo != i){
        tmp = A[piuPiccolo];
        A[piuPiccolo] = A[i];
        A[i] = tmp;
        min_heapify(A, piuPiccolo, ptr_nodo_ingrediente);
    }
}

ptr_lotti min_heap_minimo(ptr_lotti A[], nodo_lista_magazzino_t * ptr_nodo_ingrediente){
    return A[0];
}

ptr_lotti min_heap_estrai_minimo(ptr_lotti A[], nodo_lista_magazzino_t * ptr_nodo_ingrediente){
    ptr_lotti min;
    min = A[0];
    A[0] = A[(ptr_nodo_ingrediente->heap_size)-1];
    ptr_nodo_ingrediente->heap_size = ptr_nodo_ingrediente->heap_size - 1;
    min_heapify(A, 0, ptr_nodo_ingrediente);
    A = (ptr_lotti *)realloc((void *)A, sizeof(ptr_lotti)*(ptr_nodo_ingrediente->heap_size));
    ptr_nodo_ingrediente->array_lotti = A;
    return min;
}

ptr_lotti * min_heap_inserisci(ptr_lotti * A, nodo_lista_magazzino_t * ptr_nodo_ingrediente, ptr_lotti lotto_da_inserire){
    unsigned int i;
    unsigned int padre;
    ptr_lotti tmp;
    ptr_lotti * tmp_realloc;

    tmp_realloc = A;
    A = (ptr_lotti *)realloc((void *)A, sizeof(ptr_lotti)*(ptr_nodo_ingrediente->heap_size + 1));
    ptr_nodo_ingrediente->heap_size = ptr_nodo_ingrediente->heap_size + 1;
    if (A == NULL){
        printf("ri-allocazione mal terminata\n");
        ptr_nodo_ingrediente->heap_size = ptr_nodo_ingrediente->heap_size - 1;
        return tmp_realloc; /*RESTITUISCI VECCHIO ARRAY*/
    }
    ptr_nodo_ingrediente->array_lotti = A;
    /*ESPANDI HEAP E METTI IN FONDO L'OGGETTO*/
    A[(ptr_nodo_ingrediente->heap_size) - 1] = lotto_da_inserire;
    i = ptr_nodo_ingrediente->heap_size - 1;
    if ((i-1)%2 == 0) padre = (i-1)/2;
    else padre = (i-2)/2;
    /*SPOSTA L'OGGETTO CORRETTAMENTE*/
    while((i>0) && (A[padre]->scadenza > A[i]->scadenza)){
        tmp = A[padre];
        A[padre] = A[i];
        A[i] = tmp;
        /*i = PARENT(i)*/
        if ((i-1)%2 == 0) i = (i-1)/2;
        else i = (i-2)/2;
        /*padre = PARENT(i)*/
        if ((i-1)%2 == 0) padre = (i-1)/2;
        else padre = (i-2)/2;
    }
    return A;
}

void cancella_lista_ingredienti(nodo_lista_ingredienti_t * testa_lista_ingredienti){
    nodo_lista_ingredienti_t * tmp_delete = testa_lista_ingredienti;
    while (testa_lista_ingredienti != NULL){
        tmp_delete = testa_lista_ingredienti;
        testa_lista_ingredienti = testa_lista_ingredienti->next;
        free(tmp_delete);
    }
}
