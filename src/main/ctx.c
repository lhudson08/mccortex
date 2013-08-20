#include "global.h"
#include <time.h>
#include "cmd.h"
#include "util.h"

static const char usage[] =
"\n"
"usage: "CMD" <command> [options] <args>\n"
"version: "CTXVERSIONSTR"\n"
"\n"
"Command:  build       FASTA/FASTQ/BAM -> binary graph file\n"
"          view        view and check a cortex graph file (.ctx)\n"
// "          clean       clean population / sample against merged  (unfinished)\n" // 4
"          join        combine graphs, filter graph intersections\n"
"          subgraph    filter a subgraph\n"
"          reads       filter reads against a graph\n"
"          extend      extend contigs using a graph\n"
// "          contigs     pull out contigs for a sample             (unfinished)\n" // 2
"          thread      thread reads through cleaned population\n"
"          pview       view read threading information\n"
"          pmerge      merge path files (.ctp)\n"
"          call        call variants\n"
// "          diverge     path divergence caller                    (unfinished)\n" // 3
"          unique      remove duplicated bubbles, produce VCF\n"
// "          covg        add covg to a VCF file                    (unfinished)\n" // 1
"          place       place variants and genotype\n"
"\n"
"  Type a command with no arguments to see help.\n"
"\n"
"Common Options:\n"
"  -m <M>       Memory e.g. 1GB [default: 1GB]\n"
"  -h <H>       Hash entries [default: 4M (~4 million)]\n"
"  -g <G>       Species genome size [default: 3.1Gbp]\n"
"  -t <T>       Number of threads [default: 2]\n"
"  -k <K>       Kmer size [default: read from binaries]\n"
"  -f <file>    Input file\n"
"  -p <in.ctp>  Assembly file\n"
"\n";

int main(int argc, char **argv)
{
  CmdArgs args;
  time_t start,end;

  time(&start);
  if(argc == 1) print_usage(usage, NULL);
  cmd_alloc(&args, argc, argv);
  if(args.cmdidx == -1) print_usage(usage, "Unrecognised command: %s", argv[1]);
  if(strcmp(argv[1],"view") != 0) message("[cmd] %s\n", args.cmdline);
  int ret = ctx_funcs[args.cmdidx](&args);
  cmd_free(&args);
  time(&end);

  if(strcmp(argv[1],"view") != 0)
  {
    // Print time taken
    double diff = difftime (end,start);
    if(diff < 60) message("[time] %.2lf seconds\n", diff);
    else {
      char timestr[100];
      seconds_to_str(diff, timestr);
      message("[time] %.2lf seconds (%s)\n", diff, timestr);
    }
  }

  return ret;
}