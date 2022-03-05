import sys
import random

minOrderID = int(sys.argv[1])
maxOrderID = int(sys.argv[2])
count = int(sys.argv[3])
for i in range(0, count):
  print("C " + str(random.randint(minOrderID, maxOrderID-1)))