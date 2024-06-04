
# Syntax

```
use valk.mem

global g1 : int = 0 // thread local global (recommended)
shared s1 : int = 1 // shared global

struct A[T] is MyInterface, MyInterface2 {
    // Properties
    a : int = 1 // public - access anywhere
    b :: int = 2 // private - this file only
    c :ns: int = 3 // private - this namespace only
    d :pkg: int = 4 // private - this package only

    // Traits
    use MyTrait1
    use MyTrait2

    // private static function
    static fn f1(arg: int = 100) String {
        let x : int = 10
        println("Value: " + x)
        return x.to_str()
    }

    // public function
    fn f2() {
        println(this.a)
    }

    global g1 : SELF = SELF {} // define a global for this struct, aka. static property
    shared s1 : uint = 5 // define a shared global for this struct, aka. shared static property
    enum e1 : int {
        VAL1
        VAL2 = 10
    }
}

type B : A[String] // Type alias
value C : "Valk value alias" // Value alias

// Function
fn alloc(size: uint = 100) ptr {
    // Inline function 1
    def fn myfunc() { ... code ... }
    myfunc()
    // Inline function 2 (closure)
    let myfunc = fn() { ... code ... }
    myfunc()
    // Inline struct
    def struct myStruct { ... props & functions ... }
    // Inline global
    def global g1 : int = 10
    // Inline enum
    def enum e1 : {type} { ... values ... }
    return mem.alloc(size)
}

enum my_enum : int {
    VAL1 // 0
    VAL2 = 10
    VAL3 // 1
}
```