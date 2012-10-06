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

Threads
=======

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
will run. The thread continues executing instructions in this manner until
its timeslice expires.
