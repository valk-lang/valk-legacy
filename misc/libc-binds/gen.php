<?php

$imports = ['sys/stat'];
$unix_imports = ['setjmp', 'dirent', 'poll', 'sys/types', 'sys/socket', 'netdb', 'sys/time'];
$linux_imports = array_merge($imports, $unix_imports, ['sys/epoll']);
$macos_imports = array_merge($imports, $unix_imports, []);
$win_imports = array_merge($imports, ['winsock2']);

// c-name => [find => ..., name => valk-name]
$structs = [
    'struct stat',
    'struct sockaddr',
    'struct timeval',
];
$linux_structs = array_merge($structs, [
    'jmp_buf',
    'struct dirent',
    'struct addrinfo',
    'struct pollfd',
    'struct epoll_event',
    'struct timezone',
]);
$macos_structs = array_merge($structs, [
    'jmp_buf',
    'struct dirent',
    'struct addrinfo',
    'struct pollfd',
    'struct timezone',
]);
$win_structs = array_merge($structs, [
    'WSAPOLLFD',
    'ADDRINFOA',
]);

$converts = [
    [
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
    '__jmp_buf_tag' => ['name' => 'libc_jmp_buf'],
    'dirent' => ['fields' => ['d_ino', 'd_off', 'd_reclen', 'd_type', 'd_name']],
    'epoll_event' => ['fields' => ['events', 'data']],
]);
$macos_converts = array_merge($converts, [
    '__jmp_buf_tag' => ['name' => 'libc_jmp_buf'],
    'dirent' => ['fields' => ['d_ino', 'd_off', 'd_reclen', 'd_type', 'd_name']],
]);

$targets = [
    'linux-x64' => ['target' => 'x86_64-unknown-linux-gnu', 'toolchain' => 'linux-x64/x86_64-buildroot-linux-gnu/sysroot', 'imports' => $linux_imports, 'structs' => $linux_structs, 'converts' => $linux_converts],
    'macos-x64' => ['target' => 'x86_64-apple-darwin-macho', 'toolchain' => 'macos-11-3', 'imports' => $macos_imports, 'structs' => $macos_structs, 'converts' => $macos_converts],
    'win-x64' => [
        'target' => 'x86_64-pc-windows-msvc',
        'toolchain' => 'win-sdk-x64',
        'clang_args' => "-ftime-trace",
        'tc_includes' => [
            'Include/10.0.22621.0/ucrt',
            'Include/10.0.22621.0/um',
            'Include/10.0.22621.0/shared',
            'MSVC/14.36.32532/include',
        ],
        'imports' => $win_imports, 'structs' => $win_structs, 'converts' => $converts
    ],
];

// Vars
$root = __DIR__;
$tmp = $root . '/tmp';
if (!file_exists($tmp))
    mkdir($tmp);
$dist_dir = $root . '/../../dist';
$tc_dir = $dist_dir . '/toolchains';

// Generate
foreach($targets as $valk_target => $target) {

    $code = "";
    $code .= '#pragma clang diagnostic ignored "-Wpragma-pack"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wignored-attributes"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wignored-pragma-intrinsic"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wmicrosoft-anon-tag"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wdeprecated-declarations"' . "\n";

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
    $ttc_dir = $tc_dir . '/' . $target['toolchain'];
    $cmd = "clang-15 -S -emit-llvm " . ($target['clang_args'] ?? '') . " $path -o $path_ir --target=" . $target['target'] . " --sysroot=$ttc_dir";
    foreach($target['tc_includes'] ?? [] as $inc) {
        $cmd .= ' -I' . $ttc_dir . '/' . $inc;
    }
    echo $cmd . "\n";
    exec($cmd);
}
