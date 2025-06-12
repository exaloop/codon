# set -x

trap 'echo Exiting...; exit' INT

get_data() {
    git clone https://github.com/exaloop/seq
    mkdir -p test
    cp -r seq/test/* test/
    cp -r ../test/* test/
    mkdir -p build
    mkdir -p data
    curl -L https://ftp.nyse.com/Historical%20Data%20Samples/DAILY%20TAQ/EQY_US_ALL_NBBO_20250102.gz | gzip -d -c | head -n10000000 > data/taq.txt
    curl -L https://hgdownload.soe.ucsc.edu/goldenPath/hg38/chromosomes/chr22.fa.gz | gzip -d -c > data/chr22.fa
    samtools faidx data/chr22.fa
    curl -L https://github.com/lh3/biofast/releases/download/biofast-data-v1/biofast-data-v1.tar.gz | tar zxvf - -C data
    curl -L http://cb.csail.mit.edu/cb/seq/nbt/sw-data.tar.bz2 | tar jxvf - -C data
    curl -L http://cb.csail.mit.edu/cb/seq/nbt/umi-data.bz2 | bzip2 -c -d > data/hgmm_100_R1.fastq
    run/exe/bench_fasta 25000000 > data/three_.fa
    samtools faidx data/three_.fa
    samtools faidx data/three_.fa THREE > data/three.fa
    rm -f data/three_.fa
    samtools view https://hgdownload.cse.ucsc.edu/goldenPath/hg19/encodeDCC/wgEncodeSydhRnaSeq/wgEncodeSydhRnaSeqK562Ifna6hPolyaAln.bam chr22 -b -o data/rnaseq.bam
    curl -L https://hgdownload.soe.ucsc.edu/goldenPath/hg19/chromosomes/chr22.fa.gz | gzip -d -c > data/chr22_hg19.fa
    samtools faidx data/chr22_hg19.fa
    samtools index data/rnaseq.bam
}

compile() {
    name=$1
    path=$2
    extra=$3
    echo -n "====> C: ${name} ${path} "
    start=$(date +%s.%N)
    CODON_DEBUG=lt /usr/bin/time -f 'time=%e mem=%M exit=%x' \
        codon build -release $extra $path -o run/exe/${name}.exe \
        >run/log/${name}.compile.txt 2>&1
    duration=$(echo "$(date +%s.%N) $start" | awk '{printf "%.1f", $1-$2}')
    echo "[$? run/log/${name}.compile.txt ${duration}]"
}

run() {
    name=$1
    args="${@:2}"
    echo -n "      R: $name $args "
    start=$(date +%s.%N)
    eval "/usr/bin/time -o run/log/${name}.time.txt -f 'time=%e mem=%M exit=%x' run/exe/${name}.exe $args >run/log/${name}.run.txt 2>&1"
    duration=$(echo "$(date +%s.%N) $start" | awk '{printf "%.1f", $1-$2}')
    echo "[$? run/log/${name}.run.txt ${duration}]"
}

# get_data
mkdir -p run/exe
mkdir -p run/log
mkdir -p build

bench() {
    path=$1
    args=$2
    dirname=$(basename $(dirname $path))
    filename=$(basename $path)
    filename=${filename%.*}
    extra=""
    if [[ $path == *"seq/"* ]]
    then
        dirname="seq_${dirname}"
        extra="-plugin seq"
    fi
    name="${dirname}_${filename}"

    istart=$(date +%s.%N)
    compile $name $path "$extra"
    run $name $args
    duration=$(echo "$(date +%s.%N) $istart" | awk '{printf "%.1f", $1-$2}')
    echo "      T: ${duration}"
}

bench ../test/stdlib/bisect_test.codon
bench ../test/stdlib/cmath_test.codon
bench ../test/stdlib/datetime_test.codon
bench ../test/stdlib/heapq_test.codon
bench ../test/stdlib/itertools_test.codon
bench ../test/stdlib/math_test.codon
bench ../test/stdlib/operator_test.codon
bench ../test/stdlib/random_test.codon
bench ../test/stdlib/re_test.codon
bench ../test/stdlib/sort_test.codon
bench ../test/stdlib/statistics_test.codon
bench ../test/stdlib/str_test.codon

bench ../test/numpy/test_dtype.codon
bench ../test/numpy/test_fft.codon
bench ../test/numpy/test_functional.codon
bench ../test/numpy/test_fusion.codon
bench ../test/numpy/test_indexing.codon
bench ../test/numpy/test_io.codon
bench ../test/numpy/test_lib.codon
bench ../test/numpy/test_linalg.codon
bench ../test/numpy/test_loops.codon
bench ../test/numpy/test_misc.codon
bench ../test/numpy/test_ndmath.codon
bench ../test/numpy/test_npdatetime.codon
bench ../test/numpy/test_pybridge.codon
bench ../test/numpy/test_reductions.codon
bench ../test/numpy/test_routines.codon
bench ../test/numpy/test_sorting.codon
bench ../test/numpy/test_statistics.codon
bench ../test/numpy/test_window.codon

bench codon/binary_trees.codon 20 # 6s
bench codon/chaos.codon /dev/null # 1s
bench codon/fannkuch.codon 11 # 6s
bench codon/float.py
bench codon/go.codon
# TODO: bench codon/mandelbrot.codon
bench codon/nbody.py 10000000 # 6s
bench codon/npbench.codon # ...s
bench codon/set_partition.py 15 # 15s
bench codon/spectral_norm.py
bench codon/primes.codon 100000 # 3s
bench codon/sum.py
bench codon/taq.py data/taq.txt # 10s
bench codon/word_count.py data/taq.txt # 10s

bench ../seq/test/core/align.codon # 10s
bench ../seq/test/core/big.codon
bench ../seq/test/core/bltin.codon
bench ../seq/test/core/bwtsa.codon # 10s
bench ../seq/test/core/containers.codon
bench ../seq/test/core/formats.codon
bench ../seq/test/core/kmers.codon
bench ../seq/test/core/match.codon
bench ../seq/test/core/proteins.codon
bench ../seq/test/core/serialization.codon

bench ../seq/test/pipeline/canonical_opt.codon
bench ../seq/test/pipeline/interalign.codon # 25s
bench ../seq/test/pipeline/prefetch.codon
bench ../seq/test/pipeline/revcomp_opt.codon # FAILS

bench ../seq/test/bench/16mer.codon data/chr22.fa # 10s
bench ../seq/test/bench/bedcov.codon "data/biofast-data-v1/ex-anno.bed data/biofast-data-v1/ex-rna.bed" # 25s
bench ../seq/test/bench/cpg.codon data/chr22.fa # 1s
bench ../seq/test/bench/fasta.codon 25000000 # 10s
bench ../seq/test/bench/fastx.codon "data/chr22.fa data/biofast-data-v1/M_abscessus_HiSeq.fq" # 15s
# TODO: ../seq/test/bench/fmindex.codon
bench ../seq/test/bench/fqcnt.codon data/biofast-data-v1/M_abscessus_HiSeq.fq # 1s
bench ../seq/test/bench/hamming.codon data/chr22.fa # 20s
bench ../seq/test/bench/hash.codon data/chr22.fa # 15s
bench ../seq/test/bench/kmercnt.codon data/biofast-data-v1/M_abscessus_HiSeq.fq # 25s
bench ../seq/test/bench/knucleotide.codon "<data/three.fa" # 15s
bench ../seq/test/bench/match.codon data/chr22.fa # 1m
bench ../seq/test/bench/rc.codon data/chr22.fa  # 20s
bench ../seq/test/bench/revcomp.codon "<data/three.fa" # 1s
# TODO: ../seq/test/bench/sw.codon

bench ../seq/test/apps/avid/avid.codon data/wgac100.txt # 40s
# TODO: ../seq/test/apps/bwa/fastmap_build.codon
bench ../seq/test/apps/cora/hom_exact.codon "data/chr22.fa _cora" # 10s
bench ../seq/test/apps/cora/hom_inexact.codon "data/chr22.fa 1 _cora" # 3m
bench ../seq/test/apps/gatk/splitncigar.codon "data/rnaseq.bam data/chr22_hg19.fa _gatk" # 2s; TODO: OUTPUT EMPTY
# TODO: ../seq/test/apps/minimap2/sw_simple.codon data/queries.small data/targets.small # compiles but got stuck on running!
# TODO: ../seq/test/apps/minimap2/sw.codon data/queries.small data/targets.small # compiles but got stuck on running!
bench ../seq/test/apps/mrsfast/mrsfast.codon "index data/chr22.fa" # 15s
bench ../seq/test/apps/mrsfast/mrsfast.codon "search data/chr22.fa data/biofast-data-v1/M_abscessus_HiSeq.fq _mrsfast" # 20s
bench ../seq/test/apps/umi/whitelist.codon data/hgmm_100_R1.fastq # 5s

# CODON_DIR=../../install python setup_codon.py build_ext --inplace

cleanup () {
    rm -f _cora* _gatk _mrsfast testjar.bin _dump_* fmi.bin
}

cleanup
