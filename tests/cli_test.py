#!/usr/bin/env python3
"""
End-to-end tests for textcodec CLI
Invoked by CTest
Script drives program over stdin/stdout exactly as SSH caller,
asserting both produced bytes and exit code. Exit code mirrors 
textcodec::Status (see include/errors.hpp)
    0 - Ok
    2 - UsageError
    3 - InputToLarge
    4 - IoError
    5 - DecodeError
Exits 0 if every check pass, 1 otherwise
"""

import subprocess
import sys

# -- Status codes from include/errors.hpp
OK = 0
USAGE_ERROR = 2
INPUT_2_LARGE = 3
DECODE_ERROR = 5

failures = []


def run(bin, args, stdin=b""):
    proc = subprocess.run(
        [bin, *args],
        input=stdin,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE
    )
    return proc.returncode, proc.stdout

def check(name, cond, detail=""):
    # record pass/fail name assert
    if cond:
        print(f"PASS: {name}")
    else:
        print(f"FAIL: {name} {detail}")
        failures.append(name)

def main():
    if len(sys.argv) != 2:
        print("usage: cli_test.py <path-to-textcodec>", file=sys.stderr)
        return 2
    bin = sys.argv[1]

    #encode: trailing newline
    rc, out = run(bin, ["encode"], b"hello world")
    check("encode adds trailing newline", rc == OK and out == b"aGVsbG8gd29ybGQ=\n",
          f"(rc={rc}, out={out!r})")

    #encode: no newline
    rc, out = run(bin, ["encode", "-n"], b"hello world")
    check("encode -n omits trailing newline", rc == OK and out == b"aGVsbG8gd29ybGQ=",
          f"(rc={rc}, out={out!r})")

    rc, out = run(bin, ["encode", "--no-newline"], b"hello world")
    check("encode --no-newline omits trailing newline", rc == OK and out == b"aGVsbG8gd29ybGQ=",
          f"(rc={rc}, out={out!r})")

    #round-trip every byte value 0...255 through encode | decode
    original = bytes(range(256))
    rc, encoded = run(bin, ["encode", "-n"], original)
    check("encode all-256 succeeds", rc == OK, f"(rc={rc})")
    rc, decoded = run(bin, ["decode"], encoded)
    check("encode|decode round-trip is loseless",
          rc == OK and decoded == original, f"(rc={rc})")

    #decode tolerates trailing newline that encode adds by default
    rc1, encoded_n1 = run(bin, ["encode"], b"foobar")
    rc2, decoded_n1 = run(bin, ["decode"], encoded_n1)
    check("decode accepts encode's newline-terminated output",
          rc1 == OK and rc2 == OK and decoded_n1 == b"foobar",
          f"(rc1={rc1}, rc2={rc2}, out={decoded_n1!r})")

    #--help exits 0
    rc,_ = run(bin,["--help"])
    check("--help exits 0", rc == OK, f"(rc={rc})")
    rc,_ = run(bin, ["encode", "--help"])
    check("encode --help exits 0", rc == OK, f"(rc={rc})")

    #usage errors - exit 2
    rc,_ = run(bin, [])
    check("no command -> usage error", rc == USAGE_ERROR, f"(rc={rc})")
    rc,_ = run(bin,["frob"])
    check("unknown command -> usage error", rc == USAGE_ERROR, f"(rc={rc})")
    rc,_ = run(bin, ["encode", "--bogus"])
    check("unknown option -> usage error", rc == USAGE_ERROR, f"(rc={rc})")
    rc,_ = run(bin, ["encode", "--max-bytes"])
    check("--max-bytes without value -> usage error", rc == USAGE_ERROR, f"(rc={rc})")
    rc,_ = run(bin, ["encode", "--max-bytes", "0"])
    check("--max-bytes 0 -> usage error", rc == USAGE_ERROR, f"(rc={rc})")
    rc,_ = run(bin, ["encode", "--max-bytes", "abc"])
    check("--max-bytes non-num -> usage error", rc == USAGE_ERROR, f"(rc={rc})")

    #invalid base64 - exit 5
    rc,_ = run(bin,["decode"], b"not valid base64!!!")
    check("invalid base64 -> decode error", rc == DECODE_ERROR, f"(rc={rc})")

    #oversized input - exit 3
    rc,_ = run(bin, ["encode", "--max-bytes", "4"], b"more than four bytes")
    check("input over --max-bytes -> input too large", rc == INPUT_2_LARGE, f"(rc={rc})")

    #input at --max-bytes is accepted
    rc,_ = run(bin, ["encode", "--max-bytes", "5"], b"12345")
    check("input at --max-bytes is accepted", rc == OK, f"(rc={rc})")

    if failures:
        print(f"\n{len(failures)} check(s) failed: {', '.join(failures)}", file=sys.stderr)
        return 1

    print("\nAll CLI checks passed")
    return 0

if __name__ == "__main__":
    sys.exit(main())