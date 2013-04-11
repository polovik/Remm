/*
 * http.h
 */

#ifndef HTTP_H_
#define HTTP_H_

#include "connection.h"

#define OPERATION_SUCCESS	"Success. "
#define OPERATION_FAILED	"Error. "
#define DESCRIPTION_ENDING	"<br>"

//------------------------------Common routines---------------------------------

/**	Perform request to url and waiting until this page downloaded.
 *  Return 0 on success, otherwise -1(error).
 */
int http_request(const char *url);

/**	Parse substring OPERATION_SUCCESS in previously downloaded http page.
 *  result points to description of operation - either on success or on error.
 *  Return 1 if substring OPERATION_SUCCESS is founded in page, otherwise return 0.
 */
int is_operation_successfull(char **result);

//-------------Specific routines for exchange connection info-------------------

/**	Clear old connection info from server before requesting actual info.
 *  Return 0 on success, otherwise -1(error).
 */
int prepare_connection(host_side_e side);

/**	Send actual connection info to server and read candidate info of opposite side.
 *  Candidate info will be saved in candidate_info[info_length].
 *  Return 0 on success, otherwise -1(error).
 */
int send_connection_info(host_side_e side, char *foundation, unsigned int comp_id,
							 unsigned int prio, char *ip, unsigned int port, char *type,
							 char *candidate_info, int info_length);

#endif /* HTTP_H_ */
