import sys

# id range

command = sys.argv[1]
beginID = int(sys.argv[2])
endID = int(sys.argv[3])
instrument = sys.argv[4]
price = sys.argv[5]
count = sys.argv[6]

for i in range(beginID, endID):
  print(command + " " + str(i) + " " + instrument + " " + price + " " + count)