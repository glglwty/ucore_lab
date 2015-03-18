#Lab1实验报告
2012011282 计22 王天一
##练习1
###1.1 操作系统镜像文件ucore.img是如何一步一步生成的？(需要比较详细地解释Makefile中每一条相关命令和命令参数的含义，以及说明命令导致的结果)
> 生成ucore.img
> ```makefile
> $(UCOREIMG): $(kernel) $(bootblock)
> 	$(V)dd if=/dev/zero of=$@ count=10000
> 	$(V)dd if=$(bootblock) of=$@ conv=notrunc
> 	$(V)dd if=$(kernel) of=$@ seek=1 conv=notruc
> ```
> 可以看到最终步骤为直接将bootblock和kernel拼在一起。 bootblock从第0个扇区开始而kernel从第一个扇区开始。
> > 生成bootblock
> > ```makefile
> > $(bootblock): $(call toobj,$(bootfiles)) | $(call totarget,sign)
> >	@echo + ld $@
> >	$(V)$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o $(call toobj,bootblock)
> >	@$(OBJDUMP) -S $(call objfile,bootblock) > $(call asmfile,bootblock)
> >	@$(OBJCOPY) -S -O binary $(call objfile,bootblock) $(call outfile,bootblock)
> >	@$(call totarget,sign) $(call outfile,bootblock) $(bootblock)
> > ```
> > 首先编译boot目录下的文件（bootasm.s, bootmain.c)，使用的关键参数为`-nostdinc -ggdb`，分别用于禁止标准头文件和生成调试信息。
> >  之后bootasm.o和bootmain.o被ld链接。
> >  ```makefile
> >  ld -m    elf_i386 -nostdlib -N -e start -Ttext 0x7C00  obj/boot/bootasm.o obj/boot/bootmain.o -o obj/bootblock.o
> >  ```
> >  之后使用objcopy生成纯粹的内存拷贝，丢弃所有.o文件中的其他信息。
> >  最后bootblock使用sign生成。sign向bootblock中复制objcopy产生的文件，并且填充规定的字节。  
>  
>  <br>
> > 生成kernel
> > ```makefile
> > $(kernel): $(KOBJS)
> >	@echo + ld $@
> >	$(V)$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ $(KOBJS)
> >	@$(OBJDUMP) -S $@ > $(call asmfile,kernel)
> >	@$(OBJDUMP) -t $@ | $(SED) '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(call symfile,kernel)
> > ```
> > kern文件夹下的所有c代码都编译成.o文件之后，按照kernel.ld的指示链接，kernel.ld指示了各个数据段的内核虚拟地址。
###1.2 一个被系统认为是符合规范的硬盘主引导扇区的特征是什么？
共512字节，第510和511字节分别为0x55和0xAA。
##练习2
###2.1 从CPU加电后执行的第一条指令开始，单步跟踪BIOS的执行。
gdb中反汇编bios的代码结果是错的，不知道为什么。参考答案后使用qemu的参数将执行到的代码输出到log文件中。
```makefile
lab1-mon: $(UCOREIMG)
	$(V)$(TERMINAL) -e "$(QEMU) -S -s -d in_asm -D $(BINDIR)/q.log -monitor stdio -hda $< -serial null"
	$(V)sleep 2
	$(V)$(TERMINAL) -e "gdb -q -x tools/lab1init"
```
之后开始执行，得到如下指令序列： 
```asm
----------------
IN: 
0xfffffff0:  ljmp   $0xf000,$0xe05b

----------------
IN: 
0x000fe05b:  cmpl   $0x0,%cs:0x6c30

----------------
IN: 
0x000fe062:  jne    0xfd34d

----------------
IN: 
0x000fe066:  xor    %ax,%ax

----------------
IN: 
0x000fe068:  mov    %ax,%ss

----------------
IN: 
0x000fe06a:  mov    $0x7000,%esp

----------------
IN: 
0x000fe070:  mov    $0xf2e31,%edx
0x000fe076:  jmp    0xfd1be

----------------
IN: 
0x000fd1be:  mov    %eax,%ecx
0x000fd1c1:  cli    
0x000fd1c2:  cld    
0x000fd1c3:  mov    $0x8f,%eax
0x000fd1c9:  out    %al,$0x70
0x000fd1cb:  in     $0x71,%al
0x000fd1cd:  in     $0x92,%al
0x000fd1cf:  or     $0x2,%al
0x000fd1d1:  out    %al,$0x92
0x000fd1d3:  lidtw  %cs:0x6c20
0x000fd1d9:  lgdtw  %cs:0x6be0
0x000fd1df:  mov    %cr0,%eax
0x000fd1e2:  or     $0x1,%eax
0x000fd1e6:  mov    %eax,%cr0
```
bios再bootloader之前执行了一万多条指令。
###2.2 在初始化位置0x7c00设置实地址断点,测试断点正常。
先把tools/gdbinit中的continue去掉并加入b *0x7c00
执行debug命令```make debug```，qemu开始运行，自动在0x7c00暂停。
###2.3 从0x7c00开始跟踪代码运行,将单步跟踪反汇编得到的代码与bootasm.S和 bootblock.asm进行比较。
多次stepi之后得到如下输出：
```gdb
(gdb) stepi
=> 0x7c01:	cld    
0x00007c01 in ?? ()
(gdb) stepi
=> 0x7c02:	xor    %ax,%ax
0x00007c02 in ?? ()
(gdb) stepi
=> 0x7c04:	mov    %ax,%ds
0x00007c04 in ?? ()
(gdb) stepi
=> 0x7c06:	mov    %ax,%es
0x00007c06 in ?? ()
```
对比bootloader.s中的代码
```asm
cli                                  # Disable interrupts
cld                    # String operations increment
# Set up the important data segment registers (DS, ES, SS).
xorw %ax, %ax                           # Segment number zero
movw %ax, %ds                           # -> Data Segment
movw %ax, %es                           # -> Extra Segment
movw %ax, %ss                           # -> Stack Segment
```
两者是一致的。
###2.4 自己找一个bootloader或内核中的代码位置，设置断点并进行测试。
下面在kern_init处设置断点并继续运行，下面是中断位置的反汇编。
```gdb
          kern_init:
00100000:   push %ebp
00100001:   mov %esp,%ebp
00100003:   sub $0x18,%esp
 19           memset(edata, 0, end - edata);
00100006:   mov $0x10ee00,%edx
0010000b:   mov $0x10da56,%eax
```
可以看到断点工作正常。
##练习3
###3.1为何开启A20，以及如何开启A20
开启A20是为了在是模式下访问1M以上的内存，在保护模式下访问全部4G地址空间。
开启A20的方法是将8042的output port的bit1置1。具体分为4步：
1. 等待键盘不忙
2. 向64h发送写入指令
3. 等待键盘不忙
4. 向60h发送写入内容
代码在bootasm.s中：
```asm
seta20.1:
    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
    testb $0x2, %al
    jnz seta20.1

    movb $0xd1, %al                                 # 0xd1 -> port 0x64
    outb %al, $0x64                                 # 0xd1 means: write data to 8042's P2 port

seta20.2:
    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
    testb $0x2, %al
    jnz seta20.2

    movb $0xdf, %al                                 # 0xdf -> port 0x60
    outb %al, $0x60                                 # 0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1
```
###3.2 如何初始化GDT表
bootasm.s中预先置了一个gdt表。其意义为一个0x0开始，长度为0xffffffff的可读可写可执行的段。这样在切换到保护模式之后逻辑地址依然等于物理地址，装载程序可以暂时按照物理地址工作。
装载相关代码为
```asm
lgdt gdtdesc
gdt:
    SEG_NULLASM                                     # null seg
    SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)           # code seg for bootloader and kernel
    SEG_ASM(STA_W, 0x0, 0xffffffff)                 # data seg for bootloader and kernel

gdtdesc:
    .word 0x17                                      # sizeof(gdt) - 1
    .long gdt                                       # address gdt
```
###3.3 如何使能和进入保护模式
修改cr0寄存器以开启保护模式。
```asm
    movl %cr0, %eax
    orl $CR0_PE_ON, %eax
    movl %eax, %cr0
```
做一次长跳转以修改代码段寄存器。做这件事的原因是cs不能被直接写入。
```asm
    ljmp $PROT_MODE_CSEG, $protcseg
```
初始化数据段寄存器
```asm
    movw $PROT_MODE_DSEG, %ax                       # Our data segment selector
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment
```
设置堆栈并调用bootmain函数
```asm
    movl $0x0, %ebp
    movl $start, %esp
    call bootmain
```
##练习4
###4.1 bootloader如何读取硬盘扇区的？
bootmain.c中实现了readsect函数，其功能为读取一个硬盘扇区。它首先向0x1F2到0x1F7中输出要读的扇区号，读取参数等信息，之后使用insl指令完成读取。
此外还有readseg函数，读取多个扇区，实现是逐次调用readsect函数。
###4.2 bootloader是如何加载ELF格式的OS？
bootmain.c从1号扇区开始开始读取4K大小的数据到0x10000位置，之后通过magic number验证它是否是约定的ELF格式。如果是合法的ELF格式，bootloader从ELF段描述表中逐个读入指定的数据段到内存。然后调用ELF中描述的入口函数。
##练习5：实现函数调用堆栈跟踪函数 （需要编程）
见代码
实现过程：按照所给注释实现。
##练习6
###6.1 中断描述符表（也可简称为保护模式下的中断向量表）中一个表项占多少字节？其中哪几位代表中断处理代码的入口？
8字节。低16位为段内偏移量，[16,32)位为段选择子，两者共同构成终端处理代码入口。
###6.2 请编程完善kern/trap/trap.c中对中断向量表进行初始化的函数idt_ini。在idt_init函数中，依次对所有中断入口进行初始化。使用mmu.h中的SETGATE宏，填充idt数组内容。每个中断的入口由tools/vectors.c生成，使用trap.c中声明的vectors数组即可。
见代码
###6.3 请编程完善trap.c中的中断处理函数trap，在对时钟中断进行处理的部分填写trap函数中处理时钟中断的部分，使操作系统每遇到100次时钟中断后，调用print_ticks子程序，向屏幕上打印一行文字”100 ticks”。
见代码
