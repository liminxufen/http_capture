
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <nids.h>
#include <signal.h>
#include <time.h>
#include <librdkafka/rdkafka.h>

#include "stream.h"
#include "output.h"
#include "config.h"

int output_fd;
int output_size = 0;

rd_kafka_t *rk = NULL;
rd_kafka_conf_t *conf = NULL;
rd_kafka_topic_t *rkt = NULL;
rd_kafka_topic_conf_t *topic_conf = NULL;
int partition = RD_KAFKA_PARTITION_UA;

void create_conn() {
  struct sockaddr_in sin = {0};
  int x;
  output_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (output_fd == -1) {
    fprintf(stderr, "redis: socket() failed to create socket\n");
    exit(1);
  }
  sin.sin_addr.s_addr = inet_addr(REDIS_HOST);
  sin.sin_port = htons((unsigned short)REDIS_PORT);
  sin.sin_family = AF_INET;
  x = connect(output_fd, (struct sockaddr*)&sin, sizeof(sin));
  if (x != 0) {
    fprintf(stderr, "redis: connect() failed\n");
    exit(1);
  }
}

void init_kafka() {

	char errstr[256];
	conf = rd_kafka_conf_new();
	if(!(rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr)))) {
		fprintf(stderr, "%% Failed to create new producer: %s\n");
		exit(1);
	}

	if(rd_kafka_brokers_add(rk, KAFKA_BROKERS) == 0) {
		fprintf(stderr, "%% No valid brokers specified\n");
		exit(1);
	}
	
	rkt = rd_kafka_topic_new(rk, KAFKA_TOPIC, topic_conf);
	topic_conf = NULL;

}

void handle_pipe(int sig)
{
  printf("receive sig: %i\n", sig);
  create_conn();
}

void output_init() {
  if (redis_output == 1) {
  	struct sigaction action;
  	action.sa_handler = handle_pipe;
  	sigemptyset(&action.sa_mask);
  	action.sa_flags = 0;
  	create_conn();
  	sigaction(SIGPIPE, &action, NULL);
  }
  if (redis_output == -1) {
	init_kafka();	
  }
  return;
}

int output(struct stream *s)
{
  char line[60000];
  time_t tt = time(NULL);
  if(s->is_http == 0)
    return 0;
  json_object_object_add(s->json, "time", json_object_new_int(tt));
  json_object_object_add(s->json, "request.hc", json_object_new_int(s->tmp));

  const char * tmp = json_object_to_json_string(s->json);

  if (redis_output == 1) {
    	snprintf(line, sizeof(line),
            "*3\r\n"
            "$7\r\nPUBLISH\r\n"
            "$%u\r\n%s\r\n"
            "$%u\r\n%s\r\n"
            ,
            (unsigned)strlen(redis_key), redis_key,
            (unsigned)strlen(tmp), tmp
            );

  	int count = send(output_fd, line, (int)strlen(line), 0);

  	if (count != strlen(line)) {
    		printf("output socket error\n");
    		create_conn();
    		return 0;
  	}
  	return count;
  } else if (redis_output == -1) {
	char stderr[256];
	int len = sizeof(line);
	//memcat(line, '\0', len);
	snprintf(line, len, "%s", tmp);
	//printf("%s\n",line);
	if(rd_kafka_produce(rkt, partition, RD_KAFKA_MSG_F_COPY, line, strlen(line), NULL, 0, NULL) == -1) {

		fprintf(stderr, "%% Failed to produce to topic %s ""partition %i:%s\n", rd_kafka_topic_name(rkt), partition, rd_kafka_err2str(rd_kafka_last_error()));
		rd_kafka_poll(rk, 0);
		return 0;
	}

	return len;
  } else {
  	printf("%s\n", tmp);
	return 1;
  }
}
