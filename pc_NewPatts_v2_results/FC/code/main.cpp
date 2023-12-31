#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "functions.cpp"
//#include "generate_pattern_rand.cpp"
#include "generate_pattern_v2.cpp"
#include <mpi.h>
//#include <igraph.h>
// #include "/usr/include/igraph/igraph.h"

/*choose range and step of pattern storage and retrieval*/
int Pmin = 10;
int Pstep = 10;
int p;

/*choose how many runs with different sets of patterns for quenched avg*/
int number_different_set_patterns = 1;

/*NET PARAMETERS*/
int N = 2000;
int Cm;
int S = 3;
double U = 0.5;
double nu = 0.0;
double a = 0.3;

/*pattern parameters*/
int tot_num_trials = 1;
int NumSet = 10;

/*latching parameters*/
double b1 = 0.5;
double b2 = 0.0;
double b3 = 0.0;
double w = 0.0;
double g = 10.0;

/*network update*/
int Tnet = 20; 
int time_toprint = 500;

/*type of connectivity*/
int full_connectivity = 0;
int random_dilution = 1;
int variable_connectivity = 0;
int symmetric_random_dilution = 0;
int do_avg_Cij = 0;

/*cue parameters and type*/
double fraction_units_cued = 1.0;
int exp_cue = 0;
int tau = 2*N;
int full_cue = 1;
int partial_cue = 0;

/*graded or discrete update*/
int update_h = 1;
int update_r = 0;
int discrete = 0;
int graded = 1;
int beta = 200;

/*pattern correlations*/
int p_fact;
int Num_fact = 150;
double a_pf = 0.4;
double fact_eigen_slope = 0.000001;
double eps = 0.000001;

/* GLOBAL VARIABLES */
int		**xi; //pattern p*N
int		*number_connections;
double	**s; // state of network N*(S+1)
double	*sold; // previous state of network N*(S+1)
double	****J; // connections with covariance learning rule N*Cm*S*S
double	*units_receive_cue; //  to construct a cue correlated with a pattern
double	*m; // overlap of network with each of p patterns
double	**sparsity_by_state; // overlap of network with each of p patterns
double  m_retr;
int     retr; // pattern that I want to retrieve
double	**h; // field that each active state recieves due to connections during learning
double	**r; // field that each active state recieves due to connections during learning
double	**theta; // variable threshold
int 	**Permut; // sequences of random numbers without repetition used for updating
int 	**C; // a nonsymmetric dilution matrix
double 	**C_sym; // a symmetric dilution matrix
double	***fin_state; // the final state of the network needed for computing C0, C1, C2 and q
double	**C0; // fraction of units co-inactive
double	**C1; // fraction of units co-same-state-active
double	**C2; // fraction of units co-active
double	H_interaction, H_quad; // each of the energy terms
double fraction_above_seventy;
double fraction_above_eighty;
double fraction_above_ninety;

/* FILES */
FILE *storage_cap;
FILE *overlap_intime;
FILE *fin_sparsity;
FILE *fin_overlaps;
FILE *cij;
FILE *q_file;
FILE *state_file;
FILE *C0_file;
FILE *C1_file;
FILE *C2_file;
FILE *fin_state_file;
FILE *H_quad_file;
FILE *H_interaction_file;
FILE *field_file;

/*from functions*/

//extern void read_pattern(int); // reads pattern from file generated by genero_pattern
extern void initializing(); // initializes the network state
extern void construct_CJ(); // constructs J and C
extern void update_state(int, int, int); // takes trial, unit index to update and counter n as input

extern void getmemory(); 
extern void deletememory(); 
extern void SetUpTables(); // sets up update tables

extern void compute_m(); // computes overlap of network state with each pattern
extern void compute_m_retr(); // computes overlap of network state with each pattern
extern void compute_C0_C1_C2();
extern void compute_sparsity_by_state();

/*functions generate pattern v2*/

extern void GetMemory();
extern void SetFactors();
extern void AssignChildren();
extern void SetPatterns();
extern int SavePatterns();

/*functions generate pattern rand*/

extern void GenerateRandomPatterns();
//extern void SaveRandomPatterns(double, int);

/*---------------------------RUNS FOR 1 PATTERN-----------------------*/

void run_net(int mu_f)
{
	int i, n, trial, x, mu, iii, ttt;
	double t;
	fraction_above_seventy = 0;
	fraction_above_eighty = 0;
	fraction_above_ninety = 0;
	/* cueing each pattern*/
	/* for controlling the size of the data set*/
	for(mu=0; mu<mu_f; mu++)
	{
	  retr=mu; // retrieve this pattern
	  /* each trial corresponds to retrieving the same pattern, retr, corrupted differently */
	  for(trial=0;trial<tot_num_trials;trial++)
	  {
		n=0; // unit update counter
		//printf("retrieval of pattern %d\n", retr);
		/* initialize network with cue */
		initializing();
        //compute_m_retr(a);
        //printf("m_retr=%.2f\n", m_retr);
		//printf("after initializing\n");
		/* update network */
		for(ttt=0;ttt<Tnet;ttt++)
		{
		  /*x is an updating sequence*/
		  x=(int)(NumSet*drand48()); 
		  for(iii=0;iii<N;iii++)
		  {
			i=Permut[iii][x]; // for asynchronous updating without repetition!
			update_state(trial, i, n); 
		 	//if((n%time_toprint)==0)
		 	//{
		 	  //t=(double)n/N; // effective time
		 	  //compute_m_retr(a);
		 	  //printf("m_retr=%.2f\n", m_retr);
		 	//}
			n++;
		  }
		}
		/* end of update after cue */

		compute_m_retr(); // computes overlap
	
		printf( "S=%d p=%d U=%.2f a=%.2f fraction_units_cued=%.2f ret=%d m[retr]=%.2f \n ",S,p,U,a,fraction_units_cued,retr,m_retr);
		//printf("---------------------------------------\n");
		if (m_retr >= 0.7) fraction_above_seventy++;
		if (m_retr >= 0.8) fraction_above_eighty++;
		if (m_retr >= 0.9) fraction_above_ninety++;
	  }
	}	
}

/*---------------------------- START SIMULATION----------------------*/

int main(int argc, char **argv)
{

MPI_Init(NULL, NULL);

int mu_f;

Cm = atoi(argv[1]);

//srand48(time(0));

/*Initialize the MPI environment. The two arguments to MPI Init are not
currently used by MPI implementations, but are there in case future
implementations might need the arguments. */

int i;
int temp_p;
double *temp;
int file_free = 0;

/*Get the number of processes*/
int np;
MPI_Comm_size(MPI_COMM_WORLD, &np);
//printf("np=%d\n", np);

/*Get the rank of the process*/
int proc_id;
MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
MPI_Status status;
if ( proc_id == 0 ) file_free = 1;
else MPI_Recv(&file_free, 1, MPI_INT, proc_id-1, 1, MPI_COMM_WORLD, &status);
if (file_free == 1) // this process calls the routine to read and process input file
/*give read file permission to the next process*/
if (proc_id != np-1) MPI_Send(&file_free, 1, MPI_INT, proc_id+1, 1, MPI_COMM_WORLD);

temp = new double[3];

/*p related things*/
p=Pstep*proc_id+Pmin;

mu_f=10;

//if (p<200) mu_f = p;
//else mu_f = 200;

//mu_f = p;
p_fact = 0.1*p;

//printf("a=%.2f\n", a);
//printf("p=%d\n", p);
//printf("p_fact=%d\n",p_fact);

getmemory();

/*--------------------- Generate correlated patterns ------------------*/
GetMemory();
SetFactors();
AssignChildren();
SetPatterns();
//int num_children_generated;
/* to save patterns with different sparsities in different files */
//num_children_generated = SavePatterns();
//printf("number children generated = %d\n", num_children_generated);


/*--------------------- Generate random patterns ----------------------*/
//GenerateRandomPatterns();
//SaveRandomPatterns();

//compute_sparsity_by_state();
SetUpTables();

construct_CJ();
run_net(mu_f);

temp_p = p;
temp[0] = fraction_above_seventy/(float)(mu_f*tot_num_trials);
temp[1] = fraction_above_eighty/(float)(mu_f*tot_num_trials);
temp[2] = fraction_above_ninety/(float)(mu_f*tot_num_trials);

printf("%d %f %f %f\n", temp_p, temp[0], temp[1], temp[2]);

char *outdir;
outdir = new char [100*sizeof(char)];
sprintf(outdir, "mkdir -p C%i", Cm); // the -p flag means it will create dir only if it doesn't already exist
system(outdir);

FILE *test;
char *buffer; 
buffer = new char [1000*sizeof(char)];

sprintf(buffer, "C%i/p%d", Cm, p);
test = fopen(buffer, "w");
fprintf(test, "%d %f %f %f\n", temp_p, temp[0], temp[1], temp[2]);
fclose(test);

// Finalize the MPI environment. No more MPI calls can be made after this

MPI_Finalize();
return 0;

}
