#LAB3实验报告
2012011282 计22 王天一
##练习1
###设计实现过程
首先检查虚拟地址对应的PTE，如果PTE为0说明虚拟页对应的物理页不存在，需要分配一个新物理页以供使用。否则尝试将对应的物理页换进来，并且通知页替换管理器“一个新的可换页进来了”。
###问题
>请描述页目录项（Pag Director Entry）和页表（Page Table Entry）中组成部分对ucore实现页替换算法的潜在用处。   

PDE由mm_struct持有，为当前程序所持有的页目录。PTE在页替换算法中起到指示该页是否在内存中以及在硬盘的哪个扇区中的作用。PDE和PTE所控制的权限和page fault的错误码对照，以便内核识别是缺页还是非法访问。
>如果ucore的缺页服务例程在执行过程中访问内存，出现了页访问异常，请问硬件要做哪些事情？  　

缺页服务历程应该不在可交换的页索引中，所以不能出现页访问异常。
##练习2
###设计实现过程
####`_fifo_map_swappable`函数
只需简单地将传入的页添加进可替换表的末尾即可，swap_in参数在这个策略中并没有用。
####`_fifo_swap_out_victim`函数
将可替换链表中的第一个元素取出来，从链表中移除并将该页的地址返回即可。
###问题
>如果要在ucore上实现"extended clock页替换算法"请给你的设计方案，现有的swap_manager框架是否足以支持在ucore中实现此算法？如果是，请给你的设计方案。如果不是，请给出你的新的扩展和基此扩展的设计方案。并需要回答如下问题
需要被换出的页的特征是什么？ 
在ucore中如何判断具有这样特征的页？
何时进行换入和换出操作？  

是。CPU会自动维护pte的访问位和修改位，我们只需要修改`_fifo_swap_out_victim`即可。
其实现方式为遍历一遍链表，寻找pte中修改位和访问位都为0的，然后返回。如果没有找到继续按照优先级重新寻找。这样至多遍历4次后即可找到一个被替换的页面。