// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: not axe %t1.xe > %t2.txt
// RUN: cmp %t2.txt %s.expect
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: not axe %t1.xe > %t2.txt
// RUN: cmp %t2.txt %s.expect
#include <xs1.h>

.section .trap_info,"",@progbits

.Ltrap_info_entries_start0:
.word .Ltrap_info_entries_end0-.Ltrap_info_entries_start0
.word 2 // AXE should skip compile unit with unexpected version.
.word trap_loc
.word trap_bar
.Ltrap_info_entries_end0:

.Ltrap_info_entries_start1:
.word .Ltrap_info_entries_end1-.Ltrap_info_entries_start1
.word 1
.word trap_loc - 2 // This pc doesn't match.
.word trap_bar
.Ltrap_info_entries_end1:

.Ltrap_info_entries_start2:
.word .Ltrap_info_entries_end2-.Ltrap_info_entries_start2
.word 1
.word trap_loc // This should match.
.word trap_foo
.Ltrap_info_entries_end2:

.section .trap_info_str,"MS",@progbits
.space 5 // Space to ensure string is at non-zero offset.
trap_foo:
.asciiz"foo"
trap_bar:
.asciiz"bar"

.text
.globl main
.align 2
main:
  ldc r0, 0
trap_loc:
  ecallf r0
