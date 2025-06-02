from cpython.mem cimport PyMem_Realloc
from itertools import combinations, permutations
from sage.all import *
cimport cython


cdef long long int ii, j, fctrl, bnml
cdef unsigned char *P
cdef bint *B
cdef frozenset S
set_to_num = {}


def revlex_sort_key(s):
    return tuple(reversed(s))


def revlex_of(M):
    global ii, S
    for ii in range(bnml):
        B[ii] = False
    for S in M.bases():
        B[set_to_num[S]] = True
    return ''.join(['*' if B[ii] else '0' for ii in range(bnml)])


cdef bint is_canonical(M):
    global ii, j, S
    for ii in range(bnml):
        B[ii] = False
    for S in M.bases():
        B[set_to_num[S]] = True
    for ii in range(1, fctrl):
        for j in range(bnml):
            if B[P[ii * bnml + j]] != B[j]:
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

    R = sorted([x for x in combinations(range(n - 1), r) if n - 2 in x],
               key=revlex_sort_key)

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

    IC_prev_1 = list(IC(n - 1, r))
    IC_prev_2 = list(IC(n - 1, r - 1))

    global fctrl, bnml
    fctrl = factorial(n)
    bnml = binomial(n, r)

    global P, B
    P = <unsigned char *>PyMem_Realloc(P, fctrl * bnml * sizeof(unsigned char))
    if not P:
        raise MemoryError()
    B = <bint *>PyMem_Realloc(B, bnml * sizeof(bint))
    if not B:
        raise MemoryError()

    subsets = [frozenset(SS)
               for SS in sorted(combinations(range(n), r), key=revlex_sort_key)]

    global ii, j, S, set_to_num
    set_to_num = {S: ii for ii, S in enumerate(subsets)}

    for ii, p in enumerate(permutations(range(n))):
        for j, S in enumerate(subsets):
            P[ii * bnml + j] = set_to_num[frozenset([p[k] for k in S])]

    for M in IC_prev_1:  # IC(n - 1, r)
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
    for M in IC_prev_2:  # IC(n - 1, r - 1)
        yield Matroid(groundset=list(range(n)),
                      bases=[S | set([n - 1]) for S in M.bases()])
