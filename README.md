# About

`flhex` is a simple, fast tool to flatten Intel HEX files. That is, it takes a HEX file containing several separate sections and outputs a HEX file containing a single, longer section, padded where necessary.

In an ideal world, `flhex` would not be necessary. At the time of writing, however, the bootloader installed on Arduino Mega microcontrollers will incorrectly erase flash pages unless the flash memory is sent as a single, continuous partition. Ultimately, you probably should fix this by uploading a patched bootloader or just programming over SPI (relatively slow). `flhex` provides an easy workaround, however.

# Installation

Download the most recent version for your operating system from the Releases page. If you're using Windows, choose the `i686_windows.zip` version if you're running a 32-bit operating system, otherwise, `x86_64_windows.zip`. If you're using 64-bit Linux, download the `x86_64_linux.tar` file. Then unzip or untar the file, and you're ready to go.

To build from source, clone the repository or download the source code from the Releases page. Then `cd` to the `flhex/` directory, and run `make` to compile `flhex`. If you're using Windows, the process is essentially the same, except you'll have to use MSVC or something.

# Usage

`flhex` takes a single input file and several options.
```
 $ flhex file.hex                       # flatten file.hex and output to out.hex
 $ flhex file.hex -o file2.hex          # output to file2.hex
 $ flhex file.hex --padding 0           # pad with zeros
 $ flhex file.hex --count 32            # put 32 bytes in each HEX record
 $ flhex file.hex --help                # display a help message
```

Other than `-o/--output`, the default values for each option are probably the best.

# License (MIT)

Copyright 2023 Tom Copeland.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
