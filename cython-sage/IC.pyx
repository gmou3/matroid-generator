from cpython.mem cimport PyMem_Realloc
from itertools import combinations, permutations
from sage.all import *
from sage.parallel.decorate import parallel
cimport cython


cdef long long int ii, j, fctrl, bnml, index
cdef unsigned char *P
cdef bint *B, a, b
cdef long long int *indices
cdef frozenset S
set_to_num = {}


def revlex_sort_key(s):
    return tuple(reversed(s))


def revlex_of(M):
    global ii, S
    for ii in range(bnml):
        B[ii] = False
    # for S in M.bases():
    #     B[set_to_num[S]] = True
    for ii in M.bb():
        B[ii] = True
    return ''.join(['*' if B[ii] else '0' for ii in range(bnml)])


cdef bint is_canonical(M):
    global ii, j, S, a, b, index
    for ii in range(bnml):
        B[ii] = False
    # for S in M.bases():
    #     B[set_to_num[S]] = True
    for ii in M.bb():
        B[ii] = True
    for ii in range(1, fctrl):
        index = indices[ii]
        for j in range(bnml):
            a = B[P[index + j]]
            b = B[j]
            if a != b:
                if b:
                    return False
                break
    return True


def IC(n, r, ncpus=1):
    if r == 0:
        M = Matroid(groundset=list(range(n)), bases=[[]])
        # yield M
        return [M]
    elif n == r:
        M = Matroid(groundset=list(range(n)), bases=[list(range(n))])
        # yield M
        return [M]

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

    IC_prev_1 = list(IC(n - 1, r, ncpus))
    IC_prev_2 = list(IC(n - 1, r - 1, ncpus))

    global fctrl, bnml
    fctrl = factorial(n)
    bnml = binomial(n, r)

    global P, B, indices
    P = <unsigned char *>PyMem_Realloc(P, fctrl * bnml * sizeof(unsigned char))
    if not P:
        raise MemoryError()
    B = <bint *>PyMem_Realloc(B, bnml * sizeof(bint))
    if not B:
        raise MemoryError()
    indices = <long long int *>PyMem_Realloc(indices,
                                             fctrl * sizeof(long long int))
    if not indices:
        raise MemoryError()

    subsets = [frozenset(SS)
               for SS in sorted(combinations(range(n), r), key=revlex_sort_key)]

    global ii, j, S, set_to_num
    set_to_num = {S: ii for ii, S in enumerate(subsets)}

    for ii, p in enumerate(permutations(range(n))):
        for j, S in enumerate(subsets):
            P[ii * bnml + j] = set_to_num[frozenset([p[k] for k in S])]

    for ii in range(fctrl):
        indices[ii] = ii * bnml

    @parallel(ncpus=ncpus)
    def main_comp(IC_prev_1):
        cnt = 0
        tmp = []
        for M in IC_prev_1:  # IC(n - 1, r)
            taboo_flats = get_taboo_flats(M)
            # For all nonempty modular cuts
            for N in M.extensions(n - 1, subsets=taboo_flats):
            # for LS in M.linear_subclasses():
            #     flag = False
            #     for F in LS:
            #         if F in taboo_flats:
            #             flag = True
            #             break
            #     if flag:
            #         continue
            #     N = M._extension(n - 1, LS)
                if is_canonical(N):
                    tmp.append(N)
        return tmp

    divided = [[] for i in range(ncpus)]
    # q = max(ncpus, len(IC_prev_1) // ncpus + 1)
    for i in range(len(IC_prev_1)):
        divided[i % ncpus].append(IC_prev_1[i])
    tmp = [M for answer in main_comp(divided) for M in answer[1]]

    for M in IC_prev_2:  # IC(n - 1, r - 1)
        tmp.append(Matroid(groundset=list(range(n)),
                      bases=[S | set([n - 1]) for S in M.bases()]))

    return tmp

