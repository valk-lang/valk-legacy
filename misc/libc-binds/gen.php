<?php

include __DIR__ . '/gen-helpers.php';
include __DIR__ . '/gen-structs.php';
include __DIR__ . '/gen-from-json.php';
include __DIR__ . '/gen-enums.php';

$imports = ['sys/stat', 'stdio', 'fcntl', 'errno', 'limits', 'string'];
$unix_imports = ['setjmp', 'dirent', 'poll', 'sys/types', 'sys/socket', 'netdb', 'sys/time', 'unistd'];
$linux_imports = array_merge($imports, $unix_imports, ['sys/epoll']);
$macos_imports = array_merge($imports, $unix_imports, []);
$win_imports = array_merge($imports, ['winsock2', 'time']);

$vars = [
    'timespec' => [
        'ctype' => 'struct timespec',
    ],
    'timeval' => [
        'ctype' => 'struct timeval',
    ],
    'sockaddr' => [
        'ctype' => 'struct sockaddr',
    ],
    'pollfd' => [
        'ctype' => 'struct pollfd',
    ],
];
$linux_vars = array_merge($vars, [
    'jmp_buf' => [
        'ctype' => 'jmp_buf',
    ],
    'stat' => [
        'ctype' => 'struct stat',
    ],
    'dirent' => [
        'ctype' => 'struct dirent',
    ],
    'timezone' => [
        'ctype' => 'struct timezone',
    ],
    'addrinfo' => [
        'ctype' => 'struct addrinfo',
    ],
    'epoll_event' => [
        'ctype' => 'struct epoll_event',
    ],
]);
$macos_vars = array_merge($vars, [
    'jmp_buf' => [
        'ctype' => 'jmp_buf',
    ],
    'stat' => [
        'ctype' => 'struct stat',
    ],
    'dirent' => [
        'ctype' => 'struct dirent',
    ],
    'timezone' => [
        'ctype' => 'struct timezone',
    ],
    'addrinfo' => [
        'ctype' => 'struct addrinfo',
    ],
    'sockaddr' => [
        'ctype' => 'struct sockaddr',
    ],
]);
$win_vars = array_merge($vars, [
    'stat' => [
        'ctype' => 'struct stat',
    ],
    'pollfd' => [
        'ctype' => 'WSAPOLLFD',
    ],
    'addrinfo' => [
        'ctype' => 'ADDRINFOA',
    ],
    'WIN32_FIND_DATAA' => [
        'ctype' => 'WIN32_FIND_DATAA',
    ],
    'FILETIME' => [
        'ctype' => 'FILETIME',
    ],
]);

$linux_args = "-D_GNU_SOURCE";
$macos_args = "";

$targets = [
    'linux-x64' => ['target' => 'x86_64-unknown-linux-gnu', 'arch' => 'x64', 'header_dir' => 'linux/x64', 'toolchain' => 'linux-amd64', 'imports' => $linux_imports, 'vars' => $linux_vars, 'args' => $linux_args],
    'macos-x64' => ['target' => 'x86_64-apple-darwin-macho', 'arch' => 'x64', 'header_dir' => 'macos/x64', 'toolchain' => 'macos-11-3', 'imports' => $macos_imports, 'vars' => $macos_vars, 'args' => $macos_args],
    'macos-arm64' => ['target' => 'arm64-apple-darwin-macho', 'arch' => 'arm64', 'header_dir' => 'macos/arm64', 'toolchain' => 'macos-11-3', 'imports' => $macos_imports, 'vars' => $macos_vars, 'args' => $macos_args],
    'win-x64' => [
        'target' => 'x86_64-pc-windows-msvc',
        'arch' => 'x64',
        'header_dir' => 'win/x64',
        'toolchain' => 'win-sdk-x64',
        'tc_includes' => [
            'Include/10.0.22621.0/ucrt',
            'Include/10.0.22621.0/um',
            'Include/10.0.22621.0/shared',
            'MSVC/14.36.32532/include',
        ],
        'imports' => $win_imports, 'vars' => $win_vars
    ],
];

// Vars
$tmp = get_tmp_dir();

// Generate
foreach($targets as $valk_target => $target) {

    $target['name'] = $valk_target;

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
    foreach($target['vars'] as $name => $var) {
        $code .= $var['ctype'] . " var_$name;\n";
        $i++;
    }
    $code .= "\n";

    $code .= "int main() { return 0; }\n\n";

    // Write code
    $path = $tmp . '/' . $valk_target . '.c';
    file_put_contents($path, $code);

    // Paths
    $path_ir = $tmp . '/' . $valk_target . '.ir';
    $path_ast = $tmp . '/' . $valk_target . '-ast.json';
    $path_enums = $tmp . '/' . $valk_target . '-enums.txt';
    $header_dir = __DIR__ . '/../../lib/headers';

    $cmd = get_base_cmd($target);

    $ast_cmd = $cmd . " -c $path -Xclang -ast-dump=json > $path_ast";
    $ir_cmd = $cmd . " -S -emit-llvm $path -o $path_ir";
    $enu_cmd = $cmd . " -dM -E $path";

    // echo $ir_cmd . "\n";
    // exec($ir_cmd);

    echo $ast_cmd . "\n";
    exec($ast_cmd);
    $ast_json = file_get_contents($path_ast);
    $ast_data = json_decode($ast_json);

    // Defines
    echo $enu_cmd . "\n";
    $defines = [];
    exec($enu_cmd, $defines);
    file_put_contents($path_enums, implode("\n", $defines));
    $enums = gen_enums_($defines, $ast_data);
    // $enums = gen_enums($defines, $target);
    $hpath = $header_dir . '/' . $target['header_dir'] . '/libc-enums.valk.h';
    file_put_contents($hpath, $enums);

    // Gen valk structs / api
    $code = gen_valk_structs_ast($ast_data, $target);

    // $hpath = $header_dir . '/' . $target['header_dir'] . '/libc-gen.valk.h';
    // file_put_contents($hpath, $code);
}
