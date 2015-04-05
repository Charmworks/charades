#include "phold_mpi.h"

int
main(int argc, char **argv, char **env)
{
        int              i;

        // get rid of error if compiled w/ MEMORY queues
        g_tw_memory_nqueues=1;
        // set a min lookahead of 1.0
        lookahead = 1.0;
        tw_opt_add(app_opt);
        tw_opt_add(mpi_opt);
        tw_init(&argc, &argv);

        if( lookahead > 1.0 )
          tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");

        //reset mean based on lookahead
        mean = mean - lookahead;

        g_tw_memory_nqueues = 16; // give at least 16 memory queue event

        offset_lpid = g_tw_mynode * nlp_per_pe;
        g_total_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
        g_tw_events_per_pe = (mult * nlp_per_pe * start_events) +
                                optimistic_memory;
        //g_tw_rng_default = TW_FALSE;
        g_tw_lookahead = lookahead;

        printf("Total LPs: %d\n", g_total_lps);
        printf("Num PEs: %d\n", g_tw_npe);
        printf("LPs per PE: %d\n", nlp_per_pe);

        tw_define_lps(nlp_per_pe, sizeof(phold_message), 0);

        for(i = 0; i < g_tw_nlp; i++)
                tw_lp_settype(i, &mylps[0]);

        if( g_tw_mynode == 0 )
          {
            printf("========================================\n");
            printf("PHOLD Model Configuration..............\n");
            printf("   Lookahead..............%lf\n", lookahead);
            //printf("   Start-events...........%u\n", g_phold_start_events);
            printf("   stagger................%u\n", stagger);
            printf("   Mean...................%lf\n", mean);
            printf("   Mult...................%lf\n", mult);
            printf("   Memory.................%u\n", optimistic_memory);
            printf("   Remote.................%lf\n", percent_remote);
            printf("========================================\n\n");
          }

        tw_run();
        tw_end();

        return 0;
}
