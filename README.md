# Cimple Ring Buffer

Simple, yet somewhat efficient ring buffer in C written in "collaboration" with neural networks because why not. Not intended to use by anyone except me. So, please don't use it. Please? OK, maybe one day I'll make it usable.

## Why?

To use it in custom QMK code for my keyboard. And for memes.

## Maybe it's usable now?

I don't know? Anyway, there are 3 settings you can do to tweak the behavior. Do note that by default you need to use capacity that equals to power of two.

```c
// Define to preserve elements order during element removal
// Will cost additional time
#ifndef CIMPLE_RING_BUFFER_PRESERVE_ORDER
#define CIMPLE_RING_BUFFER_PRESERVE_ORDER 0
#endif

// Define to allow overwriting of oldest data when buffer is full.
// If not defined, ring_buffer_push will fail when the buffer is full.
#ifndef CIMPLE_RING_BUFFER_ALLOW_OVERWRITE
#define CIMPLE_RING_BUFFER_ALLOW_OVERWRITE 1
#endif

// Define to optimize modulo operations when capacity is a power of two.
// When enabled, capacity must be a power of two.
#ifndef CIMPLE_RING_BUFFER_POWER_OF_TWO_OPTIMIZATION
#define CIMPLE_RING_BUFFER_POWER_OF_TWO_OPTIMIZATION 1
#endif
```
