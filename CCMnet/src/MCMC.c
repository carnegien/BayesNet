/*
 *  File ergm/src/MCMC.c
 *  Part of the statnet package, http://statnet.org
 *
 *  This software is distributed under the GPL-3 license.  It is free,
 *  open source, and has the attribution requirements (GPL Section 7) in
 *    http://statnet.org/attribution
 *
 *  Copyright 2012 the statnet development team
 */
#include "MCMC.h"
#include <math.h> /* MOD ADDED RAVI */
#include "MCMC_prob.h"
#include "MCMC_prob_bi.h"
/*****************
 Note on undirected networks:  For j<k, edge {j,k} should be stored
 as (j,k) rather than (k,j).  In other words, only directed networks
 should have (k,j) with k>j.
*****************/

/*****************
 void MCMC_wrapper

 Wrapper for a call from R.

 and don't forget that tail -> head
*****************/
void MCMC_wrapper(int *dnumnets, int *nedges,
		  int *tails, int *heads,
		  int *dn, int *dflag, int *bipartite, 
		  int *nterms, char **funnames,
		  char **sonames, 
		  char **MHproposaltype, char **MHproposalpackage,
		  double *inputs, double *theta0, int *samplesize, 
		  double *sample, int *burnin, int *interval,  
		  int *newnetworktails, 
		  int *newnetworkheads, 
		  int *fVerbose, 
		  int *attribs, int *maxout, int *maxin, int *minout,
		  int *minin, int *condAllDegExact, int *attriblength, 
		  int *maxedges,
		  int *status,
        int *prob_type,   /*MOD ADDED RAVI###########*/
        int *maxdegree,
        double *meanvalues,
        double *varvalues,
        int *BayesInference,
        int *Trans_nedges,
        int *Trans_networktails,
        int *Trans_networkheads,
        double *Ia, 
        double *Il, 
        double *R_times, 
        double *beta_a, 
        double *beta_l,
        int *NetworkForecast,
        double *evolutionrate,
        double *evolutionvar,
	int *print_info
                ){
  int directed_flag;
  Vertex n_nodes, nmax, bip;
  //Edge n_networks;
  Network nw[1];
  Network TransNW[1]; /*MOD ADDED Ravi */
  Model *m;
  MHproposal MH;

  int print_info_MCMCwrapper = 0;
  
if (print_info_MCMCwrapper == 1) {
        Rprintf("Entered C Code: MCMCWrapper\n");
}
  
  n_nodes = (Vertex)*dn; 
  //n_networks = (Edge)*dnumnets; 
  nmax = (Edge)abs(*maxedges);
  bip = (Vertex)*bipartite; 

//Rprintf("Bipartite Flag - NW: MCMCWrapper %d\n", *bipartite);
//Rprintf("Bipartite Info - NW: MCMCWrapper %d\n", bip);
  
  GetRNGstate();  /* R function enabling uniform RNG */
  
  directed_flag = *dflag;

  m=ModelInitialize(*funnames, *sonames, &inputs, *nterms);

if (print_info_MCMCwrapper == 1) {  
        Rprintf("Created Model: MCMCWrapper\n");
}
  
  /* Form the network */
  nw[0]=NetworkInitialize(tails, heads, nedges[0], 
                          n_nodes, directed_flag, bip, 0, 0, NULL);

if (print_info_MCMCwrapper == 1) {  
        Rprintf("Created Network - NW: MCMCWrapper\n");
}
  
/*MOD ADDED Ravi */

//Rprintf("BayesInference Flag - NW: MCMCWrapper %d\n", BayesInference);
//Rprintf("BayesInference Flag - NW: MCMCWrapper %d\n", *BayesInference);
//Rprintf("BayesInference Flag - NW: MCMCWrapper %d\n", BayesInference[0]);
//Rprintf("BayesInference Flag - NW: MCMCWrapper %d\n", *BayesInference[0]);

//Rprintf("MCMC_Wrapper - theta0[0]: %f\n", theta0[0]);

  if ((theta0[0] < -999) && (theta0[0] > -1000)) {
        TransNW[0]=NetworkInitialize(Trans_networktails, Trans_networkheads, Trans_nedges[0], 
                          n_nodes, directed_flag, bip, 0, 0, NULL); 
  } else {
      TransNW[0]=NetworkInitialize(tails, heads, nedges[0], 
                          n_nodes, directed_flag, bip, 0, 0, NULL);
  }

if (print_info_MCMCwrapper == 1) {         
        Rprintf("Created Networks: MCMCWrapper\n");
}
  
  MH_init(&MH,
	  *MHproposaltype, *MHproposalpackage,
	  inputs,
	  *fVerbose,
	  nw, attribs, maxout, maxin, minout, minin,
	  *condAllDegExact, *attriblength);

if (print_info_MCMCwrapper == 1) { 
        Rprintf("Created MH Proposal: MCMCWrapper\n");
}
  
  *status = MCMCSample(&MH,
		       theta0, sample, *samplesize,
		       *burnin, *interval,
		       *fVerbose, nmax, nw, m,
        prob_type,   /*MOD ADDED RAVI###########*/
        maxdegree,
        meanvalues,
        varvalues,
        BayesInference,
        TransNW,
        Ia, 
        Il, 
        R_times, 
        beta_a, 
        beta_l,
        NetworkForecast,
        evolutionrate,
        evolutionvar,
        print_info
          );
  
  MH_free(&MH);
        
//Rprintf("Back! %d %d\n",nw[0].nedges, nmax); 

  /* record new generated network to pass back to R */
  if(*status == MCMC_OK && *maxedges>0 && newnetworktails && newnetworkheads)
    newnetworktails[0]=newnetworkheads[0]=EdgeTree2EdgeList(newnetworktails+1,newnetworkheads+1,nw,nmax-1);
  
  ModelDestroy(m);
  NetworkDestroy(nw);
  NetworkDestroy(TransNW); /*MOD ADD Ravi */
  PutRNGstate();  /* Disable RNG before returning */
}


/*********************
 MCMCStatus MCMCSample

 Using the parameters contained in the array theta, obtain the
 network statistics for a sample of size samplesize.  burnin is the
 initial number of Markov chain steps before sampling anything
 and interval is the number of MC steps between successive 
 networks in the sample.  Put all the sampled statistics into
 the networkstatistics array. 
*********************/
MCMCStatus MCMCSample(MHproposal *MHp,
		double *theta, double *networkstatistics, 
		int samplesize, int burnin, 
		int interval, int fVerbose, int nmax,
		Network *nwp, Model *m,
        int *prob_type,   /*MOD ADDED RAVI###########*/
        int *maxdegree,
        double *meanvalues,
        double *varvalues,
        int *BayesInference,
        Network *TransNW,
        double *Ia, 
        double *Il, 
        double *R_times, 
        double *beta_a, 
        double *beta_l,
        int *NetworkForecast,
        double *evolutionrate,
        double *evolutionvar,
	int *print_info){
  int staken, tottaken;
  int i, j;
  
  int print_info_MCMCsample = 0;  
  
if (print_info_MCMCsample == 1) {  
        Rprintf("Entered: MCMCSample\n");
}
  
  /*********************
  networkstatistics are modified in groups of m->n_stats, and they
  reflect the CHANGE in the values of the statistics from the
  original (observed) network.  Thus, when we begin, the initial 
  values of the first group of m->n_stats networkstatistics should 
  all be zero
  *********************/
/*for (j=0; j < m->n_stats; j++) */
/*  networkstatistics[j] = 0.0; */
/* Rprintf("\n"); */
/* for (j=0; j < m->n_stats; j++){ */
/*   Rprintf("j %d %f\n",j,networkstatistics[j]); */
/* } */
/* Rprintf("\n"); */

  /*********************
   Burn in step.
   *********************/
/*  Catch more edges than we can return */

if (print_info_MCMCsample == 1) {  
        Rprintf("Going to: MH\n");
}

  if (nwp->bipartite == 0) {
      if(MetropolisHastings(MHp, theta, networkstatistics, burnin, &staken,
			fVerbose, nwp, m,
        prob_type,   /*MOD ADDED RAVI###########*/
        maxdegree,
        meanvalues,
        varvalues,
        BayesInference,
        TransNW,
        Ia, 
        Il, 
        R_times, 
        beta_a, 
        beta_l,
        NetworkForecast,
        evolutionrate,
        evolutionvar,
	print_info
          )!=MCMC_OK)
    return MCMC_MH_FAILED;
  } else {
      if(MetropolisHastings_bi(MHp, theta, networkstatistics, burnin, &staken,
			fVerbose, nwp, m,
        prob_type,   /*MOD ADDED RAVI###########*/
        maxdegree,
        meanvalues,
        varvalues,
        BayesInference,
        TransNW,
        Ia, 
        Il, 
        R_times, 
        beta_a, 
        beta_l,
        NetworkForecast,
        evolutionrate,
        evolutionvar,
	print_info
          )!=MCMC_OK)
    return MCMC_MH_FAILED;      
  }
if (print_info_MCMCsample == 1) {  
  Rprintf("Just left: MH\n");
}
  
  if(nmax!=0 && nwp->nedges >= nmax-1){
    return MCMC_TOO_MANY_EDGES;
  }
  
/*   if (fVerbose){ 
       Rprintf(".");
     } */
  
  if (samplesize>1){
    staken = 0;
    tottaken = 0;
    
    /* Now sample networks */
    for (i=1; i < samplesize; i++){
      /* Set current vector of stats equal to previous vector */

//for (j=0; j < m->n_stats; j++){
//   Rprintf("MCMCSample 1 %d %d %f\n",j,nwp->nedges,networkstatistics[j]);
//}

      for (j=0; j<m->n_stats; j++){
        networkstatistics[j+m->n_stats] = networkstatistics[j];
//Rprintf("3 %d %d %f\n",j,nwp->nedges,networkstatistics[j+m->n_stats]);
      }
//Rprintf("4 %d\n",m->n_stats);

//for (j=0; j < m->n_stats; j++){
//   Rprintf("MCMCSample 2 %d %d %f\n",j,nwp->nedges,networkstatistics[j]);
//}

      networkstatistics += m->n_stats;
      /* This then adds the change statistics to these values */
      
      /* Catch massive number of edges caused by degeneracy */

//for (j=0; j < m->n_stats; j++){
//  Rprintf("MCMCSample %d %d %f\n",j,nwp->nedges,networkstatistics[j]);
//}

  if (nwp->bipartite == 0) {
      if(MetropolisHastings(MHp, theta, networkstatistics, interval, &staken,
			    fVerbose, nwp, m,
        prob_type,   /*MOD ADDED RAVI###########*/
        maxdegree,
        meanvalues,
        varvalues,
        BayesInference,
        TransNW,
        Ia, 
        Il, 
        R_times, 
        beta_a, 
        beta_l,
        NetworkForecast,
        evolutionrate,
        evolutionvar,
	print_info
              )!=MCMC_OK)
	return MCMC_MH_FAILED;
  } else {
      if(MetropolisHastings_bi(MHp, theta, networkstatistics, interval, &staken,
			    fVerbose, nwp, m,
        prob_type,   /*MOD ADDED RAVI###########*/
        maxdegree,
        meanvalues,
        varvalues,
        BayesInference,
        TransNW,
        Ia, 
        Il, 
        R_times, 
        beta_a, 
        beta_l,
        NetworkForecast,
        evolutionrate,
        evolutionvar,
	print_info
              )!=MCMC_OK)
	return MCMC_MH_FAILED;      
  }
      if(nmax!=0 && nwp->nedges >= nmax-1){
	return MCMC_TOO_MANY_EDGES;
      }
      tottaken += staken;

//for (j=0; j < m->n_stats; j++){
//   Rprintf("6 %d %f\n",j,networkstatistics[j]);
//}
      
#ifdef Win32
      if( ((100*i) % samplesize)==0 && samplesize > 500){
	R_FlushConsole();
    	R_ProcessEvents();
      }
#endif
    }
    /*********************
    Below is an extremely crude device for letting the user know
    when the chain doesn't accept many of the proposed steps.
    *********************/
    if (fVerbose){
      Rprintf("Sampler accepted %6.3f%% of %d proposed steps.\n",
      tottaken*100.0/(1.0*interval*samplesize), interval*samplesize); 
    }
  }else{
    if (fVerbose){
      Rprintf("Sampler accepted %6.3f%% of %d proposed steps.\n",
      staken*100.0/(1.0*burnin), burnin); 
    }
  }
  return MCMC_OK;
}

/*********************
 void MetropolisHastings

 In this function, theta is a m->n_stats-vector just as in MCMCSample,
 but now networkstatistics is merely another m->n_stats-vector because
 this function merely iterates nsteps times through the Markov
 chain, keeping track of the cumulative change statistics along
 the way, then returns, leaving the updated change statistics in
 the networkstatistics vector.  In other words, this function 
 essentially generates a sample of size one
*********************/


//#################################################################




//#################################################################

void MCMCPhase12 (int *tails, int *heads, int *dnedges, 
		  int *dn, int *dflag, int *bipartite, 
		  int *nterms, char **funnames,
		  char **sonames, 
		  char **MHproposaltype, char **MHproposalpackage,
		  double *inputs, 
		  double *theta0, int *samplesize,
		  double *gain, double *meanstats, int *phase1, int *nsub,
		  double *sample, int *burnin, int *interval,  
		  int *newnetworktails, 
		  int *newnetworkheads, 
		  int *fVerbose, 
		  int *attribs, int *maxout, int *maxin, int *minout,
		  int *minin, int *condAllDegExact, int *attriblength, 
		  int *maxedges,
		  int *mtails, int *mheads, int *mdnedges,
        int *prob_type,   /*MOD ADDED RAVI###########*/
        int *maxdegree,
        double *meanvalues,
        double *varvalues,
        int *BayesInference,
        int *Trans_nedges,
        int *Trans_networktails,
        int *Trans_networkheads,
        double *Ia, 
        double *Il, 
        double *R_times, 
        double *beta_a, 
        double *beta_l,
        int *NetworkForecast,
        double *evolutionrate,
        double *evolutionvar,
	int *print_info
        )  {
  int directed_flag;
  int nphase1, nsubphases;
  Vertex n_nodes, bip;
  Edge n_edges, nmax;
  Network nw[2];
  Model *m;
  MHproposal MH;
  
  Network TransNW[1];
  
  nphase1 = *phase1; 
  nsubphases = *nsub;

  n_nodes = (Vertex)*dn; 
  n_edges = (Edge)*dnedges; 
  nmax = (Edge)*maxedges; 
  bip = (Vertex)*bipartite; 
  
  GetRNGstate();  /* R function enabling uniform RNG */
  
  directed_flag = *dflag;

  m=ModelInitialize(*funnames, *sonames, &inputs, *nterms);

  /* Form the missing network */
  nw[0]=NetworkInitialize(tails, heads, n_edges,
                          n_nodes, directed_flag, bip, 0, 0, NULL);

  TransNW[0]=NetworkInitialize(Trans_networktails, Trans_networkheads, Trans_nedges[0], 
                          n_nodes, directed_flag, bip, 0, 0, NULL); /*MOD ADDED Ravi */
  
  MH_init(&MH,
	  *MHproposaltype, *MHproposalpackage,
	  inputs,
	  *fVerbose,
	  nw, attribs, maxout, maxin, minout, minin,
	  *condAllDegExact, *attriblength);
  
  MCMCSamplePhase12 (&MH,
		     theta0, *gain, meanstats, nphase1, nsubphases, sample, *samplesize,
		     *burnin, *interval,
		     (int)*fVerbose, nw, m,
        prob_type,   /*MOD ADDED RAVI###########*/
        maxdegree,
        meanvalues,
        varvalues,
        BayesInference,
        TransNW,
        Ia, 
        Il, 
        R_times, 
        beta_a, 
        beta_l,
        NetworkForecast,
        evolutionrate,
        evolutionvar,
        print_info
          );

  MH_free(&MH);
  
  /* record new generated network to pass back to R */
  if(nmax>0 && newnetworktails && newnetworkheads)
    newnetworktails[0]=newnetworkheads[0]=EdgeTree2EdgeList(newnetworktails+1,newnetworkheads+1,nw,nmax-1);

  ModelDestroy(m);

  NetworkDestroy(nw);
  PutRNGstate();  /* Disable RNG before returning */
}

/*********************
 void MCMCSamplePhase12

 Using the parameters contained in the array theta, obtain the
 network statistics for a sample of size samplesize.  burnin is the
 initial number of Markov chain steps before sampling anything
 and interval is the number of MC steps between successive 
 networks in the sample.  Put all the sampled statistics into
 the networkstatistics array. 
*********************/
void MCMCSamplePhase12(MHproposal *MHp,
		       double *theta, double gain, double *meanstats, int nphase1, int nsubphases, double *networkstatistics, 
		       int samplesize, int burnin, 
		       int interval, int fVerbose,
		       Network *nwp, Model *m,
        int *prob_type,   /*MOD ADDED RAVI###########*/
        int *maxdegree,
        double *meanvalues,
        double *varvalues,
        int *BayesInference,
        Network *TransNW,
        double *Ia, 
        double *Il, 
        double *R_times, 
        double *beta_a, 
        double *beta_l,
        int *NetworkForecast,
        double *evolutionrate,
        double *evolutionvar,
	int *print_info
        ){
  int staken, tottaken, ptottaken;
  int i, j, iter=0;
  
/*Rprintf("nsubphases %d\n", nsubphases); */

  nwp->duration_info.MCMCtimer=0;
  
  /*if (fVerbose)
    Rprintf("The number of statistics is %i and the total samplesize is %d\n",
    m->n_stats,samplesize);*/

  /*********************
  networkstatistics are modified in groups of m->n_stats, and they
  reflect the CHANGE in the values of the statistics from the
  original (observed) network.  Thus, when we begin, the initial 
  values of the first group of m->n_stats networkstatistics should 
  all be zero
  *********************/
  double *ubar, *u2bar, *aDdiaginv;
  ubar = (double *)malloc( m->n_stats * sizeof(double));
  u2bar = (double *)malloc( m->n_stats * sizeof(double));
  aDdiaginv = (double *)malloc( m->n_stats * sizeof(double));
  for (j=0; j < m->n_stats; j++){
    networkstatistics[j] = -meanstats[j];
    ubar[j] = 0.0;
    u2bar[j] = 0.0;
  }

  /*********************
   Burn in step.  While we're at it, use burnin statistics to 
   prepare covariance matrix for Mahalanobis distance calculations 
   in subsequent calls to M-H
   *********************/
/*Rprintf("MCMCSampleDyn pre burnin numdissolve %d\n", *numdissolve); */
  
    staken = 0;
    Rprintf("Starting burnin of %d steps\n", burnin);
    MetropolisHastings (MHp, theta,
		  networkstatistics, burnin, &staken,
      fVerbose,
		  nwp, m,
        prob_type,   /*MOD ADDED RAVI###########*/
        maxdegree,
        meanvalues,
        varvalues,
        BayesInference,
        TransNW,
        Ia, 
        Il, 
        R_times, 
        beta_a, 
        beta_l,
        NetworkForecast,
        evolutionrate,
        evolutionvar,
	print_info);
    Rprintf("Phase 1: %d steps (interval = %d)\n", nphase1,interval);
    /* Now sample networks */
    for (i=0; i <= nphase1; i++){
      MetropolisHastings (MHp, theta,
		  networkstatistics, interval, &staken,
      fVerbose,
		  nwp, m,
        prob_type,   /*MOD ADDED RAVI###########*/
        maxdegree,
        meanvalues,
        varvalues,
        BayesInference,
        TransNW,
        Ia, 
        Il, 
        R_times, 
        beta_a, 
        beta_l,
        NetworkForecast,
        evolutionrate,
        evolutionvar,
	print_info 
              );
      if(i > 0){
       for (j=0; j<m->n_stats; j++){
        ubar[j]  += networkstatistics[j];
        u2bar[j] += networkstatistics[j]*networkstatistics[j];
/*  Rprintf("j %d ubar %f u2bar %f ns %f\n", j,  ubar[j], u2bar[j], */
/*		  networkstatistics[j]); */
       }
      }
    }
    if (fVerbose){
      Rprintf("Returned from Phase 1\n");
      Rprintf("\n gain times inverse variances:\n");
    }
    for (j=0; j<m->n_stats; j++){
      aDdiaginv[j] = u2bar[j]-ubar[j]*ubar[j]/(1.0*nphase1);
      if( aDdiaginv[j] > 0.0){
        aDdiaginv[j] = nphase1*gain/aDdiaginv[j];
      }else{
	aDdiaginv[j]=0.00001;
      }
      if (fVerbose){ Rprintf(" %f", aDdiaginv[j]);}
    }
    if (fVerbose){ Rprintf("\n"); }
  
    staken = 0;
    tottaken = 0;
    ptottaken = 0;
    
    if (fVerbose){
      Rprintf("Phase 2: (samplesize = %d)\n", samplesize);
    }
    /* Now sample networks */
    for (i=1; i < samplesize; i++){
      
      MetropolisHastings (MHp, theta,
		  networkstatistics, interval, &staken,
      fVerbose,
		  nwp, m,
        prob_type,   /*MOD ADDED RAVI###########*/
        maxdegree,
        meanvalues,
        varvalues,
        BayesInference,
        TransNW,
        Ia, 
        Il, 
        R_times, 
        beta_a, 
        beta_l,
        NetworkForecast,
        evolutionrate,
        evolutionvar,
	print_info 
              );
    /* Update theta0 */
/*Rprintf("initial:\n"); */
      for (j=0; j<m->n_stats; j++){
        theta[j] -= aDdiaginv[j] * networkstatistics[j];
      }
/*Rprintf("\n"); */
/*    if (fVerbose){ Rprintf("nsubphases %d i %d\n", nsubphases, i); } */
      if (i==(nsubphases)){
	nsubphases = trunc(nsubphases*2.52) + 1;
        if (fVerbose){
	 iter++;
	 Rprintf("End of iteration %d; Updating the number of sub-phases to be %d\n",iter,nsubphases);
	}
        for (j=0; j<m->n_stats; j++){
          aDdiaginv[j] /= 2.0;
          if (fVerbose){Rprintf("theta_%d = %f; change statistic[%d] = %f\n",
		                 j+1, theta[j], j+1, networkstatistics[j]);}
/*        if (fVerbose){ Rprintf(" %f statsmean %f",  theta[j],(networkstatistics[j]-meanstats[j])); } */
        }
        if (fVerbose){ Rprintf("\n"); }
      }
      /* Set current vector of stats equal to previous vector */
      for (j=0; j<m->n_stats; j++){
/*      networkstatistics[j] -= meanstats[j]; */
        networkstatistics[j+m->n_stats] = networkstatistics[j];
      }
      networkstatistics += m->n_stats;
/*      if (fVerbose){ Rprintf("step %d from %d:\n",i, samplesize);} */
      /* This then adds the change statistics to these values */
      tottaken += staken;
#ifdef Win32
      if( ((100*i) % samplesize)==0 && samplesize > 500){
	R_FlushConsole();
    	R_ProcessEvents();
      }
#endif
      if (fVerbose){
        if( ((3*i) % samplesize)==0 && samplesize > 500){
        Rprintf("Sampled %d from Metropolis-Hastings\n", i);}
      }
      
      if( ((3*i) % samplesize)==0 && tottaken == ptottaken){
        ptottaken = tottaken; 
        Rprintf("Warning:  Metropolis-Hastings algorithm has accepted only "
        "%d steps out of a possible %d\n",  ptottaken-tottaken, i); 
      }
/*      Rprintf("Sampled %d from %d\n", i, samplesize); */

    /*********************
    Below is an extremely crude device for letting the user know
    when the chain doesn't accept many of the proposed steps.
    *********************/
/*    if (fVerbose){ */
/*      Rprintf("Metropolis-Hastings accepted %7.3f%% of %d steps.\n", */
/*	      tottaken*100.0/(1.0*interval*samplesize), interval*samplesize);  */
/*    } */
/*  }else{ */
/*    if (fVerbose){ */
/*      Rprintf("Metropolis-Hastings accepted %7.3f%% of %d steps.\n", */
/*	      staken*100.0/(1.0*burnin), burnin);  */
/*    } */
  }
/*  Rprintf("netstats: %d\n", samplesize); */
/*  for (i=0; i < samplesize; i++){ */
/*   for (j=0; j < m->n_stats; j++){ */
/*      Rprintf("%f ", networkstatistics[j+(m->n_stats)*(i)]); */
/*   } */
/*  Rprintf("\n"); */
/*  } */
  if (fVerbose){
    Rprintf("Phase 3: MCMC-Newton-Raphson\n");
  }

  free(ubar);
  free(u2bar);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define A 12

double sp_gamma(double z)
{
  const int a = A;
  static double c_space[A];
  static double *c = NULL;
  int k;
  double accm;
 
  if ( c == NULL ) {
    double k1_factrl = 1.0; /* (k - 1)!*(-1)^k with 0!==1*/
    c = c_space;
    c[0] = sqrt(2.0*M_PI);
    for(k=1; k < a; k++) {
      c[k] = exp(a-k) * pow(a-k, k-0.5) / k1_factrl;
	  k1_factrl *= -k;
    }
  }
  accm = c[0];
  for(k=1; k < a; k++) {
    accm += c[k] / ( z + k );
  }
  accm *= exp(-(z+a)) * pow(z+a, z+0.5); /* Gamma(z+1) */
  return accm/z;
}

int calcCNR( int n, int r)
{
    int answer = 1;
    int multiplier = n;
    int divisor = 1;
    int k;
    
    if (r < (n-r)) {
        k = r;
    } else {
        k = n-r;
    }
     
    while(divisor <= k)
    {
            answer = ((answer * multiplier) / divisor);
            multiplier--;
            divisor++;         
    }
    return answer;
}
