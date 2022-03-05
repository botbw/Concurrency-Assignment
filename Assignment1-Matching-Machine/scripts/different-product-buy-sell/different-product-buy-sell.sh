# multiple client add same or different order

# make
cd ../..
make all
cd -

# seller order, order id from 0 to 50, instrument: AAAA, price: 1, count 1
python3 ../gen.py S 0 50 "AAAA" 20 33 > seller1.input

python3 ../gen.py S 50 100 "BBBB" 50 47 > seller2.input
python3 ../gen.py S 100 150 "AAAA" 40 67 > seller3.input
python3 ../gen.py B 150 200 "AAAA" 30 17 > buyer1.input
python3 ../gen.py B 200 250 "BBBB" 55 11 > buyer2.input
python3 ../gen.py B 250 300 "AAAA" 10 3 > buyer3.input


# start engine
../../engine socket > output.txt 2> warning.txt &

sleep 1

# start clients
../../client socket < buyer1.input &
../../client socket < seller2.input &
../../client socket < seller3.input &
../../client socket < buyer3.input &
../../client socket < seller1.input &
../../client socket < buyer2.input &

sleep 2

killall engine

rm socket