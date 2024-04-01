
# Roadmap

- v0.1.0 : Have all the core compiler features ready

- v0.2.0 : Stable standard library

- v0.3.0 : Implement low priority compiler features

- v1.0.0 : Compiler is field tested by multiple projects

## TODO

```
// Prio
- split error identifiers & scope identifiers
- use number hash to determine value for errors: unsigned int err_value = num_hash("!x")
- macros (e.g. array, map)
- watch files --watch
// Other
- more string functions
- format string
- borrow & gc_check stack if thread paused for too long in order to free shared memory 
- unions
- async / await
- closures
- generics for functions & traits
- package management
- debug info
- language server
- valk make command (like: npm run)
- run command: valk run ...
- dont disable_gc in gc_share, instead disable inside hooks 
- 'clone' functions for classes in valk pkg
- more compile condition functions
- declare functions as unsafe ( unsafe fn {name} ), determines access to these functies for unsafe developers
```

## Maybe

```
- defer token
- interfaces
- multi-threaded compiling
- success chain: let str = ->r : a()->b(100, r)->c(r) ? "fail"
-- each '->' passes the return value (r) to the next function, if any fails, go to the error handler at the end
```