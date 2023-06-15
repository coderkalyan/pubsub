/* pubsub.h
 *
 * Public header functions for the pubsub library
 *
 * @author Kalyan Sriram <kalyan@coderkalyan.com>
 */

#ifndef _PUBSUB_PUBSUB_H
#define _PUBSUB_PUBSUB_H

#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/init.h>

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

/**
 * WARNING: This function is called post-kernel init by `PUBSUB_TOPIC_DEFINE`.
 * Don't call this funcion directly; use `PUBSUB_TOPIC_DEFINE` instead.
 *
 * Initialize a topic struct prior to usage (publishing or subscribing).
 *
 * @param topic to the topic to initialize
 * @param size size of the data type (struct) you're storing in this topic (i.e. from sizeof)
 */
void pubsub_topic_init(struct pubsub_topic_s *topic, size_t size);

/**
 * WARNING: This function is called post-kernel init by `PUBSUB_SUBSCRIBER_DEFINE`.
 * Don't call this funcion directly; use `PUBSUB_SUBSCRIBER_DEFINE` instead.
 *
 * Initialize a subscriber and register it to subscribe to a specified topic, on
 * a specified channel.
 *
 * @param topic the topic to register this subscriber to
 * @param subscriber the subscriber to initialize and register
 * @param channel data channel on the topic to subscribe to. If unsure, use `0`.
 */
void pubsub_subscriber_register(struct pubsub_topic_s *topic,
		struct pubsub_subscriber_s *subscriber, size_t channel);

/**
 * Publish a message to a topic, on a specified channel.
 *
 * @param topic the topic to publish to.
 * @param channel data channel on the topic to publish to. If unsure, use `0`.
 * @param data pointer to the data to publish.
 */
void pubsub_publish(struct pubsub_topic_s *topic, size_t channel, void *data);

/**
 * Check if a subscriber has new data. That is, check if a publisher has published
 * new data to the subscribed topic that this client hasn't yet responded to.
 *
 * Upon calling this, the subscriber's internal `updated` flag is unset until more
 * data is published.
 *
 * @param subscriber the subscriber to check.
 * @return whether or not the subscriber has received new data.
 */
bool pubsub_subscriber_updated(struct pubsub_subscriber_s *subscriber);

/**
 * WARNING: you probably want `pubsub_copy`. This function is pseudo-deprecated
 * and should only be used if you understand the implications.
 *
 * Get a pointer to the latest data sample on the topic. This is useful if you
 * want to peak at the data without having to copy it into your own local buffer,
 * but this is unsafe since the data could be updated at any time. 
 *
 * This function works at any time, whether or not the subscriber has any `updated`
 * data. That is, you can always fetch a pointer to the same data over and over again.
 *
 * @param subscriber the subscriber whose topic data is desired
 * @return pointer to the subscriber's topic's internal data
 */
void *pubsub_get(struct pubsub_subscriber_s *subscriber);

/**
 * Copy the latest data sample on the topic to a local copy for processing.
 *
 * This function works at any time, whether or not the subscriber has any `updated`
 * data. That is, you can always fetch the same data over and over again.
 *
 * @param subscriber the subscriber whose data to copy
 * @param msg pointer to the destination to copy the data
 */
void pubsub_copy(struct pubsub_subscriber_s *subscriber, void *msg);

/**
 * Use the kernel polling functionality to poll for new data. This essentially
 * suspends the calling thread until new data has arrived, or the request has timed out.
 * This results in a very low-overhead, low-latency way to subscribe to high frequency
 * data samples (like from a high rate sensor).
 *
 * returns:
 * 0 if the poll timed out
 * a positive value if the poll succeeded
 * a negative value if the poll failed due to some error that should be handled
 *
 * @param subscriber the subscriber to poll
 * @timeout how long to wait before the poll times out
 * @return poll result
 */
int pubsub_poll(struct pubsub_subscriber_s *subscriber, k_timeout_t timeout);

#define PUBSUB_TOPIC_INIT_PRIORITY 0
#define PUBSUB_SUBSCRIBER_INIT_PRIORITY 1

/**
 * Statically declare and initialize a new topic.
 *
 * This automatically attaches a hook to initialize the topic
 * using `pubsub_topic_init` during the APPLICATION hook of
 * kernel boot time.
 *
 * @param topic_name name of the topic to declare
 * @param topic_msg_size size of the data struct to be stored
 * in this topic (i.e. from sizeof)
 */
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

/**
 * Statically declare and initialize/register a new subscriber.
 *
 * This automatically attaches a hook to initialize the subscriber
 * using `pubsub_subscriber_init` during the APPLICATION hook of
 * kernel boot time.
 *
 * @param topic_name name of the topic to subscribe to
 * @param subscriber_name name of the subscriber to declare
 * @param channel data channel on the topic to publish to. If unsure, use `0`.
 */
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
