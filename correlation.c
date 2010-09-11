/*
 * =====================================================================================
 *
 *       Filename:  correlation.c
 *
 *    Description:  Runs the correlation algorithm of the alerts
 *
 *        Version:  0.1
 *        Created:  07/09/2010 22:04:27
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

#include	<unistd.h>
#include	<sys/stat.h>
#include 	<pthread.h>
#include	<libxml/xmlreader.h>

/** \defgroup correlation Module for the correlation of hyperalerts
 * @{ */

#ifndef 	LIBXML_READER_ENABLED
#error 	"libxml reader not enabled\n"
#endif

/** Enumeration for the types of XML tags */
enum  { inHyperAlert, inSnortIdTag, inPreTag, inPostTag, TAG_NUM };

PRIVATE AI_hyperalert_info  *hyperalerts = NULL;
PRIVATE AI_config           *conf        = NULL;
PRIVATE AI_snort_alert      *alerts      = NULL;

/**
 * \brief  Substitute the macros in hyperalert pre-conditions and post-conditions with their associated values
 * \param  alert 	Reference to the hyperalert to work on
 */

void
_AI_macro_subst ( AI_snort_alert **alert )
{
	/*
	 * Recognized macros:
	 * +SRC_ADDR+, +DST_ADDR+, +SRC_PORT+, +DST_PORT+, +ANY_ADDR+, +ANY_PORT+
	 */

	int  i;
	char src_addr[INET_ADDRSTRLEN], dst_addr[INET_ADDRSTRLEN];
	char src_port[10], dst_port[10];
	char *tmp;

	for ( i=0; i < (*alert)->hyperalert->n_preconds; i++ )
	{
		tmp = (*alert)->hyperalert->preconds[i];
		(*alert)->hyperalert->preconds[i] = str_replace_all ( (*alert)->hyperalert->preconds[i], " ", "" );
		free ( tmp );

		if ( strstr ( (*alert)->hyperalert->preconds[i], "+SRC_ADDR+" ))
		{
			inet_ntop ( AF_INET, &((*alert)->ip_src_addr), src_addr, INET_ADDRSTRLEN );
			tmp = (*alert)->hyperalert->preconds[i];
			(*alert)->hyperalert->preconds[i] = str_replace ( (*alert)->hyperalert->preconds[i], "+SRC_ADDR+", src_addr );
			free ( tmp );
		}
		
		if ( strstr ( (*alert)->hyperalert->preconds[i], "+DST_ADDR+" )) {
			inet_ntop ( AF_INET, &((*alert)->ip_dst_addr), dst_addr, INET_ADDRSTRLEN );
			tmp = (*alert)->hyperalert->preconds[i];
			(*alert)->hyperalert->preconds[i] = str_replace ( (*alert)->hyperalert->preconds[i], "+DST_ADDR+", dst_addr );
			free ( tmp );
		}
		
		if ( strstr ( (*alert)->hyperalert->preconds[i], "+ANY_ADDR+" )) {
			tmp = (*alert)->hyperalert->preconds[i];
			(*alert)->hyperalert->preconds[i] = str_replace ( (*alert)->hyperalert->preconds[i], "+ANY_ADDR+", "0.0.0.0" );
			free ( tmp );
		}
		
		if ( strstr ( (*alert)->hyperalert->preconds[i], "+SRC_PORT+" )) {
			snprintf ( src_port, sizeof ( src_port ), "%d", ntohs ((*alert)->tcp_src_port) );
			tmp = (*alert)->hyperalert->preconds[i];
			(*alert)->hyperalert->preconds[i] = str_replace ( (*alert)->hyperalert->preconds[i], "+SRC_PORT+", src_port );
			free ( tmp );
		}
		
		if ( strstr ( (*alert)->hyperalert->preconds[i], "+DST_PORT+" )) {
			snprintf ( dst_port, sizeof ( dst_port ), "%d", ntohs ((*alert)->tcp_dst_port) );
			tmp = (*alert)->hyperalert->preconds[i];
			(*alert)->hyperalert->preconds[i] = str_replace ( (*alert)->hyperalert->preconds[i], "+DST_PORT+", dst_port );
			free ( tmp );
		}
		
		if ( strstr ( (*alert)->hyperalert->preconds[i], "+ANY_PORT+" )) {
			tmp = (*alert)->hyperalert->preconds[i];
			(*alert)->hyperalert->preconds[i] = str_replace ( (*alert)->hyperalert->preconds[i], "+ANY_PORT+", "0" );
			free ( tmp );
		}
	}

	for ( i=0; i < (*alert)->hyperalert->n_postconds; i++ )
	{
		tmp = (*alert)->hyperalert->postconds[i];
		(*alert)->hyperalert->postconds[i] = str_replace_all ( (*alert)->hyperalert->postconds[i], " ", "" );
		free ( tmp );

		if ( strstr ( (*alert)->hyperalert->postconds[i], "+SRC_ADDR+" ))
		{
			inet_ntop ( AF_INET, &((*alert)->ip_src_addr), src_addr, INET_ADDRSTRLEN );
			tmp = (*alert)->hyperalert->postconds[i];
			(*alert)->hyperalert->postconds[i] = str_replace ( (*alert)->hyperalert->postconds[i], "+SRC_ADDR+", src_addr );
			free ( tmp );
		}
		
		if ( strstr ( (*alert)->hyperalert->postconds[i], "+DST_ADDR+" )) {
			inet_ntop ( AF_INET, &((*alert)->ip_dst_addr), dst_addr, INET_ADDRSTRLEN );
			tmp = (*alert)->hyperalert->postconds[i];
			(*alert)->hyperalert->postconds[i] = str_replace ( (*alert)->hyperalert->postconds[i], "+DST_ADDR+", dst_addr );
			free ( tmp );
		}
		
		if ( strstr ( (*alert)->hyperalert->postconds[i], "+ANY_ADDR+" )) {
			tmp = (*alert)->hyperalert->postconds[i];
			(*alert)->hyperalert->postconds[i] = str_replace ( (*alert)->hyperalert->postconds[i], "+ANY_ADDR+", "0.0.0.0" );
			free ( tmp );
		}
		
		if ( strstr ( (*alert)->hyperalert->postconds[i], "+SRC_PORT+" )) {
			snprintf ( src_port, sizeof ( src_port ), "%d", ntohs ((*alert)->tcp_src_port) );
			tmp = (*alert)->hyperalert->postconds[i];
			(*alert)->hyperalert->postconds[i] = str_replace ( (*alert)->hyperalert->postconds[i], "+SRC_PORT+", src_port );
			free ( tmp );
		}
		
		if ( strstr ( (*alert)->hyperalert->postconds[i], "+DST_PORT+" )) {
			snprintf ( dst_port, sizeof ( dst_port ), "%d", ntohs ((*alert)->tcp_dst_port) );
			tmp = (*alert)->hyperalert->postconds[i];
			(*alert)->hyperalert->postconds[i] = str_replace ( (*alert)->hyperalert->postconds[i], "+DST_PORT+", dst_port );
			free ( tmp );
		}
		
		if ( strstr ( (*alert)->hyperalert->postconds[i], "+ANY_PORT+" )) {
			tmp = (*alert)->hyperalert->postconds[i];
			(*alert)->hyperalert->postconds[i] = str_replace ( (*alert)->hyperalert->postconds[i], "+ANY_PORT+", "0" );
			free ( tmp );
		}
	}
}		/* -----  end of function _AI_macro_subst  ----- */

/**
 * \brief  Parse info about a hyperalert from a correlation XML file, if it exists
 * \param  key 	Key (gid, sid, rev) identifying the alert
 * \return A hyperalert structure containing the info about the current alert, if the XML file was found
 */

PRIVATE AI_hyperalert_info*
_AI_hyperalert_from_XML ( AI_hyperalert_key key )
{
	char                  hyperalert_file[1024] = {0};
	char                  snort_id[1024]        = {0};
	BOOL                  xmlFlags[TAG_NUM]     = { false };
	struct stat           st;
	xmlTextReaderPtr      xml;
	const xmlChar         *tagname, *tagvalue;
	AI_hyperalert_info    *hyp;

	if ( !( hyp = ( AI_hyperalert_info* ) malloc ( sizeof ( AI_hyperalert_info ))))
	{
		_dpd.fatalMsg ( "AIPreproc: Fatal memory allocation error at %s:%d\n", __FILE__, __LINE__ );
	}

	memset ( hyp, 0, sizeof ( AI_hyperalert_info ));
	memset ( hyperalert_file, 0, sizeof ( hyperalert_file ));
	
	hyp->key = key;
	snprintf ( hyperalert_file, sizeof ( hyperalert_file ), "%s/%d-%d-%d.xml",
			conf->corr_rules_dir, key.gid, key.sid, key.rev );

	if ( stat ( hyperalert_file, &st ) < 0 )
		return NULL;

	LIBXML_TEST_VERSION

	if ( !( xml = xmlReaderForFile ( hyperalert_file, NULL, 0 )))
		return NULL;

	while ( xmlTextReaderRead ( xml ))
	{
		if ( !( tagname = xmlTextReaderConstName ( xml )))
			continue;

		if ( xmlTextReaderNodeType ( xml ) == XML_READER_TYPE_ELEMENT )
		{
			if ( !strcasecmp ((const char*) tagname, "hyperalert" ))
			{
				if ( xmlFlags[inHyperAlert] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': the hyperalert tag was opened twice\n", hyperalert_file );
				else
					xmlFlags[inHyperAlert] = true;
			} else if ( !strcasecmp ((const char*) tagname, "snort-id" )) {
				if ( xmlFlags[inSnortIdTag] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': 'snort-id' tag open inside of another 'snort-id' tag\n", hyperalert_file );
				else if ( !xmlFlags[inHyperAlert] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': 'snort-id' tag open outside of 'hyperalert' tag\n", hyperalert_file );
				else
					xmlFlags[inSnortIdTag] = true;
			} else if ( !strcasecmp ((const char*) tagname, "pre" )) {
				if ( xmlFlags[inPreTag] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': 'pre' tag open inside of another 'pre' tag\n", hyperalert_file );
				else if ( !xmlFlags[inHyperAlert] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': 'pre' tag open outside of 'hyperalert' tag\n", hyperalert_file );
				else
					xmlFlags[inPreTag] = true;
			} else if ( !strcasecmp ((const char*) tagname, "post" )) {
				if ( xmlFlags[inPostTag] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': 'post' tag open inside of another 'post' tag\n", hyperalert_file );
				else if ( !xmlFlags[inHyperAlert] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': 'post' tag open outside of 'hyperalert' tag\n", hyperalert_file );
				else
					xmlFlags[inPostTag] = true;
			} else {
				_dpd.fatalMsg ( "AIPreproc: Unrecognized tag '%s' in XML file '%s'\n", tagname, hyperalert_file );
			}
		} else if ( xmlTextReaderNodeType ( xml ) == XML_READER_TYPE_END_ELEMENT ) {
			if ( !strcasecmp ((const char*) tagname, "hyperalert" ))
			{
				if ( !xmlFlags[inHyperAlert] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': hyperalert tag closed but never opend\n", hyperalert_file );
				else
					xmlFlags[inHyperAlert] = false;
			} else if ( !strcasecmp ((const char*) tagname, "snort-id" )) {
				if ( !xmlFlags[inSnortIdTag] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': snort-id tag closed but never opend\n", hyperalert_file );
				else
					xmlFlags[inSnortIdTag] = false;
			} else if ( !strcasecmp ((const char*) tagname, "pre" )) {
				if ( !xmlFlags[inPreTag] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': pre tag closed but never opend\n", hyperalert_file );
				else
					xmlFlags[inPreTag] = false;
			} else if ( !strcasecmp ((const char*) tagname, "post" )) {
				if ( !xmlFlags[inPostTag] )
					_dpd.fatalMsg ( "AIPreproc: Error in XML file '%s': post tag closed but never opend\n", hyperalert_file );
				else
					xmlFlags[inPostTag] = false;
			} else {
				_dpd.fatalMsg ( "AIPreproc: Unrecognized tag '%s' in XML file '%s'\n", tagname, hyperalert_file );
			}
		} else if ( xmlTextReaderNodeType ( xml ) == XML_READER_TYPE_TEXT ) {
			if ( !( tagvalue = xmlTextReaderConstValue ( xml )))
				continue;

			if ( xmlFlags[inSnortIdTag] )
			{
				snprintf ( snort_id, sizeof ( snort_id ), "%d.%d.%d",
						key.gid, key.sid, key.rev );

				if ( strcmp ( snort_id, (const char*) tagvalue ))
				{
					_dpd.errMsg ( "AIPreproc: Found the file associated to hyperalert: '%s', "
						"but the 'snort-id' field in there has a different value\n",
						hyperalert_file );
					return NULL;
				}
			} else if ( xmlFlags[inPreTag] ) {
				if ( !( hyp->preconds = (char**) realloc ( hyp->preconds, (++hyp->n_preconds)*sizeof(char*) )))
					_dpd.fatalMsg ( "AIPreproc: Fatal allocation memory error at %s:%d\n",
						__FILE__, __LINE__ );

				hyp->preconds[hyp->n_preconds-1] = strdup ((const char*) tagvalue );
			} else if ( xmlFlags[inPostTag] ) {
				if ( !( hyp->postconds = (char**) realloc ( hyp->postconds, (++hyp->n_postconds)*sizeof(char*) )))
					_dpd.fatalMsg ( "AIPreproc: Fatal allocation memory error at %s:%d\n",
						__FILE__, __LINE__ );

				hyp->postconds[hyp->n_postconds-1] = strdup ((const char*) tagvalue );
			}
		}
	}

	xmlFreeTextReader ( xml );
	xmlCleanupParser();
	return hyp;
}		/* -----  end of function _AI_hyperalert_from_XML  ----- */

/**
 * \brief  Thread for correlating clustered alerts
 * \param  arg 	Void pointer to module's configuration
 */

void*
AI_alert_correlation_thread ( void *arg )
{
	int                    i;
	struct stat            st;
	AI_hyperalert_key      key;
	AI_hyperalert_info     *hyp   = NULL;
	AI_snort_alert         *tmp   = NULL;
	FILE *fp;
	conf = (AI_config*) arg;

	while ( 1 )
	{
		sleep ( conf->correlationGraphInterval );

		if ( stat ( conf->corr_rules_dir, &st ) < 0 )
		{
			_dpd.errMsg ( "AIPreproc: Correlation rules directory '%s' not found, the correlation thread won't be active\n",
					conf->corr_rules_dir );
			pthread_exit (( void* ) 0 );
			return ( void* ) 0;
		}

		if ( !( alerts = AI_get_clustered_alerts() ))
			continue;

		for ( tmp = alerts; tmp; tmp = tmp->next )
		{
			/* Check if my hash table of hyperalerts already contains info about this alert */
			key.gid = tmp->gid;
			key.sid = tmp->sid;
			key.rev = tmp->rev;
			HASH_FIND ( hh, hyperalerts, &key, sizeof ( AI_hyperalert_key ), hyp );

			/* If not, try to read info from the XML file, if it exists */
			if ( !hyp )
			{
				/* If there is no hyperalert knowledge on XML for this alert, ignore it and get the next one */
				if ( !( hyp = _AI_hyperalert_from_XML ( key )))
					continue;

				/* If the XML file exists and it's valid, add the hypertalert to the hash table */
				HASH_ADD ( hh, hyperalerts, key, sizeof ( AI_hyperalert_key ), hyp );
			}

			/* Fill the hyper alert info for the current alert */
			if ( !( tmp->hyperalert = ( AI_hyperalert_info* ) malloc ( sizeof ( AI_hyperalert_info ))))
				_dpd.fatalMsg ( "AIPreproc: Fatal memory allocation error at %s:%d\n", __FILE__, __LINE__ );
			
			tmp->hyperalert->key         = hyp->key;
			tmp->hyperalert->n_preconds  = hyp->n_preconds;
			tmp->hyperalert->n_postconds = hyp->n_postconds;
			
			if ( !( tmp->hyperalert->preconds = ( char** ) malloc ( tmp->hyperalert->n_preconds * sizeof ( char* ))))
				_dpd.fatalMsg ( "AIPreproc: Fatal memory allocation error at %s:%d\n", __FILE__, __LINE__ );
			
			for ( i=0; i < tmp->hyperalert->n_preconds; i++ )
				tmp->hyperalert->preconds[i] = strdup ( hyp->preconds[i] );

			if ( !( tmp->hyperalert->postconds = ( char** ) malloc ( tmp->hyperalert->n_postconds * sizeof ( char* ))))
				_dpd.fatalMsg ( "AIPreproc: Fatal memory allocation error at %s:%d\n", __FILE__, __LINE__ );
			
			for ( i=0; i < tmp->hyperalert->n_postconds; i++ )
				tmp->hyperalert->postconds[i] = strdup ( hyp->postconds[i] );

			_AI_macro_subst ( &tmp );

			fp = fopen ( "/home/blacklight/LOG", "a" );
			fprintf ( fp, "pre: %s\n", (tmp->hyperalert->n_preconds > 0) ? tmp->hyperalert->preconds[0] : "()" );
			fprintf ( fp, "post: %s\n", (tmp->hyperalert->n_postconds > 0) ? tmp->hyperalert->postconds[0] : "()" );
			fclose ( fp );
		}

		AI_free_alerts ( alerts );
	}

	pthread_exit (( void* ) 0 );
	return (void*) 0;
}		/* -----  end of function AI_alert_correlation_thread  ----- */

/** @} */

