/*
 *    This file is part of acados.
 *
 *    acados is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    acados is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with acados; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

// external
#include <stdio.h>
#include <stdlib.h>

// acados
#include <acados/utils/print.h>
#include <acados/ocp_qp/ocp_qp_sparse_solver.h>
#include <acados/ocp_qp/ocp_qp_full_condensing_solver.h>
#include <acados/ocp_qp/ocp_qp_hpipm.h>
#ifdef ACADOS_WITH_HPMPC
#include <acados/ocp_qp/ocp_qp_hpmpc.h>
#endif
#ifdef ACADOS_WITH_QPDUNES
#include <acados/ocp_qp/ocp_qp_qpdunes.h>
#endif
#include <acados/dense_qp/dense_qp_hpipm.h>
#include <acados/dense_qp/dense_qp_qpoases.h>
#ifdef ACADOS_WITH_QORE
#include <acados/dense_qp/dense_qp_qore.h>
#endif

// c interface
#ifdef ACADOS_WITH_C_INTERFACE
#include <acados_c/ocp_qp.h>
#include <acados_c/options.h>
#include <acados_c/legacy_create.h>
#endif

// mass spring
ocp_qp_in *create_ocp_qp_in_mass_spring();



#ifndef ACADOS_WITH_QPDUNES
#define ELIMINATE_X0
#endif
#define GENERAL_CONSTRAINT_AT_TERMINAL_STAGE



#define NREP 100



//#include "./mass_spring.c"



int main() {
    printf("\n");
    printf("\n");
    printf("\n");
    printf(" mass spring example: acados ocp_qp solvers\n");
    printf("\n");
    printf("\n");
    printf("\n");

    /************************************************
     * ocp qp
     ************************************************/

    ocp_qp_in *qp_in = create_ocp_qp_in_mass_spring();

    ocp_qp_dims *qp_dims = qp_in->dim;

    /************************************************
     * ocp qp solution
     ************************************************/

#ifdef ACADOS_WITH_C_INTERFACE
    ocp_qp_out *qp_out = create_ocp_qp_out(qp_dims);
#else // ! ACADOS_WITH_C_INTERFACE 
	int qp_out_size = ocp_qp_out_calculate_size(qp_dims);
	void *qp_out_mem = malloc(qp_out_size);
	ocp_qp_out *qp_out = assign_ocp_qp_out(qp_dims, qp_out_mem);
#endif

    /************************************************
     * ocp qp solvers
     ************************************************/

    // choose values for N2 in partial condensing solvers
    int num_N2_values = 3;
    int N2_values[3] = {15, 5, 3};

    int ii_max = 6;
    #ifndef ACADOS_WITH_HPMPC
    ii_max--;
    #endif
    #ifndef ACADOS_WITH_QPDUNES
    ii_max--;
    #endif
    #ifndef ACADOS_WITH_QORE
    ii_max--;
    #endif



#ifdef ACADOS_WITH_C_INTERFACE



    // choose ocp qp solvers
    ocp_qp_solver_t ocp_qp_solvers[] =
    {
		PARTIAL_CONDENSING_HPIPM,
        #if ACADOS_WITH_HPMPC
        PARTIAL_CONDENSING_HPMPC,
        #endif
        #if ACADOS_WITH_QPDUNES
        PARTIAL_CONDENSING_QPDUNES,
        #endif
        FULL_CONDENSING_HPIPM,
        #ifdef ACADOS_WITH_QORE
        FULL_CONDENSING_QORE,
        #endif
        FULL_CONDENSING_QPOASES,
    };

    for (int ii = 0; ii < ii_max; ii++)
    {
        ocp_qp_solver_plan plan;
        plan.qp_solver = ocp_qp_solvers[ii];

        void *args = ocp_qp_create_args(&plan, qp_dims);

        for (int jj = 0; jj < num_N2_values; jj++)
        {
            int N2 = N2_values[jj];

            // NOTE(nielsvd): needs to be implemented using the acados_c/options.h interface
            switch (plan.qp_solver)
            {
                case PARTIAL_CONDENSING_HPIPM:
                    printf("\nPartial condensing + HPIPM (N2 = %d):\n\n", N2);
                    ((ocp_qp_partial_condensing_args *)((ocp_qp_sparse_solver_args *)args)->pcond_args)->N2 = N2;
                    ((ocp_qp_hpipm_args *)((ocp_qp_sparse_solver_args *)args)->solver_args)->hpipm_args->iter_max = 30;
                    break;
                case PARTIAL_CONDENSING_HPMPC:
#ifdef ACADOS_WITH_HPMPC
                    printf("\nPartial condensing + HPMPC (N2 = %d):\n\n", N2);
                    ((ocp_qp_partial_condensing_args *)((ocp_qp_sparse_solver_args *)args)->pcond_args)->N2 = N2;
                    ((ocp_qp_hpmpc_args *)((ocp_qp_sparse_solver_args *)args)->solver_args)->max_iter = 30;
#endif
                    break;
                case PARTIAL_CONDENSING_QPDUNES:
#ifdef ACADOS_WITH_QPDUNES
                    printf("\nPartial condensing + qpDUNES (N2 = %d):\n\n", N2);
                    #ifdef ELIMINATE_X0
                    assert(1==0 && "qpDUNES does not support ELIMINATE_X0 flag!");
                    #endif
                    ocp_qp_sparse_solver_args *solver_args = (ocp_qp_sparse_solver_args *)args;
                    ocp_qp_qpdunes_args *qpdunes_args = (ocp_qp_qpdunes_args *)solver_args->solver_args;
                    #ifdef GENERAL_CONSTRAINT_AT_TERMINAL_STAGE
                    qpdunes_args->stageQpSolver = QPDUNES_WITH_QPOASES;
                    #endif
                    qpdunes_args->warmstart = 0;
                    ((ocp_qp_partial_condensing_args *)((ocp_qp_sparse_solver_args *)args)->pcond_args)->N2 = N2;
#endif
                    break;
                case FULL_CONDENSING_HPIPM:
                    printf("\nFull condensing + HPIPM:\n\n");
                    // default options
                    break;
                case FULL_CONDENSING_QORE:
#ifdef ACADOS_WITH_QORE
                    printf("\nFull condensing + QORE:\n\n");
                    // default options
                    break;
#endif
                case FULL_CONDENSING_QPOASES:
                    printf("\nFull condensing + QPOASES:\n\n");
                    // default options
                    break;
                case PARTIAL_CONDENSING_OOQP:
                    break;
            }

            ocp_qp_solver *qp_solver = ocp_qp_create(&plan, qp_dims, args);

            int acados_return = 0;

            ocp_qp_info *info = (ocp_qp_info *)qp_out->misc;
            ocp_qp_info min_info;

            // run QP solver NREP times and record min timings
            for (int rep = 0; rep < NREP; rep++)
            {
                acados_return += ocp_qp_solve(qp_solver, qp_in, qp_out);

                if (rep == 0)
                {
                    min_info.num_iter = info->num_iter;
                    min_info.total_time = info->total_time;
                    min_info.condensing_time = info->condensing_time;
                    min_info.solve_QP_time = info->solve_QP_time;
                    min_info.interface_time = info->interface_time;
                }
                else
                {
                    assert(min_info.num_iter == info->num_iter && "QP solver not cold started!");

                    if (info->total_time < min_info.total_time)
                        min_info.total_time = info->total_time;
                    if (info->condensing_time < min_info.condensing_time)
                        min_info.condensing_time = info->condensing_time;
                    if (info->solve_QP_time < min_info.solve_QP_time)
                        min_info.solve_QP_time = info->solve_QP_time;
                    if (info->interface_time < min_info.interface_time)
                        min_info.interface_time = info->interface_time;
                }
            }

            /************************************************
             * compute residuals
             ************************************************/

            ocp_qp_res *qp_res = create_ocp_qp_res(qp_dims);
            ocp_qp_res_ws *res_ws = create_ocp_qp_res_ws(qp_dims);
            compute_ocp_qp_res(qp_in, qp_out, qp_res, res_ws);

            /************************************************
             * print solution
             ************************************************/

//			 print_ocp_qp_out(qp_out);

            /************************************************
             * print residuals
             ************************************************/

//			 print_ocp_qp_res(qp_res);

            /************************************************
             * compute infinity norm of residuals
             ************************************************/

            double res[4];
            compute_ocp_qp_res_nrm_inf(qp_res, res);
            double max_res = 0.0;
            for (int ii = 0; ii < 4; ii++)
                max_res = (res[ii] > max_res) ? res[ii] : max_res;

            /************************************************
             * print stats
             ************************************************/

            printf("\ninf norm res: %e, %e, %e, %e\n", res[0], res[1], res[2], res[3]);

            print_ocp_qp_info(&min_info);

            /************************************************
             * free memory
             ************************************************/

            free(qp_solver);

            if (plan.qp_solver >= FULL_CONDENSING_HPIPM) break;
        }
        free(args);
    }



#else // ! ACADOS_WITH_C_INTERFACE 



	ii_max = 4; // HPIPM

    for (int ii = 0; ii < ii_max; ii++)
    {

        for (int jj = 0; jj < num_N2_values; jj++)
        {
            int N2 = N2_values[jj];

			ocp_qp_solver_config solver_config;
			ocp_qp_xcond_solver_config xcond_solver_config;

            ocp_qp_info *info = (ocp_qp_info *)qp_out->misc;
            ocp_qp_info min_info;

			int solver_opts_size;
			void *solver_opts_mem;
			void *solver_opts;
			ocp_qp_sparse_solver_opts *sparse_solver_opts;
			ocp_qp_hpipm_opts *hpipm_opts;
			int solver_mem_size;
			void *solver_mem_mem;
			void *solver_mem;
			int solver_work_size;
			void *solver_work;
			dense_qp_hpipm_opts *dense_hpipm_opts;

			switch(ii)
			{
				case 0: // HPIPM
                    printf("\nHPIPM\n\n");

					// config
					ocp_qp_hpipm_config_initialize_default(&solver_config);

					// opts
					solver_opts_size = solver_config.opts_calculate_size(&solver_config, qp_dims);
//					printf("\nopts size = %d\n", solver_opts_size);
					solver_opts_mem = malloc(solver_opts_size);
					solver_opts = solver_config.opts_assign(&solver_config, qp_dims, solver_opts_mem);
					solver_config.opts_initialize_default(&solver_config, solver_opts);
					hpipm_opts = solver_opts;
					hpipm_opts->hpipm_opts->iter_max = 30;

					// memory
					solver_mem_size = solver_config.memory_calculate_size(&solver_config, qp_dims, solver_opts);
//					printf("\nmem size = %d\n", solver_mem_size);
					solver_mem_mem = malloc(solver_mem_size);
					solver_mem = solver_config.memory_assign(&solver_config, qp_dims, solver_opts, solver_mem_mem);

					// workspace
					solver_work_size = solver_config.workspace_calculate_size(&solver_config, qp_dims, solver_opts);
//					printf("\nwork size = %d\n", solver_work_size);
					solver_work = malloc(solver_work_size);

					// solve
					solver_config.evaluate(&solver_config, qp_in, qp_out, solver_opts, solver_mem, solver_work);

					// info
                    min_info.num_iter = info->num_iter;
                    min_info.total_time = info->total_time;
                    min_info.condensing_time = info->condensing_time;
                    min_info.solve_QP_time = info->solve_QP_time;
                    min_info.interface_time = info->interface_time;

					break;

				case 1: // PARTIAL_CONDENSING_HPIPM
                    printf("\nPARTIAL_CONDENSING_HPIPM, N2 = %d\n\n", N2);

					// config
					ocp_qp_hpipm_config_initialize_default(&solver_config);
					ocp_qp_sparse_solver_config_initialize_default(&xcond_solver_config);
					xcond_solver_config.qp_solver = &solver_config;
					xcond_solver_config.N2 = N2;

					// opts
					solver_opts_size = xcond_solver_config.opts_calculate_size(&xcond_solver_config, qp_dims);
//					printf("\nopts size = %d\n", solver_opts_size);
					solver_opts_mem = malloc(solver_opts_size);
					solver_opts = xcond_solver_config.opts_assign(&xcond_solver_config, qp_dims, solver_opts_mem);
					xcond_solver_config.opts_initialize_default(&xcond_solver_config, solver_opts);
					sparse_solver_opts = solver_opts;
					hpipm_opts = sparse_solver_opts->qp_solver_opts;
					hpipm_opts->hpipm_opts->iter_max = 30;

					// memory
					solver_mem_size = xcond_solver_config.memory_calculate_size(&xcond_solver_config, qp_dims, solver_opts);
//					printf("\nmem size = %d\n", solver_mem_size);
					solver_mem_mem = malloc(solver_mem_size);
					solver_mem = xcond_solver_config.memory_assign(&xcond_solver_config, qp_dims, solver_opts, solver_mem_mem);

					// workspace
					solver_work_size = xcond_solver_config.workspace_calculate_size(&xcond_solver_config, qp_dims, solver_opts);
//					printf("\nwork size = %d\n", solver_work_size);
					solver_work = malloc(solver_work_size);

					// solve
					xcond_solver_config.evaluate(&xcond_solver_config, qp_in, qp_out, solver_opts, solver_mem, solver_work);

					// info
                    min_info.num_iter = info->num_iter;
                    min_info.total_time = info->total_time;
                    min_info.condensing_time = info->condensing_time;
                    min_info.solve_QP_time = info->solve_QP_time;
                    min_info.interface_time = info->interface_time;

					break;

				case 2: // FULL_CONDENSING_HPIPM
                    printf("\nFULL_CONDENSING_HPIPM\n\n");

					// config
					dense_qp_hpipm_config_initialize_default(&solver_config);
					ocp_qp_full_condensing_solver_config_initialize_default(&xcond_solver_config);
					xcond_solver_config.qp_solver = &solver_config;

					// opts
					solver_opts_size = xcond_solver_config.opts_calculate_size(&xcond_solver_config, qp_dims);
//					printf("\nopts size = %d\n", solver_opts_size);
					solver_opts_mem = malloc(solver_opts_size);
					solver_opts = xcond_solver_config.opts_assign(&xcond_solver_config, qp_dims, solver_opts_mem);
					xcond_solver_config.opts_initialize_default(&xcond_solver_config, solver_opts);
					sparse_solver_opts = solver_opts;
					dense_hpipm_opts = sparse_solver_opts->qp_solver_opts;
					dense_hpipm_opts->hpipm_opts->iter_max = 30;

					// memory
					solver_mem_size = xcond_solver_config.memory_calculate_size(&xcond_solver_config, qp_dims, solver_opts);
//					printf("\nmem size = %d\n", solver_mem_size);
					solver_mem_mem = malloc(solver_mem_size);
					solver_mem = xcond_solver_config.memory_assign(&xcond_solver_config, qp_dims, solver_opts, solver_mem_mem);

					// workspace
					solver_work_size = xcond_solver_config.workspace_calculate_size(&xcond_solver_config, qp_dims, solver_opts);
//					printf("\nwork size = %d\n", solver_work_size);
					solver_work = malloc(solver_work_size);

					// solve
					xcond_solver_config.evaluate(&xcond_solver_config, qp_in, qp_out, solver_opts, solver_mem, solver_work);

					// info
                    min_info.num_iter = info->num_iter;
                    min_info.total_time = info->total_time;
                    min_info.condensing_time = info->condensing_time;
                    min_info.solve_QP_time = info->solve_QP_time;
                    min_info.interface_time = info->interface_time;

					break;

				case 3: // FULL_CONDENSING_QPOASES
                    printf("\nFULL_CONDENSING_QPOASES\n\n");

					// config
					dense_qp_qpoases_config_initialize_default(&solver_config);
					ocp_qp_full_condensing_solver_config_initialize_default(&xcond_solver_config);
					xcond_solver_config.qp_solver = &solver_config;

					// opts
					solver_opts_size = xcond_solver_config.opts_calculate_size(&xcond_solver_config, qp_dims);
//					printf("\nopts size = %d\n", solver_opts_size);
					solver_opts_mem = malloc(solver_opts_size);
					solver_opts = xcond_solver_config.opts_assign(&xcond_solver_config, qp_dims, solver_opts_mem);
					xcond_solver_config.opts_initialize_default(&xcond_solver_config, solver_opts);
//					sparse_solver_opts = solver_opts;
//					dense_hpipm_opts = sparse_solver_opts->qp_solver_opts;
//					dense_hpipm_opts->hpipm_opts->iter_max = 30;

					// memory
					solver_mem_size = xcond_solver_config.memory_calculate_size(&xcond_solver_config, qp_dims, solver_opts);
//					printf("\nmem size = %d\n", solver_mem_size);
					solver_mem_mem = malloc(solver_mem_size);
					solver_mem = xcond_solver_config.memory_assign(&xcond_solver_config, qp_dims, solver_opts, solver_mem_mem);

					// workspace
					solver_work_size = xcond_solver_config.workspace_calculate_size(&xcond_solver_config, qp_dims, solver_opts);
//					printf("\nwork size = %d\n", solver_work_size);
					solver_work = malloc(solver_work_size);

					// solve
					xcond_solver_config.evaluate(&xcond_solver_config, qp_in, qp_out, solver_opts, solver_mem, solver_work);

					// info
                    min_info.num_iter = info->num_iter;
                    min_info.total_time = info->total_time;
                    min_info.condensing_time = info->condensing_time;
                    min_info.solve_QP_time = info->solve_QP_time;
                    min_info.interface_time = info->interface_time;

					break;

				default:
					printf("\nWrong solver name!!!\n\n");
			}

            /************************************************
            * print solution
            ************************************************/

//			 print_ocp_qp_out(qp_out);

            /************************************************
            * compute residuals
            ************************************************/

			int res_size = ocp_qp_res_calculate_size(qp_dims);
//			printf("\nres size = %d\n", res_size);
			void *res_mem = malloc(res_size);
			ocp_qp_res *qp_res = assign_ocp_qp_res(qp_dims, res_mem);

			int res_work_size = ocp_qp_res_ws_calculate_size(qp_dims);
//			printf("\nres work size = %d\n", res_work_size);
			void *res_work_mem = malloc(res_work_size);
			ocp_qp_res_ws *res_ws = assign_ocp_qp_res_ws(qp_dims, res_work_mem);

            compute_ocp_qp_res(qp_in, qp_out, qp_res, res_ws);

            /************************************************
             * print residuals
             ************************************************/

//			 print_ocp_qp_res(qp_res);

            /************************************************
            * compute infinity norm of residuals
            ************************************************/

            double res[4];
            compute_ocp_qp_res_nrm_inf(qp_res, res);
            double max_res = 0.0;
            for (int ii = 0; ii < 4; ii++)
                max_res = (res[ii] > max_res) ? res[ii] : max_res;

            /************************************************
            * print stats
            ************************************************/

            printf("\ninf norm res: %e, %e, %e, %e\n", res[0], res[1], res[2], res[3]);

            print_ocp_qp_info(&min_info);

            /************************************************
            * free memory
            ************************************************/

			// TODO



			if (ii==0 | ii==2 | ii==3)
				break;
		
		}

	}

#endif



    free(qp_in);
    free(qp_out);

    return 0;
}
