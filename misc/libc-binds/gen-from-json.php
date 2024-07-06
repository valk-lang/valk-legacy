<?php

function extract_c_base_type(string $ctype): string {
    $result = $ctype;

    $p = strpos($result, '*');
    if ($p !== false)
        $result = substr($result, 0, $p);
    $p = strpos($result, '[');
    if ($p !== false)
        $result = substr($result, 0, $p);

    return $result;
}

function mark_used_struct($struct, $lookup) {
    // if(isset($lookup->set_name)) {
    //     $struct->name = $lookup->set_name;
    //     $lookup->set_name = null;
    // }
    if($struct->used)
        return;
    $struct->used = true;
    foreach($struct->fields as $field) {
        mark_used_type($field->type, $lookup);
    }
}

function mark_used_td($td, $lookup) {
    if ($td->kind == 'BuiltinType') {
        return;
    }
    if ($td->kind == 'TypedefType') {
        if(isset($td->decl->id)) {
            $subtd = $lookup->typedefs->{$td->decl->id};
            return mark_used_td($subtd[0], $lookup);
        }
        foreach($td->inner as $td_item) {
            mark_used_td($td_item, $lookup);
        }
        return;
    }
    if ($td->kind == 'ElaboratedType') {
        foreach($td->inner as $td_item) {
            mark_used_td($td_item, $lookup);
        }
        return;
    }
    if ($td->kind == 'ConstantArrayType') {
        foreach($td->inner as $td_item) {
            mark_used_td($td_item, $lookup);
        }
        return;
    }
    if ($td->kind == 'RecordType') {
        $struct_id = $td->decl->id;
        $struct = $lookup->structs_by_id->{$struct_id};
        mark_used_struct($struct, $lookup);
    }
}

function mark_used_type($type, $lookup) {
    if(isset($type->typeAliasDeclId)) {
        $id = $type->typeAliasDeclId;
        $typedef = $lookup->typedefs->$id;
        $td = $typedef[0];
        mark_used_td($td, $lookup);
        return;
    }

    $qt = $type->qualType;
    if(isset($lookup->structs->$qt)) {
        $struct = $lookup->structs->$qt;
        mark_used_struct($struct, $lookup);
        return;
    }
}

function convert_type_td($td, $lookup) : String {
    if($td->kind == 'ConstantArrayType') {
        $sub_td = $td->inner[0];
        return "inline [" . convert_type_td($sub_td, $lookup) . ", " . $td->size . "]";
    }
    if($td->kind == 'BuiltinType') {
        return convert_type($td->type, $lookup);
    }
    if ($td->kind == 'ElaboratedType') {
        return convert_type_td($td->inner[0], $lookup);
    }
    if ($td->kind == 'TypedefType') {
        if(isset($td->decl->id)) {
            $subtd = $lookup->typedefs->{$td->decl->id};
            return convert_type_td($subtd[0], $lookup);
        }
        return convert_type_td($td->inner[0], $lookup);
    }
    if ($td->kind == 'RecordType') {
        $struct_id = $td->decl->id;
        $struct = $lookup->structs_by_id->{$struct_id};
        return "inline libc_gen_" .  $struct->name;
    }
    var_dump("Unknown kind: $td->kind");
    exit(1);
}

function convert_type_qt(string $qt, object $lookup) : String {

    // Struct
    if(isset($lookup->structs->$qt)) {
        $struct = $lookup->structs->$qt;
        return "inline libc_gen_" .  $struct->name;
    }

    // Array
    if(str_ends_with($qt, ']')) {
        $p = strrpos($qt, '[');
        $subqt = substr($qt, 0, $p);
        $end = strlen($qt) - 1;
        $size = substr($qt, $p + 1, $end - $p - 1);
        $size = intval($size);
        $conv = convert_type_qt($subqt, $lookup);
        return "inline [$conv, $size]";
    }
    if(str_ends_with($qt, '*')) {
        $subqt = rtrim(substr($qt, 0, -1));
        if($subqt == 'char') 
            return 'cstring';
        if($subqt == 'void') 
            return 'ptr';
        $conv = convert_type_qt($subqt, $lookup);
        if(!str_starts_with($conv, 'inline '))
            return "ptr";
        return substr($conv, 7, strlen($conv) - 7);
    }

    // Ints
    if($qt == 'char')
        return "i8";
    if($qt == 'unsigned char')
        return "u8";
    if($qt == 'short')
        return "i16";
    if($qt == 'unsigned short')
        return "u16";
    if($qt == 'int')
        return "i32";
    if($qt == 'unsigned int')
        return "u32";
    if($qt == 'long')
        return "int";
    if($qt == 'unsigned long')
        return "uint";
    if($qt == 'long long')
        return "int";
    if($qt == 'unsigned long long')
        return "uint";

    if(isset($lookup->typedefs_by_name->$qt)) {
        $td = $lookup->typedefs_by_name->$qt;
        return convert_type_td($td[0], $lookup);
    }

    var_dump("Cannot convert qual type: $qt");
    exit(1);
}

function convert_type(object $type, object $lookup) : String {
    $result = "";

    if(isset($type->typeAliasDeclId)) {
        $id = $type->typeAliasDeclId;
        $typedef = $lookup->typedefs->$id;
        $td = $typedef[0];
        return convert_type_td($td, $lookup);
    }

    // Struct
    $qt = $type->qualType ?? null;
    if($qt) {
        return convert_type_qt($qt, $lookup);
    }

    var_dump("Cannot convert type: " . json_encode($type));
    exit(1);
}

function gen_valk_structs_ast(string $json, array $target): string
{
    $code = "";
    $data = json_decode($json)->inner;

    $vars = $target['vars'];

    $anon_count = 0;
    $jvars = (object)[];
    $structs = (object)[];
    $structs_by_id = (object)[];
    $typedefs = (object)[];
    $typedefs_by_name = (object)[];
    $lookup = (object)[
        'structs' => $structs,
        'structs_by_id' => $structs_by_id,
        'typedefs' => $typedefs,
        'typedefs_by_name' => $typedefs_by_name,
    ];

    foreach($data as $item) {
        // Vars
        if($item->kind == 'VarDecl') {
            $jvars->{$item->name} = $item->type;
            continue;
        }
        // Structs
        if($item->kind == 'RecordDecl') {
            if (!isset($item->inner)) {
                continue;
            }
            $bitfields = 0;
            $fields = (object)[];
            foreach($item->inner as $field) {
                if ($field->kind != 'FieldDecl')
                    continue;
                if(!isset($field->name)) {
                    $field->name = 'bitfield_' . ($bitfields++);
                }
                $fields->{$field->name} = (object)[
                    'type' => $field->type
                ];
            }

            $name = $item->name ?? 'anon_struct_' . ($anon_count++);

            $struct = (object)[
                'name' => $name,
                'used' => false,
                'fields' => $fields,
            ];
            $structs->{'struct ' . $name} = $struct;
            $structs_by_id->{$item->id} = $struct;
        }
        // Typedefs
        if($item->kind == 'TypedefDecl') {
            $typedefs->{$item->id} = $item->inner;
            if(isset($item->name)) {
                $typedefs_by_name->{$item->name} = $item->inner;
            }
        }
    }

    foreach($vars as $valk_name => $var) {
        $type = $jvars->{'var_' . $valk_name};
        // $lookup->set_name = $valk_name;
        mark_used_type($type, $lookup);
        // $type->valk_name = $valk_name;
    }

    // // Remove un-used
    foreach($structs as $name => $struct) {
        if($struct->used)
            continue;
        unset($structs->$name);
    }

    // file_put_contents(__DIR__ . '/tmp/debug.json', json_encode($lookup, JSON_PRETTY_PRINT));
    // file_put_contents(__DIR__ . '/tmp/debug-vars.json', json_encode($jvars, JSON_PRETTY_PRINT));

    $code .= "\n";
    foreach($vars as $valk_name => $var) {
        $type = $jvars->{'var_' . $valk_name};
        $conv = convert_type($type, $lookup);
        $conv = substr($conv, 7, strlen($conv) - 7);
        $code .= "type libc_$valk_name ($conv)\n";
    }

    // Write code
    $code .= "\n";
    foreach($structs as $name => $struct) {
        $code .= "cstruct libc_gen_" . $struct->name . " {\n";
        foreach ($struct->fields as $name => $field) {
            $type = convert_type($field->type, $lookup);
            $code .= "    $name: " . $type . "\n";
        }
        $code .= "}\n\n";
    }

    // file_put_contents(__DIR__ . '/tmp/result.va', $code);
    // exit;

    return $code;
}