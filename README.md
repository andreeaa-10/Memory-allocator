<h1>
    Memory allocator
</h1>
I developed a <b>custom memory allocator in C</b> that efficiently manages dynamic memory allocations using a <b>metadata structure</b> for each memory block. 
It supports both <b>malloc</b> and <b>calloc</b>, and implements techniques for managing fragmentation, including coalescing free blocks to reduce external fragmentation.
<h2>
    Memory Management Strategies
</h2>
<ul>
  <li>Used <b>brk()</b> for small allocations and <b>mmap()</b> for large allocations.</li>
  <li>Implemented <b>best-fit allocation</b> to minimize fragmentation.</li>
  <li>Block splitting and coalescing to optimize memory reuse.</li>
</ul>
<h2>
    Performance Enhancements
</h2>
<ul>
  <li><b>Heap preallocation</b> to reduce syscall overhead.</li>
  <li>Metadata structure (block_meta) for tracking memory blocks efficiently.</li>
  <li>Memory alignment (8 bytes) for better access performance.</li>
</ul>


