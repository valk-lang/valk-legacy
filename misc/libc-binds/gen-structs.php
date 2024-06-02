<?php

function type_struct_name(string $type): ?string
{
    if(strpos($type, "%struct.") === 0) {
        return substr($type, 8);
    }
    return NULL;
}

function get_union_type(array $types): string
{
    $type = $types[0];
    $size = get_type_size($type);
    foreach($types as $t) {
        $s = get_type_size($t);
        if($s > $size) {
            $size = $s;
            $type = $t;
        }
    }
    return $type;
}
function get_type_size(string $type): int
{
    if($type == '[') {
        $split = explode(' x ', trim($type, "[]"));
        return get_type_size($split[1]) * intval($split[0]);
    }
    if($type == 'ptr') return 8;
    if($type == 'i64') return 8;
    if($type == 'i32') return 4;
    if($type == 'i16') return 2;
    if($type == 'i8') return 1;
    // if(strpos($type, "%struct.") === 0) return 0;
    var_dump('Cannot find type size for "' . $type . '"');
    exit;
}

function get_valk_type(string $type, array &$structs, array &$unions): string
{
    if($type[0] == '[') {
        $split = explode(' x ', trim($type, "[]"));
        return 'inline [' . get_valk_type($split[1], $structs, $unions) . ', ' . $split[0] . ']';
    }
    if (strpos($type, '%struct.') === 0) {
        $cname = str_replace('%struct.', '', $type);
        return 'inline ' . ($structs[$cname]['valk_name'] ?? $cname);
    }
    if (strpos($type, '%union.') === 0) {
        $uname = str_replace('%union.', '', $type);
        return get_valk_type($unions[$uname], $structs, $unions);
    } 
    return $type;
}

function gen_valk_structs(string $ir, array $vars): string
{
    $structs = [];
    $unions = [];

    // Scan structs
    $mm = [];
    preg_match_all('/\n%struct\.([a-zA-Z0-9_]+) = type (<?){([^}]+)}/', $ir, $mm);
    foreach($mm[1] as $k => $name) {
        $fields = trim($mm[3][$k]);
        $packed = $mm[2][$k] === '<';
        $types = explode(", ", $fields);
        $structs[$name] = ['types' => $types, 'packed' => $packed];
    }

    // Scan unions
    $mm = [];
    preg_match_all('/\n%union\.([a-zA-Z0-9_]+) = type {([^}]+)}/', $ir, $mm);
    foreach($mm[1] as $k => $name) {
        $types = trim($mm[2][$k]);
        $types = explode(", ", $types);
        $unions[$name] = get_union_type($types);
    }

    // Scan vars
    $mm = [];
    preg_match_all('/\n@var_([a-zA-Z0-9_]+) = (dso_local )?global ([^,]+) zeroinitializer/', $ir, $mm);
    foreach($mm[1] as $k => $name) {
        $type = trim($mm[3][$k]);
        $struct_name = type_struct_name($type);

        if(!$struct_name) {
            // Inline type
            $structs[$name] = ['types' => [$type], 'valk_name' => 'libc_' . $name, 'packed' => false];
            $struct_name = $name;
        }

        if(isset($vars[$name]['fields'])) {
            $fields = $vars[$name]['fields'];
            $structs[$struct_name]['fields'] = $fields;
            $structs[$struct_name]['valk_name'] = 'libc_' . $name;
            if(count($fields) !== count($structs[$struct_name]['types'])) {
                var_dump($structs[$struct_name]);
                var_dump($struct_name);
                echo "Field names & struct types count do not match\n";
                exit;
            }
        }
    }

    // Gen valk
    $code = "";
    foreach($structs as $cname => $s) {
        $valk_name = $s['valk_name'] ?? $cname;
        $code .= "\nstruct " . $valk_name  . ($s['packed'] ? ' packed' : '') . " {\n";
        foreach($s['types'] as $i => $type) {
            $names = $s['fields'] ?? [];
            $fn = $names[$i] ?? ('prop_' . $i);
            $vtype = get_valk_type($type, $structs, $unions);
            $code .= "    $fn: $vtype\n";
        }
        $code .= "}\n";
    }

    // var_dump($code);

    return $code;
}
