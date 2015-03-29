#include <defs.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <kdebug.h>
#include <picirq.h>
#include <trap.h>
#include <clock.h>
#include <intr.h>
#include <pmm.h>
#include <vmm.h>
#include <ide.h>
#include <swap.h>
#include <proc.h>
#include <fs.h>
#include "../mp/mp.h"
#include "kmalloc.h"

#include <kmonitor.h>

int kern_init(void) __attribute__((noreturn));
void grade_backtrace(void);
static void lab1_switch_test(void);

int
kern_init(void) {
    extern char edata[], end[];
    memset(edata, 0, end - edata);
    
    cons_init();                // init the console


    const char *message = "(THU.CST) os is loading ...";
    cprintf("%s\n\n", message);

    print_kerninfo();

    grade_backtrace();

    pmm_init();                 // init physical memory management
    cprintf("pmm_init done\n");
    mpinit();//mp
    cprintf("mpinit done\n");


    lapicinit();//mp

    cprintf("lapicinip start done\n");

    pic_init();                 // init interrupt controller

    cprintf("pic_init_done\n");

    ioapicinit();//mp

    cprintf("ioapicinit done\n");

    idt_init();


    startothers();   // start other processors

    while(1);
    cprintf("idt init done\n");
    vmm_init();                 // init virtual memory management

    cprintf("vmm_init done\n");
    sched_init();               // init scheduler

    cprintf("sched_init done\n");
    proc_init();                // init process table

    cprintf("proc_init done\n");
    
    ide_init();                 // init ide devices

    cprintf("ide_init done\n");
    swap_init();                // init swap

    cprintf("swap_init done\n");
    fs_init();                  // init fs


/*
    cprintf("fs_init done\n");
    clock_init();               // init clock interrupt

    cprintf("clock_init done\n");
    intr_enable();              // enable irq interrupt

    cprintf("intr_enable done\n");
    */

    extern int ismp;
    if(!ismp)
        clock_init();           // init clock interrupt
    startothers();   // start other processors
    
    mpmain();                 // run idle process
}

void __attribute__((noinline))
grade_backtrace2(int arg0, int arg1, int arg2, int arg3) {
    mon_backtrace(0, NULL, NULL);
}

void __attribute__((noinline))
grade_backtrace1(int arg0, int arg1) {
    grade_backtrace2(arg0, (int)&arg0, arg1, (int)&arg1);
}

void __attribute__((noinline))
grade_backtrace0(int arg0, int arg1, int arg2) {
    grade_backtrace1(arg0, arg2);
}

void
grade_backtrace(void) {
    grade_backtrace0(0, (int)kern_init, 0xffff0000);
}

static void
lab1_print_cur_status(void) {
    static int round = 0;
    uint16_t reg1, reg2, reg3, reg4;
    asm volatile (
            "mov %%cs, %0;"
            "mov %%ds, %1;"
            "mov %%es, %2;"
            "mov %%ss, %3;"
            : "=m"(reg1), "=m"(reg2), "=m"(reg3), "=m"(reg4));
    cprintf("%d: @ring %d\n", round, reg1 & 3);
    cprintf("%d:  cs = %x\n", round, reg1);
    cprintf("%d:  ds = %x\n", round, reg2);
    cprintf("%d:  es = %x\n", round, reg3);
    cprintf("%d:  ss = %x\n", round, reg4);
    round ++;
}

static void
lab1_switch_to_user(void) {
    //LAB1 CHALLENGE 1 : TODO
}

static void
lab1_switch_to_kernel(void) {
    //LAB1 CHALLENGE 1 :  TODO
}

static void
lab1_switch_test(void) {
    lab1_print_cur_status();
    cprintf("+++ switch to  user  mode +++\n");
    lab1_switch_to_user();
    lab1_print_cur_status();
    cprintf("+++ switch to kernel mode +++\n");
    lab1_switch_to_kernel();
    lab1_print_cur_status();
}

//warning! smp!

void __attribute__((noreturn))
mpenter(void)
{
    cprintf("ap: a newcomer in mpenter\n");
    boot_pgdir[0] = boot_pgdir[PDX(KERNBASE)];
    cprintf("ap: before paging\n");
    enable_paging();
    cprintf("ap: enable paging done\n");
    gdt_init();
    cprintf("ap: gdt_init done\n");
    boot_pgdir[0] = 0;
    cprintf("set boot_pgdir back\n");
    lapicinit();
    cprintf("ap: lapicinit done\n");
    idt_init();                 // init interrupt descriptor table

    cprintf("ap: idt init done\n");
    mpmain();
}

// Common CPU setup code.
void __attribute__((noreturn))
mpmain(void)
{
    cprintf("cpu%d: starting\n", cpu->id);
    xchg(&cpu->started, 1); // tell startothers() we're up
    while(1);
    cpu_idle();     // start running processes
}


// Start the non-boot (AP) processors.
void
startothers(void)
{
    cprintf("startothers");
    extern uint8_t _binary_obj_entryother_o_start[], _binary_obj_entryother_o_size[];
    uint8_t *code;
    struct cpu *c;
    char *stack;

    // Write entry code to unused memory at 0x7000.
    // The linker has placed the image of entryother.S in
    // _binary_entryother_obj_start.
    code = KADDR(0x7000);
    memmove(code, _binary_obj_entryother_o_start, (uint32_t) _binary_obj_entryother_o_size);

    for(c = cpus; c < cpus+ncpu; c++){
        if(c == cpus+cpunum())  // We've started already.
            continue;

        // Tell entryother.S what stack to use, where to enter, and what
        // pgdir to use. We cannot use kpgdir yet, because the AP processor
        // is running in low  memory, so we use entrypgdir for the APs too.
        stack = kmalloc(KSTACKSIZE);
        *(void**)(code-4) = stack + KSTACKSIZE;  //kva
        *(void**)(code-8) = mpenter; //kva
        *(int**)(code-12) = (void *)boot_cr3; //useless here..

        lapicstartap(c->id, PADDR(code));

        // wait for cpu to finish mpmain()
        while(c->started == 0)
            ;
    }
}