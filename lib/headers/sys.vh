
fn exit(code: i32) void
fn malloc(size: uint) ptr
fn free(adr: ptr) i32
fn write(fd: i32, data: ptr, length: uint) i32

fn nanosleep(req: libc_timespec, rem: libc_timespec) i32;

struct libc_timespec {
	tv_sec: int // seconds
	tv_nsec: int // nanoseconds
}

fn epoll_create(size: i32) i32;
fn epoll_wait(epfd: i32, events: ptr, maxevents: i32, timeout: i32) i32;
fn epoll_ctl(epfd: i32, op: i32, fd: i32, event: cstruct_epoll_event) i32;

struct cstruct_epoll_event {
    events: u32 // events
    data: ptr // data
}

value EPOLLERR (8)
value EPOLLET (-2147483648)
value EPOLLHUP (16)
value EPOLLIN (1)
value EPOLLMSG (1024)
value EPOLLONESHOT (1073741824)
value EPOLLOUT (4)
value EPOLLPRI (2)
value EPOLLRDBAND (128)
value EPOLLRDHUP (8192)
value EPOLLRDNORM (64)
value EPOLLWRBAND (512)
value EPOLLWRNORM (256)
value EPOLL_CLOEXEC (524288)
value EPOLL_CTL_ADD (1)
value EPOLL_CTL_DEL (2)
value EPOLL_CTL_MOD (3)
value EPOLL_NONBLOCK (2048)