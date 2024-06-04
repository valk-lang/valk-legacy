
# Syntax

```
use valk.mem

g1 : global int = 0 // thread local global (recommended)
s1 : shared int = 1 // shared global

A : struct[T] is MyInterface, MyInterface2 {
    a : int = 1 // public - access anywhere
    b :: int = 2 // private - this file only
    c :ns: int = 3 // private - this namespace only
    d :pkg: int = 4 // private - this package only
    -

    use MyTrait1
    use MyTrait2

    // private static function
    f1 : static fn(arg: int = 100) String {
        let x : int = 10
        println("Value: " + x)
        return x.to_str()
    }

    // public function
    f2 : fn() {
        println(this.a)
    }

    g1 : global SELF = SELF {} // define a global for this struct, aka. static property
    s1 : shared uint = 5 // define a shared global for this struct, aka. shared static property
    e1 : enum int {
        VAL1
        VAL2 = 10
    }
}

B : type A[String] // Type alias
C : value "Valk value alias" // Value alias

alloc : fn(size: uint = 100) ptr {
    return mem.alloc(size)
}

my_enum : enum int {
    VAL1 // 0
    VAL2 = 10
    VAL3 // 1
}
```