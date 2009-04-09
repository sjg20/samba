/* ======================================================================
 *
 * File    : $RCSfile: environment.h,v $
 * Date    : $Date: 2007-08-23 15:40:14 +1200 (Thu, 23 Aug 2007) $
 * Project : Scallop
 * Author  : Andre Renaud
 * Company : Bluewater Systems Ltd
 *           http://www.bluewatersys.com
 *
 * $Id: environment.h 776 2007-08-23 03:40:14Z andre $
 *
 * Manage upgrading the U-boot environment variables
 *
 * ======================================================================
 */

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

int environment_init (int length);
int environment_data (char **data, int *length);

int environment_get (const char *variable, char *buffer, int max_len);
int environment_get_raw (const char *variable, char *buffer, int max_len);
int environment_set (const char *variable, const char *value);
int environment_clear (const char *variable);
int environment_update_from_file (const char *filename);

#endif

