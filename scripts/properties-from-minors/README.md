# properties-from-minors workflow

**Main script:** [scripts/properties-from-minors/main-workflow.sh](scripts/properties-from-minors/main-workflow.sh)

This script orchestrates the property computation pipeline for canonical
matroids from their minors (deletion and contraction of the last element).

## Prerequisites

- Built binaries `build/{IC, sz}`: run `make`.
- `sage` available on PATH: invoked via `sage -python`.

## Usage

```shell
./scripts/properties-from-minors/main-workflow.sh <r> <n> [threads] [--realizable] [--char <c>]
```

- `<r>`: rank
- `<n>`: ground set size
- `[threads]`: number of threads (default: `1`)
- `--realizable`: compute realizability of matroids (slow)
- `--char <c>`: specify field characteristic (default: `None`)

## Main steps

- Compute canonical `(r, n)` matroids (`output/r<rr>n<nn>.sz`).
- Sort canonical `(r, n)` matroids by suffix
  (`output/r<rr>n<nn>-suffix-sorted.sz`).
- Compute canonical `(r - 1, n - 1)` matroids, all of their colex permutations
  (`output/r<rr1>n<nn1>-all.sz`), and an index file
  (`output/r<rr1>n<nn1>-all-to-canonical_idx.txt`).
- Run the main property computation with
  `sage -python scripts/properties-from-minors/parallel-scan-and-compute-properties.py`,
  which can efficiently retrieve the minor properties for each (r, n) matroid.

If some required file pre-exists, the relevant step is skipped.

## Output

The final output consists of detailed property results
(`output/r<rr>n<nn>-properties.json`) and their counts
(`output/r<rr>n<nn>-properties-counts.json`). These filenames may differ
depending on the input.

Example for (3, 9) matroids:
```json
{
  "all": 1275,
  "loopless": 950,
  "coloopless": 1217,
  "simple": 383,
  "connected": 901,
  "paving": 383,
  "binary": 131,
  "ternary": 299,
  "quaternary": 504,
  "regular": 127,
  "graphic": 127
}
```

Each entry of the detailed results is formatted as follows:
```json
"************************************************************************************": {
  "loopless": true,
  "coloopless": true,
  "simple": true,
  "connected": true,
  "paving": true,
  "binary": false,
  "ternary": false,
  "quaternary": false,
  "regular": false,
  "graphic": false,
  "T20": 74,
  "T02": 438,
  "T11": 84,
  "beta_invariant": 21
}
```
