
fn malloc(size: uint) ptr;
fn free(adr: ptr) i32;

fn exit(code: i32) void;
fn signal(signum: i32, handler: ?fn(i32)(void)) void;
fn raise(sig: i32) i32;
fn __errno_location() ptr;

fn read(fd: i32, buf: ptr, size: uint) int;
fn write(fd: i32, data: ptr, length: uint) i32;
fn open(path: ptr, flags: i32, mode: i32) i32;
fn close(fd: i32) i32;

fn recv(fd: i32, buf: ptr, len: uint, flags: i32) int;
fn send(fd: i32, buf: ptr, len: uint, flags: i32) int;

fn socket(domain: i32, type: i32, protocol: i32) i32;
fn connect(sockfd: i32, addr: libc_sockaddr, addrlen: u32) i32;
fn accept(sockfd: i32, addr: ?libc_sockaddr, addrlen: ?ptr) i32;
fn accept4(sockfd: i32, addr: ?libc_sockaddr, addrlen: ?ptr, flags: i32) i32;
fn shutdown(sockfd: i32, how: i32) i32;
fn bind(sockfd: i32, addr: libc_sockaddr, addrlen: u32) i32;
fn listen(sockfd: i32, backlog: i32) i32;

fn getsockopt(sockfd: i32, level: i32, optname: i32, optval: ptr, optlen: u32) i32;
fn setsockopt(sockfd: i32, level: i32, optname: i32, optval: ptr, optlen: u32) i32;
fn getaddrinfo(host: ptr, port: ptr, hints: libc_addrinfo, res: ptr) i32;
fn freeaddrinfo(info: libc_addrinfo) i32;

fn epoll_create(size: i32) i32;
fn epoll_wait(epfd: i32, events: ptr, maxevents: i32, timeout: i32) i32;
fn epoll_ctl(epfd: i32, op: i32, fd: i32, event: libc_epoll_event) i32;

fn nanosleep(req: libc_timespec, rem: libc_timespec) i32;
