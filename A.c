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
    mosquitto_subscribe(mosq, NULL, "itpm_A", 0);
}

static void mqtt_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("disconnect callback, rc=%d\n", result);
}

static void mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    bool match = 0;
    printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

    mosquitto_topic_matches_sub("itpm_A", message->topic, &match);
    if (match)
    {
         printf("got message for ADC topic\n");
    }

}

int main()
{
    int32_t rc;
    signal(SIGINT, sig_handler);

    mosquitto_lib_init();
    mosq = mosquitto_new("A", true, 0);

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
              rc = mosquitto_loop(mosq, -1, 1);
              if(rc)
              {
                  printf("connection error!\n");
                  sleep(10);
                  mosquitto_reconnect(mosq);
              }
              else
              {
                  mosquitto_publish(mosq, NULL, "itpm_B", 12, "Hello World!", 2, 0);
                  printf("Publish payload: Hello World!\n");
                  sleep(1);
              }
         }
    }
}
