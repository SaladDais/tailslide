import os.path

SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
SYNTAX_PATH = os.path.join(SCRIPT_PATH, "builtins.txt")
CONST_TEMPLATE = 'const %(const_type)s %(name)s = %(val)s'
FUNC_TEMPLATE = '%(func_type)s %(name)s( %(args)s )'
EVENT_TEMPLATE = 'event %(name)s( %(args)s )'


def _escape_cstr_char(char: str):
    if char == "\\":
        return "\\\\"
    if char == "\n":
        return "\\n"
    if char == "\"":
        return r'\"'
    return char


def main():
    with open(SYNTAX_PATH) as f:
        all_code = tuple(f.readlines())

    with open('libtailslide/builtins_txt.cc', 'w') as f:
        f.write("""#ifdef _WIN32
#pragma execution_character_set("utf-8")
#endif
namespace Tailslide {
const char *builtins_txt[] = {
""")
        for line in all_code:
            line = line.strip()
            if not line:
                continue
            f.write('"')
            f.write("".join(_escape_cstr_char(x) for x in line))
            f.write('",\n')

        f.write("(char*)nullptr\n};\n}\n")


if __name__ == "__main__":
    main()
