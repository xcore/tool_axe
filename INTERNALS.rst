AXE's Simulation Model
======================

AXE models the system being simulated as a set of entities (e.g. Threads,
Ports, etc.) that communicate (e.g. by calling each other's member
functions). An entity may update its state either when it receives a message
or when time is advanced.

Entities that update their state as time is advanced inherit from the
Runnable class and provide a run() method that simulates the behavior of that
entity. Runnables are scheduled using a priority queue that is ordered by the
time at which the entity should next run.

Conceptually the run() advances the state of the Runnable until either
it either runs out of work to or until the time exceeds the time at which the
next Runnable is due to run. When the time splice expires if the Runnable has
more work to do it should schedule itself so it gets run when the time of the
next activity for that Runnable is reached.

However a Runnable is allowed to break these rules so long as it externally
appears to behave as if these rules are followed. This opens the door to
some optimizations that improve simulator performance:

- The run() method may simulate past the end of its timeslice so long as
  it doesn't affect other entities and it can't be affected by other
  entities. Note that the run() method is still required to eventually yield
  to allow other Runnables to make progress.
- When the state of a Runnable is due to change in future it only needs to
  schedule if the change might affect other entities. If the change won't
  affect other entites it may choose to delay updating its state until it
  becomes necessary (e.g. when it is queried by another entity).

To further improve performance we allow a Thread's run method to also
simulate past the end of its timeslice in the following cases:

- A thread does not need to yield after every instruction even though another
  thread might affect it by writing a register with tsetr or by killing it
  with freer.
- A thread does not need to yield before do a memory operation even if
  another thread may read or write to that memory location.

Although this could cause the behaviour of an application to change due to
a different interleaving of actions by a pair of threads, in practice most
applications do not rely on exact timing of instruction execution to order
interactions between threads.

Instruction dispatch
====================

The run() method of the Thread class is responsible for executing
instructions. AXE emulates the XCore's instruction set using a mixture of
interpretation and dynamic binary translation.

For each memory in the system there is a decode cache that maps addresses to
functions pointers that should be called when the pc of a Thread is equal to
that address.

Initially the decode cache maps every address to the DECODE function. When
the DECODE function is called it decodes the instruction and updates decode
cache entry to map to a function pointer that implements the decoded
instruction. When the DECODE function retuns the thread will re-execute the
decode cache entry and the function implementing the decoded instruction
will run. This will perform the operation and advance the pc to the address
of the next instruction. The thread will continue to execute instructions
using this dispatch mechanism until its timeslice expires.

Dynamic binary translation
==========================

To improve performance AXE makes uses of dynamic binary translation. Instead
of inserting function pointers that implement single instructions into the
decode cache AXE can insert functions that implement entire blocks of
instructions. These functions are generated at runtime using LLVM's
just-in-time (JIT) code generation system.

JIT compilation provides the following speed ups over iterpretation:

- With interpretation one instruction is executed in each iteration of
  the dispatch loop. However with JIT compilation an entire block of
  instructions can be dispatched in one go, reducing the average overhead of
  dispatching instructions.
- The generated code can be optimized across instruction boundaries. For
  example machine state can be cached in registers within the generated
  function.

The possible speedups must be balanced against the time required to generate
the code. If a block of instructions is executed only a few times then the
amount of time saved executing the instructions may be less than the extra
time spent JIT compiling the code. Because of this AXE only JIT compiles
blocks that are frequently executed. 

The executionFrequency member of the DecodeCache class tracks the number of
times each candidate for starting a new block has executed. When an
instruction that may branch is executed we increment the execution count of
the next address to execute after the instruction. When the execution count
of an instruction reaches a certain threshold we use the JIT compiler to
compile the block of instructions starting with that instruction into native
machine code. The block is terminated by the first instruction that may
branch.

Basic block chaining
====================

To further improve performance AXE tries to chain together compiled blocks
using jumps in order to avoid unnecessarily returning to the dispatch loop.
This is implemented as follows:

- When generating code for a block the function is compiled using LLVM's
  fastcc calling convention to guarantee efficient tail calls. For each block
  we also generate a trapoline function that uses the standard C calling
  convention and calls the function for the block. This trampoline allows the
  block function to be called from the dispatch loop.
- When a block is compiled if a successor of block has already been compiled
  a tail call to the successor block is inserted. LLVM's tail call recursion
  will turn this into a jump.
- When a block is compiled if a successor block has not been compiled a stub
  function is created for that successor and a tail call to the stub is
  inserted. The stub function increments the execution frequency of the
  successor block and returns to the dispatch loop.
- When a stub has been executed anough the block it corresponds to is
  JIT compiled. LLVM's LLVMRecompileAndRelinkFunction() is called to replace
  the stub with the newly compiled function for the block. Any predecessor
  that jumped to the stub will now jump to function for the block.
