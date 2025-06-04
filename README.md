# matroid-generator

A C++ implementation of the matroid generation algorithm described in [[1]](#1).

This algorithm relies on a canonical representation of matroids and avoids all
isomorphism testing.

## Build

Compile the source code to create the `IC` executable:
```bash
g++ -fopenmp -O3 -o IC IC.cpp matroid.cpp extension.cpp
```

## Usage

To generate all (canonical) matroids over `n` elements of rank `r`, run
```bash
./IC <n> <r> [<num_threads>]
```
WARNING: Memory usage scales with `n` and `r`.

A raw example:
```bash
$ ./IC 5 2
**********
0****0****
0*********
00*0**0***
000******0
000*******
0000**0**0
0000**0***
00000*00**
000000****
0000000***
00000000**
000000000*
```

For a timed and enumerated output, run, e.g.,
```bash
time ./IC 8 4 | cat -n
```

## Notes

Each matroid/line of the output is encoded as follows:

Each character signifies whether an `r`-set is included (`'*'`) or excluded
(`'0'`). The order of the `r`-sets is lexicographic on their reverse sorting.
E.g., `{1, 2, 3} < {0, 1, 4}`, because `(3, 2, 1) < (4, 1, 0)`.

The algorithm works recursively and uses single-element matroid extensions.
There is a bijection between single-element matroid extensions and modular cuts.
The empty modular cut produces the extension by a coloop. We encode the nonempty
modular cuts using the notion of linear subclasses. The DFS search over linear
subclasses is adapted from the files `src/sage/matroids/extension.*` in Sage
[https://github.com/sagemath/sage]. Given a linear subclass, we translate it to
a matroid extension, and, if it is canonical, we output it.

See [[2]](#2) for the relevant background knowledge. Of particular interest is
Chapter 7, where one can find the definitions of modular cut (p. 266), and
linear subclass (p. 271).

## References

<a id="1">[1]</a>
Matsumoto, Y., Moriyama, S., Imai, H., & Bremner, D. (2012). Matroid
enumeration for incidence geometry. Discrete & Computational Geometry, 47, 17-43.

<a id="2">[2]</a>
Oxley, James G. "Matroid Theory." (2011).
