#!/usr/bin/env python3

import argparse
import codecs
import json
import re
import struct
import sys
import termios
import tty
import random
from dataclasses import asdict, dataclass
from datetime import datetime
from enum import Enum
from pathlib import Path
from typing import (
    Literal,
    Optional,
    TypeGuard,
    get_args,
    overload,
)


class BlolaException(Exception):
    pass


CppType = Literal[
    "signed char",
    "short int",
    "int",
    "long int",
    "long long int",
    "unsigned char",
    "unsigned short int",
    "unsigned int",
    "unsigned long int",
    "unsigned long long int",
    "float",
    "double",
    "void*",
    "intmax_t",
    "uintmax_t",
    "size_t",
]

SignedInt = Literal["i8", "i16", "i32", "i64"]
UnsignedInt = Literal["u8", "u16", "u32", "u64"]
Int = SignedInt | UnsignedInt
Float = Literal["f32", "f64"]
RealType = Int | Float


Platform = dict[CppType, RealType]


def signed_typeof(size: int) -> SignedInt:
    signed_map: dict[int, SignedInt] = {1: "i8", 2: "i16", 4: "i32", 8: "i64"}
    return signed_map[size]


def unsigned_typeof(size: int) -> UnsignedInt:
    unsigned_map: dict[int, UnsignedInt] = {1: "u8", 2: "u16", 4: "u32", 8: "u64"}
    return unsigned_map[size]


def sizeof(type: RealType) -> int:
    size_map: dict[RealType, int] = {
        "i8": 1,
        "i16": 2,
        "i32": 4,
        "i64": 8,
        "u8": 1,
        "u16": 2,
        "u32": 4,
        "u64": 8,
        "f32": 4,
        "f64": 8,
    }
    return size_map[type]


def is_signed(type: RealType) -> TypeGuard[SignedInt]:
    return type in get_args(SignedInt)


def is_unsigned(type: RealType) -> TypeGuard[UnsignedInt]:
    return type in get_args(UnsignedInt)


def is_float(type: RealType) -> TypeGuard[Float]:
    return type in get_args(Float)


@dataclass
class RawLog:
    file: str
    line: int
    message: str

    def asLog(self, sourceDir: Path, platform: Platform):
        fmt, types = parse_message(self.message)
        realTypes: tuple[RealType, ...] = tuple(platform[x] for x in types)
        return Log(
            id=(
                poly_hash_utf8(
                    poly_hash_utf8(poly_hash_uint(0, self.line), self.file),
                    self.message,
                )
                & 0xFFFF
            ),
            file=str(Path(self.file).resolve().relative_to(sourceDir.resolve())),
            line=self.line,
            message=fmt,
            args=realTypes,
        )


@dataclass
class Log:
    id: int
    file: str
    line: int
    message: str
    args: tuple[RealType, ...]


class LogDb:
    def __init__(self, logs: list[Log]):
        self.map = {x.id: x for x in logs}

    def with_id(self, id: int) -> Optional[Log]:
        return self.map.get(id)


class RawTerminal:
    def __init__(self, stream=sys.stdin):
        self.stream = stream
        self.fd = self.stream.fileno()
        self._old_attrs = None

    def __enter__(self):
        self._old_attrs = termios.tcgetattr(self.fd)
        tty.setraw(self.fd)
        return self

    def __exit__(self, *_):
        if self._old_attrs is not None:
            termios.tcsetattr(self.fd, termios.TCSADRAIN, self._old_attrs)


class AcceptableStream:
    def __init__(self, stream=sys.stdin):
        self.stream = stream
        self.rejected = bytes()
        self.not_accepted = bytes()

    class Reader:
        def __init__(self, parent: "AcceptableStream"):
            self.parent = parent

        def read(self, nbytes: int) -> bytes:
            result = bytes()
            if self.parent.rejected:
                take = min(len(self.parent.rejected), nbytes)
                result += self.parent.rejected[:take]
                self.parent.rejected = self.parent.rejected[take:]
                nbytes -= take
            if nbytes > 0:
                chunk = self.parent.stream.buffer.read(nbytes)
                if chunk == b"":
                    raise EOFError
                result += chunk
            self.parent.not_accepted += result
            return result

        def accept(self):
            self.parent.not_accepted = bytes()

    def __enter__(self):
        return self.__class__.Reader(self)

    def __exit__(self, *_):
        self.rejected += self.not_accepted
        self.not_accepted = bytes()


STYLE_DISABLE = False


class AnsiStyle(str, Enum):
    def __str__(self):
        return "" if STYLE_DISABLE else self.value

    # === Reset ===
    RESET = "\033[0m"

    # === Text styles ===
    BOLD = "\033[1m"
    DIM = "\033[2m"
    ITALIC = "\033[3m"
    UNDERLINE = "\033[4m"
    BLINK = "\033[5m"
    REVERSE = "\033[7m"
    HIDDEN = "\033[8m"
    STRIKETHROUGH = "\033[9m"

    # === Normal foreground colors ===
    BLACK = "\033[30m"
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    MAGENTA = "\033[35m"
    CYAN = "\033[36m"
    WHITE = "\033[37m"

    # === Bright foreground colors ===
    BRIGHT_BLACK = "\033[90m"
    BRIGHT_RED = "\033[91m"
    BRIGHT_GREEN = "\033[92m"
    BRIGHT_YELLOW = "\033[93m"
    BRIGHT_BLUE = "\033[94m"
    BRIGHT_MAGENTA = "\033[95m"
    BRIGHT_CYAN = "\033[96m"
    BRIGHT_WHITE = "\033[97m"

    # === Background colors ===
    BG_BLACK = "\033[40m"
    BG_RED = "\033[41m"
    BG_GREEN = "\033[42m"
    BG_YELLOW = "\033[43m"
    BG_BLUE = "\033[44m"
    BG_MAGENTA = "\033[45m"
    BG_CYAN = "\033[46m"
    BG_WHITE = "\033[47m"

    # === Bright background colors ===
    BG_BRIGHT_BLACK = "\033[100m"
    BG_BRIGHT_RED = "\033[101m"
    BG_BRIGHT_GREEN = "\033[102m"
    BG_BRIGHT_YELLOW = "\033[103m"
    BG_BRIGHT_BLUE = "\033[104m"
    BG_BRIGHT_MAGENTA = "\033[105m"
    BG_BRIGHT_CYAN = "\033[106m"
    BG_BRIGHT_WHITE = "\033[107m"


def extract_cpp_strings(code: str):
    pattern = re.compile(
        r"""
        ("(?P<regular>(?:\\.|[^"\\])*)")
        |(
          R"(?P<delimiter>[^()\s\\]*)
              \((?P<raw>(.|\s)*?)\)
            (?P=delimiter)"
        )
        """,
        re.VERBOSE,
    )

    results = []
    for match in pattern.finditer(code):
        if regular := match.group("regular"):
            results.append(codecs.decode(regular, "unicode_escape"))
        elif raw := match.group("raw"):
            results.append(raw)

    return results


def extract_blola_log_ids(code: str) -> list[RawLog]:
    pattern = re.compile(
        r"::blola::logId\((?P<file>.+?)\s*,\s*(?P<line>\d+)\s*,\s*\((?P<message>.+?)\)\s*,\s*\"BLOLA_LOG_ID_END\"\)",
        re.DOTALL,
    )

    return [
        RawLog(
            "".join(extract_cpp_strings(m.group("file"))),
            int(m.group("line")),
            "".join(extract_cpp_strings(m.group("message"))),
        )
        for m in pattern.finditer(code)
    ]


def extract_blola_platform(code: str) -> list[Platform]:
    struct_pattern = re.compile(
        r"struct\s+blola::platform\s*{(?P<body>.*?)};",
        re.S,
    )
    type_size_pattern = re.compile(r"auto size_(?P<type>\w+)\s*=\s*(?P<size>\d+);")

    platforms: list[Platform] = []

    for struct_match in struct_pattern.finditer(code):
        body = struct_match.group("body")

        sizes = {
            m.group("type"): int(m.group("size"))
            for m in type_size_pattern.finditer(body)
        }

        platform: Platform = {}

        platform["float"] = "f32"
        platform["double"] = "f64"

        platform["signed char"] = "i8"
        platform["unsigned char"] = "u8"

        platform["short int"] = signed_typeof(sizes["short"])
        platform["unsigned short int"] = unsigned_typeof(sizes["short"])

        platform["int"] = signed_typeof(sizes["int"])
        platform["unsigned int"] = unsigned_typeof(sizes["int"])

        platform["long int"] = signed_typeof(sizes["long"])
        platform["unsigned long int"] = unsigned_typeof(sizes["long"])

        platform["long long int"] = signed_typeof(sizes["long_long"])
        platform["unsigned long long int"] = unsigned_typeof(sizes["long_long"])

        platform["size_t"] = unsigned_typeof(sizes["size"])
        platform["void*"] = unsigned_typeof(sizes["ptr"])

        platform["intmax_t"] = platform["long long int"]
        platform["uintmax_t"] = platform["unsigned long long int"]

        platforms.append(platform)

    return platforms


def parse_message(printf_format: str) -> tuple[str, tuple[CppType, ...]]:
    pattern = re.compile(r"%%|%(?P<formatting>[^a-zA-Z]*)(?P<spec>[lhjz]*[a-zA-Z])")
    result: list[str] = []
    types: list[CppType] = []

    def expand[T](keys, value: T) -> dict[str, T]:
        return {k: value for k in keys}

    spec_map: dict[str, CppType] = {
        **expand(("hhd", "hhi"), "signed char"),
        **expand(("hd", "hi"), "short int"),
        **expand(("d", "i"), "int"),
        **expand(("ld", "li"), "long int"),
        **expand(("lld", "lli"), "long long int"),
        **expand(("jd", "ji"), "intmax_t"),
        **expand(("hhu", "hhx", "hhX", "hho"), "unsigned char"),
        **expand(("hu", "hx", "hX", "ho"), "unsigned short int"),
        **expand(("u", "x", "X", "o"), "unsigned int"),
        **expand(("lu", "lx", "lX", "lo"), "unsigned long int"),
        **expand(("llu", "llx", "llX", "llo"), "unsigned long long int"),
        **expand(("ju", "jx", "jX", "jo"), "uintmax_t"),
        **expand(("zu", "zx", "zX", "zo"), "size_t"),
        **expand(("f", "e", "E"), "float"),
        **expand(("lf", "le", "lE"), "double"),
        "p": "void*",
    }

    format_map = {**expand(("i", "u"), "d"), "p": "X"}

    pos = 0
    for m in pattern.finditer(printf_format):
        start, end = m.span()
        result.append(printf_format[pos:start])

        token = m.group(0)
        if token == "%%":
            result.append("%")
        else:
            formatting = m.group("formatting")
            spec = m.group("spec")
            spec_type = spec[-1]
            result.append(f"{{:{formatting}{format_map.get(spec_type, spec_type)}}}")
            types.append(spec_map[spec])

        pos = end
    result.append(printf_format[pos:])

    return "".join(result), tuple(types)


def poly_hash_utf8(h: int, s: str) -> int:
    data = s.encode("utf-8")
    for b in data:
        h = h * 31 + b
    return h


def poly_hash_uint(h: int, x: int, *, nbytes: int = 4) -> int:
    for i in range(nbytes):
        byte = (x >> (8 * i)) & 0xFF
        h = h * 31 + byte
    return h


def xor_pairs(data_list: list[bytes]) -> int:
    xor_value = 0
    for data in data_list:
        if len(data) % 2 != 0:
            data += b"\x00"

        for i in range(0, len(data), 2):
            pair = data[i : i + 2]
            value = int.from_bytes(pair, byteorder="little")
            xor_value ^= value

    return xor_value


@overload
def parse(b: bytes, type: Int) -> int: ...
@overload
def parse(b: bytes, type: Float) -> float: ...
def parse(b: bytes, type: RealType) -> int | float:
    if is_float(type):
        match len(b):
            case 4:
                return struct.unpack(">f", b)[0]
            case 8:
                return struct.unpack(">d", b)[0]
            case _:
                raise ValueError(f"{type} is not parsable")
    return int.from_bytes(b, "little", signed=is_signed(type))


@dataclass
class LogContext:
    year: int
    month: int
    day: int
    hour: int
    minute: int
    second: int
    milisecond: int
    microsecond: int

    file: str
    line: int
    maxFileLength: int
    maxLineLength: int

    status: str
    style: type[AnsiStyle]

    message: str


@dataclass
class Printer:
    maxFileLength: int
    maxLineLength: int
    format: str

    def print(self, message: str, *, file: str, line: int, error=False):
        now = datetime.now()
        er = f"{AnsiStyle.RED}ER{AnsiStyle.RESET}"
        ok = f"{AnsiStyle.GREEN}ok{AnsiStyle.RESET}"
        context = LogContext(
            year=now.year,
            month=now.month,
            day=now.day,
            hour=now.hour,
            minute=now.minute,
            second=now.second,
            milisecond=now.microsecond // 1000,
            microsecond=now.microsecond,
            status=er if error else ok,
            file=file,
            line=line,
            maxFileLength=self.maxFileLength,
            maxLineLength=self.maxLineLength,
            message=message,
            style=AnsiStyle,
        )
        print(self.format.format(**asdict(context)))


def read_log(
    logs: LogDb,
    reader: AcceptableStream.Reader,
    printer: Printer,
):
    @overload
    def read(type: Int) -> tuple[int, bytes]: ...
    @overload
    def read(type: Float) -> tuple[float, bytes]: ...
    def read(type: RealType) -> tuple[int | float, bytes]:
        b = reader.read(sizeof(type))
        return parse(b, type), b

    id, id_bytes = read("u16")
    reader.accept()
    log = logs.with_id(id)
    if not log:
        printer.print(
            f"Unknown log ID: {id:5} (0x{id:04X})", file="", line=0, error=True
        )
        return
    hash, _ = read("u16")
    args = [read(x) for x in log.args]
    args_v = [x[0] for x in args]
    args_b = [x[1] for x in args]

    calculated_hash = xor_pairs([id_bytes, *args_b])

    if hash == calculated_hash:
        reader.accept()

    printer.print(
        log.message.format(*args_v),
        file=log.file,
        line=log.line,
        error=(hash != calculated_hash),
    )


def assert_platforms_equal(platforms: list[Platform]):
    if not platforms:
        raise ValueError("Platform struct not found")

    first = platforms[0]
    for i, d in enumerate(platforms[1:], start=1):
        if d != first:
            raise ValueError(
                f"Mismatch between dict[0] and dict[{i}]:\n"
                f"dict[0] = {first}\n"
                f"dict[{i}] = {d}"
            )


def cmd_collect(args):
    out_file = Path(args.o)
    source_dir = Path(args.s).resolve()

    raw_logs: list[RawLog] = []
    platforms: list[Platform] = []
    for path in map(Path, args.files):
        if not path.is_file():
            raise Exception(f"'{path.resolve()}' not exists or not a file")
        with open(path, "r") as f:
            code = f.read()
            raw_logs += extract_blola_log_ids(code)
            platforms += extract_blola_platform(code)

    assert_platforms_equal(platforms)

    platform = platforms[0]
    logs = [x.asLog(source_dir, platform) for x in raw_logs]
    logs.sort(key=lambda x: (x.file, x.line))

    with open(out_file, "w") as f:
        json.dump(list(map(asdict, logs)), f, indent=2)


def cmd_log(args):
    global STYLE_DISABLE
    STYLE_DISABLE = args.no_colors

    with open(Path(args.i), "r") as f:
        logs = LogDb(json.load(f, object_hook=lambda x: Log(**x)))

    stream = AcceptableStream()
    printer = Printer(
        maxFileLength=max([len(x.file) for x in logs.map.values()]),
        maxLineLength=max([len(str(x.line)) for x in logs.map.values()]),
        format=args.f,
    )
    while True:
        with stream as reader:
            read_log(logs, reader, printer)


class FriendlyParser(argparse.ArgumentParser):
    def error(self, message):
        print(f"\nMistake: {message}\n", file=sys.stderr)
        self.print_help(sys.stderr)
        exit(1)


def main():
    parser = FriendlyParser(
        description="It is tool for decode blola logs",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    subparsers = parser.add_subparsers(required=True)

    collectParser = subparsers.add_parser(
        "collect",
        description="Collect all blola IDs from preprocessed C/C++ files",
        help="Collect all blola IDs from preprocessed C/C++ files",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    collectParser.add_argument("files", nargs="*", help="preprocessed C/C++ files")
    collectParser.add_argument(
        "-o",
        metavar="file",
        default="blola.json",
        help="output file with ID list and other meta info",
    )
    collectParser.add_argument(
        "-s",
        metavar="dir",
        default=".",
        help="direcotry for relative path in logs",
    )
    collectParser.set_defaults(func=cmd_collect)

    logParser = subparsers.add_parser(
        "log",
        description="Decode blola bin stream from stdin (raw bytes) to stdout",
        help="Decode blola bin stream",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    logParser.add_argument(
        "-i",
        metavar="in_collect_file",
        default="blola.json",
        help="input file with ID list and other meta info",
    )
    logParser.add_argument(
        "-f",
        metavar="format",
        default="{month:02}-{day:02} {hour:02}:{minute:02}:{second:02}.{milisecond:03} ({status}) {style.BRIGHT_BLACK}{file:>{maxFileLength}}:{line:<{maxLineLength}}{style.RESET} {message}",
        help="format string for each log line",
    )
    logParser.add_argument("--no-colors", action="store_true")
    logParser.set_defaults(func=cmd_log)

    args = parser.parse_args()
    try:
        args.func(args)
    except BlolaException as e:
        print(f"Error: {e}", file=sys.stderr)
        exit(2)
    except EOFError:
        print("EOF")
    except KeyboardInterrupt:
        print("Interupted")


if __name__ == "__main__":
    main()
