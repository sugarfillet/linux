// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 * Copyright (C) 2018 Christoph Hellwig
 */

#include <linux/entry-common.h>
#include <linux/interrupt.h>
#include <linux/irqchip.h>
#include <linux/seq_file.h>
#include <asm/smp.h>
#include <asm/vmap_stack.h>
#include <asm/softirq_stack.h>

#ifdef CONFIG_IRQ_STACKS
static DEFINE_PER_CPU(ulong *, irq_stack_ptr);

#ifdef CONFIG_VMAP_STACK
static void init_irq_stacks(void)
{
	int cpu;
	ulong *p;

	for_each_possible_cpu(cpu) {
		p = arch_alloc_vmap_stack(IRQ_STACK_SIZE, cpu_to_node(cpu));
		per_cpu(irq_stack_ptr, cpu) = p;
	}
}
#else
/* irq stack only needs to be 16 byte aligned - not IRQ_STACK_SIZE aligned. */
DEFINE_PER_CPU_ALIGNED(ulong [IRQ_STACK_SIZE/sizeof(ulong)], irq_stack);

static void init_irq_stacks(void)
{
	int cpu;

	for_each_possible_cpu(cpu)
		per_cpu(irq_stack_ptr, cpu) = per_cpu(irq_stack, cpu);
}
#endif /* CONFIG_VMAP_STACK */

#ifndef CONFIG_PREEMPT_RT
static void do_riscv_softirq(struct pt_regs *regs)
{
	__do_softirq();
}

void do_softirq_own_stack(void)
{
	ulong *sp = per_cpu(irq_stack_ptr, smp_processor_id());

	call_on_stack(NULL, sp, do_riscv_softirq, 0);
}
#endif /* CONFIG_PREEMPT_RT */

#else
static void init_irq_stacks(void) {}
#endif /* CONFIG_IRQ_STACKS */

int arch_show_interrupts(struct seq_file *p, int prec)
{
	show_ipi_stats(p, prec);
	return 0;
}

void __init init_IRQ(void)
{
	init_irq_stacks();
	irqchip_init();
	if (!handle_arch_irq)
		panic("No interrupt controller found.");
}

static void noinstr handle_riscv_irq(struct pt_regs *regs)
{
	struct pt_regs *old_regs;

	irq_enter_rcu();
	old_regs = set_irq_regs(regs);
	handle_arch_irq(regs);
	set_irq_regs(old_regs);
	irq_exit_rcu();
}

asmlinkage void noinstr do_riscv_irq(struct pt_regs *regs)
{
	irqentry_state_t state = irqentry_enter(regs);
#ifdef CONFIG_IRQ_STACKS
	ulong *sp = per_cpu(irq_stack_ptr, smp_processor_id());

	if (on_thread_stack())
		call_on_stack(regs, sp, handle_riscv_irq, 0);
	else
#endif
		handle_riscv_irq(regs);

	irqentry_exit(regs, state);
}
