// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: %sim %t1.xe
.text
foo:
    .issue_mode dual
    dualentsp 1

    // Load the link register into r0
    ldaw r2, sp[0]
    add r1, r2, 4
    set sp, r1
    ldw r0, sp[0]
    set sp, r2

    // Check the first bit of the link register is set
    mkmsk r1, 1
    and r0, r0, r1
    bf r0, return_false
    ldc r0, 0
    retsp 1
return_false:
    ldc r0, 1
    retsp 1

.globl	main
main:
	.issue_mode dual
	DUALENTSP_lu6 1
    ldap r11, foo
    bla r11
	retsp 1
