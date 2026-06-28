# CCVizTracer
A debugging and profiling tool that can trace and visualize C/C++ code execution.

<figure>
  <img src="./img/multithread_example.png" alt="multithread example compiled with gcc">
  <figcaption align="center"><b>Figure 1:</b> execution flow generated from example/multithread.cpp (compiled with gcc)</figcaption>
  <img src="./img/multithread_example_wt_source_code_preview.png" alt="multithread example source preview">
  <figcaption align="center"><b>Figure 2:</b> execution flow generated from example/multithread.cpp (compiled with clang) with source code preview</figcaption>
</figure>

## Usage
### Configuration
Create a `.cctracer.ini` at your home directory or project root directory, check out the `.cctracer.ini` file in this repo for detailed example.
Or you can just set env variable `CCTRACER_ENABLE=1` to enable ccviztracer.

### Generate Trace
#### LLVM clang
If you are using `clang` then add `-fpass-plugin=cctracer_pass.dylib` and link against `cctracer`. Don't forget to add `-g` or `-gline-tables-only` flag.
```sh
# c
clang your_source_file.c -g -fpass-plugin=cctracer_pass.dylib -lcctracer
# c++
clang++ your_source_file.c -g -fpass-plugin=cctracer_pass.dylib -lcctracer
```

#### GNU gcc
For `gcc` users, add `-fplugin=cctracer_pass.dylib` and link against `cctracer`. Don't forget to add `-g` or `-gline-tables-only` flag.
```sh
# c
gcc your_source_file.c -g -fplugin=cctracer_pass.dylib -lcctracer
# c++
g++ your_source_file.c -g -fplugin=cctracer_pass.dylib -lcctracer
```

#### CMake
Check out `CMakeLists.txt` for CMake usage example.

### Execute
After execution, a `result.json` file will be produced. Put that file in [ui.perfetto.dev](https://ui.perfetto.dev/), then you will see the visualized result supported by perfetto.

### View Trace
Run tools/vizcctracer.py and open `127.0.0.1:<port>` to view the trace
```sh
python3 tools/vizcctracer.py -h
usage: vizcctracer.py [-h] [-tf TRACE_FILE] [-p PORT]

VizCCtracer

options:
  -h, --help            show this help message and exit
  -tf, --trace_file TRACE_FILE
                        path to the trace file (e.g. result.json or result.pftrace) , you can also skip this and open the trace file within the ui
  -p, --port PORT       port preferred, a random one will be selected if not provided

IMPORTANT: run vizcctracer at the root of your project folder so that vizcctracer can find your source code
```

## Use Case
<figure>
  <img src="./img/cpython.png" alt="cpython use case">
  <figcaption align="center"><b>Figure 3:</b> My own use case for exploring CPython source code execution</figcaption>
</figure>


---
*Inspired by GaoTian's viztracer(python)(https://github.com/gaogaotiantian/viztracer), Chrome's DevTools and Perfetto.*
