import sys
import random
# id range
beginID = int(sys.argv[1])
endID = int(sys.argv[2])

instruments = ["AAAA", "BBBB", "CCCC", "DDDD"]
command = ["S", "B", "C"]

for i in range(beginID, endID):
  commandID = random.randint(0, len(command)-1)
  if command[commandID] == "C":
    print("C " + str(random.randint(beginID, i)))
    continue
  instrumentsID = random.randint(0, len(instruments)-1)
  print(command[commandID] + " " + str(i) + " " + instruments[instrumentsID] + " 1 1")