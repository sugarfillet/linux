/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 */

#ifndef _ASM_RISCV_IRQ_H
#define _ASM_RISCV_IRQ_H

#include <linux/interrupt.h>
#include <linux/linkage.h>

#include <asm-generic/irq.h>

extern void __init init_IRQ(void);
asmlinkage void call_on_stack(struct pt_regs *regs, ulong *sp,
				     void (*fn)(struct pt_regs *), ulong tmp);
asmlinkage void noinstr do_riscv_irq(struct pt_regs *regs);

#endif /* _ASM_RISCV_IRQ_H */
