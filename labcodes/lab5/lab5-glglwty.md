#lab5实验报告
##练习1: 加载应用程序并执行（需要编码）
###设计实现过程
按照注释逐句翻译
###问题
>请在实验报告中描述当创建一个用户态进程并加载了应用程序后，CPU是如何让这个应用程序最终在用户态执行起来的。即这个用户态进程被ucore选择占用CPU执行（RUNNING态）到具体执行应用程序第一条指令的整个经过。  

该进程进入running状态后，在某次schedule后执行，进入user_main函数，user_main执行系统调用sys_exec，然后将用户程序装载入内存并更新自己的页表，在内核态将自己的trapframe，cs，ds，es，ss全部设定为用户态段选择子，将eip设置为用户函数入口，然后处理中断完成执行iret直接切入用户态开始执行。
##练习2: 父进程复制自己的内存空间给子进程（需要编码）
###设计实现过程
一共就两行，照着注释写就行了。
###问题
>请在实验报告中简要说明如何设计实现”Copy on Write 机制“，给出概要设计，鼓励给出详细设计。  

首先在fork是将父子进程的页表设定为shared状态，即共用一个mm。使用pte中的一个系统保留位来标记这种fork后没有修改的状态。然后将pte全部设定为没有写权限。这样有人要写这些地方时就会page fault，这时候处理复制的问题，并重置页表。
##练习3: 阅读分析源代码，理解进程执行 fork/exec/wait/exit 的实现，以及系统调用的实现（不需要编码）
>请在实验报告中简要说明你对 fork/exec/wait/exit函数的分析。并回答如下问题：
请分析fork/exec/wait/exit在实现中是如何影响进程的执行状态的？
请给出ucore中一个用户态进程的执行状态生命周期图（包执行状态，执行状态之间的变换关系，以及产生变换的事件或函数调用）。（字符方式画即可）

###我对这些函数的分析
fork函数会创建一个一模一样的进程。  
exec会讲自己替换为指定的用户进程，偷梁换柱，不论自己是否是内核进程。  
wait会等待某件事，比如我儿子变成僵尸了，有人要杀我之类的，他会讲自己变成sleeping并用schedule去执行其他任务。  
exit会让自己变成僵尸并且如果父亲在等我就把他叫醒，如果自己还有儿子的话把它们交给initproc，如果儿子里面有僵尸的话把initproc也叫醒。  
###生命周期图
```painting
nothing
|  syscall(sys_fork) by parent
|  
V  
[kernel runnable]
|scedule by whoever
|
V
[kernel running]
|syscall(sys_exec, ...)
| 
V
[user running]
|syscall(sys_wait, ...)
|
V
[user sleeping]
|a child called syscall(sys_exit)
|someone called kill or message passing or some interruption
|kernel want you to wake up
|wakeup_proc()
V
[user runnable]
|schedule by whoever
|
V
[user running]
|syscall(sys_exit, ...)
|
V
[user zombie]
|removed by parent if he is waiting, or initproc otherwise.
|
V
nothing

```