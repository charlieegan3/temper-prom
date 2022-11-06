#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <microhttpd.h>

#include "temper.h"

temper_t t;
temper_stick_t **sticks=NULL;
float *temperatures=NULL;
size_t num_sticks=0;
struct MHD_Daemon *deamon;

void cleanup(void) {
    if(sticks) {
        for(size_t i=0; i<num_sticks; ++i)
            temper_stick_free(sticks[i]);
        free(sticks);
    }

    temper_free(&t);
    free(temperatures);
}

void handler(int e) {
    MHD_stop_daemon(deamon);
    cleanup();
    exit(EXIT_SUCCESS);
}

uint8_t initialize_sticks(void) {
    if(temper_init(&t))
        return 1;

    num_sticks=temper_count_sticks(&t);
    if(num_sticks==0) {
        fprintf(stderr, "Error: no sticks found\n");
        goto fail_ft;
    }

    if(!(sticks=malloc(sizeof(temper_stick_t *)*num_sticks))) {
        fprintf(stderr, "Error: failed allocating space for sticks!\n");
        goto fail_fs;
    }

    memset(sticks, 0, sizeof(temper_stick_t *)*num_sticks);

    for(size_t i=0; i<num_sticks; ++i) {
        sticks[i]=temper_get_stick(&t, i);

        if(!sticks[i]) {
            fprintf(stderr, "stick==NULL\n");
            goto fail_fs;
        }
    }

    return 0;

fail_fs:
    for(size_t i=0; sticks[i]; ++i)
        temper_stick_free(sticks[i]);

fail_ft:
    temper_free(&t);

    return 1;
}

#define PAGE	"# HELP temper_temp The temperature measured by a TEMPer USB stick\n"\
             	"# TYPE temper_temp gauge\n"

#define ROW		"temper_temp{stick=\"%lu\"} %f\n"

static enum MHD_Result show_temper_results(void * cls,
        struct MHD_Connection * connection,
        const char * url,
        const char * method,
        const char * version,
        const char * upload_data,
        size_t * upload_data_size,
        void ** ptr) {

    struct MHD_Response * response;
    int ret;

    if(strcmp(method, "GET"))
        return MHD_NO;

    if(0!=*upload_data_size)
        return MHD_NO;

    const size_t row_len=strlen(ROW)+32;
    const size_t resp_s=strlen(PAGE)+row_len*num_sticks;

    char *resp=malloc(resp_s);

    size_t written=snprintf(resp, resp_s, "%s", PAGE);
    for(size_t i=0; i<num_sticks; ++i)
        written+=snprintf(resp+written, row_len, ROW, i, temperatures[i]);

    response = MHD_create_response_from_buffer (written,
               (void*) resp,
               MHD_RESPMEM_MUST_FREE);
    ret = MHD_queue_response(connection,
                             MHD_HTTP_OK,
                             response);
    MHD_destroy_response(response);
    return ret;
}

int main(int ac, char *as[]) {

    if(ac!=2) {
        fprintf(stderr, "Usage: %s [port]\n", as[0]);
        return EXIT_FAILURE;
    }

    if(initialize_sticks())
        return EXIT_FAILURE;

    if(!(temperatures=malloc(sizeof(float)*num_sticks))) {
        fprintf(stderr, "Failed allocating space for temperatures!\n");
        cleanup();
        return EXIT_FAILURE;
    }

    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    deamon=MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
                            atoi(as[1]),
                            NULL,
                            NULL,
                            &show_temper_results,
                            NULL,
                            MHD_OPTION_END);

    if(!deamon) {
        fprintf(stderr, "Failed starting webserver!\n");
        cleanup();
        return EXIT_FAILURE;
    }

    for(;;) {
        for(size_t i=0; i<num_sticks; ++i) {
            if(temper_stick_get_temp(sticks[i], 0, &temperatures[i])) {
                MHD_stop_daemon(deamon);
                cleanup();
                return EXIT_FAILURE;
            }
        }

        sleep(1);
    }

    return 0;
}
