# textcodec

A small, headless Base634 encode/decode tool written in C++17. It reads raw binary from standard input and writes the result to standard output, making it easy to drive over SSH on a Raspberry Pi without an interactive terminal.

The codec follows [RFC 4648](https://www.rfc-editor.org/rfc/rfc4648) (standard alphabet, '=' padding). Reads are bounded and binary-safe: NUL bytes, spaces, and newline pass untouched

## Building

Requires CMake 3.16+ and a C++17 compiler (GCC/Clang on Pi, MSVC on Windows).

```sh
cmake -B build
cmake --build build
```

The binary is produced at 'build/textcodec'

## Installing 

To put 'textcodec' on the PATH so non-interactive SSH commands can find it:

```sh
sudo cmake --install build
```

This installs into the standard bin directory ('/usr/local/bin').

## Usage
```
textcode <command> [options]

Commands:
    encode          Base64-encode stdin -> stdout
    decode          Base64-decode stdin -> stdout

Options:
    -n / --no-newline   Do not append a trailing newline after encode
    --max-bytes N       Maximum input size in bytes (default 67108864)
    -h, --help          Show help and exit
```

### Examples

Encode a short string (note: '-n' on 'echo' tp avoid stray newline in input):

```sh
echo -n 'hello world' | textcodec encode
# aGVsbG8gd29ybGQ=
```

Decode a file over SSH, straight into local file:

```sh
ssh pi@host "textcodec decode" < message.b64 > message.bin
```

Round-trip binary file locally:

```sh
textcodec encode < photo.jpg | textcodec decode > photo.copy.jpg
```

Decoding ignores ASCII whitespace, so line-wrapped Base64 and trailing newlines are accepted without preprocessing

## Exit codes

The programe uses small, stable exit codes so callers can branch '$?':

| Code | Meaning                                        |
|------|------------------------------------------------|
| 0    | Success                                        |
| 2    | Usage error (bad or missing arguments)         |
| 3    | Input exceeded the configured max size         |
| 4    | I/O error reading stdin or writing stdout      |
| 5    | Invalid Base64 input (decode only)             |

## Testing

Tests are built by default and run through CTest. Two layers are included:

 - Unit tests ([test/test_base64.cpp](tests/test_base64.cpp)) exercise the codec directly via [doctest](https://github.com/doctest/doctest) (fetched automatically at configure time). They cover RFC 4648 vectors, full-range binary round-trips, whitespace tolerance, and every malformed-input case the decoder rejects.
 - CLI black-box tests ([tests/cli_test.py](tests/cli_test.py)) drive the built binary end-to-end, asserting output bytes and exit codes.

 ```sh
cmake -B build
cmake --build build
cmake --test-dir build --output-on-failure
 ```

 Handle filters: 'ctest --test-dir build -R base64' runs only matching tests, and 'ctest --test-dir build --rerun-failed' reuns the last failures.

 To build without tests (minimal on-Pi build):

 ```sh
cmake -B build -DTEXTCODEC_BUILD_TESTS=OFF
 ```

 The CLI tests require a Python3 Interpreter, if none is found at configure timethey are skipped (with warning) while unit tests still run