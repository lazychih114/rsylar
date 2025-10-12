
pkill raftCoreRun

cd ../bin
rm -rf log/
mkdir log
./raftCoreRun 0 29016 "conf0" >> ./log/std0.log 2>&1 &
./raftCoreRun 1 29017 "conf1" >> ./log/std1.log 2>&1 &
./raftCoreRun 2 29018 "conf2" >> ./log/std2.log 2>&1 &