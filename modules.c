/*
 * =====================================================================================
 *
 *       Filename:  modules.c
 *
 *    Description:  Support for extra correlation modules
 *
 *        Version:  0.1
 *        Created:  26/10/2010 01:11:25
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  BlackLight (http://0x00.ath.cx), <blacklight@autistici.org>
 *        Licence:  GNU GPL v.3
 *        Company:  DO WHAT YOU WANT CAUSE A PIRATE IS FREE, YOU ARE A PIRATE!
 *
 * =====================================================================================
 */

#include	"spp_ai.h"

#include	<dirent.h>
#include	<dlfcn.h>
#include	<stdlib.h>
#include	<unistd.h>

/** \defgroup modules Software component for loading extra user-provided modules for correlating alerts
 * @{ */

PRIVATE double (**corr_functions)(const AI_snort_alert*, const AI_snort_alert*) = NULL;
PRIVATE size_t n_corr_functions = 0;

PRIVATE double (**weight_functions)() = NULL;
PRIVATE size_t n_weight_functions = 0;

/**
 * \brief  Get the correlation functions from the extra correlation modules as array of function pointers
 * \param  n_functions 	Number of function pointers in the array
 * \return The array of correlation functions
 */

double
(**AI_get_corr_functions ( size_t *n_functions )) (const AI_snort_alert*, const AI_snort_alert*)
{
	*n_functions = n_corr_functions;
	return corr_functions;
}		/* -----  end of function AI_get_corr_functions  ----- */

/**
 * \brief  Get the weights of the correlation extra modules as array of function pointers
 * \param  n_functions 	Number of function pointers in the array
 * \return The array of correlation weights functions
 */

double
(**AI_get_corr_weights ( size_t *n_functions )) ()
{
	*n_functions = n_weight_functions;
	return weight_functions;
}		/* -----  end of function AI_get_corr_weights  ----- */

/**
 * \brief  Initialize the extra modules provided by the user
 */

void
AI_init_corr_modules ()
{
	void   **dl_handles = NULL;
	DIR    *dir         = NULL;
	char   *err         = NULL;
	char   *fname       = NULL;
	size_t n_dl_handles = 0;
	struct dirent *dir_info = NULL;

	if ( !( dir = opendir ( config->corr_modules_dir )))
	{
		return;
	}

	while (( dir_info = readdir ( dir )))
	{
		if ( preg_match ( "(\\.(l|s)o)|(\\.l?a)", dir_info->d_name, NULL, NULL ))
		{
			if ( !( dl_handles = (void**) realloc ( dl_handles, (++n_dl_handles) * sizeof ( void* ))))
			{
				AI_fatal_err ( "Fatal dynamic memory allocation error", __FILE__, __LINE__ );
			}

			if ( !( fname = (char*) malloc ( strlen ( config->corr_modules_dir ) + strlen ( dir_info->d_name ) + 4 )))
			{
				AI_fatal_err ( "Fatal dynamic memory allocation error", __FILE__, __LINE__ );
			}

			sprintf ( fname, "%s/%s", config->corr_modules_dir, dir_info->d_name );

			if ( !( dl_handles[n_dl_handles-1] = dlopen ( fname, RTLD_LAZY )))
			{
				if (( err = dlerror() ))
				{
					_dpd.errMsg ( "dlopen: %s\n", err );
				}

				AI_fatal_err ( "dlopen error", __FILE__, __LINE__ );
			}

			free ( fname );
			fname = NULL;

			if ( !( corr_functions = (double(**)(const AI_snort_alert*, const AI_snort_alert*))
						realloc ( corr_functions, (++n_corr_functions) * sizeof ( double(*)(const AI_snort_alert*, const AI_snort_alert*) ))))
			{
				AI_fatal_err ( "Fatal dynamic memory allocation error", __FILE__, __LINE__ );
			}

			*(void**) (&(corr_functions[ n_corr_functions - 1 ])) = dlsym ( dl_handles[n_dl_handles-1], "AI_corr_index" );

			if ( !corr_functions[ n_corr_functions - 1 ] )
			{
				if (( err = dlerror() ))
				{
					_dpd.errMsg ( "dlsym: %s\n", err );
				}

				AI_fatal_err ( "dlsym error", __FILE__, __LINE__ );
			}

			if ( !( weight_functions = (double(**)()) realloc ( weight_functions, (++n_weight_functions) * sizeof ( double(*)() ))))
			{
				AI_fatal_err ( "Fatal dynamic memory allocation error", __FILE__, __LINE__ );
			}

			*(void**) (&(weight_functions[ n_weight_functions - 1 ])) = dlsym ( dl_handles[n_dl_handles-1], "AI_corr_index_weight" );

			if ( !weight_functions[ n_weight_functions - 1 ] )
			{
				if (( err = dlerror() ))
				{
					_dpd.errMsg ( "dlsym: %s\n", err );
				}

				AI_fatal_err ( "dlsym error", __FILE__, __LINE__ );
			}
		}
	}

	closedir ( dir );
}		/* -----  end of function AI_init_corr_modules  ----- */

/** @} */
