#ifndef MINITCP_SRC_COMMON_BARRIER_H_
#define MINITCP_SRC_COMMON_BARRIER_H_

static inline void compiler_barrier() { asm volatile("" ::: "memory"); }

static inline void smp_memory_barrier() { asm volatile("mfence" ::: "memory"); }

static inline void smp_read_barrier() { asm volatile("lfence" ::: "memory"); }

static inline void smp_write_barrier() { asm volatile("sfence" ::: "memory"); }

#endif  // ! MINITCP_SRC_COMMON_BARRIER_H_