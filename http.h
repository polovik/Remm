/*
 * http.h
 */

#ifndef HTTP_H_
#define HTTP_H_

#define OPERATION_SUCCESS	"Success. "
#define OPERATION_FAILED	"Error. "
#define DESCRIPTION_ENDING	"<br>"
#define URL_PREPARE_CONNECTION	"http://copter.ho.ua/connect.cgi?host=client&cmd=prepare"

/**	Perform request to url and waiting until this page downloaded.
 *  Return 0 on success, otherwise -1(error).
 */
int http_request(const char *url);

/**	Parse substring OPERATION_SUCCESS in previously downloaded http page.
 *  result points to description of operation - either on success or on error.
 *  Return 1 if substring OPERATION_SUCCESS is founded in page, otherwise return 0.
 */
int is_operation_successfull(char **result);

#endif /* HTTP_H_ */
