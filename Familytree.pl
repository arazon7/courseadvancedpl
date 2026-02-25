%%%%%%%%%%%%%%
%% Facts
%%%%%%%%%%%%%%

% People (group by predicate to avoid 'discontiguous' warnings)
male(john).
male(bob).
male(eric).
male(frank).
male(ian).

female(anna).
female(clara).
female(diana).
female(eva).
female(grace).
female(helen).

% Parent relationships
parent(john,  bob).
parent(anna,  bob).
parent(john,  clara).
parent(anna,  clara).

parent(bob,   eva).
parent(diana, eva).
parent(bob,   frank).
parent(diana, frank).

parent(clara, grace).
parent(eric,  grace).

parent(frank, ian).
parent(helen, ian).

%%%%%%%%%%%%%%
%% Derived relationships
%%%%%%%%%%%%%%

% Siblings share at least one parent; exclude self
sibling(X, Y) :-
    parent(P, X),
    parent(P, Y),
    X \= Y.

% Recursive ancestor
ancestor(A, D) :-
    parent(A, D).
ancestor(A, D) :-
    parent(A, M),
    ancestor(M, D).

% Depth-limited ancestor: exactly N generations
ancestor_n(A, D, 1) :-
    parent(A, D).
ancestor_n(A, D, N) :-
    N > 1,
    parent(A, M),
    N1 is N - 1,
    ancestor_n(M, D, N1).

% Grandparent via recursion (exactly 2 generations)
grandparent(GP, GC) :-
    ancestor_n(GP, GC, 2).

% Descendant (inverse direction), recursive
descendant(A, D) :-
    parent(A, D).
descendant(A, D) :-
    parent(A, M),
    descendant(M, D).

% Cousins: their parents are siblings; exclude self
cousin(X, Y) :-
    X \= Y,
    parent(PX, X),
    parent(PY, Y),
    sibling(PX, PY).
