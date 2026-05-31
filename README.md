# CCVizTracer
A debugging and profiling tool that can trace and visualize C/C++ code execution.

<figure>
  <img src="./img/multithread_example.png" alt="multithread example">
  <figcaption align="center"><b>Figure 1:</b> execution flow generated from example/multithread.cpp</figcaption>
</figure>

## Usage
### LLVM clang
If you are using `clang` then add `-fpass-plugin=cctracer_pass.dylib` and link against `cctracer`
```sh
# c
clang your_source_file.c -fpass-plugin=cctracer_pass.dylib -lcctracer
# c++
clang++ your_source_file.c -fpass-plugin=cctracer_pass.dylib -lcctracer
```

### GNU gcc
For `gcc` users, add `-fplugin=cctracer_pass.dylib` and link against `cctracer`
```sh
# c
gcc your_source_file.c -fplugin=cctracer_pass.dylib -lcctracer
# c++
g++ your_source_file.c -fplugin=cctracer_pass.dylib -lcctracer
```

### Execute
After execution, a `result.json` file will be produced. Put that file in [ui.perfetto.dev](https://ui.perfetto.dev/), then you will see the visualized result supported by perfetto.

## Use Case
<figure>
  <img src="./img/cpython.png" alt="cpython use case">
  <figcaption align="center"><b>Figure 2:</b> My own use case for exploring CPython source code execution</figcaption>
</figure>


---
*Inspired by GaoTian's viztracer(python)(https://github.com/gaogaotiantian/viztracer), Chrome's DevTools and Perfetto.*
