
# Roadmap

- v0.1.0 : Have all the core compiler features ready

- v0.2.0 : Stable standard library

- v0.3.0 : Implement low priority compiler features

- v1.0.0 : Compiler is field tested by multiple projects

## TODO

```
// v0.0.1
- ci
- make prebuilt releases (version 0.0.1)

// v0.0.2
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
- vali make command (like: npm run)
- run command: vali run ...
- dont disable_gc in gc_share, instead disable inside hooks 
- 'clone' functions for classes in vali pkg
- more compile condition functions
```

## Maybe

```
- defer token
- interfaces
- multi-threaded compiling
```