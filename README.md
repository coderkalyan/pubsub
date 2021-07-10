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
TODO

## Porting
To port pubsub to another framework, you will have to provide the following interfaces:
* a C heap implementation (malloc/free)
* a linked list implementation
* semaphores/mutexes for multithreading
* (optional) a kernel polling API for poll based usage
* a high resolution timestamping implementation is very useful for usage. Microsecond resolution is preferred but millisecond resolution is sufficient.
