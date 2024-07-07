<?php

function gen_enums(array $defines): string
{
    $code = "\n";
    // e.g. "#define linux 1"
    foreach($defines as $define) {
        $m = [];
        if(preg_match('/^#define ([A-Z_]+) ([0-9A-Z_]+)$/', $define, $m)) {
            $name = $m[1];
            $value = $m[2];
            if($value[0] == '0' && $value != "0") {
                $value = '0c' . substr($value, 1, strlen($value) - 1);
            }
            $code .= "value $name ($value)\n";
        }
    }
    $code .= "\n";
    return $code;
}