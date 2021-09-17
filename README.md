# pubsub

## Introduction
This is a very lightweight publish - subscribe framework for embedded systems, written in C89. It is targeted for the Zephyr RTOS framework, and therefore uses some Zephyr libraries - but it can be easily ported to other frameworks, see below. The library consists of a single source file (src/pubsub.c) and a single header (include/pubsub/pubsub.h). It currently supports a single publisher and multiple subscribers per data topic. It is designed to be thread safe and as efficient as possible, with minimal overhead while offering a very simple API. This makes it suitable for anything from periodic events to high rate (kHz) sensor data exchange between threads.

## Warning
Pubsub is early in development. It has been shown to work reliably for "standard" use cases involving inter-thread communication of sensor data using Zephyr on an STM32 device (although there is no STM32 specific code other than higher resolution timestamping). It may not work for certain use cases or environments. If you find a bug, please file an issue or send a PR!

## Installation
### West + Zephyr
For west-managed zephyr workspaces, just add this project to your west manifest:
```yaml
manifest:
  remotes:
    - name: pubsub
      url-base: https://github.com/coderkalyan
  projects:
    - name: pubsub
      path: modules/lib/pubsub
      remote: pubsub
```

### Non-west/zephyr
If you are not using west or Zephyr, you can simply copy `src/pubsub.c` and `include/pubsub/pubsub.h` into your own tree. Keep in mind that if you are not using Zephyr, you will have to port the library by providing replacements for the (few) Zephyr specific APIs used.

## Usage
A complete example can be found in the `examples/` directory. The code here is shortened for brevity.

Pubsub follows a single-publisher, multi-subscriber architecture - but first we need to define the data type we want to publish!

Create a file in your project, `include/topics/my_message.h`:
```c
#include <pubsub/pubsub.h>

/* Pubsub types are just C structs contained in header files like this one. */
struct my_message_s {
	/* add your data types here */
	int counter1;
	int counter2;

	/* all messages should contain (64 bit) microsecond timestamps. At time of writing,
	the internal library does not make use of this timestamp, but this field is reserved
	for future use. Additionally, many/most applications are likely to benefit from message
	timestamping, so it's a good idea to include it. */
	int64_t timestamp;
};

/* the following line allows the message topic to be shared between the publisher and subscriber(s). */
extern struct pubsub_topic_s my_message_topic;
```

Now that we have a data type, we can start publishing to it:
`src/publisher.c`
```c
#include <pubsub/pubsub.h>
#include "topics/my_message.h"

/* this macro statically initializes the topic, and can only be used in a single file (per topic) */
PUBSUB_TOPIC_DEFINE(my_message_topic, sizeof(struct my_message_s));

static void publisher_thread_entry_point(void)
{
	struct my_message_s message;
	memset(message, 0, sizeof(struct my_message_s));

	while (1) {
		/* how you want to handle timestamps is up to you, but it's good practice to populate it with something, even if it isn't accurate down to the microsecond */
		message.timestamp = get_current_time_us(); /* not a real function */

		message.counter1 += 1;
		message.counter2 += 2;

		/* publish the message on the `my_message_topic` topic, on channel 0. Channels allow
		you to publish multiple streams on the same topic. For instance, you might have a temperature sample topic, and want to publish independent samples from 2 separate temperature sensors. If you only plan to publish to a single channel, use channel 0. */
		pubsub_publish(&my_message_topic, 0, &message);

		sleep_ms(500); /* not a real function */
	}
}
```

Now that we have a sample being published at 2Hz, we can subscribe to it. There are 2 main ways to do this - by checking for updates on a topic, or using the polling API.

Let's start with manually checking for updates. If your subscriber thread only needs to check for new data relatively infrequently, or you want to synchronize new data subscriptions to the subscriber thread's clock, this is what you want.

`src/subscriber1.c`:
```c
#include <pubsub/pubsub.h>
#include "topics/my_message.h"

/* this macro statically defines a subscriber `my_message_sub` and subscribes it to `my_message_topic` on channel 0. */
PUBSUB_SUBSCRIBER_DEFINE(my_message_topic, my_message_sub, 0);

static void subscriber_thread_entry_point(void)
{
	struct my_message_s message;

	while (1) {
		/* have we gotten any new published data since last time? */
		if (pubsub_subscriber_updated(my_message_sub)) {
			/* if so, copy it into our own struct */
			pubsub_copy(my_message_sub, message);

			printf("Received counter1: %d, counter2: %d\n", message.counter1, message.counter2);
		}

		sleep_ms(1000); /* not a real function */
	}
}
```

## Porting
To port pubsub to another framework, you will have to provide the following interfaces:
* a C heap implementation (malloc/free)
* a linked list implementation
* semaphores/mutexes for multithreading
* (optional) a kernel polling API for poll based usage
* a high resolution timestamping implementation is very useful for usage. Microsecond resolution is preferred but millisecond resolution is sufficient.
