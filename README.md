<h1>
    Memory allocator
</h1>
I developed a custom memory allocator in C that efficiently manages dynamic allocations using a block metadata structure and a ring buffer for optimized memory reuse. It supports both malloc and calloc, handling fragmentation and alignment to enhance performance. 
<br>
Implemented brk() and mmap() for dynamic allocation, with memory alignment, block splitting, and coalescing to reduce fragmentation. Used a best-fit strategy for efficient block reuse and preallocated memory chunks to minimize syscalls. Ensured robust error handling and optimized performance through system call reduction and metadata tracking.
 </br>
