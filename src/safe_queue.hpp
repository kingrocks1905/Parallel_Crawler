#pragma once

// Thread-safe unbounded BFS queue
// Uses: std::mutex, std::condition_variable

// Key operations:
//   push(item)                    — enqueue + notify one waiting thread
//   try_pop(item, timeout)        — dequeue with timeout (for termination detection)
//   empty()                       — check if queue is empty
//
