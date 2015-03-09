#include <stdio.h>
#include "system.hpp"

/*
<testinfo>
   test_generator="gens/mixed-generator -a --no-warmup-threads|--warmup-threads"
   test_generator_ENV=( "NX_TEST_MAX_CPUS=1"
                        "NX_TEST_SCHEDULE=bf"
                        "test_architecture=smp")
   test_ignore_fail=1
</testinfo>
*/

#define ITERS 1000

int main ( int argc, char *argv[])
{
   int i, error = 0;
   unsigned nths = 0;
   

   for ( i=0; i<ITERS; i++ ) {

      nths = ((nths) % 4) + 1;

      sys.updateActiveWorkers( nths );

      fprintf(stdout,"[%d/%d] Team final size is %d and %d is expected\n", i, ITERS, (int) myThread->getTeam()->getFinalSize(), nths );

      if ( myThread->getTeam()->getFinalSize() != nths ) error++;
   }

   fprintf(stdout,"Result is %s\n", error? "UNSUCCESSFUL":"successful");

   return error;
}
