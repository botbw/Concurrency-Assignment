# two buyer and one seller

# make
cd ../..
make all
cd -

# generate data
# seller order, order id from 1 to 50, instrument: "AAAA", unit price 50, count: 20
python3 ../gen.py S 0 50 AAAA 50 31 > seller.input
# buyer order, order id from 50 to 100, instrument: "AAAA", unit price: 51, count: 7
python3 ../gen.py B 50 100 AAAA 51 7 > buyer1.input
# buyer order, order id from 100 to 150, instrument: "AAAA"< unit price 50, count: 13
python3 ../gen.py B 100 150 AAAA 50 13 > buyer2.input


# one engine
../../engine socket > output.txt 2> warning.txt &

# wait 1 second
sleep 1

# 1 buyer
../../client socket < seller.input &
# 2 seller
../../client socket < buyer1.input &
../../client socket < buyer2.input &

sleep 2

killall engine

rm socket