#Lab1实验报告
2012011282 计22 王天一
##练习1
##练习2
###2.1 从CPU加电后执行的第一条指令开始，单步跟踪BIOS的执行。
见练习2.2
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
