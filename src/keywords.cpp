#include "keywords.h"

const QStringList& cppKeywords() {
    static const QStringList keywords = {
        "auto", "break", "case", "catch", "char", "class", "const", "constexpr",
        "continue", "default", "delete", "do", "double", "else", "enum", "explicit",
        "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int",
        "long", "mutable", "namespace", "new", "nullptr", "operator", "private",
        "protected", "public", "register", "return", "short", "signed", "sizeof",
        "static", "struct", "switch", "template", "this", "throw", "true", "try",
        "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual",
        "void", "volatile", "while", "override", "final", "static_assert",
        "cout", "cin", "cerr", "endl", "string", "vector", "map", "set",
        "pair", "list", "deque", "queue", "stack", "array", "unordered_map",
        "unordered_set", "shared_ptr", "unique_ptr", "weak_ptr", "make_shared",
        "make_unique", "function", "bind", "thread", "mutex", "lock_guard",
        "unique_lock", "condition_variable", "future", "promise", "async",
        "printf", "scanf", "malloc", "free", "calloc", "realloc", "memcpy",
        "memset", "strlen", "strcpy", "strcat", "strcmp", "fprintf", "fscanf",
        "fopen", "fclose", "fread", "fwrite", "include", "define", "ifdef",
        "ifndef", "endif", "pragma", "error", "warning", "line",
    };
    return keywords;
}

const QStringList& csharpKeywords() {
    static const QStringList keywords = {
        "abstract", "as", "base", "bool", "break", "byte", "case", "catch",
        "char", "checked", "class", "const", "continue", "decimal", "default",
        "delegate", "do", "double", "else", "enum", "event", "explicit", "extern",
        "false", "finally", "fixed", "float", "for", "foreach", "goto", "if",
        "implicit", "in", "int", "interface", "internal", "is", "lock", "long",
        "namespace", "new", "null", "object", "operator", "out", "override",
        "params", "private", "protected", "public", "readonly", "ref", "return",
        "sbyte", "sealed", "short", "sizeof", "stackalloc", "static", "string",
        "struct", "switch", "this", "throw", "true", "try", "typeof", "uint",
        "ulong", "unchecked", "unsafe", "ushort", "using", "var", "virtual",
        "void", "volatile", "while", "async", "await", "dynamic", "yield",
        "Console", "WriteLine", "ReadLine", "String", "List", "Dictionary",
        "Array", "IEnumerable", "ICollection", "IList", "Task",
    };
    return keywords;
}

const QStringList& javaKeywords() {
    static const QStringList keywords = {
        "abstract", "assert", "boolean", "break", "byte", "case", "catch",
        "char", "class", "const", "continue", "default", "do", "double", "else",
        "enum", "extends", "final", "finally", "float", "for", "goto", "if",
        "implements", "import", "instanceof", "int", "interface", "long", "native",
        "new", "null", "package", "private", "protected", "public", "return",
        "short", "static", "strictfp", "super", "switch", "synchronized", "this",
        "throw", "throws", "transient", "try", "var", "void", "volatile", "while",
        "String", "System", "out", "println", "Integer", "Double", "Boolean",
        "ArrayList", "HashMap", "HashSet", "List", "Map", "Set", "Collection",
        "Arrays", "Collections", "Optional", "Stream", "Runnable", "Thread",
    };
    return keywords;
}

const QStringList& pythonKeywords() {
    static const QStringList keywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
        "while", "with", "yield",
        "print", "len", "range", "int", "str", "float", "list", "dict",
        "set", "tuple", "bool", "type", "isinstance", "issubclass", "hasattr",
        "getattr", "setattr", "delattr", "super", "property", "staticmethod",
        "classmethod", "enumerate", "zip", "map", "filter", "sorted", "reversed",
        "open", "input", "format", "abs", "all", "any", "bin", "chr", "ord",
        "hex", "max", "min", "sum", "round", "iter", "next", "slice",
        "self", "__init__", "__str__", "__repr__", "__len__", "__getitem__",
        "__setitem__", "__delitem__", "__iter__", "__next__", "__enter__",
        "__exit__", "__call__", "__eq__", "__ne__", "__lt__", "__le__",
        "__gt__", "__ge__", "__add__", "__sub__", "__mul__", "__truediv__",
        "os", "sys", "json", "re", "math", "datetime", "collections",
        "itertools", "functools", "pathlib", "typing",
    };
    return keywords;
}

const QStringList& javascriptKeywords() {
    static const QStringList keywords = {
        "break", "case", "catch", "class", "const", "continue", "debugger",
        "default", "delete", "do", "else", "export", "extends", "false",
        "finally", "for", "function", "if", "import", "in", "instanceof",
        "new", "null", "return", "super", "switch", "this", "throw", "true",
        "try", "typeof", "var", "void", "while", "with", "yield", "let",
        "async", "await", "of",
        "console", "log", "warn", "error", "info", "table", "dir",
        "document", "window", "navigator", "location", "history",
        "Array", "Object", "String", "Number", "Boolean", "Date", "RegExp",
        "Map", "Set", "WeakMap", "WeakSet", "Promise", "Symbol", "Proxy",
        "JSON", "Math", "parseInt", "parseFloat", "isNaN", "isFinite",
        "setTimeout", "setInterval", "clearTimeout", "clearInterval",
        "fetch", "XMLHttpRequest", "addEventListener", "removeEventListener",
        "querySelector", "querySelectorAll", "getElementById", "getElementsByClassName",
        "createElement", "appendChild", "removeChild", "innerHTML", "textContent",
        "preventDefault", "stopPropagation",
        "map", "filter", "reduce", "forEach", "find", "findIndex", "some", "every",
        "includes", "indexOf", "slice", "splice", "push", "pop", "shift", "unshift",
        "toString", "valueOf", "hasOwnProperty", "keys", "values", "entries",
    };
    return keywords;
}

const QStringList& htmlKeywords() {
    static const QStringList keywords = {
        "html", "head", "title", "body", "div", "span", "p", "a", "img", "ul",
        "ol", "li", "table", "tr", "td", "th", "form", "input", "button", "select",
        "option", "textarea", "label", "fieldset", "legend", "h1", "h2", "h3",
        "h4", "h5", "h6", "header", "footer", "nav", "main", "section", "article",
        "aside", "figure", "figcaption", "details", "summary", "dialog", "canvas",
        "svg", "video", "audio", "source", "iframe", "embed", "object", "param",
        "script", "link", "meta", "base", "br", "hr", "col", "colgroup",
        "caption", "thead", "tbody", "tfoot", "dl", "dt", "dd",
        "class", "id", "style", "href", "src", "alt", "width", "height",
        "type", "name", "value", "placeholder", "disabled", "required", "checked",
        "selected", "readonly", "maxlength", "min", "max", "step", "pattern",
        "target", "rel", "media", "charset", "content", "lang", "dir",
    };
    return keywords;
}

const QStringList& cssKeywords() {
    static const QStringList keywords = {
        "color", "background", "background-color", "background-image",
        "background-repeat", "background-position", "background-size",
        "border", "border-color", "border-style", "border-width", "border-radius",
        "margin", "margin-top", "margin-right", "margin-bottom", "margin-left",
        "padding", "padding-top", "padding-right", "padding-bottom", "padding-left",
        "font", "font-family", "font-size", "font-weight", "font-style",
        "text-align", "text-decoration", "text-transform", "line-height",
        "letter-spacing", "word-spacing", "white-space", "overflow",
        "display", "position", "top", "right", "bottom", "left", "z-index",
        "width", "height", "min-width", "min-height", "max-width", "max-height",
        "flex", "flex-direction", "flex-wrap", "justify-content", "align-items",
        "grid", "grid-template-columns", "grid-template-rows", "grid-gap",
        "opacity", "visibility", "cursor", "transition", "transform",
        "animation", "box-shadow", "text-shadow", "outline", "list-style",
        "float", "clear", "vertical-align", "box-sizing", "content",
    };
    return keywords;
}

const QStringList& xmlKeywords() {
    static const QStringList keywords = {
        "xml", "version", "encoding", "standalone",
        "DOCTYPE", "ELEMENT", "ATTLIST", "ENTITY", "NOTATION",
        "xmlns", "xsi", "schemaLocation",
    };
    return keywords;
}

const QStringList& sqlKeywords() {
    static const QStringList keywords = {
        "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE", "CREATE",
        "DROP", "ALTER", "TABLE", "INTO", "VALUES", "SET", "JOIN", "INNER",
        "LEFT", "RIGHT", "FULL", "OUTER", "ON", "AND", "OR", "NOT", "IN",
        "LIKE", "BETWEEN", "IS", "NULL", "AS", "ORDER", "BY", "GROUP",
        "HAVING", "LIMIT", "OFFSET", "UNION", "ALL", "DISTINCT", "COUNT",
        "SUM", "AVG", "MIN", "MAX", "INDEX", "VIEW", "PRIMARY", "KEY",
        "FOREIGN", "REFERENCES", "CONSTRAINT", "DEFAULT", "CHECK", "UNIQUE",
        "INTEGER", "VARCHAR", "CHAR", "TEXT", "BOOLEAN", "DATE", "TIME",
        "TIMESTAMP", "DECIMAL", "FLOAT", "DOUBLE", "REAL", "BLOB",
        "EXISTS", "CASE", "WHEN", "THEN", "ELSE", "END", "CAST", "COALESCE",
        "TRUNCATE", "RENAME", "ADD", "COLUMN", "MODIFY", "DESC", "ASC",
    };
    return keywords;
}

const QStringList& typescriptKeywords() {
    static const QStringList keywords = {
        "break", "case", "catch", "class", "const", "continue", "debugger",
        "default", "delete", "do", "else", "enum", "export", "extends", "false",
        "finally", "for", "function", "if", "implements", "import", "in",
        "instanceof", "interface", "let", "new", "null", "of", "package",
        "private", "protected", "public", "return", "static", "super", "switch",
        "this", "throw", "true", "try", "type", "typeof", "var", "void",
        "while", "with", "yield", "async", "await", "readonly", "abstract",
        "as", "any", "boolean", "number", "string", "symbol", "undefined",
        "never", "object", "unknown", "bigint", "namespace", "module",
        "declare", "from", "require", "keyof", "infer", "satisfies",
        "console", "log", "warn", "error", "info",
        "Array", "Object", "String", "Number", "Boolean", "Date", "RegExp",
        "Map", "Set", "Promise", "Symbol", "JSON", "Math",
        "setTimeout", "setInterval", "clearTimeout", "clearInterval",
        "fetch", "addEventListener", "removeEventListener",
        "map", "filter", "reduce", "forEach", "find", "findIndex", "some", "every",
        "includes", "indexOf", "slice", "splice", "push", "pop", "shift", "unshift",
    };
    return keywords;
}

const QStringList& rustKeywords() {
    static const QStringList keywords = {
        "as", "async", "await", "break", "const", "continue", "crate", "dyn",
        "else", "enum", "extern", "false", "fn", "for", "if", "impl", "in",
        "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return",
        "self", "Self", "static", "struct", "super", "trait", "true", "type",
        "union", "unsafe", "use", "where", "while", "yield",
        "i8", "i16", "i32", "i64", "i128", "isize",
        "u8", "u16", "u32", "u64", "u128", "usize",
        "f32", "f64", "bool", "char", "str", "String", "Vec",
        "Option", "Result", "Box", "Rc", "Arc", "Cell", "RefCell",
        "HashMap", "HashSet", "BTreeMap", "BTreeSet",
        "Some", "None", "Ok", "Err",
        "println", "print", "format", "panic", "assert", "assert_eq",
        "vec", "clone", "unwrap", "expect", "map", "and_then",
        "iter", "into_iter", "collect", "fold", "filter",
    };
    return keywords;
}

const QStringList& goKeywords() {
    static const QStringList keywords = {
        "break", "case", "chan", "const", "continue", "default", "defer",
        "else", "fallthrough", "for", "func", "go", "goto", "if",
        "import", "interface", "map", "package", "range", "return",
        "select", "struct", "switch", "type", "var",
        "bool", "byte", "complex64", "complex128", "error",
        "float32", "float64", "int", "int8", "int16", "int32", "int64",
        "rune", "string", "uint", "uint8", "uint16", "uint32", "uint64",
        "uintptr", "true", "false", "iota", "nil",
        "append", "cap", "close", "copy", "delete", "len", "make",
        "new", "panic", "print", "println", "recover",
        "stringer", "fmt", "Printf", "Sprintf", "Fprintf",
        "http", "json", "ioutil", "strconv", "strings", "bytes",
        "File", "Reader", "Writer", "Handler", "ServeMux",
    };
    return keywords;
}

const QStringList& markdownKeywords() {
    static const QStringList keywords = {
        "#", "##", "###", "####", "#####", "######",
        "*", "**", "***", "_", "__", "___",
        ">", ">>", ">>>",
        "-", "+",
        "1.", "2.", "3.",
        "```", "~~~",
        "[", "]", "(", ")",
        "!", "|",
        "---",
        "title", "author", "date",
        "table", "thead", "tbody", "th", "tr", "td",
    };
    return keywords;
}

const QStringList& bashKeywords() {
    static const QStringList keywords = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for", "while",
        "until", "do", "done", "function", "select", "in", "return", "exit",
        "break", "continue", "shift", "exec", "eval", "set", "unset",
        "export", "readonly", "local", "declare", "typeset", "source",
        "echo", "printf", "read", "test", "true", "false", "let",
        "alias", "unalias", "cd", "pwd", "pushd", "popd", "dirs",
        "ls", "cat", "grep", "sed", "awk", "cut", "sort", "uniq", "wc",
        "head", "tail", "find", "xargs", "tee", "tr", "chmod", "chown",
        "mkdir", "rmdir", "rm", "cp", "mv", "ln", "tar", "gzip", "gunzip",
        "ssh", "scp", "rsync", "curl", "wget", "ping", "netstat",
        "sudo", "su", "whoami", "id", "ps", "kill", "killall", "top",
        "env", "printenv", "which", "whereis", "man", "info", "history",
        "$0", "$1", "$2", "$@", "$#", "$?", "$$", "$!", "$-", "$_",
        "$HOME", "$PATH", "$USER", "$PWD", "$OLDPWD", "$SHELL", "$LANG",
        "$TERM", "$EDITOR", "$VISUAL", "$TMPDIR",
    };
    return keywords;
}

const QStringList& cmakeKeywords() {
    static const QStringList keywords = {
        "add_compile_definitions", "add_compile_options", "add_custom_command",
        "add_custom_target", "add_definitions", "add_dependencies",
        "add_executable", "add_library", "add_subdirectory",
        "add_test", "aux_source_directory", "break", "build_command",
        "build_id", "cmake_minimum_required", "configure_file",
        "continue", "create_test_sourcelist", "ctest_build",
        "ctest_configure", "ctest_coverage", "ctest_empty_binary_directory",
        "ctest_memcheck", "ctest_read_custom_files", "ctest_run_script",
        "ctest_sleep", "ctest_start", "ctest_submit", "ctest_test",
        "ctest_update", "ctest_upload", "define_property",
        "else", "elseif", "enable_language", "enable_testing",
        "endforeach", "endfunction", "endif", "endmacro", "endwhile",
        "execute_process", "export", "file", "find_file", "find_library",
        "find_package", "find_path", "find_program", "fltk_wrap_ui",
        "foreach", "function", "get_cmake_property", "get_directory_property",
        "get_filename_component", "get_property", "get_source_file_property",
        "get_target_property", "get_test_property", "if", "include",
        "include_directories", "include_external_msproject",
        "include_regular_expression", "install", "install_files",
        "install_programs", "install_targets", "link_directories",
        "list", "load_cache", "load_command", "macro",
        "make_directory", "mark_as_advanced", "math", "message",
        "option", "output_required_files", "project", "qt_wrap_cpp",
        "qt_wrap_ui", "remove_definitions", "return", "separate_arguments",
        "set", "set_directory_properties", "set_property",
        "set_source_files_properties", "set_target_properties",
        "set_tests_properties", "site_name", "source_group",
        "string", "target_compile_definitions", "target_compile_features",
        "target_compile_options", "target_include_directories",
        "target_link_directories", "target_link_libraries",
        "target_link_options", "target_precompile_headers",
        "target_sources", "try_compile", "try_run", "unset",
        "variable_watch", "while", "write_file",
        "CMAKE_BINARY_DIR", "CMAKE_BUILD_TYPE", "CMAKE_CACHEFILE_DIR",
        "CMAKE_CACHE_MINOR_VERSION", "CMAKE_COMMAND", "CMAKE_CPACK_COMMAND",
        "CMAKE_CTEST_COMMAND", "CMAKE_CURRENT_BINARY_DIR",
        "CMAKE_CURRENT_LIST_DIR", "CMAKE_CURRENT_LIST_FILE",
        "CMAKE_CURRENT_LIST_LINE", "CMAKE_CURRENT_SOURCE_DIR",
        "CMAKE_DIRECTORY_LABELS", "CMAKE_DL_LIBS", "CMAKE_EXECUTABLE_SUFFIX",
        "CMAKE_EXTRA_GENERATOR", "CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES",
        "CMAKE_GENERATOR", "CMAKE_HOME_DIRECTORY", "CMAKE_INSTALL_PREFIX",
        "CMAKE_MAJOR_VERSION", "CMAKE_MATCH_0", "CMAKE_MATCH_1",
        "CMAKE_MATCH_2", "CMAKE_MATCH_3", "CMAKE_MATCH_4",
        "CMAKE_MATCH_5", "CMAKE_MATCH_6", "CMAKE_MATCH_7",
        "CMAKE_MATCH_8", "CMAKE_MATCH_9", "CMAKE_MINIMUM_REQUIRED_VERSION",
        "CMAKE_MINOR_VERSION", "CMAKE_MODULE_PATH", "CMAKE_OSX_ARCHITECTURES",
        "CMAKE_OSX_DEPLOYMENT_TARGET", "CMAKE_OSX_SYSROOT",
        "CMAKE_PARENT_LIST_FILE", "CMAKE_PATCH_VERSION", "CMAKE_PROJECT_NAME",
        "CMAKE_RUNTIME_OUTPUT_DIRECTORY", "CMAKE_SOURCE_DIR",
        "CMAKE_STATIC_LIBRARY_PREFIX", "CMAKE_STATIC_LIBRARY_SUFFIX",
        "CMAKE_TWEAK_VERSION", "CMAKE_VERBOSE_MAKEFILE",
        "CMAKE_VERSION", "PROJECT_BINARY_DIR", "PROJECT_NAME",
        "PROJECT_SOURCE_DIR", "PROJECT_VERSION", "PROJECT_VERSION_MAJOR",
        "PROJECT_VERSION_MINOR", "PROJECT_VERSION_PATCH",
        "${CMAKE_CURRENT_SOURCE_DIR}", "${CMAKE_BINARY_DIR}",
        "${CMAKE_SOURCE_DIR}", "${CMAKE_CURRENT_LIST_DIR}",
        "${PROJECT_BINARY_DIR}", "${PROJECT_SOURCE_DIR}",
        "${PROJECT_NAME}", "${CMAKE_INSTALL_PREFIX}",
    };
    return keywords;
}
