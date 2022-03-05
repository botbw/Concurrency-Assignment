echo """\
S 100 AAAA 30 10
B 105 AAAA 20 1
S 102 AAAA 50 20
S 110 AAAA 30 20
S 184 AAAA 20 10
B 107 AAAA 40 32
C 100
C 110
C 102
C 184
C 107
C 105
""" > client1.input

# make
cd ../..
make all
cd -

# one engine
../../engine socket > output.txt 2> warning.txt &

# wait 1 second
sleep 1

# client
../../client socket < client1.input &

sleep 1

killall engine

rm socket

vimdiff output.txt referential_output.txt