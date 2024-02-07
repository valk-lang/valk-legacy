
fn malloc(size: uint) ptr
fn write(fd: i32, data: ptr, length: uint) i32

fn nanosleep(req: libc_timespec, rem: libc_timespec) i32;

struct libc_timespec {
	tv_sec: int // seconds
	tv_nsec: int // nanoseconds
}
