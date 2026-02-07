make
python ../../buildbinary.py bmbinary.s68 hpfrotz.bin
python ../../lifutils.py put frotz.lif hpfrotz.bin SYSTEM_ c001
python ../../lifutils.py put frotz.lif zork1.z3 1
python ../../lifutils.py put frotz.lif nord.z4 1
(cd ../.. && ./lif2hfe.sh c/hpfrotz/frotz)
cp frotz.hfe e:/DSKA0005.HFE