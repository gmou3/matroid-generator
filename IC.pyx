from sage.all import *
from itertools import combinations, permutations
cimport cython


cdef unsigned char P[3628800][210]
cdef unsigned int ii, j, fctrl, bnml
set_to_num = {}
cdef bint B[210]


def revlex_sort_key(s):
    return tuple(reversed(s))


def revlex_of(M):
    global ii
    for ii in range(bnml):
        B[ii] = False
    for S in M.bases():
        B[set_to_num[S]] = True
    return ''.join(['*' if B[ii] else '0' for ii in range(bnml)])


cdef bint is_canonical(M):
    global ii, j
    for ii in range(bnml):
        B[ii] = False
    for S in M.bases():
        B[set_to_num[S]] = True
    for ii in range(1, fctrl):
        for j in range(bnml):
            if B[P[ii][j]] != B[j]:
                if B[j]:
                    return False
                break
    return True


def IC(n, r):
    if r == 0:
        M = Matroid(groundset=list(range(n)), bases=[[]])
        yield M
        return
    elif n == r:
        M = Matroid(groundset=list(range(n)), bases=[list(range(n))])
        yield M
        return

    subsets = [frozenset(S)
               for S in sorted(combinations(range(n), r), key=revlex_sort_key)]
    R = sorted([x for x in combinations(range(n - 1), r) if n - 2 in x],
               key=revlex_sort_key)

    global set_to_num
    set_to_num = {S: i for i, S in enumerate(subsets)}

    global fctrl, bnml
    fctrl = factorial(n)
    bnml = binomial(n, r)

    for i, p in enumerate(permutations(range(n))):
        for j, S in enumerate(subsets):
            P[i][j] = set_to_num[frozenset([p[k] for k in S])]

    def get_taboo_flats(M):
        # Prop. 1
        hyperplanes = set(M.flats(r - 1))
        mx = 0
        for H in hyperplanes:
            if mx < len(H):
                mx = len(H)
        taboo_flats = set([H for H in hyperplanes if len(H) == mx])

        # Prop. 2
        for S in R:
            SS = frozenset(S)
            if M.is_dependent(SS):
                break
            taboo_flats.add(M.closure(SS - set([n - 2])))

        return taboo_flats

    for M in IC(n - 1, r):
        taboo_flats = get_taboo_flats(M)
        # For all nonempty modular cuts
        for LS in M.linear_subclasses():
            flag = False
            for F in LS:
                if F in taboo_flats:
                    flag = True
                    break
            if flag:
                continue
            N = M._extension(n - 1, LS)
            if is_canonical(N):
                yield N
    for M in IC(n - 1, r - 1):
        yield Matroid(groundset=list(range(n)),
                      bases=[S | set([n - 1]) for S in M.bases()])

