#include <pjlib.h>
#include <pjlib-util.h>
#include <pjlib-util/http_client.h>
#include <pjnath.h>
#include "http.h"

#define THIS_FILE	    "http"

static char http_page[100000];

static void on_complete(pj_http_req *hreq, pj_status_t status,
                        const pj_http_resp *resp)
{
    PJ_UNUSED_ARG(hreq);

    if (status != PJ_SUCCESS) {
    	printf("ERROR %s() HTTP request completed with error", __FUNCTION__);
        return;
    }
    printf("INFO  %s() Page is downloaded: %d bytes.\n", __FUNCTION__, resp->size);
    memset(http_page, 0x00, sizeof(http_page));
    if (resp->size > 0 && resp->data) {
    	if (resp->size < sizeof(http_page)) {
			strncpy(http_page, resp->data, resp->size);
			http_page[resp->size] = 0x00;
    	} else {
			strncpy(http_page, resp->data, sizeof(http_page));
			http_page[sizeof(http_page) - 1] = 0x00;
    	}
    }
}

int http_request(const char *url)
{
	pj_caching_pool cp;
    pj_http_req_callback hcb;
    pj_timer_heap_t *timer_heap;
    pj_pool_t *pool;
    pj_pool_factory *mem;
    pj_status_t status;

    printf("INFO  %s() Start request to %s.\n", __FUNCTION__, url);
    memset(http_page, 0x00, sizeof(http_page));
    pj_caching_pool_init(&cp, NULL, 0);
    mem = &cp.factory;

    pj_bzero(&hcb, sizeof(hcb));
    hcb.on_complete = &on_complete;

    /* Create pool, timer, and ioqueue */
    pool = pj_pool_create(mem, NULL, 8192, 4096, NULL);
    status = pj_timer_heap_create(pool, 16, &timer_heap);
    if (status == PJ_SUCCESS) {
    	pj_ioqueue_t *ioqueue;
		status = pj_ioqueue_create(pool, 16, &ioqueue);
		if (status == PJ_SUCCESS) {
		    pj_str_t pj_url;
		    pj_http_req *http_req;
			pj_strdup2(pool, &pj_url, url);
			status = pj_http_req_create(pool, &pj_url, timer_heap, ioqueue, NULL, &hcb, &http_req);
			if (status == PJ_SUCCESS) {
				status = pj_http_req_start(http_req);
				if (status == PJ_SUCCESS) {
					while (pj_http_req_is_running(http_req)) {
						pj_time_val delay = {0, 50};
						pj_ioqueue_poll(ioqueue, &delay);
						pj_timer_heap_poll(timer_heap, NULL);
					}
				} else {
					printf("ERROR %s() pj_http_req_start failed.\n", __FUNCTION__);
				}
				pj_http_req_destroy(http_req);
			} else {
				printf("ERROR %s() pj_http_req_create failed.\n", __FUNCTION__);
			}
			pj_ioqueue_destroy(ioqueue);
		} else {
			printf("ERROR %s() pj_ioqueue_create failed.\n", __FUNCTION__);
		}
		pj_timer_heap_destroy(timer_heap);
    } else {
		printf("ERROR %s() pj_timer_heap_create failed.\n", __FUNCTION__);
	}

    pj_pool_release(pool);
    pj_caching_pool_destroy(&cp);
    printf("INFO  %s() Request finished.\n", __FUNCTION__);

    if (status != PJ_SUCCESS)
    	return -1;
    else
    	return 0;
}

int is_operation_successfull(char **result)
{
	char *description;
	char *end;
	*result = NULL;
	if (strlen(http_page) == 0)
		return 0;

	description = strstr(http_page, OPERATION_SUCCESS);
	if (description != NULL) {
		*result = description + strlen(OPERATION_SUCCESS);
		end = strstr(http_page, DESCRIPTION_ENDING);
		if (end != NULL)
			end[0] = 0x00;
		return 1;
	}

	description = strstr(http_page, OPERATION_FAILED);
	if (description != NULL) {
		*result = description + strlen(OPERATION_FAILED);
		end = strstr(http_page, DESCRIPTION_ENDING);
		if (end != NULL)
			end[0] = 0x00;
		return 0;
	}

	return 0;
}
