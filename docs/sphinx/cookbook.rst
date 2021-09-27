Cookbook
========

.. contents::

Subsequence extraction
----------------------

.. code-block:: seq

    myseq  = s'CAATAGAGACTAAGCATTAT'
    sublen = 5
    stride = 2

    # explicit for-loop
    for subseq in myseq.split(sublen, stride):
        print(subseq)

    # pipelined
    myseq |> split(sublen, stride) |> print

k-mer extraction
----------------

.. code-block:: seq

    myseq = s'CAATAGAGACTAAGCATTAT'
    stride = 2

    # explicit for-loop
    for subseq in myseq.kmers(stride, k=5):
        print(subseq)

    # pipelined
    myseq |> kmers(stride, k=5) |> print

Reverse complementation
-----------------------

.. code-block:: seq

    # sequences
    s = s'GGATC'
    print(~s)     # GATCC

    # k-mers
    k = k'GGATC'
    print(~k)     # GATCC

k-mer Hamming distance
----------------------

.. code-block:: seq

    k1 = k'ACGTC'
    k2 = k'ACTTA'
    #        ^ ^
    print(abs(k1 - k2))  # Hamming distance = 2

k-mer Hamming neighbors
-----------------------

.. code-block:: seq

    def neighbors(kmer):
        for i in range(len(kmer)):
            for b in (k'A', k'C', k'G', k'T'):
                if kmer[i] != b:
                    yield kmer |> base(i, b)

    print(list(neighbors(k'AGC')))  # CGC, GGC, etc.

k-mer minimizer
---------------

.. code-block:: seq

    def minimizer(s, k: Static[int]):
        assert len(s) >= k
        kmer_min = Kmer[k](s[:k])
        for kmer in s[1:].kmers(k=k, step=1):
            kmer = min(kmer, ~kmer)
            if kmer < kmer_min: kmer_min = kmer
        return kmer_min

    print(minimizer(s'ACGTACGTACGT', 10))

de Bruijn edge
--------------

.. code-block:: seq

    def de_bruijn_edge(a, b):
        a = a |> base(0, k'A')  # reset first base: [T]GAG -> [A]GAG
        b = b >> s'A'           # shift right to A: [GAG]C -> A[GAG]
        return a == b           # suffix of a == prefix of b

    print(de_bruijn_edge(k'TGAG', k'GAGC'))  # True
    print(de_bruijn_edge(k'TCAG', k'GAGC'))  # False

Count bases
-----------

.. code-block:: seq

    @tuple
    class BaseCount:
        A: int
        C: int
        G: int
        T: int

        def __add__(self, other: BaseCount):
            a1, c1, g1, t1 = self
            a2, c2, g2, t2 = other
            return (a1 + a2, c1 + c2, g1 + g2, t1 + t2)

    def count_bases(s):
        match s:
            case 'A*': return count_bases(s[1:]) + (1,0,0,0)
            case 'C*': return count_bases(s[1:]) + (0,1,0,0)
            case 'G*': return count_bases(s[1:]) + (0,0,1,0)
            case 'T*': return count_bases(s[1:]) + (0,0,0,1)
            case _: return BaseCount(0,0,0,0)

    print(count_bases(s'ACCGGGTTTT'))  # (A: 1, C: 2, G: 3, T: 4)

Spaced seed search
------------------

.. code-block:: seq

    def has_spaced_acgt(s):
        match s:
            case 'A_C_G_T*':
                return True
            case t if len(t) >= 8:
                return has_spaced_acgt(s[1:])
            case _:
                return False

    print(has_spaced_acgt(s'AAATCTGTTAAA'))  # True
    print(has_spaced_acgt(s'ACGTACGTACGT'))  # False

Reverse-complement palindrome
-----------------------------

.. code-block:: seq

    def is_own_revcomp(s):
        match s:
            case 'A*T' | 'T*A' | 'C*G' | 'G*C':
                return is_own_revcomp(s[1:-1])
            case '':
                return True
            case _:
                return False

    print(is_own_revcomp(s'ACGT'))  # True
    print(is_own_revcomp(s'ATTA'))  # False

Sequence alignment
------------------

.. code-block:: seq

    # default parameters
    s1 = s'CGCGAGTCTT'
    s2 = s'CGCAGAGTT'
    aln = s1 @ s2
    print(aln.cigar, aln.score)  # 3M1I6M -3

    # custom parameters
    # match = 2; mismatch = 4; gap1(k) = 2k + 4; gap2(k) = k + 13
    aln = s1.align(s2, a=2, b=4, gapo=4, gape=2, gapo2=13, gape2=1)
    print(aln.cigar, aln.score)  # 3M1D3M2I2M 2

Reading FASTA/FASTQ
-------------------

.. code-block:: seq

    # iterate over everything
    for r in FASTA('genome.fa'):
        print(r.name)
        print(r.seq)

    # iterate over sequences
    for s in FASTA('genome.fa') |> seqs:
        print(s)

    # iterate over everything
    for r in FASTQ('reads.fq'):
        print(r.name)
        print(r.read)
        print(r.qual)

    # iterate over sequences
    for s in FASTQ('reads.fq') |> seqs:
        print(s)

Reading paired-end FASTQ
------------------------

.. code-block:: seq

    for r1, r2 in zip(FASTQ('reads_1.fq'), FASTQ('reads_2.fq')):
        print(r1.name, r2.name)
        print(r1.read, r2.read)
        print(r1.qual, r2.qual)

Parallel FASTQ processing
-------------------------

.. code-block:: seq

    def process(s: seq):
        ...

    # OMP_NUM_THREADS environment variable controls threads
    FASTQ('reads.fq') |> iter ||> process

    # Sometimes batching reads into blocks can improve performance,
    # especially if each is quick to process.
    FASTQ('reads.fq') |> blocks(size=1000) ||> iter |> process

Reading SAM/BAM/CRAM
--------------------

.. code-block:: seq

    # iterate over everything
    for r in SAM('alignments.sam'):
        print(r.name)
        print(r.read)
        print(r.pos)
        print(r.mapq)
        print(r.cigar)
        print(r.reversed)
        # etc.

    for r in BAM('alignments.bam'):
        # ...

    for r in CRAM('alignments.cram'):
        # ...

    # iterate over sequences
    for s in SAM('alignments.sam') |> seqs:
        print(s)

    for s in BAM('alignments.bam') |> seqs:
        print(s)

    for s in CRAM('alignments.cram') |> seqs:
        print(s)

DNA to protein translation
--------------------------

.. code-block:: seq

    dna = s'AGGTCTAACGGC'
    protein = dna |> translate
    print(protein)  # RSNG

Reading protein sequences from FASTA
------------------------------------

.. code-block:: seq

    for s in pFASTA('seqs.fasta') |> seqs:
        print(s)
