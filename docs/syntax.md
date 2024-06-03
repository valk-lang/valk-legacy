
# Syntax

```
use valk.mem

A : struct[T] {
    a : int = 1 // public
    b :: int = 2 // private - except this file
    c :ns: int = 3 // private - except this namespace
    d :pkg: int = 4 // private - except this package

    // private static function
    f1 :: static fn(arg: int = 100) String {
        let x : int = 10
        println("Value: " + x)
        return x.to_str()
    }

    // public function
    f2 : fn() {
        println(this.a)
    }
}

B : type A[String] // Type alias
C : value "Valk value alias" // Value alias

alloc : fn(size: uint = 100) ptr {
    return mem.alloc(size)
}
```