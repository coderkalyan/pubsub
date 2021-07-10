#include <string.h>
#include <stdbool.h>

#include <sys/slist.h>
#include <zephyr.h>

#include <pubsub/pubsub.h>

void pubsub_topic_init(struct pubsub_topic_s *topic, size_t size)
{
	topic->init = false;
	
	sys_slist_init(&(topic->subscribers));
	for (size_t i = 0;i < MAX_CHANNELS;i++) {
		topic->message[i] = k_malloc(size);
	}

	topic->size = size;

	topic->init = true;
}

void pubsub_subscriber_register(struct pubsub_topic_s *topic,
		struct pubsub_subscriber_s *subscriber, size_t channel)
{
	int key = irq_lock();

	subscriber->topic = topic;
	subscriber->channel = channel;
	k_sem_init(&(subscriber->updated), 0, 1);
	k_poll_event_init(&(subscriber->updated_event),
					  K_POLL_TYPE_SEM_AVAILABLE,
					  K_POLL_MODE_NOTIFY_ONLY,
					  &(subscriber->updated));

	sys_slist_append(&(topic->subscribers), &(subscriber->node));

	irq_unlock(key);
}

void pubsub_subscriber_notify(struct pubsub_subscriber_s *subscriber)
{
	k_sem_give(&(subscriber->updated));
}

void pubsub_publish(struct pubsub_topic_s *topic, size_t channel, void *data)
{
	int key = irq_lock();

	memcpy(topic->message[channel], data, topic->size);

	struct pubsub_subscriber_s *sub;
	struct pubsub_subscriber_s *s_sub;
	
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&(topic->subscribers), sub, s_sub, node) {
		if (sub->channel == channel) {
			pubsub_subscriber_notify(sub);
		}
	}

	irq_unlock(key);
}

bool pubsub_subscriber_updated(struct pubsub_subscriber_s *subscriber)
{
	return k_sem_count_get(&(subscriber->updated)) > 0;
}

void *pubsub_get(struct pubsub_subscriber_s *subscriber)
{
	int key = irq_lock();

	if (!subscriber->topic || !(subscriber->topic->init)) return NULL;
	k_sem_reset(&(subscriber->updated));
	void *ptr = subscriber->topic->message[subscriber->channel];

	irq_unlock(key);

	return ptr;
}

void pubsub_copy(struct pubsub_subscriber_s *subscriber, void *msg)
{
	int key = irq_lock();

	if (!subscriber->topic || !(subscriber->topic->init)) return;

	k_sem_reset(&(subscriber->updated));
	void *ptr = subscriber->topic->message[subscriber->channel];

	memcpy(msg, ptr, subscriber->topic->size);

	irq_unlock(key);
}

int pubsub_poll(struct pubsub_subscriber_s *subscriber, k_timeout_t timeout)
{
	int ret;

	struct k_poll_event *event = &(subscriber->updated_event);
	ret = k_poll(event, 1, timeout);

	if (ret == -EAGAIN) {
		return 0;
	} else if (ret < 0) {
		return ret;
	} else if (ret == 0) {
		if (event->state == K_POLL_STATE_SEM_AVAILABLE) {
			/* take the semaphore - we shouldn't have to wait since we just received
			 * a poll event that it's available */
			k_sem_take(&(subscriber->updated), K_NO_WAIT);
			event->state = K_POLL_STATE_NOT_READY; /* reset poll event */

			return 1;
		}
	} else {
		/* k poll should never return anything greater than 0 */
		return -EINVAL;
	}

	return -EINVAL;
}
