#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <mosquitto.h>

struct mosquitto *mosq = NULL;

void sig_handler(int signo)
{
    if (signo == SIGINT)
    {
         mosquitto_destroy(mosq);
         mosquitto_lib_cleanup();
         exit(0);
    }
}

static void mqtt_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("connect callback, rc=%d\n", result);
    mosquitto_subscribe(mosq, NULL, "itpm_B", 0);
}

static void mqtt_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("disconnect callback, rc=%d\n", result);
}

static void mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	char *temp = NULL;
	int message_len;
	char *send_payload = NULL;
    struct mosquitto_message *temp_message = malloc(sizeof(struct mosquitto_message));
    int result = 0;
    int i = 0;
    bool match = 0;
    result  = mosquitto_message_copy(temp_message,message);
    printf("got message '%.*s' for topic '%s'\n", temp_message->payloadlen, (char*) temp_message->payload, temp_message->topic);

    mosquitto_topic_matches_sub("itpm_B", temp_message->topic, &match);
    if (match)
    {
        temp = (char*)temp_message->payload;
		message_len = temp_message->payloadlen;
		send_payload = malloc(sizeof(char)*message_len);
		for(; i< message_len; i++)
		{
		    send_payload[i] = temp[message_len-1-i];
		}
		send_payload[message_len]='\0';
		printf("Publish payload: %s\n", send_payload);

		result = mosquitto_publish(mosq, NULL, "itpm_A", message_len, send_payload, 2, 0);
		free(send_payload);
    }
    mosquitto_message_free(&temp_message);
}

int main()
{
    int32_t rc;
    struct mosquitto *mosq = NULL;

    mosquitto_lib_init();
    mosq = mosquitto_new("B", true, 0);

    if(mosq == NULL)
    {
        exit(1);
    }

    if(mosq)
    {
        mosquitto_connect_callback_set(mosq, mqtt_connect_callback);
        mosquitto_message_callback_set(mosq, mqtt_message_callback);
        mosquitto_disconnect_callback_set(mosq, mqtt_disconnect_callback);

        mosquitto_connect(mosq, "broker.hivemq.com", 1883, 60);
        for(;;)
        {
              rc = mosquitto_loop(mosq, 10, 1);
              if(rc)
              {
                  printf("connection error!\n");
                  sleep(10);
                  mosquitto_reconnect(mosq);
              }
         }
    }
}
