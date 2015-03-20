#Lab4实验报告
2012011282 计22 王天一
##练习1
###设计实现过程
给pid赋－1，给state赋PROC_UNINIT，给cr3赋boot_cr3，其他全部赋0.
###问题
>请说明proc_struct中struct context context和struct trapframe *tf成员变量含义和在本实验中的作用是啥？（提示通过看代码和编程调试可以判断出来）  

context是在通过schedule完成任务切换时保存并转移现场用的。在本实验中用来保存forkret开始执行时的所有现场的。switch_to会为forkret准备环境并使用ret跳转。
tf是用来从中断返回的。它的低字节是ucore定义的，高字节是x86定义的。使用iret是就会执行终端返回机制，使得进程切换似乎是从中断返回一样。再次实验中执行iret之后就会切换到kernel_thread_entry函数，该函数调用新线程的入口。
##练习2
###设计实现过程
首先给创建proc_struct,然后给内核栈分配页面，然后设置线程的入口为forkret，设置trapframe，然后将新线程加入链表并唤醒。
###问题
>请说明ucore是否做到给每个新fork的线程一个唯一的id？请说明你的分析和理由。

做到了，get_pid函数实现了寻找没人用过的pid。
##练习3
>阅读代码，理解 proc_run 函数和它调用的函数如何完成进程切换的。
>请在实验报告中简要说明你对proc_run函数的分析。  

proc_run首先设定新proc为current proc，然后将tss的esp设定为next->kstack + KSTACKSIZE，这个地址实际上是trapframe的末地址。然后使用lcr3装载新线程的页表，最后使用switch_to进行context切换。switch_to会将forkret的参数放进栈中，然后将栈切换到新线程的栈空间中，然后使用ret调用forkret。forkret将非x86定义的寄存器从trapframe中弹出，然后调用iret切换到trapframe描述的状态中。

>在本实验的执行过程中，创建且运行了几个内核线程？

2个，idle_proc和init_proc。
>语句local_intr_save(intr_flag);....local_intr_restore(intr_flag);在这里有何作用?请说明理由

禁止中断，因为正在修改全局的链表，此时中断的话其他内核线程执行会使得执行混乱。