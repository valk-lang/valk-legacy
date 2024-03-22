
struct libc_timespec {
	tv_sec: int // seconds
	tv_nsec: int // nanoseconds
}

struct cstruct_epoll_event packed {
    events: u32 // events
    data: ptr // data
}

struct cstruct_addrinfo {
    ai_flags: i32 (0)
    ai_family: i32 (0)
    ai_socktype: i32 (0)
    ai_protocol: i32 (0)
    ai_addrlen: u32 (0)
    ai_addr: cstruct_sockaddr
    ai_canonname: ptr (null)
    ai_next: ?cstruct_addrinfo (null)
}

struct cstruct_sockaddr {
	sa_family: u16 (0)
	sa_data_1: u32 (0)
	sa_data_2: u32 (0)
	sa_data_3: u32 (0)
	sa_data_4: u16 (0)
}
