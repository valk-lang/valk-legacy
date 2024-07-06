
cstruct libc_timespec {
    tv_sec: i64
    tv_nsec: i32
}

cstruct libc_timeval {
    tv_sec: i32
    tv_usec: i32
}

cstruct libc_sockaddr {
    sa_family: i16
    sa_data: inline [i8, 14]
}

cstruct libc_pollfd {
    fd: i64
    events: i16
    revents: i16
}

// cstruct libc_stat {
//     st_gid: i32
//     st_atime: i16
//     st_ctime: i16
//     st_dev: i16
//     st_ino: i16
//     st_mode: i16
//     st_mtime: i32
//     st_nlink: i32
//     st_rdev: i64
//     st_size: i64
//     st_uid: i64
// }

cstruct libc_addrinfo {
    ai_flags: i32
    ai_family: i32
    ai_socktype: i32
    ai_protocol: i32
    ai_addrlen: i64
    ai_addr: ptr
    ai_canonname: ptr
    ai_next: ptr
}

cstruct libc_WIN32_FIND_DATAA {
    dwFileAttributes: i32
    ftCreationTime: inline libc_FILETIME
    ftLastAccessTime: inline libc_FILETIME
    ftLastWriteTime: inline libc_FILETIME
    nFileSizeHigh: i32
    nFileSizeLow: i32
    dwReserved0: i32
    dwReserved1: i32
    cFileName: inline [i8, 260]
    cAlternateFileName: inline [i8, 14]
}

cstruct libc_FILETIME {
    dwLowDateTime: i32
    dwHighDateTime: i32
}
