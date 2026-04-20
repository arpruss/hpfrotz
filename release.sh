make
lif=binaries/DSKA0000.LIF
hfe=binaries/DSKA0000.HFE
python ../../lifutils.py put $hfe hpfrotz.bin SYSTEM_ c001
python ../../lifutils.py put $lif hpfrotz.bin SYSTEM_ c001
echo $hfe $lif
cp -r ../libhp165x binaries/
( cd binaries/libhp165x && make clean )
mkdir source
make clean
cp -r Makefile *.sh *.py font3.txt COPYING common hp165x source
zip -9r release binaries source
