/*
 * $QNXLicenseC:
 * Copyright 2007, 2009, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable license fees to QNX
 * Software Systems before you may reproduce, modify or distribute this software,
 * or any work that includes all or part of this software.   Free development
 * licenses are available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review this entire
 * file for other proprietary rights or license notices, as well as the QNX
 * Development Suite License Guide at http://licensing.qnx.com/license-guide/
 * for other information.
 * $
 */

#ifndef __SYSPAGE_H_INCLUDED
#define __SYSPAGE_H_INCLUDED

#ifndef __PLATFORM_H_INCLUDED
#include <sys/platform.h>
#endif

#define SYSPAGE_VERSION_MAJOR   2
#define SYSPAGE_VERSION_MINOR   0

__BEGIN_DECLS

#if defined(__CLOCKADJUST)
struct _clockadjust __CLOCKADJUST;
#undef __CLOCKADJUST
#endif

struct _process_local_storage;
struct syspage_entry;

extern struct syspage_entry				*_syspage_ptr;

typedef struct {
	_Uint16t		entry_off;
	_Uint16t		entry_size;
} syspage_entry_info;

typedef struct {
	_Uint16t		entry_off;
	_Uint16t		entry_size;
	_Uint16t		element_size;
} syspage_array_info;

/*
 * Include CPU specific definition for network queries.
 */
#if	!defined(SYSPAGE_TARGET_ALL)				\
	&& !defined(SYSPAGE_TARGET_X86_64)			\
	&& !defined(SYSPAGE_TARGET_AARCH64)
#if defined(__X86_64__)
#define SYSPAGE_TARGET_X86_64
#elif defined(__aarch64__)
#define SYSPAGE_TARGET_AARCH64
#else
#error not configured for system
#endif
#endif

#if defined(SYSPAGE_TARGET_ALL) || defined(SYSPAGE_TARGET_X86_64)
#include _NTO_HDR_(x86_64/syspage.h)
#endif
#if defined(SYSPAGE_TARGET_ALL) || defined(SYSPAGE_TARGET_AARCH64)
#include _NTO_HDR_(aarch64/syspage.h)
#endif

struct cpupage_entry {
	_Uint32t								cpu;
	_Uint32t								state;
	union {
#if defined(SYSPAGE_TARGET_ALL) || defined(SYSPAGE_TARGET_AARCH64)
		struct aarch64_cpupage_entry		aarch64;
#endif
		struct {
			_Uint32t	dummy[8];
		}									dummy;
	} un;
	struct _process_local_storage			*pls;

	_Uint32t								spare[8];
};

enum {
	CPUPAGE_ADDR = -1,	/* R */
	CPUPAGE_TLS = 0,	/* R */
	CPUPAGE_PLS,		/* RW */
	CPUPAGE_SYSPAGE,	/* R */
	CPUPAGE_CPU,		/* R */
	CPUPAGE_STATE,		/* R */
	CPUPAGE_MAX
};

/*
 *  Architecture independent system page definitions
 */

/*
 *	System page types
 */
enum {
    /* technically not needed */
    SYSPAGE_64BIT = 0x100,
    /* syspage now has a version field */
    SYSPAGE_VERSIONED = 0x1000,
    SYSPAGE_X86_64 = (_Uint16t)SYSPAGE_VERSIONED | (_Uint16t)SYSPAGE_64BIT,
    SYSPAGE_AARCH64,
};

#define QTIME_FLAG_CHECK_STABLE			0x00000002u
#define QTIME_FLAG_TICKLESS				0x00000004u
#define QTIME_FLAG_GLOBAL_CLOCKCYCLES	0x00000008u /* ClockCycles() is synchronized between all processors */

struct qtime_entry {
    /**
     * Cycles-per-second, for ClockCycles()
     */
    _Uint64t                    cycles_per_sec;

    /**
     * Time-of-day adjustment for CLOCK_REALTIME/CLOCK_SOFTTIME and their timers.
     */
    _Uint64t volatile           nsec_tod_adjust;

    /**
     * Number of nanoseconds-per-tick: the tick period.
     */
    _Uint32t                    nsec_inc;

    /**
     * UTC seconds when machine booted.
     */
    _Uint32t                    boot_time;

    /**
     * Incremental CLOCK_REALTIME adjustment in progress (ClockAdjust()).
     */
    struct _clockadjust         adjust;

    /**
     * times 10^timer_scale
     */
    _Uint32t                    timer_period;

    /**
     * Exponent for timer_period.
     */
    _Int32t                     timer_scale;

    /**
     * Interrupt number for CPU0.
     */
    _Int32t                     intr;

    /**
     * 1970.
     */
    _Uint32t                    epoch;

    /**
     * QTIME_FLAG_* flags.
     */
    _Uint32t                    flags;

    /**
     * Round-robin interval in ticks.
     */
    _Uint32t                    rr_interval_mul;

    /**
     * The max absolute value that can be written in the the hardware timer's
     * comparator. For correct operation, this value must never be reached
     * during the system's lifetime.
     */
    _Uint64t                    timer_load_max;

    /**
     * Boot time in clock cycles. This is the epoch for all calculations done
     * on time in clock cycles.
     */
    _Uint64t                    boot_cc;

    /**
     * Exact tick period used by the kernel, in clock cycles.
     *
     * The kernel computes the closest tick period value that can be
     * represented in clock cycles from the tick frequency passed to procnto.
     * Depending on the frequency requested and the hardware CPS, it is
     * possible that corresponding value in nanoseconds cannot be represented
     * as an integer number of nanoseconds. When that is the case, 'nsec_inc'
     * is only an approximation in nanoseconds of tick_period_cc.
     * This can happen if the kernel is configured with a weird tick frequency
     * (e.g. the system configuration passing -C 347 to procnto) that is not a
     * factor of a sensible CPS, or if the CPS itself is weird (e.g.
     * startup-x86 calibration finding the CPS to be 1.804807028 GHz).
     */
    _Uint64t                    tick_period_cc;

    _Uint64t                    spare[3];
};

struct intrspin;

struct debug_callout {
	void	(*display_char)(struct syspage_entry *__syspage, char __char);
	int		(*poll_key)(struct syspage_entry *__syspage);
	int		(*break_detect)(struct syspage_entry *__syspage);
	void	*spare;
};

typedef enum {
	DEBUG_WATCHDOG_STOP = 0,
	DEBUG_WATCHDOG_CONTINUE,
	DEBUG_WATCHDOG_FEED
} debug_watchdog_cmd;

struct callout_entry {
	void					(*reboot)(struct syspage_entry *__syspage, int __flags);
	struct debug_callout	debug[2];
	void					(*debug_watchdog)(struct syspage_entry *__syspage, debug_watchdog_cmd __cmd);

	_Uint64t 				(*custom)(struct syspage_entry *__syspage, _Uint32t __type_id, _Uint64t *__regs);

	_Uint64t	spare[2];
};

#define AS_NULL_OFF			((_Uint16t)-1)

#define AS_ATTR_KIDS		0x0010u
#define AS_ATTR_CONTINUED	0x0020u

#define AS_PRIORITY_DEFAULT	100

struct asinfo_entry {
	_Uint64t			start;
	_Uint64t			end;
	_Uint16t			owner;
	_Uint16t			name;
	_Uint16t			attr;
	_Uint16t			priority;
};

struct hwinfo_entry {
	__FLEXARY(_Uint32t, data); /* data[] variable sized, see <hw/sysinfo.h> */
};

/**
 * Reboot in case of an abnormal shutdown.
 * This flag is set by passing the -A option to startup.
 * While not the default behaviour of startup, it should be
 * used for safe systems.
 */
#define SYSTEM_PRIVATE_FLAG_ABNORMAL_REBOOT	0x00000001u
#define SYSTEM_PRIVATE_FLAG_TRACE_ACTIVE	0x00000004u
#define SYSTEM_PRIVATE_FLAG_ETB_RUNNING		0x00000008u

struct system_private_entry {
	_Uint32t						cpupage_spacing;
	_Uint32t						private_flags;
	_Uint32t						pagesize;
	_Uint32t						num_ready;
    _Uint32t                        traced_classes;
	_Uint32t						spare;
	_Paddr64t						kdump_info;
	struct cpupage_entry			*user_cpupageptr;
	struct syspage_entry			*user_syspageptr;
	struct cpupage_entry			*kern_cpupageptr;
	struct syspage_entry			*kern_syspageptr;
	struct kdebug_info				*kdebug_info;
	struct kdebug_callback			*kdebug_call;
    _Uint64t						entropy[2];
};

/*
 * System independent CPU flags (system dependent grow from bottom up)
 */
#define CPU_FLAG_FPU		(1U <<  31)  /* CPU has floating point support */
#define CPU_FLAG_MMU		(1U <<  30)  /* MMU is active */

struct cpuinfo_entry {
	_Uint32t		cpu;
	_Uint32t		speed;
	_Uint64t		flags;
	_Uint64t		smp_hwcoreid;
	_Uint32t		spare;
	_Uint16t		name;
	_Uint8t			ins_cache;
	_Uint8t			data_cache;
};

#define CACHE_LIST_END		0xffU	/* terminate a cache hierarchy list */

/*
 * Cache flags
 */
#define CACHE_FLAG_INSTR	0x0001U	/* cache holds instructions */
#define CACHE_FLAG_DATA		0x0002U	/* cache holds data */
#define CACHE_FLAG_UNIFIED	0x0003U	/* cache holds both ins & data */
#define CACHE_FLAG_SHARED	0x0004U	/* cache is shared between multiple  */
									/* processors in an SMP system */
#define CACHE_FLAG_SNOOPED	0x0008U	/* cache implements a bus snooping */
									/* protocol */
#define CACHE_FLAG_VIRT_TAG	0x0010U	/* cache is virtually tagged */
#define CACHE_FLAG_VIRTUAL	0x0010U	/* backwards compatability flag for above */
#define CACHE_FLAG_WRITEBACK 0x0020U/* cache does writeback, not writethru */
#define CACHE_FLAG_CTRL_PHYS 0x0040U/* control function takes 32-bit paddrs */
#define CACHE_FLAG_SUBSET	0x0080U	/* this cache obeys the 'subset' property */
#define CACHE_FLAG_NONCOHERENT 0x0100U /* cache is non-coherent on SMP */
#define CACHE_FLAG_NONISA	0x0200U /* cache doesn't obey ISA cache instr */
#define CACHE_FLAG_NOBROADCAST 0x0400U /* cache ops aren't broadcast on bus for SMP */
#define CACHE_FLAG_VIRT_IDX	0x0800U	/* cache is virtually indexed */
#define CACHE_FLAG_CTRL_PHYS64 0x1000U /* control function takes 64-bit paddrs */

struct cacheattr_entry;
typedef unsigned __cache_rtn(_Paddr64t, unsigned, int, struct cacheattr_entry *, volatile struct syspage_entry *);
struct cacheattr_entry {
	_Uint32t	next;
	_Uint32t	line_size;
	_Uint32t	num_lines;
	_Uint32t	flags;
	_Uint16t	ways;
	_Uint16t	spare0[3];
	__cache_rtn	*control;
};

struct typed_strings_entry {
	char				data[4]; /* variable size */
};

struct strings_entry {
	char				data[4]; /* variable size */
};

/*
 * System independent interrupt flags
 * (system dependent grow from bottom up).
 */
#define INTR_FLAG_CPU_FAULT				0x2000U
/* INTR_FLAG_MSI allows a range of vectors to be flagged for use as MSI vectors */
#define INTR_FLAG_MSI                   0x0400U

/*
 * System independent interrupt code gen flags
 * (system dependent grow from bottom up).
 */
#define INTR_GENFLAG_LOAD_SYSPAGE		0x8000U
#define INTR_GENFLAG_LOAD_INTRINFO		0x4000U
#define INTR_GENFLAG_LOAD_LEVEL			0x2000U
#define INTR_GENFLAG_LOAD_INTRMASK		0x1000U
#define INTR_GENFLAG_NOGLITCH			0x0800U
#define INTR_GENFLAG_LOAD_CPUNUM		0x0400U
#define INTR_GENFLAG_ID_LOOP			0x0200U

struct __intrgen_data {
	_Uint16t	genflags;
	_Uint16t	size;
	_Uint32t	spare;
	void		(*rtn)(void);
};

#define INTR_CONFIG_FLAG_PREATTACH	0x0001U
#define INTR_CONFIG_FLAG_DISALLOWED	0x0002U
#define INTR_CONFIG_FLAG_IPI		0x0004U
/* system wide interrupt, not per-cpu*/
#define INTR_CONFIG_FLAG_SYS		0x0008U

/**
 * Describes an interrupt controller and its relationship to other interrupt
 * controllers. Controllers may be cascaded and form a tree-like structure.
 *
 * An instance of this structure is initialized by startup for every interrupt
 * controller attached to the system.
 */
struct intrinfo_entry {
	/**
	 * Logical interrupt vector used as the lower bound for the set of
	 * interrupts handled by this controller.
	 */
	_Uint32t				vector_base;

	/**
	 * Number of logical interrupt vectors handled by this controller. The upper
	 * bound of interrupts handled by this controller is `vector_base +
	 * num_vectors`.
	 */
	_Uint32t				num_vectors;

	/**
	 * Logical interupt vector that this controller cascades into, i.e. the
	 * logical interrupt vector that this controller is a child of.
	 */
	_Uint32t				cascade_vector;

	/**
	 * Base hardware interrupt vector for this controller. The meaning of this
	 * value is architecture dependent; for example, on x86-64 it corresponds to
	 * the base IDT entry for this controller while on aarch64 it is unused.
	 */
	_Uint32t				cpu_intr_base;

	/**
	 * Spacing between interrupt vector entries when using autovectoring.
	 */
	_Uint16t				cpu_intr_stride;

	/**
	 * An OR of `INTR_FLAG_*` values.
	 */
	_Uint16t				flags;

	/**
	 * Code fragment that identifies the source of this interrupt.
	 */
	struct __intrgen_data	id;

	/**
	 * Code fragment that signals the end of interrupt handling.
	 */
	struct __intrgen_data	eoi;

	/**
	 * A kernel callout to mask the interrupt source at the controller.
	 */
	int (*mask)(struct syspage_entry * spp, int id);

	/**
	 * A kernel callout to unmask the interrupt source at the controller.
	 */
	int (*unmask)(struct syspage_entry * spp, int id);

	/**
	 * A kernel callout to query the configuration of  the interrupt source at the controller.
	 */
	unsigned (*config)(struct syspage_entry * spp, struct intrinfo_entry * iip, int id);

	/**
	 * The per-CPU stride of this interrupt controller. If set to zero then this
	 * controller does not describe per-CPU interrupts. If non-zero then this
	 * controller does describe per-CPU interrupts and the value is the stride
	 * to be added to `vector_base` per-CPU. Example: a system with four CPU's, a
	 * `vector_base` of 0x300, and `local_stride` set to 0x100 would have the
	 * per-CPU interrupts for CPU3 start at logical interrupt vector 0x600.
	 */
	_Uint32t				local_stride;

	/**
	 * Reserved.
	 */
	_Uint32t				spare[3];
};

struct smp_entry {
	_Uint32t			pending;
	_Uint32t			spare;
	void                (*send_ipi)(struct syspage_entry * __syspage,
                                _Uint64t __target, unsigned __cmd, unsigned *__cmdsp);
	void                *start_addr;
};

struct hypinfo_entry {
    _Uint64t            flags;
};
#define HYPINFO_ENABLED 0x1UL

struct cluster_entry {
	_Uint64t			cpumask; /* bitmask of the CPUs included in a cluster */
	_Uint16t			name;
	_Uint16t			spare[3];
};
#define SYSTEM_CLUSTER_NAME_PREFIX '_'


struct syspage_entry {
	_Uint16t			size;		/* size of syspage_entry */
	_Uint16t			total_size;	/* size of system page */
	_Uint16t			type;
	_Uint16t			num_cpu;
	syspage_entry_info	system_private;
	struct {
		_Uint16t    major;
		_Uint16t    minor;
	}					version;
	syspage_entry_info	hwinfo;
	syspage_entry_info	qtime;
	syspage_entry_info	callout;
	syspage_entry_info	typed_strings;
	syspage_entry_info	strings;
	syspage_entry_info	smp;
	union {
#if defined(SYSPAGE_TARGET_ALL) || defined(SYSPAGE_TARGET_X86_64)
		struct x86_64_syspage_entry	x86_64;
#endif
#if defined(SYSPAGE_TARGET_ALL) || defined(SYSPAGE_TARGET_AARCH64)
		struct aarch64_syspage_entry	aarch64;
#endif
		_Uint64t					filler[20];
	} un;
	syspage_array_info  asinfo;
	syspage_array_info  cpuinfo;
	syspage_array_info  cacheattr;
	syspage_array_info  intrinfo;
	syspage_entry_info  hypinfo;
	syspage_array_info	cluster;
};

/* <STAN_MACRO5> */
#define _SYSPAGE_ENTRY_SIZE( __base, __field )	\
	((__base)->__field.entry_size)
#define _SYSPAGE_ELEMENT_SIZE( __base, __field )	\
    ((__base)->__field.element_size)

#define SYSPAGE_ENTRY_SIZE( __field )       (_SYSPAGE_ENTRY_SIZE( _syspage_ptr, __field ))

#define SYSPAGE_ELEMENT_SIZE(__field)		(_SYSPAGE_ELEMENT_SIZE(_syspage_ptr, __field))

#define _SYSPAGE_ENTRY( __base, __field )								\
	((struct __field##_entry *)(void *)((char *)(__base) + (__base)->__field.entry_off))

#define _SYSPAGE_CPU_ENTRY_SIZE( __base, __cpu, __field )					\
	((__base)->un.__cpu.__field.entry_size)
#define _SYSPAGE_CPU_ENTRY( __base, __cpu, __field )					\
	((struct __cpu##_##__field##_entry *)(void *)((char *)(__base) + (__base)->un.__cpu.__field.entry_off))

#define SYSPAGE_ENTRY( __field )			(_SYSPAGE_ENTRY( _syspage_ptr, __field ))
#define SYSPAGE_CPU_ENTRY( __cpu, __field )	(_SYSPAGE_CPU_ENTRY( _syspage_ptr, __cpu, __field ))

#define SYSPAGE_CPU_ENTRY_SIZE(__cpu, __field) (_SYSPAGE_CPU_ENTRY_SIZE(_syspage_ptr, __cpu, __field))

#define SYSPAGE_ARRAY_ADJ_OFFSET(__field, __p, __adj_off)	\
				((struct __field##_entry *)((_Uintptrt)(__p) + (__adj_off)))
#define _SYSPAGE_ARRAY_IDX(__base, __field, __idx)	\
				(SYSPAGE_ARRAY_ADJ_OFFSET(__field, _SYSPAGE_ENTRY((__base), __field), (__idx) * _SYSPAGE_ELEMENT_SIZE((__base), __field)))
#define SYSPAGE_ARRAY_IDX(__field, __idx)	(_SYSPAGE_ARRAY_IDX(_syspage_ptr, __field,( __idx)))
/* </STAN_MACRO5> */

__END_DECLS

#endif /* __SYSPAGE_H_INCLUDED */
