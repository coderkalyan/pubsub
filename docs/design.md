# Design

## Rationale
Pubsub is extremely simple at heart. And I mean simple: `src/pubsub.c` is only 122 lines of C code at time of writing. Instead of anything complicated like kernel-based shared memory, queues, network stack, or virtual filesystem, it achieves a single publisher, multi subscriber workflow using just a bit of `memcpy` and convenient uses of `extern struct`. It may not support every complex use case or memory manipulation trick in the book, but keeping things simple minimizes overhead and developer confusion.

## Architecture

### Topic
Pubsub is built on a simple struct called a topic (`struct pubsub_topic_s`). A topic is a broker that keeps track of published data and provides it to subscribers. Topics are created once per data type (the struct you're passing around, for instance). They are declared and initialized by the publisher of some data, and shared to subscribers through a header. The topic struct keeps a pointer to the latest data sample published (unfortunately no support for a backlog of messages at time of writing, but if you want to implement this, feel free to send a patch!). The topic also keeps a linked list of subscribers to be notified when new data arrives.

### Channels
In many cases, you may want to publish the same data multiple times. For instance, you might have a `struct temperature_sample_s` that keeps track of data from a temperature sensor. If you're measuring SoC internal temperature and external board temperature via two sensors, you'd want to keep these seperate. For this reason, topics support channels, which allow a publisher or subscriber to differentiate between these different data flows. Internally, this is implemented simply by using arrays to store pointers per channel. The default number of channels provided is `4` per topic; you can change this by editing the value of `MAX_CHANNELS` in `include/pubsub/pubsub.h`.

### Subscribers
Subscribers (`struct pubsub_subscriber_s`) are handles used to, well, subscribe to a topic. When a subscriber is initialized, it has a pointer to the topic it is subscribing to, the channel of data on that subscriber to watch, and is added to the topic's linked list of subscribers. Subscribers also contain a flag (semaphore) keeping track of whether or not there's new data. When new data arrives, the publisher enumerates each of its attached subscribers and updates this flag. Subscribers **DO NOT** store any data - it just notifies the client that new data has arrived, either through manual checking through `pubsub_subscriber_updated()` or the polling API (more on that next). The client code can then choose to copy new data from the provided handle.

### Polling
Checking `pubsub_subscriber_updated()` in a while loop gets old (and is inefficient). Thankfully, the Zephyr kernel provides a *polling* API. This allows client code to poll for new data, which pauses the thread until data has arrived. `pubsub_poll` is a very thin abstraction for the Zephyr kernel `k_poll` function that initializes the required boiler plate and runs the poll.
