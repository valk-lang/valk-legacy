<?php

function gen_enums(array $defines, array $target): string
{
    $names = [];
    foreach ($defines as $define) {
        if (preg_match('/^#define ([A-Z_]+) /', $define, $m)) {
            $name = $m[1];
            $names[] = $name;
        }
    }

    $code = "\n";
    $code .= '#pragma clang diagnostic ignored "-Wpragma-pack"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wignored-attributes"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wignored-pragma-intrinsic"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wmicrosoft-anon-tag"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wdeprecated-declarations"' . "\n";

    foreach($target['imports'] as $import) {
        $code .= "#include <$import.h>\n";
    }
    $code .= "\n";

    $code .= "\n";

    $code .= "int main() {\n";
    foreach($names as $name) {
        $code .= "printf(\"value $name (%d)\\n\", $name);";
    }
    $code .= "}\n";

    $tmp = get_tmp_dir();
    $cpath = $tmp . '/gen-enums.c';
    $opath = $tmp . '/gen-enums';
    file_put_contents($cpath, $code);

    $cmd = get_base_cmd($target);
    $cmd = $cmd . " -c $cpath -o $opath";
    echo $cmd . "\n";
    exec($cmd);
    $out = [];
    echo $opath . "\n";
    exec($opath, $out);
    var_dump($out);
    exit;
}
function gen_enums_(array $defines, object $ast_data): string
{

    $data = $ast_data->inner;
    $lookup = (object)[];
    foreach($data as $item) {
        if($item->kind == 'EnumDecl') {
            $subs = $item->inner;
            foreach($subs as $sub) {
                if ($sub->kind == 'EnumConstantDecl') {
                    $name = $sub->name;
                    if (isset($sub->inner[0]->value)) {
                        $value = $sub->inner[0]->value;
                        $lookup->$name = $value;
                    }
                }
            }
        }
    }

    $code = "\n";
    // e.g. "#define linux 1"
    foreach($defines as $define) {
        $m = [];
        // Octal
        if(preg_match('/^#define ([A-Z_]+) (0[0-9]+)$/', $define, $m)) {
            $name = $m[1];
            $value = $m[2];
            $value = '0c' . substr($value, 1, strlen($value) - 1);
            $code .= "value $name ($value)\n";
            continue;
        }
        // Numbers & Hex
        if(preg_match('/^#define ([A-Z_]+) ((0x[0-9a-fA-F]+)|([0-9]+)L?)$/', $define, $m)) {
            $name = $m[1];
            $value = $m[2];
            if(str_ends_with($value, 'L')) {
                $value = rtrim($value, "L");
            }
            $code .= "value $name ($value)\n";
            continue;
        }
        if(preg_match('/^#define ([A-Z_]+) ([0-9A-Z_x\|\&\^\-\+\ ()]+)$/', $define, $m)) {
            $name = $m[1];
            $value = $m[2];
            if(!is_numeric($value) && isset($lookup->$value)) {
                $value = $lookup->$value;
            }
            $code .= "value $name ($value)\n";
            continue;
        }
    }
    $code .= "\n";
    return $code;
}