mkdir $dest
cp -r src cmake examples CMakeLists.txt README.md LICENSE version $dest
mkdir dist
tar czf dist/$dest.tar.gz $dest
rm -r $dest
