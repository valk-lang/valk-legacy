<?php

function get_tmp_dir(): string
{
    $tmp = __DIR__ . '/tmp';
    if (!file_exists($tmp))
        mkdir($tmp);
    return $tmp;
}

function get_base_cmd(array $target, bool $runnable = false): string
{
    $dist_dir = __DIR__ . '/../../dist';
    $tc_dir = $dist_dir . '/toolchains';

    $ttc_dir = $tc_dir . '/' . $target['toolchain'];
    $args = $target['args'] ?? '';
    $trg = $target['target'];
    $t = $runnable ? "" : "--target=$trg";
    $cmd = "clang-15 $args $t --sysroot=$ttc_dir";
    foreach($target['tc_includes'] ?? [] as $inc) {
        $cmd .= ' -I' . $ttc_dir . '/' . $inc;
    }
    return $cmd;
}
