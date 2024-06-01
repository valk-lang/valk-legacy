<?php

$imports = ['setjmp', 'dirent', 'sys/stat', 'poll', 'sys/types', 'sys/socket', 'netdb', 'sys/time'];
$linux_imports = array_merge($imports, ['sys/epoll']);

// c-name => [find => ..., name => valk-name]
$structs = [
    'jmp_buf',
    'struct dirent',
    'struct stat',
    'struct pollfd',
    'struct addrinfo',
    'struct sockaddr',
    'struct timeval',
    'struct timezone',
];
$linux_structs = array_merge($structs, [
    'struct epoll_event'
]);

$converts = [
    '__jmp_buf_tag' => ['name' => 'libc_jmp_buf'],
    'dirent' => ['fields' => ['d_ino', 'd_off', 'd_reclen', 'd_type', 'd_name']],
    'stat' => [
        'fields' => ['st_dev', 'st_ino', 'st_nlink', 'st_mode', 'st_uid', 'st_gid', '__pad0', 'st_rdev', 'st_size', 'st_blksize', 'st_blocks', 'st_atime', 'st_mtime', 'st_ctime', '__unused']
    ],
    'timespec' => ['fields' => ['tv_sec', 'tv_nsec']],
    'timeval' => ['fields' => ['tv_sec', 'tv_nsec']],
    'timezone' => ['fields' => ['tz_minuteswest', 'tz_dsttime']],
    'pollfd' => ['fields' => ['fd', 'events', 'revents']],
    'addrinfo' => ['fields' => ['ai_flags', 'ai_family', 'ai_socktype', 'ai_protocol', 'ai_addrlen', 'ai_addr', 'ai_canonname', 'ai_next']],
    'sockaddr' => ['fields' => ['sa_family', 'sa_data']],
];
$linux_converts = array_merge($converts, [
    'epoll_event' => ['fields' => ['events', 'data']],
]);

$targets = [
    'linux-x64' => ['target' => 'x86_64-unknown-linux-gnu', 'toolchain' => 'macos-11-3', 'imports' => $linux_imports, 'structs' => $linux_structs, 'converts' => $linux_converts],
];

// Vars
$root = __DIR__;
$tmp = $root . '/tmp';
if (!file_exists($tmp))
    mkdir($tmp);
$dist_dir = $root . '/../../dist';

// Generate
foreach($targets as $valk_target => $target) {

    $code = "";

    foreach($target['imports'] as $import) {
        $code .= "#include <$import.h>\n";
    }
    $code .= "\n";

    $i = 0;
    foreach($target['structs'] as $c_name) {
        $code .= "$c_name var_$i;\n";
        $i++;
    }
    $code .= "\n";

    $code .= "int main() { return 0; }\n\n";

    // Write code
    $path = $tmp . '/' . $valk_target . '.c';
    file_put_contents($path, $code);

    $path_ir = $tmp . '/' . $valk_target . '.ir';
    exec("clang-15 -S -emit-llvm $path -o $path_ir --target=" . $target['target']);
}
