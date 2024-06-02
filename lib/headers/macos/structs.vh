
struct libc_jmp_buf {
    #if ARCH == arm64
    data: inline [i64, 22]
    #else
    data: inline [i64, 8]
    #end
    num: i32
    sig: inline libc_sigset
}
struct libc_sigset {
    data: inline [i64, 16]
}

#if ARCH == x64
struct libc_dirent {
   d_ino: uint // Inode number
   d_off: uint // Not an offset see below
   d_reclen: u16 // Length of this record
   d_type: u8 // Type of file
   d_namelen: u8
   d_name: inline [u8, 255]
}
#elif ARCH == arm64
struct libc_dirent {
   d_ino: uint // Inode number
   d_off: uint // Not an offset see below
   d_reclen: u16 // Length of this record
   d_namelen: u16
   d_type: u8 // Type of file
   d_name: inline [u8, 1024]
}
#end

struct libc_stat {
    st_dev: uint
    st_ino: uint
    st_nlink: uint
    st_mode: u32
    st_uid: u32
    st_gid: u32
    __pad0: u32
    st_rdev: uint
    st_size: int
    st_blksize: int
    st_blocks: int // Number 512-byte blocks allocated
    st_atime: uint
    st_atime_nsec: uint
    st_mtime: uint
    st_mtime_nsec: uint
    st_ctime: uint
    st_ctime_nsec: uint
    __unused_1: int
    __unused_2: int
    __unused_3: int
}

struct libc_timespec {
	tv_sec: int // seconds
	tv_nsec: int // nanoseconds
}

struct libc_timeval {
	tv_sec: int // seconds
	tv_usec: int // microseconds
}

struct libc_pollfd {
	fd: i32
	events: i16  // detect events
	revents: i16 // detected events
}

struct libc_addrinfo {
    ai_flags: i32
    ai_family: i32
    ai_socktype: i32
    ai_protocol: i32
    ai_addrlen: u32
    ai_canonname: cstring
    ai_addr: libc_sockaddr
    ai_next: ?libc_addrinfo
}

struct libc_sockaddr {
	sa_family: u16
	sa_data_1: u32
	sa_data_2: u32
	sa_data_3: u32
	sa_data_4: u16
}

struct libc_timezone {
	tz_minuteswest: i32 // Minutes west of GMT
	tz_dsttime: i32 // Nonzero if DST is ever in effect
}
