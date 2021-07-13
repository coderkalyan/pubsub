#ifndef _PUBSUB_PUBSUB_H
#define _PUBSUB_PUBSUB_H

#include <stdbool.h>

#include <sys/slist.h>
#include <zephyr.h>
#include <init.h>

#define MAX_CHANNELS 4

struct pubsub_topic_s {
	bool init;
	sys_slist_t subscribers;

	size_t size;
	void *message[MAX_CHANNELS];
};

struct pubsub_subscriber_s {
	sys_snode_t node;

	struct pubsub_topic_s *topic;
	size_t channel;
	struct k_sem updated;
	struct k_poll_event updated_event;
};

void pubsub_topic_init(struct pubsub_topic_s *topic, size_t size);
void pubsub_subscriber_register(struct pubsub_topic_s *topic,
		struct pubsub_subscriber_s *subscriber, size_t channel);
void pubsub_subscriber_notify(struct pubsub_subscriber_s *subscriber);
void pubsub_publish(struct pubsub_topic_s *topic, size_t channel, void *data);
bool pubsub_subscriber_updated(struct pubsub_subscriber_s *subscriber);
void *pubsub_get(struct pubsub_subscriber_s *subscriber);
void pubsub_copy(struct pubsub_subscriber_s *subscriber, void *msg);
int pubsub_poll(struct pubsub_subscriber_s *subscriber, k_timeout_t timeout);

#define PUBSUB_TOPIC_INIT_PRIORITY 0
#define PUBSUB_SUBSCRIBER_INIT_PRIORITY 1

#define PUBSUB_TOPIC_DEFINE(topic_name, topic_msg_size)			\
	struct pubsub_topic_s topic_name;  							\
																\
	static int pubsub_topic_init_##topic_name(const struct device *dev)		\
	{ 															\
		ARG_UNUSED(dev); 										\
																\
		pubsub_topic_init(&topic_name, topic_msg_size); 		\
		return 0; 												\
	} 															\
																\
	SYS_INIT(pubsub_topic_init_##topic_name, 					\
			APPLICATION, PUBSUB_TOPIC_INIT_PRIORITY); 			\

#define PUBSUB_SUBSCRIBER_DEFINE(topic_name, subscriber_name, channel) 	\
	static struct pubsub_subscriber_s subscriber_name; 				\
																\
	static int pubsub_sub_init_##subscriber_name(const struct device *dev) { \
		ARG_UNUSED(dev); 										\
																\
		pubsub_subscriber_register(&topic_name, &subscriber_name, channel); \
																\
		return 0; 												\
	} 															\
																\
	SYS_INIT(pubsub_sub_init_##subscriber_name,					\
			APPLICATION, PUBSUB_SUBSCRIBER_INIT_PRIORITY); 		\

#endif /* _PUBSUB_PUBSUB_H */
