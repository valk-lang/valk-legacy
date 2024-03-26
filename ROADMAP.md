
# Roadmap

- v0.1.0 : Have all the core compiler features ready

- v0.2.0 : Stable standard library

- v0.3.0 : Implement low priority compiler features

- v1.0.0 : Compiler is field tested by multiple projects

## TODO

```
// v0.0.1
- make prebuilt releases (version 0.0.1)

// v0.0.2
- split error identifiers & scope identifiers
- ignore error handler if void return
- octal integers
- use number hash to determine value for errors: unsigned int err_value = num_hash("!x")
- func_ref type to str

- macros (e.g. array, map)
- string functions
- format string
- borrow & gc_check stack if thread paused for too long in order to free shared memory 
- watch files --watch
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
```

## Maybe

```
- defer token
- interfaces
- multi-threaded compiling
- success chain: let str = ->r : a()->b(100, r)->c(r) ? "fail"
-- each '->' passes the return value (r) to the next function, if any fails, go to the error handler at the end
```