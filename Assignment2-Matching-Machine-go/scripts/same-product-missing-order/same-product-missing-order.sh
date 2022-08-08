# two buyer and one seller

# make
cd ../..
make all
cd -

# generate data
# seller order, order id from 1 to 50, instrument: "AAAA", unit price 1, count: 1
python3 ../gen.py S 0 50 AAAA 1 1 > seller1.input
# buyer order, order id from 50 to 100, instrument: "AAAA", unit price: 1, count: 1
python3 ../gen.py B 50 100 AAAA 1 1 > buyer1.input
# buyer order, order id from 100 to 150, instrument: "AAAA"< unit price 1, count: 1
python3 ../gen.py B 100 150 AAAA 1 1 > buyer2.input

python3 ../gen.py S 150 200 AAAA 1 1 > seller2.input


# one engine
../../engine socket > output.txt 2> warning.txt &

# wait 1 second
sleep 1

# 2 buyer
../../client socket < seller1.input &
../../client socket < seller2.input &
# 2 seller
../../client socket < buyer1.input &
../../client socket < buyer2.input &

sleep 2

killall engine

rm socket

echo "AAAA occurance should be: 100"
echo "AAAA occurance is:"
grep -o -i AAAA output.txt | wc -l
echo "E occurance should be: 100"
echo "E occurance is:"
grep -o -i E output.txt | wc -l