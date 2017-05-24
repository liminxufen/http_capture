#include <string.h>
#include <nids.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hash.h"
#include "stream.h"
#include "output.h"

/* the following line has been stolen from the official libnids example :)
 i'm not sure why libnids isn't using struct in_addr if their own example relies on its existence */
#define int_ntoa(x)     inet_ntoa(*((struct in_addr *)&x))

unsigned long package_count = 0;
unsigned long open_count = 0;
unsigned long data_count = 0;
unsigned long close_count = 0;

char * adres (struct tuple4 addr){
    static char buf[256];
    strcpy (buf, int_ntoa (addr.saddr));
    // sprintf (buf + strlen (buf), ",%i,", addr.source);
    strcat (buf, int_ntoa (addr.daddr));
    // sprintf (buf + strlen (buf), ",%i", addr.dest);
    return buf;
}

void testCallback(struct tcp_stream *ns, void **param) {
    if (package_count%100000 == 0)
        printf("count : %lu-%lu-%lu-%lu-%ld\n", package_count, open_count, close_count, open_count-close_count, time((time_t*)NULL));
    package_count ++;
    if (ns->nids_state == NIDS_JUST_EST){
        open_count ++;
        ns->client.collect++; // we want data received by a client
        ns->server.collect++; // and by a server, too
    } else if (ns->nids_state == NIDS_DATA){
        data_count ++;
    } else {
        close_count ++;
    }
    return;
}

void nidsCallback(struct tcp_stream *ns, void **param) {
    if (package_count%100000 == 0)
        printf("count---%lu-%lu-%lu-%lu-%lu-%ld\n", package_count, open_count, data_count, close_count, open_count-close_count, time((time_t*)NULL));
    
    //if (close_count >= 1000000) exit(1);
    package_count ++;
    struct stream *s;
    struct half_stream *hs;
    struct tuple4 a;
    
    char buffer[65535];
    
    if (debug > 5) printf("\n*** Callback from libnids ***\n");
    
    memcpy(&a, &ns->addr, sizeof(struct tuple4));
    
    switch (ns->nids_state) {
            
        case NIDS_JUST_EST:
            //hashLen();
            //if (ns->addr.dest != 80 && ns->addr.dest != 443) break;
            //if (ns->addr.dest != 80) break;
            ns->client.collect++;
            ns->server.collect++;
            open_count++;
            
            if ((s = hashFind(&a))) {
                printf("hashfind\n");
                if (!(s = hashDelete(&(s->addr)))) {
                    if (debug > 2) printf("nidsCallback(): Oops: Attempted to close unexisting connection. Ignoring.\n");
                    return;
                }
                json_object_put(s->json);
                free(s);
            }
            
            if (!(s = malloc(sizeof(struct stream)))) {
                if (debug > 2) printf("nidsCallback(): Oops: Could not allocate stream memory.\n");
                return;
            }
            
            if(!hashAdd(&a, s)) {
                printf("hashaddfailed\n");
                if (debug > 2) printf("nidsCallback(): Oops: Connection limit exceeded.\n");
                free(s);
                return;
            }
            streamOpen(s, &a);
            
            break;
            
        case NIDS_DATA:
            data_count++;
            if (!(s = hashFind(&a))) {
                if (debug > 2) printf("nidsCallback(): Oops: Received data for unexisting connection. Ignoring.\n");
                break;
            }
            if (ns->client.count_new) {
                hs = &ns->client;
                //printf("client receive %i\n", hs->count_new);
                memcpy(buffer, (char *)hs->data, hs->count_new);
                streamWriteResponse(s, buffer, hs->count_new);
            } else {
                hs = &ns->server;
                //printf("server receive %i\n", hs->count_new);
                memcpy(buffer, (char *)hs->data, hs->count_new);
                streamWriteRequest(s, buffer, hs->count_new);
            }
            break;
        default:
            close_count++;
            streamDelete(&a);
    }
}

static void hc_syslog(int type, int errnum, struct ip *iph, void *data) {
    return;
}

int nidsRun(char *filter) {
    nids_params.pcap_filter = filter;
    // nids_params.n_tcp_streams = (MAX_CONNECTIONS * 2) / 3 + 1; /* libnids allows 3/4 * n simultaneous twin connections */
    nids_params.n_tcp_streams = 10000;
    
    struct nids_chksum_ctl temp;
    temp.netaddr = 0;
    temp.mask = 0;
    temp.action = 1;
    nids_register_chksum_ctl(&temp,1);
    
    //nids_params.syslog = hc_syslog;
    
    if (!nids_init())
        return (-1);
    
    hashInit();
    streamInit();
    output_init();
    nids_register_tcp(nidsCallback);
    fprintf(stderr, "start nids run\n");
    nids_run();
    fprintf(stderr, "finish nids run\n");
    
    return (0);
}

int nids_device(char *dev, char *filter) {
    nids_params.device = dev;
    //nids_params.multiproc = 1;
    //nids_params.queue_limit = 20000;
    printf("%s\n", dev);
    return (nidsRun(filter));
}

int nids_file(char *filename, char *filter) {
    nids_params.filename = filename;
    return (nidsRun(filter));
}

