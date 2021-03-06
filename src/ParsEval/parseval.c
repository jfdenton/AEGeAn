#include <string.h>
#include "AgnLocusIndex.h"
#include "AgnGeneLocus.h"
#include "AgnUtils.h"
#include "PeProcedure.h"
#include "PeReports.h"

// Main method
int main(int argc, char * const argv[])
{
  // Initialize ParsEval
  gt_lib_init();
  char *start_time_str = pe_get_start_time();
  GtTimer *timer = gt_timer_new();
  gt_timer_start(timer);
  fputs("[ParsEval] Begin ParsEval\n", stderr);

  PeOptions options;
  pe_set_option_defaults(&options);
  pe_parse_options(argc, argv, &options);
  if(options.refrfile == NULL || options.predfile == NULL)
  {
    fprintf(stderr, "[ParsEval] error: could not parse input filenames\n");
    return EXIT_FAILURE;
  }

  // Load data into memory
  AgnLogger *logger = agn_logger_new();
  AgnLocusIndex *locusindex;
  GtArray *loci;
  GtStrArray *seqids;
  GtUword totalloci = pe_load_and_parse_loci(&locusindex, &loci, &seqids,
                                             &options, logger);
  bool haderror = agn_logger_print_all(logger, stderr, NULL);
  if(haderror) return EXIT_FAILURE;

  // Main comparison procedure
  if(totalloci == 0)
  {
    fprintf(stderr, "[ParsEval] Warning: found no loci to analyze\n");
    fclose(options.outfile);
  }
  else
  {
    GtHashmap *       comp_evals;
    GtHashmap *       locus_summaries;
    AgnCompEvaluation overall_eval;
    GtArray *         seqlevel_evals;

    GtArray *seqfiles = pe_prep_output(seqids, &options);
    pe_comparative_analysis(locusindex, &comp_evals, &locus_summaries, seqids,
                            seqfiles,loci, &options);
    pe_aggregate_results(&overall_eval, &seqlevel_evals, loci, seqfiles,
                         comp_evals, locus_summaries, &options);
    pe_print_summary(start_time_str, argc, argv, seqids, &overall_eval,
                     seqlevel_evals, options.outfile, &options);
    pe_print_combine_output(seqids, seqfiles, &options);

    gt_array_delete(seqfiles);
    gt_hashmap_delete(comp_evals);
    gt_hashmap_delete(locus_summaries);
    gt_array_delete(seqlevel_evals);
  }

  // All done!
  gt_timer_stop(timer);
  gt_timer_show_formatted(timer, "[ParsEval] ParsEval complete! (total runtime:"
                          " %ld.%06ld seconds)\n\n", stderr );

  // Free up memory
  gt_array_delete(loci);
  gt_free(start_time_str);
  agn_logger_delete(logger);
  agn_locus_index_delete(locusindex);
  gt_timer_delete(timer);
  if(gt_lib_clean() != 0)
  {
    fputs("error: issue cleaning GenomeTools library\n", stderr);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
