# MetricStreamLib

<div align="center">
    <img src="img/logo.png" width=40%>
</div>
<br>
<div align="center">
  <img src="https://img.shields.io/badge/Linux-%23.svg?logo=linux&color=black&logoColor=yellow"/>
  <img src="https://img.shields.io/badge/Windows-%23.svg?logo=windows&color=black&logoColor=blue"/>
  <img src="https://img.shields.io/badge/-C/C++-black?logo=c&logoColor=blue">
  <img src="https://img.shields.io/badge/-CMake-black?logo=cmake&logoColor=yellow">
</div>

## Information

The library provides a generic interface and in-memory implementation of a key-value storage with TTL (time-to-live) support and key sorting.
It is designed for fast data access and automatic expiration management.

## Features

- Time-to-live (TTL) support for entries.
- Ability to retrieve multiple entries in lexicographical key order.
- Efficient removal of expired entries.
- Template-based implementation — supports any types of keys and values.
- Efficient rehashing without thread downtime.

## Asymptotics

| Action                        | Time        | Comment                                                 |
|-------------------------------|-------------|---------------------------------------------------------|
| get(key)                      | O(1)        | This is achieved by using a hash table.                 |
| set(key, value, ttl)          | O(log(N))   | Cause of the addition to the set                        |
| remove(key)                   | O(log(N))   | Cause of the remove from the set                        |
| getManySorted(key, count)     | O(logN + K) | Cause of the time to find in the set and read K elements |
| removeOneExpired()            | O(log(N))   | Cause of the remove from the set                                                      |



## Overhead
Overhead in a key-value storage refers to the extra memory and processing cost beyond storing the raw key and value themselves

| Component                                                           | Overhead per entry                                   |
|---------------------------------------------------------------------|------------------------------------------------------|
| `shared_ptr<Key>`  2x                                               | 32  bytes                                            |
| `shared_ptr<Value>`                                                 | 16  bytes                                            |
| `shared_ptr<Ttl>`                                                   | 16  bytes                                            |
| `_setSortedKeyPtr` (`std::shared<Key>`)                             | 16  bytes                                            |
| `_setSortedTtlPtr` (`std::pair<std::shared<Key>,std::shared<Ttl>>`) | 32-48 bytes                                          |
| **Total (without actual key/value)**                                | \~112 bytes per entry (rough estimate, TTL included) |


## Documentation

See: https://claucdan.github.io/KVStorage/

## Requirements

- `CMake >= 3.10`
- `g++ >= 15.1` or `clang >= 20.1`

## Dependencies / Used Libraries
This project relies on the following libraries:

- **C++ Standard Library**
  - `<string>` — for string manipulation and storage.
  - `<cstdint>` — for fixed-width integer types.
  - `<optional>` — to represent optional return values.
  - `<utility>` — for `std::pair` and `std::move`.
  - `<vector>` — for storing sequences of key-value pairs.
  - `<memory>` — for `std::shared_ptr` usage in memory management.
  - `<set>` — for sorted storage of keys and TTL entries.
  - `<queue>` — for potential future queue-based operations.
  - `<span>` — for passing ranges of entries efficiently.
  - `<future>` — included in the interface for potential asynchronous operations.

- **Ready-made libraries**
  - `<spdlog>` — the library used for logging.

## Installation & Usage

### Build only library

```bash
git clone ...
mkdir build && cd build
cmake .. 
cmake --build .
```

Builded library you can find in `build` or `build/bin/`