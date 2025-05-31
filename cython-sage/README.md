# matroid-generator

A Sage script implementing the matroid generation algorithm described in [[1]](#1).

This algorithm relies on a canonical representation of matroids and avoids all isomorphism testing.

## Requirements

- Sage [https://github.com/sagemath/sage].\
  We assume that Sage can be called by running `sage`.

## Build

Compile the Cython file `IC.pyx` using
```shell
sage --python setup.py build_ext --inplace
```

## Usage example

Within the Sage interactive shell run

```python
sage: from IC import *
sage: for M in IC(n=5, r=2):
....:     print(revlex_of(M))
....:
**********
0*********
0****0****
00*0**0***
000*******
000******0
0000**0***
0000**0**0
00000*00**
000000****
0000000***
00000000**
000000000*
```

For a timed and enumerated output, run, e.g.,
```python
sage: %time for i, M in enumerate(IC(8, 4)): print(f'#{str(i + 1).center(7)} | {revlex_of(M)}')
```

## References

<a id="1">[1]</a>
Matsumoto, Y., Moriyama, S., Imai, H., & Bremner, D. (2012). Matroid enumeration for incidence geometry. Discrete & Computational Geometry, 47, 17-43.