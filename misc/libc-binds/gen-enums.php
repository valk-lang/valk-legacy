<?php

function gen_enums(array $defines, array $target): string
{
    $names = [];
    $ignores = ['SCM_SRCRT'];
    foreach ($defines as $define) {
        if (preg_match('/^#define ([A-Z_]+) /', $define, $m)) {
            $name = $m[1];
            if(str_starts_with($name, "_"))
                continue;
            if(in_array($name, $ignores, true))
                continue;
            $names[] = $name;
        }
    }

    $code = "\n";
    $code .= '#pragma clang diagnostic ignored "-Wformat"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wpragma-pack"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-W#pragma-messages"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wignored-attributes"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wignored-pragma-intrinsic"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wmicrosoft-anon-tag"' . "\n";
    $code .= '#pragma clang diagnostic ignored "-Wdeprecated-declarations"' . "\n";
    $code .= '#define typename(x) _Generic((x),                                                 \
            _Bool: "_Bool",                  unsigned char: "unsigned char",          \
             char: "char",                     signed char: "signed char",            \
        short int: "short int",         unsigned short int: "unsigned short int",     \
              int: "int",                     unsigned int: "unsigned int",           \
         long int: "long int",           unsigned long int: "unsigned long int",      \
    long long int: "long long int", unsigned long long int: "unsigned long long int", \
            float: "float",                         double: "double",                 \
      long double: "long double",                   char *: "pointer to char",        \
           void *: "pointer to void",                int *: "pointer to int",         \
          default: "other")' . "\n\n";

    foreach($target['imports'] as $import) {
        $code .= "#include <$import.h>\n";
    }
    $code .= "\n";

    $code .= "\n";

    $code .= "int main() {\n";
    foreach($names as $i => $name) {
        $code .= "if(strcmp(typename($name), \"int\") == 0) ";
        $code .= "printf(\"value $name (%d)\\n\", $name);\n";
        $code .= "if(strcmp(typename($name), \"long int\") == 0) ";
        $code .= "printf(\"value $name (%ld)\\n\", $name);\n";
        $code .= "if(strcmp(typename($name), \"unsigned int\") == 0) ";
        $code .= "printf(\"value $name (%u)\\n\", $name);\n";
        $code .= "if(strcmp(typename($name), \"unsigned long int\") == 0) ";
        $code .= "printf(\"value $name (%lu)\\n\", $name);\n";
    }
    $code .= "}\n";

    $tmp = get_tmp_dir();
    $cpath = $tmp . '/gen-enums.c';
    $opath = $tmp . '/gen-enums';
    file_put_contents($cpath, $code);

    $cmd = get_base_cmd($target);
    $cmd = $cmd . " $cpath -o $opath";
    echo $cmd . "\n";
    exec($cmd);
    exec("chmod +x $opath");
    $out = [];
    echo $opath . "\n";
    exec($opath, $out);
    // var_dump($out);
    // exit;
    return implode("\n", $out);
}
function gen_enums_(array $defines, object $ast_data): string
{
    // Lookups
    $consts = (object)[];
    $lookup = (object)[];
    $params = (object)[];

    $data = $ast_data->inner;
    foreach($data as $item) {
        if($item->kind == 'EnumDecl') {
            $subs = $item->inner;
            foreach($subs as $sub) {
                if ($sub->kind == 'EnumConstantDecl') {
                    $name = $sub->name;
                    if (isset($sub->inner[0]->value)) {
                        $value = $sub->inner[0]->value;
                        $consts->$name = $value;
                    }
                }
            }
        }
    }

    foreach($defines as $i => $define) {
        if(preg_match('/^#define ([A-Z_]+) (.+)$/', $define, $m)) {
            $name = $m[1];
            $value = $m[2];

            if($name == $value) {
                if(!isset($consts->$name)) {
                    unset($defines[$i]);
                    continue;
                }
                $value = $consts->$name;
            }

            $lookup->$name = $value;
            if(!empty($args)) {
                $params->$name = explode(",", str_replace(' ', '', $args));
            }
        }
    }

    $code = "\n";
    // e.g. "#define linux 1"
    foreach($defines as $define) {
        $m = [];

        if(preg_match('/^#define ([A-Z_]+) (.+)$/', $define, $m)) {
            $name = $m[1];
            $value = $m[2];

            if(str_starts_with($name, "_"))
                continue;

            if($name == $value) {
                if(!isset($consts->$name))
                    continue;
                $value = $consts->$name;
            }

            $m = [];
            while (true) {
                // Octal
                $value = preg_replace('/([^0-9a-z]|^)0([0-9]+)/', '${1}0c$2', $value);
                // LL
                $value = preg_replace('/([0-9]+)(ull|ULL|ul|UL|u|U|ll|LL|l|L)([^a-zA-Z_]|$)/', '${1}${3}', $value);
                // var_dump($value);

                $value = trim($value, "()");

                if(preg_match('/[()]+/', $value)) {
                    $value = null;
                    break;
                } 

                if(!preg_match('/([^0-9A-Za-z_]|^)([A-Za-z_][A-Za-z0-9_]+)/', $value, $m, PREG_OFFSET_CAPTURE)) 
                    break;
                $word = $m[2][0];
                $pos = $m[2][1];

                if(!isset($lookup->$word)) {
                    $value = null;
                    break;
                }
                $repv = $lookup->$word;

                $value = substr($value, 0, $pos) . $repv . substr($value, $pos + strlen($word), strlen($value) - $pos + strlen($word));
            }

            if($value) {
                $code .= "value $name ($value)\n";
            }
            continue;
        }
    }
    $code .= "\n";
    // var_dump($code);
    // exit;
    return $code;
}