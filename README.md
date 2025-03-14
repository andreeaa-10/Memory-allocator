<h1>
    Memory allocator
</h1>
I developed a custom memory allocator in C that efficiently manages dynamic memory allocations using a metadata structure for each memory block. 
It supports both malloc and calloc, and implements techniques for managing fragmentation, including coalescing free blocks to reduce external fragmentation.
<h2>
    Memory Management Strategies
</h2>
<ul>
  <li>Used brk() for small allocations and mmap() for large allocations.</li>
  <li>Implemented best-fit allocation to minimize fragmentation.</li>
  <li>Block splitting and coalescing to optimize memory reuse.</li>
</ul>


