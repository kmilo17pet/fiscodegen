# fiscodegen
C++ code generation from FIS Type-1

`fiscodegen` Is a simple command that takes a FIS object created from the
Fuzzy Logic Toolbox and generates C++ code based on the qlibs-FIS engine. The
generated code is portable and can be used in any embedded system that supports
floating-point operations.

The tool is written in C++11 in a single .cpp file. Honestly, I haven't been
very rigorous in the code and its organization, given its purpose, but the tool
is functional and portable, so you can easily compile it for any platform.

### Usage

```console
fiscodegen <path_to_fis_file>
```

### Example

```console
fiscodegen /home/user/fisExample/example.fis
```