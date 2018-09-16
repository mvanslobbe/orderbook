#ifndef COMMON_H
#define COMMON_H

// from the linux kernel
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#endif // COMMON_H
