#lab6实验报告
2012011282 计22 王天一
##练习1 使用 Round Robin 调度算法（不需要编码
###请理解并分析sched_calss中各个函数指针的用法，并接合Round Robin 调度算法描ucore的调度执行过程
``init``在初始化时使用，具体来说是从kern_init的sched_init()时被使用的。RR调度中它被用来初始化链表和进程数。  
`RR_enqueue`将一个进程加入runable队列。它在新进程产生，进程被从running状态由于任务切换或者自己yield而进入runnable状态是被调用。  
`RR_dequeue`将一个进程从runable队列中取出来，它在proc_run中被调用，取出优先级最高的进程。  
`RR_pick_next`获得下一个优先级最高的进程，也是在proc_run中被调用，被取出的进程将马上被执行。  
`RR_proc_tick`将降低当前进程的剩余时间，在run_timer_list中被使用，实际上就是时钟tick变化时被使用。  
RR调度执行过程：每一个进程加入runnable队列时设定time_slice，每一个tick time_slice减一，当time_slice为0是need_reschedule，然后下一个时钟中断进schedule，取出runnable队头，把自己插到尾部，执行队头进程。
###请在实验报告中简要说明如何设计实现”多级反馈队列调度算法“，给出概要设计，鼓励给出详细设计
在scheduler中对于多个优先级设定多个队列。proc_struct中设定该进程当前的优先级。每一个进程初始情况下加入最高优先级队列，time_slice为1。每次time_tick时讲当前进程剩余时间片-1，如果为0则将其加入低一级优先级队列，重新寻找优先级最高的进程执行。否则继续执行当前进程。如果一个进程yield的话就将其优先级重置为最高优先级。
##练习2
###设计实现过程
注释给的太详细了，但是不争气的我第一次还是写错了。没有判断priority=0的情况，heap_init的时候直接调用了skew_heap_init，其实应该设成NULL才对。  
以上是我调了半天过不了之后去看答案的感慨，后来我发现lab6_result也过不了make grade，我也不知道我写的对不对。