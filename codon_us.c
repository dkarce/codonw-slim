/**************************************************************************/
/* CodonW codon usage analysis package                                    */
/* Copyright (C) 2005            John F. Peden                            */
/* This program is free software; you can redistribute                    */
/* it and/or modify it under the terms of the GNU General Public License  */
/* as published by the Free Software Foundation; version 2 of the         */
/* License,                                                               */
/*                                                                        */
/* This program is distributed in the hope that it will be useful, but    */
/* WITHOUT ANY WARRANTY; without even the implied warranty of             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           */
/* GNU General Public License for more details.                           */
/* You should have received a copy of the GNU General Public License along*/
/* with this program; if not, write to the Free Software Foundation, Inc.,*/
/* 675 Mass Ave, Cambridge, MA 02139, USA.                                */
/*                                                                        */
/*                                                                        */
/* The author can be contacted by email (jfp#hanson-codonw@yahoo.com Anti-*/
/* Spam please change the # in my email to an _)                          */
/*                                                                        */
/* For the latest version and information see                             */
/* http://codonw.sourceforge.net 					  */
/**************************************************************************/
/*                                                                        */
/* -----------------------        codon_us.C     ------------------------ */
/* This file contains most of the codon usage analysis subroutines        */
/* except for the COA analysis                                            */
/* Internal subroutines and functions                                     */
/* initialize_point    assigns genetic code dependent parameters to structs*/
/* codon_usage_tot    Counts codon and amino acid usage                   */
/* ident_codon        Converts codon into a numerical value in range 1-64 */
/* codon_usage_out    Write out Codon Usage to file                       */
/* codon_error        Called after all codons read, checks data was OK    */
/* rscu_usage_out     Write out RSCU                                      */
/* raau_usage_out     Write out normalised amino acid usage               */
/* aa_usage_out       Write out amino acid usage                          */
/* how_synon          Calculates how synonymous each codon is             */
/* how_synon_aa       Calculates how synonymous each AA is                */
/* clean_up           Re-zeros various internal counters and arrays       */
/* base_sil_us_out    Write out base composition at silent sites          */
/* cai_out            Write out CAI usage                                 */
/* cbi_out            Write out codon bias index                          */
/* fop_out            Write out Frequency of Optimal codons               */
/* enc_out            Write out Effective Number of codons                */
/* gc_out             Writes various analyses of base usage               */
/* cutab_out          Write a nice tabulation of the RSCU+CU+AA           */
/* dinuc_count        Count the dinucleotide usage                        */
/* dinuc_out          Write out dinucleotide usage                        */
/* hydro_out          Write out Protein hydropathicity                    */
/* aromo_out          Write out Protein aromaticity                       */
/*                                                                        */
/*                                                                        */
/* External subroutines to codon_us.c                                     */
/* my_exit            Controls exit from CodonW closes any open files     */
/* tidy               reads the input data                                */
/* output             called from tidy to decide what to do with the data */
/* open_file          Open files, checks for existing files               */
/* fileclose          Closes files and returns a NULL pointer or exits    */
/*                                                                        */
/**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>

#include "codonW.h"

static int ident_codon(char *codon);
static int* how_synon(GENETIC_CODE_STRUCT *pcu);
static int* how_synon_aa(GENETIC_CODE_STRUCT *pcu);

/********************* Initilize Pointers**********************************/
/* Various pointers to structures are assigned here dependent on the      */
/* genetic code chosen.                                                   */
/* paa                points to a struct containing Amino Acid names      */
/* pap                points to amino acid properties                     */
/* pcai               points to Adaptation values used to calc CAI        */
/* pfop               points to a struct describing optimal codons        */
/* pcbi               points to the same structure as pfop                */
/* pcu                points to data which has the translation of codons  */
/* ds                 is a struct describing how synonymous a codon is    */
/* da                 is a struct describing the size of each AA family   */
/*                    included/excluded from any COA analysis             */
/**************************************************************************/
int initialize_point(char code, char fop_species, char cai_species, MENU_STRUCT *pm, REF_STRUCT *ref)
{
   pm->paa = ref->amino_acids;
   pm->pap = ref->amino_prop;
   pm->pcai = &(ref->cai[cai_species]);
   pm->pfop = &(ref->fop[fop_species]);
   pm->pcbi = &(ref->fop[fop_species]);
   pm->pcu = &(ref->cu[code]);
   pm->ds = how_synon(pm->pcu);
   pm->da = how_synon_aa(pm->pcu);

   fprintf(pm->my_err, "Genetic code set to %s %s\n", pm->pcu->des, pm->pcu->typ);

   return 0;
}

/*******************How Synonymous is this codon  *************************/
/* Calculate how synonymous a codon is by comparing with all other codons */
/* to see if they encode the same AA                                      */
/**************************************************************************/
static int* how_synon(GENETIC_CODE_STRUCT *pcu)
{
   static int dds[65];
   int x, i;

   for (x = 0; x < 65; x++)
      dds[x] = 0;

   for (x = 1; x < 65; x++)
      for (i = 1; i < 65; i++)
         if (pcu->ca[x] == pcu->ca[i])
            dds[x]++;

   return dds;
}
/*******************How Synonymous is this AA     *************************/
/* Calculate how synonymous an amino acid is by checking all codons if    */
/* they encode this same AA                                               */
/**************************************************************************/
static int* how_synon_aa(GENETIC_CODE_STRUCT *pcu)
{
   static int dda[22];
   int x;

   for (x = 0; x < 22; x++)
      dda[x] = 0;

   for (x = 1; x < 65; x++)
      dda[pcu->ca[x]]++;

   return dda;
}

/****************** Codon Usage Counting      *****************************/
/* Counts the frequency of usage of each codon and amino acid this data   */
/* is used throughout CodonW                                              */
/* pcu->ca contains codon to amino acid translations for the current code */
/* and is assigned in initialise point                                    */
/**************************************************************************/
int codon_usage_tot(char *seq, long *codon_tot, long ncod[], long naa[], MENU_STRUCT *pm)
{
   char codon[4];
   int icode;
   unsigned int i;
   unsigned int seqlen = (int)strlen(seq);

   for (i = 0; i < seqlen - 2; i += 3)
   {
      strncpy(codon, (seq + i), 3);
      icode = ident_codon(codon);
      ncod[icode]++;         /*increment the codon count */
      naa[pm->pcu->ca[icode]]++; /*increment the AA count    */
      (*codon_tot)++;
   }

   if (seqlen % 3)
   {             /*if last codon was partial */
      icode = 0; /*set icode to zero and     */
      ncod[0]++; /*increment untranslated    */
   }             /*codons                    */

   if (pm->pcu->ca[icode] == 11)
      valid_stops++;

   return icode;
}

/****************** Ident codon               *****************************/
/* Converts each codon into a numerical array (codon) and converts this   */
/* array into a numerical value in the range 0-64, zero is reserved for   */
/* codons that contain at least one unrecognised base                     */
/**************************************************************************/
static int ident_codon(char *codon)
{
   int icode = 0;
   int x;

   for (x = 0; x < 3; x++)
   {
      switch (codon[x])
      {
      case 'T':
      case 't':
      case 'U':
      case 'u':
         codon[x] = (char)1;
         continue;
      case 'C':
      case 'c':
         codon[x] = (char)2;
         continue;
      case 'A':
      case 'a':
         codon[x] = (char)3;
         continue;
      case 'G':
      case 'g':
         codon[x] = (char)4;
         continue;
      case '\0':
         return 0;
      default:
         codon[x] = (char)0;
         break;
      }
   }
   if (codon[0] * codon[1] * codon[2] != 0)
      icode = (codon[0] - 1) * 16 + codon[1] + (codon[2] - 1) * 4;
   else
      icode = 0;

   return icode;
}

/****************** Codon error               *****************************/
/* Does some basic error checking for the input data, it can be called    */
/* using different error levels, thus generating different types of       */
/* messages. Basically checks for start, stop codons and internal stop    */
/* codons. As well as non-translatable and partial codons                 */
/**************************************************************************/
long codon_error(int x, int y, char *ttitle, char error_level, MENU_STRUCT *pm)
{
   GENETIC_CODE_STRUCT *pcu = pm->pcu; 

   long ns = 0; /* number of stops       */
   long loc_cod_tot = 0;
   int i;
   bool valid_start = true; // FIXME

   for (i = 1, ns = 0; i < 65; i++)
   {
      loc_cod_tot += ncod[i];
      if (pcu->ca[i] == 11)
         ns += ncod[i]; /*count  stop codons     */
   }

   // TODO: CHECK FOR ERRORS WITHIN HERE ITSELF
   // if (in[1] == 'T' && (in[0] == 'A' || in[2] == 'G'))
   //    valid_start = true; /* Yeup it could be a start codon   */
   // else
   //    valid_start = false; /* Nope it doesn't seem to be one   */

   switch (error_level)
   {
   case 1: /*internal stop codons    */
      ns = ns - valid_stops;
      /* a stop was a valid_stop if it was the last codon of a sequence  */

      if (!valid_start && pm->warn)
      {
         fprintf(pm->my_err, "\nWarning: Sequence %3li \"%-20.20s\" does "
                             "not begin with a recognised start codon\n",
                 num_sequence, ttitle);
      }

      if (ns && pm->warn)
      {
         if (pm->totals && pm->warn)
            fprintf(pm->my_err, "\nWarning: some sequences had internal stop"
                                " codons (found %li such codons)\n",
                    ns);
         else
            fprintf(pm->my_err, "\nWarning: Sequence %3li \"%-20.20s\" has "
                                "%li internal stop codon(s)\n",
                    num_sequence, ttitle, ns);
         num_seq_int_stop++;
      }
      break;
   case 2:
      if (ncod[0] == 1 && pcu->ca[x] != 11 && pm->warn)
      { /*  last codon was partial */
         fprintf(pm->my_err,
                 "\nWarning: Sequence %3li \"%-20.20s\" last codon was partial\n", num_sequence, ttitle);
      }
      else
      {
         if (ncod[0] && pm->warn)
         { /* non translatable codons */
            if (pm->totals)
               fprintf(pm->my_err,
                       "\nWarning: some sequences had non translatable"
                       " codons (found %li such codons)\n",
                       ncod[0]);
            else
               fprintf(pm->my_err,
                       "\nWarning: sequence %3li \"%-20.20s\" has %li non translatable"
                       " codon(s)\n",
                       num_sequence, ttitle, ncod[0]);
         }
         if (pcu->ca[x] != 11 && pm->warn)
         {
            if (!pm->totals)
            {
               fprintf(pm->my_err,
                       "\nWarning: Sequence %3li \"%-20.20s\" is not terminated by"
                       " a stop codon\n",
                       num_sequence, ttitle);
            }
         }
      }
      break;
   case 3:
      /* Nc error routines see codon_us      */
      if (x == 3)
         x = 4; /* if x=3 there are no 3 or 4 fold AA  */
      if (pm->warn)
      {
         fprintf(pm->my_err,
                 "\nSequence %li \"%-20.20s\" contains ", num_sequence, ttitle);
         (y) ? fprintf(pm->my_err, "only %i ", (int)y) : fprintf(pm->my_err, "no ");
         fprintf(pm->my_err, "amino acids with %i synonymous codons\n", x);
         fprintf(pm->my_err, "\t--Nc was not calculated \n");
      }
      break;
   case 4: /* run silent                          */
      break;
   default:
      my_exit(99, "Programme error in codon_error\n");
   }

   return loc_cod_tot; /* Number of codons counted            */
}

/****************** Codon Usage Out           *****************************/
/* Writes codon usage output to file. Note this subroutine is only called */
/* when machine readable output is selected, otherwise cutab_out is used  */
/**************************************************************************/
int codon_usage_out(FILE *fblkout, long *nncod, int last_aa,
                    int vvalid_stops, char *ttitle, MENU_STRUCT *pm)
{
   GENETIC_CODE_STRUCT *pcu = pm->pcu; 

   long ccodon_tot = 0;
   int x;
   char sp = pm->separator;

   ccodon_tot = codon_error(last_aa, vvalid_stops, "", (char)4, pm); /*dummy*/

   /*example of output                                                     */
   /*0,0,0,0,3,2,2,0,0,0,0,0,0,3,0,0,                                      */
   /*0,0,0,4,3,4,1,7,0,0,0,0,3,1,3,1,Codons=100                              */
   /*0,0,0,0,10,6,3,0,0,0,0,0,1,1,12,0,Universal Genetic code              */
   /*0,0,0,3,7,5,7,9,0,1,1,1,8,4,5,0,MLSPCOPER.PE1                         */

   for (x = 1; x < 65; x++)
   {

      fprintf(fblkout, "%ld%c", nncod[x], sp);

      switch (x)
      {
      case 16:
         fprintf(fblkout, "\n");
         break;
      case 32:
         fprintf(fblkout, "Codons=%ld\n", ccodon_tot);
         break;
      case 48:
         fprintf(fblkout, "%.30s\n", pcu->des);
         break;
      case 64:
         fprintf(fblkout, "%.20s\n", ttitle);
         break;
      default:
         break;
      }
   }
   return 0;
}
/******************  RSCU  Usage out          *****************************/
/* Writes Relative synonymous codon usage output to file. Note this subrou*/
/* tine is only called if machine readable output is selected             */
/* If human readable format was selected then what the user really wanted */
/* was cutab so this is automatically selected in codons.c                */
/* RSCU values are genetic codon dependent                                */
/**************************************************************************/
int rscu_usage_out(FILE *fblkout, long *nncod, long *nnaa, MENU_STRUCT *pm)
{
   GENETIC_CODE_STRUCT *pcu = pm->pcu; 
   int *ds = pm->ds;

   int x;
   char sp = pm->separator;

   /* ds points to an array[64] of synonym values i.e. how synon its AA is  */

   for (x = 1; x < 65; x++)
   {
      if (nnaa[pcu->ca[x]] != 0)
         fprintf(fblkout, "%5.3f%c",
                 ((float)nncod[x] / (float)nnaa[pcu->ca[x]]) * ((float)*(ds + x)), sp);
      else
         fprintf(fblkout, "0.000%c", sp);

      if (x == 64)
         fprintf(fblkout, "%-20.20s", title);

      if (!(x % 16))
         fprintf(fblkout, "\n");
   }
   return 0;
}
/******************   RAAU output             *****************************/
/* Writes Relative amino acid usage output to file. Amino Acid usage is   */
/* normalised for gene length                                             */
/**************************************************************************/
int raau_usage_out(FILE *fblkout, long *nnaa, MENU_STRUCT *pm)
{
   AMINO_STRUCT *paa = pm->paa;

   long aa_tot = 0;
   static char first_line = true;
   int i, x;
   char sp;

   sp = '\t';

   if (first_line)
   { /* if true write a header*/
      fprintf(fblkout, "%s", "Gene_name");

      for (i = 0; i < 22; i++)
         fprintf(fblkout, "%c%s", sp, paa->aa3[i]); /* three letter AA names*/
      fprintf(fblkout, "\n");
      first_line = false;
   }
   for (i = 1; i < 22; i++)
      if (i != 11)
         aa_tot += nnaa[i]; /* total No. of AAs      */

   fprintf(fblkout, "%.30s", title);

   for (x = 0; x < 22; x++)
      if (x == 11)
         fprintf(fblkout, "%c0.0000", sp); /* report 0 for stops    */
      else if (aa_tot)
         fprintf(fblkout, "%c%.4f", sp, (double)nnaa[x] / (double)aa_tot);
      else /*What no AminoAcids!!!! */
         fprintf(fblkout, "%c%c", sp, sp);

   fprintf(fblkout, "\n");
   return 0;
}
/******************   AA usage output         *****************************/
/* Writes amino acid usage output to file.                                */
/**************************************************************************/
int aa_usage_out(FILE *fblkout, long *nnaa, MENU_STRUCT *pm)
{
   AMINO_STRUCT *paa = pm->paa;

   static char first_line = true;
   int i;
   char sp = pm->separator;

   if (first_line)
   {
      fprintf(fblkout, "%s", "Gene_name");

      for (i = 0; i < 22; i++)
         fprintf(fblkout, "%c%s", sp, paa->aa3[i]); /* 3 letter AA code     */

      fprintf(fblkout, "\n");
      first_line = false;
   }
   fprintf(fblkout, "%.20s", title);

   for (i = 0; i < 22; i++)
      fprintf(fblkout, "%c%li", sp, nnaa[i]);

   fprintf(fblkout, "\n");
   return 0;
}
/******************  Base Silent output     *******************************/
/* Calculates and write the base composition at silent sites              */
/* normalised as a function of the possible usage at that silent site with*/
/* changing the amino acid composition of the protein. It is inspired by  */
/* GC3s but is much more complicated to calculate as not every AA has the */
/* option to use any base at the third position                           */
/* All synonymous AA can select between a G or C though                   */
/**************************************************************************/
int base_sil_us_out(FILE *foutput, long *nncod, long *nnaa, MENU_STRUCT *pm)
{
   GENETIC_CODE_STRUCT *pcu = pm->pcu; 
   int *ds = pm->ds;
   int *da = pm->da;

   int id, i, x, y, z;
   long bases_s[4]; /* synonymous GCAT bases               */

   long cb[4]; /* codons that could have been GCAT    */
   int done[4];
   char sp = pm->separator;

   for (x = 0; x < 4; x++)
   {
      cb[x] = 0;
      bases_s[x] = 0;
   } /* blank the arrays                    */

   for (x = 1; x < 5; x++)
      for (y = 1; y < 5; y++)
         for (z = 1; z < 5; z++)
         { /* look at all 64 codons               */
            id = (x - 1) * 16 + y + (z - 1) * 4;

            if (*(ds + id) == 1 || pcu->ca[id] == 11)
               continue;                 /* if no synon skip to next       codon */
            bases_s[z - 1] += nncod[id]; /* count No. codon ending in base X     */
         }

   for (i = 1; i < 22; i++)
   {
      for (x = 0; x < 4; x++) /* don't want to count bases in 6 fold  */
         done[x] = false;     /* sites twice do we so we remember     */

      if (i == 11 || *(da + i) == 1)
         continue; /* if stop codon skip, or AA not synony */

      for (x = 1; x < 5; x++) /* else add aa to could have ended count */
         for (y = 1; y < 5; y++)
            for (z = 1; z < 5; z++)
            {
               id = (x - 1) * 16 + y + (z - 1) * 4;
               /* assign codon values in range 1-64                           */
               if (pcu->ca[id] == i && done[z - 1] == false)
               {
                  /* encode AA i which we know to be synon so add could_be_x ending*/
                  /* by the Number of that amino acid                              */
                  cb[z - 1] += nnaa[i];
                  done[z - 1] = true; /* don't look for any more or we might   */
                                      /* process leu+arg+ser twice             */
               }
            }
   }

   /* Now the easy bit ... just output the results to file                */
   for (i = 0; i < 4; i++)
   {
      if (cb[i] > 0)
         fprintf(foutput, "%6.4f%c", (double)bases_s[i] / (double)cb[i], sp);
      else
         fprintf(foutput, "0.0000%c", sp);
   }

   return 0;
}
/******************  Clean up               *******************************/
/* Called after each sequence has been completely read from disk          */
/* It re-zeros all the main counters, but is not called when concatenating*/
/* sequences together                                                     */
/**************************************************************************/
int clean_up(long *nncod, long *nnaa)
{
   int x;
   int i;

   for (x = 0; x < 65; x++)
      nncod[x] = 0;
   for (x = 0; x < 23; x++)
      nnaa[x] = 0;
   /* dinucleotide count remembers the   */
   dinuc_count(" ", 1); /* last_base from the last fragment   */
                        /* this causes the last base to be "" */
   for (x = 0; x < 3; x++)
      for (i = 0; i < 16; i++)
         din[x][i] = 0;

   dinuc_count(" ", 1);
   valid_stops = codon_tot = fram = 0;
   return 0;
}
/*****************Codon Adaptation Index output   *************************/
/* Codon Adaptation Index (CAI) (Sharp and Li 1987). CAI is a measurement */
/* of the relative adaptiveness of the codon usage of a gene towards the  */
/* codon usage of highly expressed genes. The relative adaptiveness (w) of*/
/* each codon is the ratio of the usage of each codon, to that of the most*/
/* abundant codon for the same amino acid. The relative adaptiveness of   */
/* codons for albeit a limited choice of species, can be selected from the*/
/* Menu. The user can also input a personal choice of values. The CAI     */
/* index is defined as the geometric mean of these relative adaptiveness  */
/* values. Non-synonymous codons and termination codons (genetic code     */
/* dependent) are excluded. To aid computation, the CAI is calculated as  */
/* using a natural log summation, To prevent a codon having a relative    */
/* adaptiveness value of zero, which could result in a CAI of zero;       */
/* these codons have fitness of zero (<.0001) are adjusted to 0.01        */
/**************************************************************************/
int cai_out(FILE *foutput, long *nncod, MENU_STRUCT *pm)
{
   CAI_STRUCT *pcai = pm->pcai;
   GENETIC_CODE_STRUCT *pcu = pm->pcu; 
   int *ds = pm->ds;

   long totaa = 0;
   double sigma;
   float ftemp;
   int x;
   char sp = pm->separator;
   static char cai_ttt = false;
   static char description[61];
   static char reference[61];

   static CAI_STRUCT user_cai;

   if (!cai_ttt)
   {                              /* have we been called already   */
      user_cai.des = description; /* assign an array to a pointer  */
      user_cai.ref = reference;   /* as above                      */

      if (pm->caifile)
      {
         rewind(pm->caifile); /* unlikely unless fopfile = caifile   */
         x = 0;
         strcpy(user_cai.des, "User supplied CAI adaptation values ");
         strcpy(user_cai.ref, "No reference");
         user_cai.cai_val[x++] = 0.0F;

         while ((fscanf(pm->caifile, "%f ", &ftemp)) != EOF)
         {
            /* if any bad CAI values are read EXIT*/
            if (ftemp < 0 || ftemp > 1.0)
            {
               printf("\nError CAI %f value out of range\nEXITING", ftemp);
               my_exit(99, "cai_out");
            }
            user_cai.cai_val[x++] = ftemp; /* assign value */
         }                                 /* end of while */
         if (x != 65)
         { /*             wrong number of codons */
            fprintf(pm->my_err, "\nError in CAI file, found %i values"
                                " expected 64 values EXITING\n",
                    x - 1);
            my_exit(99, "cai_out");
         }
         pcai = &user_cai; /* assigns pointer to user CAI values */
      }                    /*        matches if( pm->caifile...  */

      fprintf(stderr, "Using %s (%s) w values to calculate "
                      "CAI \n",
              pcai->des, pcai->ref);
      cai_ttt = true; /*stops this "if" from being entered  */

   } /* matches if (!cai_ttt )             */

   for (x = 1, sigma = 0; x < 65; x++)
   {
      if (pcu->ca[x] == 11 || *(ds + x) == 1)
         continue;
      if (pcai->cai_val[x] < 0.0001)     /* if value is effectively zero       */
         pcai->cai_val[x] = 0.01F;       /* make it .01 */
      sigma += (double)*(nncod + x) * log((double)pcai->cai_val[x]);
      totaa += *(nncod + x);
   }

   if (totaa)
   { /* catch floating point overflow error*/
      sigma = sigma / (double)totaa;
      sigma = exp(sigma);
   }
   else
      sigma = 0;

   fprintf(foutput, "%5.3f%c", sigma, sp);
   return 0;
}
/*****************Codon Bias Index output        **************************/
/* Codon bias index is a measure of directional codon bias, it measures   */
/* the extent to which a gene uses a subset of optimal codons.            */
/* CBI = ( Nopt-Nran)/(Nopt-Nran) Where Nopt = number of optimal codons;  */
/* Ntot = number of synonymous codons; Nran = expected number of optimal  */
/* codons if codons were assigned randomly. CBI is similar to Fop as used */
/* by Ikemura, with Nran used as a scaling factor. In a gene with extreme */
/* codon bias, CBI will equal 1.0, in a gene with random codon usage CBI  */
/* will equal 0.0. Note that it is possible for Nopt to be less than Nran.*/
/* This results in a negative value for CBI.                              */
/* ( Bennetzen and Hall 1982 )                                            */
/**************************************************************************/
int cbi_out(FILE *foutput, long *nncod, long *nnaa, MENU_STRUCT *pm)
{
   AMINO_STRUCT *paa = pm->paa;
   GENETIC_CODE_STRUCT *pcu = pm->pcu; 
   FOP_STRUCT *pfop = pm->pfop;
   FOP_STRUCT *pcbi = pm->pcbi;
   CAI_STRUCT *pcai = pm->pcai;
   AMINO_PROP_STRUCT *pap = pm->pap;
   int *ds = pm->ds;
   int *da = pm->da;

   long tot_cod = 0;
   long opt = 0;
   float exp_cod = 0.0F;
   float fcbi;
   int c, x;
   char str[2];
   char sp = pm->separator;
   char message[MAX_MESSAGE_LEN];

   static char description[61];
   static char reference[61];
   static char first_call_cbi = true;
   static char has_opt_info[22];
   static FOP_STRUCT user_cbi;

   if (first_call_cbi)
   { /* have we been called already   */

      user_cbi.des = description; /* assign a pointer to array     */
      user_cbi.ref = reference;

      if (pm->cbifile)
      {
         rewind(pm->cbifile); /* fopfile can be the same as cbifile */
         strcpy(user_cbi.des, "User supplied choice");
         strcpy(user_cbi.ref, "No reference");
         x = 0;
         user_cbi.fop_cod[x++] = 0;

         while ((c = fgetc(pm->cbifile)) != EOF && x <= 66)
         {
            sprintf(str, "%c", c);
            if (isdigit(c) && atoi(str) >= 0 && atoi(str) <= 3)
            {
               user_cbi.fop_cod[x++] = (char)atoi(str);

            } /*                             isdigit */
         }    /*                        end of while */

         if (x != 65)
         { /*              wrong number of codons */
            sprintf(message, "\nError in CBI file %i digits found,  "
                                  "expected 64 EXITING\n",
                    x - 1);
            my_exit(99, message);
         }
         pcbi = (&user_cbi);
      } /*             matches if(pm->cbifile)  */

      printf("Using %s (%s) \noptimal codons to calculate "
             "CBI\n",
             pcbi->des, pcbi->ref);

      /* initilise has_opt_info             */
      for (x = 1; x < 22; x++)
         has_opt_info[x] = 0;

      for (x = 1; x < 65; x++)
      {
         if (pcu->ca[x] == 11 || *(ds + x) == 1)
            continue;
         if (pcbi->fop_cod[x] == 3)
            has_opt_info[pcu->ca[x]]++;
      }

      first_call_cbi = false; /*      this won't be called again      */
   }                          /*          matches if (first_call_cbi) */

   for (x = 1; x < 65; x++)
   {
      if (!has_opt_info[pcu->ca[x]])
         continue;
      switch ((int)pcbi->fop_cod[x])
      {
      case 3:
         opt += nncod[x];
         tot_cod += nncod[x];
         exp_cod += (float)nnaa[pcu->ca[x]] / (float)da[pcu->ca[x]];
         break;
      case 2:
      case 1:
         tot_cod += *(nncod + x);
         break;
      default:
         sprintf(message, " Serious error in CBI information found"
                           " an illegal CBI value of %c for codon %i"
                           " permissible values are \n 1 for non-optimal"
                           " codons\n 2 for common codons\n"
                           " 3 for optimal codons\n"
                           " EXITING ",
                 pcbi->fop_cod[x], x);

         my_exit(99, message);
         break;
      } /*                   end of switch     */
   }    /*                   for (    )        */

   if (tot_cod - exp_cod)
      fcbi = (opt - exp_cod) / (tot_cod - exp_cod);
   else
      fcbi = 0.0F;

   fprintf(foutput, "%5.3f%c", fcbi, sp); /* CBI     QED     */

   return 0;
}

/****************** Frequency of OPtimal codons output  ********************/
/* Frequency of Optimal codons (Fop) (Ikemura 1981). This index, is ratio  */
/* of optimal codons to synonymous codons (genetic code dependent). Optimal*/
/* codons for several species are in-built and can be selected using Menu 3*/
/* By default, the optimal codons of E. coli are assumed. The user may also*/
/* enter a personal choice of optimal codons. If rare synonymous codons    */
/* have been identified, there is a choice of calculating the original Fop */
/* index or a modified index. Fop values for the original index are always */
/* between 0 (where no optimal codons are used) and 1 (where only optimal  */
/* codons are used). When calculating the modified Fop index, any negative */
/* values are adjusted to zero.                                            */
/***************************************************************************/
int fop_out(FILE *foutput, long *nncod, MENU_STRUCT *pm)
{
   AMINO_STRUCT *paa = pm->paa;
   GENETIC_CODE_STRUCT *pcu = pm->pcu; 
   FOP_STRUCT *pfop = pm->pfop;
   FOP_STRUCT *pcbi = pm->pcbi;
   CAI_STRUCT *pcai = pm->pcai;
   AMINO_PROP_STRUCT *pap = pm->pap;
   int *ds = pm->ds;

   long nonopt = 0;
   long std = 0;
   long opt = 0;
   float ffop;
   int c, x;
   char nonopt_codons = false;
   
   char str[2];
   char message[MAX_MESSAGE_LEN];

   char sp = pm->separator;

   static char first_call = true;
   static char description[61];
   static char reference[61];
   static char asked_about_fop = false;
   static char factor_in_rare = false;
   static char has_opt_info[22];
   static FOP_STRUCT user_fop;

   if (first_call)
   { /* have I been called previously      */
      user_fop.des = description;
      user_fop.ref = reference;

      if (pm->fopfile)
      {
         rewind(pm->fopfile); /*    possible for fopfile = cbifile */
         strcpy(user_fop.des, "User supplied choice");
         strcpy(user_fop.ref, "No reference");
         x = 0;
         user_fop.fop_cod[x++] = 0;

         while ((c = fgetc(pm->fopfile)) != EOF && x <= 66)
         {
            sprintf(str, "%c", c);

            if (isdigit(c) && atoi(str) >= 0 && atoi(str) <= 3)
            {
               user_fop.fop_cod[x++] = (char)atoi(str);
            } /*                       test isdigit */
         }    /*                       end of while */

         if (x != 65)
         { /*             wrong number of codons */
            sprintf(message, "\nError in Fop file %i values found,  "
                                  "expected 64 EXITING\n",
                    x - 1);
            my_exit(99, message);
         }
         pfop = &user_fop; /*  assigns pointer to user fop values*/
      }

      printf("Using %s (%s)\noptimal codons to calculate Fop\n",
             pfop->des, pfop->ref);

      /* initilise has_opt_info             */
      for (x = 1; x < 22; x++)
         has_opt_info[x] = 0;

      for (x = 1; x < 65; x++)
      {
         if (pcu->ca[x] == 11 || *(ds + x) == 1)
            continue;
         if (pfop->fop_cod[x] == 3)
            has_opt_info[pcu->ca[x]]++;

         if (pfop->fop_cod[x] == 1)
         {
            // FIX: MODIFIED FOP OPTION
            // This should be a global option
            //  In the set of optimal codons you have selected,
            //  non-optimal codons have been identified\nThey can be 
            //  used in the calculation of a modified Fop,
            //  (Fop=(opt-rare)/total)\n else the original formulae
            //   will be used (Fop=opt/total)\n\n\t\tDo you wish
            /// calculate a modified fop (y/n) [n] ");
            //  Y: factor_in_rare = true;

            if (factor_in_rare == true)
               has_opt_info[pcu->ca[x]]++;
         }
      } /*    matches for (x=1           */
      first_call = false;
   } /*    matches if ( !first_call ) */

   for (x = 1; x < 65; x++)
   {
      if (!has_opt_info[pcu->ca[x]])
         continue;

      switch ((int)pfop->fop_cod[x])
      {
      case 3:
         opt += *(nncod + x);
         break;
      case 2:
         std += *(nncod + x);
         break;
      case 1:
         nonopt_codons = true;
         nonopt += *(nncod + x);
         break;
      default:
         sprintf(message,  " Serious error in fop information found"
                           " an illegal fop value of %c for codon %i"
                           " permissible values are \n 1 for non-optimal"
                           " codons\n 2 for common codons\n"
                           " 3 for optimal codons\n"
                           " EXITING ",
                 pfop->fop_cod[x], x);
         printf("opt %ld, std %ld, nonopt %ld\n", opt, std, nonopt);
         my_exit(99, message);
         break;
      }
   }
   /* only ask this once  ...            */

   if (factor_in_rare && (opt + nonopt + std))
      ffop = (float)(opt - nonopt) / (float)(opt + nonopt + std);
   else if ((opt + nonopt + std))
      ffop = (float)opt / (float)(opt + nonopt + std);
   else
      ffop = 0.0;

   fprintf(foutput, "%5.3f%c", ffop, sp);

   return 0;
}

/***************  Effective Number of Codons output   *********************/
/* The effective number of codons (NC) (Wright 1990). This index is a     */
/* simple measure of overall codon bias and is analogous to the effective */
/* number of alleles measure used in population genetics. Knowledge of the*/
/* optimal codons or a reference set of highly expressed genes is not     */
/* needed when calculating this index. Initially the homozygosity for each*/
/* amino acid is estimated from the squared codon frequencies.            */
/**************************************************************************/
float enc_out(FILE *foutput, long *nncod, long *nnaa, MENU_STRUCT *pm)
{
   GENETIC_CODE_STRUCT *pcu = pm->pcu; 
   int *da = pm->da;

   int numaa[9];
   int fold[9];
   int error_t = false;
   int i, z, x;
   double totb[9];
   double averb = 0, bb = 0, k2 = 0, s2 = 0;
   float enc_tot = 0.0F;
   char sp = pm->separator;

   /* don't assume that 6 is the largest possible amino acid family assume 9*/
   for (i = 0; i < 9; i++)
   {
      fold[i] = 0; /* initialise arrays to zero             */
      totb[i] = 0.0;
      numaa[i] = 0;
   }

   for (i = 1; i < 22; i++)
   { /* for each amino acid                  */
      if (i == 11)
         continue; /* but not for stop codons              */

      if (*(nnaa + i) <= 1) /* if this aa occurs once then skip     */
         bb = 0;
      else
      {
         for (x = 1, s2 = 0; x < 65; x++)
         {
            /* Try all codons but we are only looking for those that encode*/
            /* amino amid i, saves having to hard wire in any assumptions  */
            if (pcu->ca[x] != i)
               continue; /* skip is not i       */

            if (*(nncod + x) == 0) /* if codons not used then              */
               k2 = 0.0;           /* k2 = 0                               */
            else
               k2 = pow(((double)*(nncod + x) / (double)*(nnaa + i)),
                        (double)2);

            s2 += k2; /* sum of all k2's for aa i             */
         }
         bb = (((double)*(nnaa + i) * s2) - 1.0) / /* homozygosity        */
              (double)(*(nnaa + i) - 1.0);
      }

      if (bb > 0.0000001)
      {
         totb[*(da + i)] += bb; /* sum of all bb's for amino acids  */
                                /* which have z alternative codons  */
         numaa[*(da + i)]++;    /* where z = *(da+i)                */
      }
      /* numaa is no of aa that were z    */
      fold[*(da + i)]++; /* fold z=4 can have 9 in univ code */
   }                     /* but some aa may be absent from   */
                         /* gene therefore numaa[z] may be 0 */
   enc_tot = (float)fold[1];

   for (z = 2, averb = 0, error_t = false; z <= 8; z++)
   {
      /* look at all values of z if there  */
      if (fold[z])
      { /* are amino acids that are z fold   */
         if (numaa[z] && totb[z] > 0)
            averb = totb[z] / numaa[z];
         else if (z == 3 && numaa[2] && numaa[4] && fold[z] == 1)
            /* special case                      */
            averb = (totb[2] / numaa[2] + totb[4] / numaa[4]) * 0.5;
         else
         { /* write error to stderr             */
            codon_error(z, numaa[z], title, 3, pm);
            error_t = true; /* error catch for strange genes     */
            break;
         }
         enc_tot += (float)fold[z] / (float)averb;
         /* the calculation                   */
      }
   }

   if (error_t)
      fprintf(foutput, "*****%c", sp);
   else if (enc_tot <= 61)
      fprintf(foutput, "%5.2f%c", enc_tot, sp);
   else
      fprintf(foutput, "61.00%c", sp);

   return 0;
}

/*******************   G+C output          *******************************/
/* This function is a real work horse, initially it counts base composit */
/* ion in all frames, length of gene, num synonymous codons, number of   */
/* non synonymous codons. Then dependent on the value for which used in  */
/* switch statement. We return various analyses of this data             */
/* if which ==1 then the output is very detailed, base by base etc.      */
/* if which ==2 then the output is for GC content only                   */
/* if which ==3 then the output is for GC3s (GC at synonymous 3rd posit) */
/* if which ==4 then the output is for L_sym                             */
/* if which ==5 then the output is for L_aa                              */
/* The output from this subroutine is in a tabular format if human read- */
/* able output is selected, and in columns if machine readable. Also the */
/* number of values reported changes as it is assumed the user has access*/
/* to a spreadsheet type programme if they are requesting tabular output */
/*************************************************************************/
int gc_out(FILE *foutput, FILE *fblkout, int which, MENU_STRUCT *pm)
{
   AMINO_STRUCT *paa = pm->paa;
   GENETIC_CODE_STRUCT *pcu = pm->pcu; 
   FOP_STRUCT *pfop = pm->pfop;
   FOP_STRUCT *pcbi = pm->pcbi;
   CAI_STRUCT *pcai = pm->pcai;
   AMINO_PROP_STRUCT *pap = pm->pap;
   int *ds = pm->ds;

   long id;
   long bases[5]; /* base that are synonymous GCAT     */
   long base_tot[5];
   long base_1[5];
   long base_2[5];
   long base_3[5];
   long tot_s = 0;
   long totalaa = 0;
   static char header = false;
   int x, y, z;
   char sp = pm->separator;

   typedef double lf;

   for (x = 0; x < 5; x++)
   {
      bases[x] = 0; /* initialise array values to zero    */
      base_tot[x] = 0;
      base_1[x] = 0;
      base_2[x] = 0;
      base_3[x] = 0;
   }

   for (x = 1; x < 5; x++)
      for (y = 1; y < 5; y++)
         for (z = 1; z < 5; z++)
         { /* look at all 64 codons              */
            id = (x - 1) * 16 + y + (z - 1) * 4;

            if (pcu->ca[id] == 11)
               continue;             /* skip if a stop codon               */
            base_tot[x] += ncod[id]; /* we have a codon xyz therefore the  */
            base_1[x] += ncod[id];   /* frequency of each position for base*/
            base_tot[y] += ncod[id]; /* x,y,z are equal to the number of   */
            base_2[y] += ncod[id];   /* xyz codons .... easy               */
            base_tot[z] += ncod[id]; /* will be fooled a little if there   */
            base_3[z] += ncod[id];   /* non translatable codons, but these */
                                     /* are ignored when the avg is calc   */
            totalaa += ncod[id];

            if (*(ds + id) == 1)
               continue; /* if not synon  skip codon           */

            bases[z] += ncod[id]; /* count no of codons ending in Z     */

            tot_s += ncod[id]; /* count tot no of silent codons      */
         }

   if (!tot_s || !totalaa)
   {
      fprintf(pm->my_err, "Warning %.20s appear to be too short\n", title);
      fprintf(pm->my_err, "No output was written to file   \n");
      return 1;
   }
   switch ((int)which)
   {
   case 1: /* exhaustive output for analysis     */
      if (!header)
      { /* print a first line                 */
         fprintf(fblkout,
                  "Gene_description%cLen_aa%cLen_sym%cGC%cGC3s%cGCn3s%cGC1%cGC2"
                  "%cGC3%cT1%cT2%cT3%cC1%cC2%cC3%cA1%cA2%cA3%cG1%cG2%cG3\n",
                  sp, sp, sp, sp, sp, sp, sp, sp, sp, sp, sp, sp, sp, sp, sp, sp, sp, sp, sp, sp);
         header = true;
      }
      /* now print the information          */
      fprintf(fblkout, "%-.20s%c", title, sp);
      fprintf(fblkout,
               "%ld%c%ld%c%5.3f%c%5.3f%c%5.3f%c%5.3f%c%5.3f%c%5.3f%c"
               "%5.3f%c%5.3f%c%5.3f%c%5.3f%c%5.3f%c%5.3f%c%5.3f%c"
               "%5.3f%c%5.3f%c%5.3f%c%5.3f%c%5.3f\n",
               totalaa, sp,
               tot_s, sp,
               (lf)(base_tot[2] + base_tot[4]) / (lf)(totalaa * 3), sp,
               (lf)(bases[2] + bases[4]) / (lf)tot_s, sp,
               (lf)(base_tot[2] + base_tot[4] - bases[2] - bases[4]) / (lf)(totalaa * 3 - tot_s), sp,
               (lf)(base_1[2] + base_1[4]) / (lf)(totalaa), sp,
               (lf)(base_2[2] + base_2[4]) / (lf)(totalaa), sp,
               (lf)(base_3[2] + base_3[4]) / (lf)(totalaa), sp,
               (lf)base_1[1] / (lf)totalaa, sp,
               (lf)base_2[1] / (lf)totalaa, sp,
               (lf)base_3[1] / (lf)totalaa, sp,
               (lf)base_1[2] / (lf)totalaa, sp,
               (lf)base_2[2] / (lf)totalaa, sp,
               (lf)base_3[2] / (lf)totalaa, sp,
               (lf)base_1[3] / (lf)totalaa, sp,
               (lf)base_2[3] / (lf)totalaa, sp,
               (lf)base_3[3] / (lf)totalaa, sp,
               (lf)base_1[4] / (lf)totalaa, sp,
               (lf)base_2[4] / (lf)totalaa, sp,
               (lf)base_3[4] / (lf)totalaa);
      break;
   case 2: /* a bit more simple ... GC content   */
      fprintf(foutput, "%5.3f%c", (lf)((base_tot[2] + base_tot[4]) / (lf)(totalaa * 3)), sp);
      break;
   case 3: /* GC3s                               */
      fprintf(foutput, "%5.3f%c", (lf)(bases[2] + bases[4]) / (lf)tot_s, sp);
      break;
   case 4: /* Number of synonymous codons        */
      fprintf(foutput, "%3li%c", tot_s, sp);
      break;
   case 5: /* Total length in translatable AA    */
      fprintf(foutput, "%3li%c", totalaa, sp);
      break;
   }

   return 0;
}

/**********************   cutab_out     ***********************************/
/* Generates a formatted table of codon, RSCU and amino acid usage        */
/* ds points to an array[64] of synonymous values                         */
/* it reveals how many synonyms there are for each aa                     */
/**************************************************************************/
int cutab_out(FILE *fblkout, long *nncod, long *nnaa, MENU_STRUCT *pm)
{
   AMINO_STRUCT *paa = pm->paa;
   GENETIC_CODE_STRUCT *pcu = pm->pcu;
   int *ds = pm->ds;

   int last_row[4];
   int x;
   char sp = pm->separator;

   for (x = 0; x < 4; x++)
      last_row[x] = 0;

   codon_tot = codon_error(1, 1, "", (char)4, pm); /*  dummy*/

   for (x = 1; x < 65; x++)
   {
      if (last_row[x % 4] != pcu->ca[x])
         fprintf(fblkout, "%s%c%s%c", paa->aa3[pcu->ca[x]], sp, paa->cod[x], sp);
      else
         fprintf(fblkout, "%c%s%c", sp, paa->cod[x], sp);
      /* Sample of output *******************************************************/
      /*Phe UUU    0 0.00 Ser UCU    1 0.24 Tyr UAU    1 0.11 Cys UGU    1 0.67 */
      /*    UUC   22 2.00     UCC   10 2.40     UAC   17 1.89     UGC    2 1.33 */
      /*Leu UUA    0 0.00     UCA    1 0.24 TER UAA    0 0.00 TER UGA    1 3.00 */
      /*    UUG    1 0.12     UCG    6 1.44     UAG    0 0.00 Trp UGG    4 1.00 */
      /**************************************************************************/
      fprintf(fblkout, "%i%c%.2f%c",
               (int)nncod[x],
               sp, (nncod[x]) ? ((float)nncod[x] / (float)nnaa[pcu->ca[x]]) * (float)(*(ds + x)) : 0, sp);

      last_row[x % 4] = pcu->ca[x];

      if (!(x % 4))
         fprintf(fblkout, "\n");
      if (!(x % 16))
         fprintf(fblkout, "\n");
   }
   fprintf(fblkout, "%li codons in %16.16s (used %22.22s)\n\n",
           (long)codon_tot, title, pcu->des);
   return 0;
}
/********************  Dinuc_count    *************************************/
/* Count the frequency of all 16 dinucleotides in all three possible      */
/* reading frames. This one of the few functions that does not use the    */
/* codon and amino acid usage arrays ncod and naa to measure the parameter*/
/* rather they use the raw sequence data                                  */
/**************************************************************************/
int dinuc_count(char *seq, long ttot)
{
   char last_base;
   static char a = 0;
   int i;

   for (i = 0; i < ttot; i++)
   {
      last_base = a;
      switch (seq[i])
      {
      case 't':
      case 'T':
      case 'u':
      case 'U':
         a = 1;
         break;
      case 'c':
      case 'C':
         a = 2;
         break;
      case 'a':
      case 'A':
         a = 3;
         break;
      case 'g':
      case 'G':
         a = 4;
         break;
      default:
         a = 0;
         break;
      }
      if (!a || !last_base)
         continue; /* true if either of the base is not  */
                   /* a standard UTCG, or the current bas*/
                   /* is the start of the sequence       */
      din[fram][((last_base - 1) * 4 + a) - 1]++;
      if (++fram == 3)
         fram = 0; /* resets the frame to zero           */
   }
   return 0;
}
/***************** Dinuc_out           ************************************/
/* Outputs the frequency of dinucleotides, either in fout rows per seq    */
/* if the output is meant to be in a human readable form, each row repre- */
/* senting a reading frame. The fourth row is the total of the all the    */
/* reading frames. Machine readable format writes all the data into a     */
/* single row                                                             */
/**************************************************************************/
int dinuc_out(FILE *fblkout, char *ttitle, char sp)
{
   static char called = false;
   char bases[5] = {'T', 'C', 'A', 'G'};
   long dinuc_tot[4];
   int i, x, y;

   for (x = 0; x < 4; x++)
      dinuc_tot[x] = 0;

   for (x = 0; x < 3; x++)
      for (i = 0; i < 16; i++)
      {
         dinuc_tot[x] += din[x][i]; /* count dinuc usage in each frame   */
         dinuc_tot[3] += din[x][i]; /* and total dinuc usage,            */
      }

   if (!called)
   { /* write out the first row as a header*/
      called = true;

      fprintf(fblkout, "%s", "title");
      for (y = 0; y < 4; y++)
      {
         fprintf(fblkout, "%c%s", sp, "frame");
         for (x = 0; x < 4; x++)
            for (i = 0; i < 4; i++)
               fprintf(fblkout, "%c%c%c", sp, bases[x], bases[i]);
      }

      fprintf(fblkout, "\n");
   } /* matches if (!called)               */

   /*Sample output   truncated  **********************************************/
   /*title         frame TT    TC    TA    TG    CT    CC    CA    CG    AT  */
   /*MLSPCOPER.PE1__ 1:2 0.024 0.041 0.016 0.008 0.049 0.041 0.033 0.098 ... */
   /*MLSPCOPER.PE1__ 2:3 0.000 0.195 0.000 0.098 0.000 0.138 0.008 0.073 ... */
   /*MLSPCOPER.PE1__ 3:1 0.008 0.016 0.000 0.033 0.033 0.107 0.172 0.262 ... */
   /*MLSPCOPER.PE1__ all 0.011 0.084 0.005 0.046 0.027 0.095 0.071 0.144 ... */
   /*MLSPCOPER.PE2__ 1:2 0.026 0.026 0.009 0.009 0.053 0.035 0.053 0.061 ... */
   /**************************************************************************/
   for (x = 0; x < 4; x++)
   {
      if (x == 0)
         fprintf(fblkout, "%-.15s%c", ttitle, sp);

      switch (x)
      {
      case 0:
         fprintf(fblkout, "1:2%c", sp);
         break;
      case 1:
         fprintf(fblkout, "2:3%c", sp);
         break;
      case 2:
         fprintf(fblkout, "3:1%c", sp);
         break;
      case 3:
         fprintf(fblkout, "all%c", sp);
         break;
      }

      if (x == 3)
      {
         for (i = 0; i < 16; i++)
            if (dinuc_tot[x])
               fprintf(fblkout, "%5.3f%c",
                       (float)(din[0][i] + din[1][i] + din[2][i]) /
                           (float)dinuc_tot[x],
                       sp);
            else
               fprintf(fblkout, "%5.3f%c", 0.00, sp);
      }
      else
      {
         for (i = 0; i < 16; i++)
            if (dinuc_tot[x])
               fprintf(fblkout, "%5.3f%c",
                       (float)din[x][i] / (float)dinuc_tot[x], sp);
            else
               fprintf(fblkout, "%5.3f%c", 0.00, sp);
      }

      if (x == 3)
         fprintf(fblkout, "\n");
   }
   return 0;
}

/*********************  hydro_out        **********************************/
/* The general average hydropathicity or (GRAVY) score, for the hypothet- */
/* ical translated gene product. It is calculated as the arithmetic mean  */
/* of the sum of the hydropathic indices of each amino acid. This index   */
/* was used to quantify the major COA trends in the amino acid usage of   */
/* E. coli genes (Lobry, 1994).                                           */
/* Calculates and outputs total protein hydropathicity based on the Kyte  */
/* and Dolittle Index of hydropathicity (1982)                            */
/* nnaa               Array with frequency of amino acids                 */
/* paa                points to a struct containing Amino Acid values     */
/* pap->hydro         Pointer to hydropathicity values for each AA        */
/**************************************************************************/
int hydro_out(FILE *foutput, long *nnaa, MENU_STRUCT *pm)
{
   AMINO_PROP_STRUCT *pap = pm->pap;

   long a2_tot = 0;
   float hydro = 0.0F;
   int i;
   char sp = pm->separator;

   for (i = 1; i < 22; i++)
      if (i != 11)
         a2_tot += nnaa[i];

   if (!a2_tot)
   { /* whow   .. no amino acids what happened     */
      fprintf(pm->my_err, "Warning %.20s appear to be too short\n", title);
      fprintf(pm->my_err, "No output was written to file\n");
      return 1;
   }

   for (i = 1; i < 22; i++)
      if (i != 11)
         hydro += ((float)nnaa[i] / (float)a2_tot) * (float)pap->hydro[i];

   fprintf(foutput, "%8.6f%c", hydro, sp);

   return 0;
}
/**************** Aromo_out ***********************************************/
/* Aromaticity score of protein. This is the frequency of aromatic amino  */
/* acids (Phe, Tyr, Trp) in the hypothetical translated gene product      */
/* nnaa               Array with frequency of amino acids                 */
/* paa                points to a struct containing Amino Acid values     */
/* pap->aromo         Pointer to aromaticity values for each AA           */
/**************************************************************************/
int aromo_out(FILE *foutput, long *nnaa, MENU_STRUCT *pm)
{
   AMINO_PROP_STRUCT *pap = pm->pap;

   long a1_tot = 0;
   float aromo = 0.0F;
   int i;
   char sp = pm->separator;

   for (i = 1; i < 22; i++)
      if (i != 11)
         a1_tot += nnaa[i];

   if (!a1_tot)
   {
      fprintf(pm->my_err, "Warning %.20s appear to be too short\n", title);
      fprintf(pm->my_err, "No output was written to file\n");
      return 1;
   }
   for (i = 1; i < 22; i++)
      if (i != 11)
         aromo += ((float)nnaa[i] / (float)a1_tot) * (float)pap->aromo[i];

   fprintf(foutput, "%8.6f%c", aromo, sp);
   return 0;
}
