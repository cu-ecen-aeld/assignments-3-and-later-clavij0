## Assignment 7: Error ##

## echo “hello_world” > /dev/faulty ##

Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000  
Mem abort info:  
  ESR = 0x0000000096000045  
  EC = 0x25: DABT (current EL), IL = 32 bits  
  SET = 0, FnV = 0  
  EA = 0, S1PTW = 0  
  FSC = 0x05: level 1 translation fault  
Data abort info:  
  ISV = 0, ISS = 0x00000045  
  CM = 0, WnR = 1  
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041e7c000  
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000  
Internal error: Oops: 0000000096000045 [#1] SMP  
Modules linked in: hello(O) faulty(O) scull(O)  
CPU: 0 PID: 160 Comm: sh Tainted: G           O       6.1.44 #1  
Hardware name: linux,dummy-virt (DT)  
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)  
pc : faulty_write+0x10/0x20 [faulty]  
lr : vfs_write+0xc4/0x380  
sp : ffffffc008d93d20  
x29: ffffffc008d93d20 x28: ffffff8001aa4f80 x27: 0000000000000000  
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000  
x23: 0000000000000012 x22: 0000000000000012 x21: ffffffc008d93df0  
x20: 000000555c3e3990 x19: ffffff8001b71d00 x18: 0000000000000000  
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000  
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000  
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000  
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000  
x5 : 0000000000000001 x4 : ffffffc000777000 x3 : ffffffc008d93df0  
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000  
Call trace:  
 faulty_write+0x10/0x20 [faulty]  
 ksys_write+0x70/0x110  
 __arm64_sys_write+0x1c/0x30  

# Analysis of Null error #
> ## Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
* This is giving us a hint, the virtual address zero that looks like it's probably a **null** pointer reference This is the key part of the error. It indicates that the kernel tried to access memory at address 0x0000000000000000 (NULL). This usually happens when a pointer that should have been initialized is NULL, and an attempt is made to dereference it. *

Mem abort info:  
  ESR = 0x0000000096000045  
  EC = 0x25: DABT (current EL), IL = 32 bits  
  SET = 0, FnV = 0  
  EA = 0, S1PTW = 0  
  FSC = 0x05: level 1 translation fault  
Data abort info:  
  ISV = 0, ISS = 0x00000045  
  CM = 0, WnR = 1  
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041e7c000  
# [0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000 #
** pgd=0000000000000000, p4d=0000000000000000: The page table entries are zeroed out, meaning the memory address being accessed is not mapped, confirming a NULL pointer dereference. **

# Internal error: Oops: 0000000096000045 [#1] SMP # ##
* "Oops: 0000000096000045 [#1] SMP" indicates an oops error with a specific code 0x96000045, which matches the ESR value seen earlier .*

## CPU and Process Information ##
Modules linked in: hello(O) faulty(O) scull(O)
CPU: 0 PID: 160 Comm: sh Tainted: G           O       6.1.44 #1

+ CPU: 0: This oops occurred on CPU core 0.
+ PID: 157 Comm: sh: The error occurred in process ID 157, initiated by the shell (sh).
+ pstate: 80000005: This register value shows the processor state at the time of the error.

Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
> ## pc : faulty_write+0x10/0x20 [faulty] ##
*   Indicates that the error occurred at an offset of 0x10 in the faulty_write function within the faulty module. This helps pinpoint the line in the faulty driver code where the issue likely occurred   
lr : vfs_write+0xc4/0x380
sp : ffffffc008d93d20
x29: ffffffc008d93d20 x28: ffffff8001aa4f80 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000000000012 x22: 0000000000000012 x21: ffffffc008d93df0
x20: 000000555c3e3990 x19: ffffff8001b71d00 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc000777000 x3 : ffffffc008d93df0
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000
## Call trace: ##
* The call trace shows the sequence of function calls leading to the error:
faulty_write+0x10/0x20 [faulty]: Error within the faulty_write function.
ksys_write, __arm64_sys_write, invoke_syscall, etc., trace back to the system call that initiated the write operation. *

 faulty_write+0x10/0x20 [faulty]
 ksys_write+0x70/0x110
 __arm64_sys_write+0x1c/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x2c/0xc0
 el0_svc+0x2c/0x90
 el0t_64_sync_handler+0xf4/0x120
 el0t_64_sync+0x18c/0x190
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 0000000000000000 ]---
