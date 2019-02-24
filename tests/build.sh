mkdir build
cd build
cmake ../..
make klient
make serwer
cd ..

mv build/serwer serwer
mv build/klient klient
rm -rf build

