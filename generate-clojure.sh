I=./integration_tests
O=~/tmp/lpython-clojure
FILES="${I}/*.py"
rm -rf $O 2> /dev/null || true
mkdir -p $O
for F in ${FILES}
do
    X=${F%.py}
    Y=${X##*/}.clj
    Z=${O}/${Y}
    # echo "${X}, ${Y}, ${Z}"
    echo "processing ${F} into ${Z} ..."
    ./src/bin/lpython --show-asr --no-color ${F} > ${Z}
done
