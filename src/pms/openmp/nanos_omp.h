/*************************************************************************************/
/*      Copyright 2015 Barcelona Supercomputing Center                               */
/*                                                                                   */
/*      This file is part of the NANOS++ library.                                    */
/*                                                                                   */
/*      NANOS++ is free software: you can redistribute it and/or modify              */
/*      it under the terms of the GNU Lesser General Public License as published by  */
/*      the Free Software Foundation, either version 3 of the License, or            */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      NANOS++ is distributed in the hope that it will be useful,                   */
/*      but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*      GNU Lesser General Public License for more details.                          */
/*                                                                                   */
/*      You should have received a copy of the GNU Lesser General Public License     */
/*      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.             */
/*************************************************************************************/

#ifndef _NANOS_OMP_H_
#define _NANOS_OMP_H_

#include "nanos.h"
#include "nanos_reduction.h"
#include "nanos_atomic.h"

typedef void * nanos_cpu_set_t;

typedef enum nanos_omp_sched_t {
   nanos_omp_sched_static = 1,
   nanos_omp_sched_dynamic = 2,
   nanos_omp_sched_guided = 3,
   nanos_omp_sched_auto = 4
} nanos_omp_sched_t;

#ifdef __cplusplus
extern "C" {
#endif

#define NANOS_OMP_WS_TSIZE 5

NANOS_API_DECL(nanos_err_t, nanos_omp_single, ( bool *));
NANOS_API_DECL(nanos_err_t, nanos_omp_barrier, ( void ));

NANOS_API_DECL(nanos_err_t, nanos_omp_set_implicit, ( nanos_wd_t uwd ));

/* API calls that are generated by the compiler */
NANOS_API_DECL(int, nanos_omp_get_max_threads, ( void ));
NANOS_API_DECL(int, nanos_omp_get_num_threads, ( void ));
NANOS_API_DECL(int, nanos_omp_get_thread_num, ( void ));
NANOS_API_DECL(int, nanos_omp_set_num_threads, ( int nthreads ));

NANOS_API_DECL(nanos_ws_t, nanos_omp_find_worksharing, ( nanos_omp_sched_t kind ));
NANOS_API_DECL(nanos_err_t, nanos_omp_get_schedule, ( nanos_omp_sched_t *kind, int *modifier ));

NANOS_API_DECL(int, nanos_omp_get_num_threads_next_parallel, ( int threads_requested ));

NANOS_API_DECL(void, nanos_omp_get_process_mask,( nanos_cpu_set_t cpu_set ));
NANOS_API_DECL(int, nanos_omp_set_process_mask,( const nanos_cpu_set_t cpu_set ));
NANOS_API_DECL(void, nanos_omp_add_process_mask,( const nanos_cpu_set_t cpu_set ));

NANOS_API_DECL(void, nanos_omp_get_active_mask,( nanos_cpu_set_t cpu_set ));
NANOS_API_DECL(int, nanos_omp_set_active_mask,( const nanos_cpu_set_t cpu_set ));
NANOS_API_DECL(void, nanos_omp_add_active_mask,( const nanos_cpu_set_t cpu_set ));

NANOS_API_DECL(int, nanos_omp_get_max_processors,( void ));

#ifdef __cplusplus
}
#endif

#endif
