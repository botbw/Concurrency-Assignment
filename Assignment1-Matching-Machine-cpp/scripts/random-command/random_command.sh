# random command, random instrument in {"AAAA", "BBBB", "CCCC", "DDDD"}, orderID from 0 to 100

python3 random_gen.py 0 100 > client1.input
python3 random_gen.py 100 200 > client2.input
python3 random_gen.py 200 300 > client3.input
python3 random_gen.py 300 400 > client4.input

# make
cd ../..
make all
cd -

# start engine
../../engine socket > output.txt 2> warning.txt &

sleep 1

# start clients
../../client socket < client1.input &
../../client socket < client2.input &
../../client socket < client3.input &
../../client socket < client4.input &

sleep 1

killall engine

rm socket
