#!/usr/bin/python
# Author: Justin Mammarella
# Description: Checks to see if a project was initialised
#              correctly by counting occupied/unoccupied slots
#              For new projects all slots should be unoccupied.
#
# Usage: ./project_check.py projectname.mcl

import sys

GRID_WIDTH = 22
GRID_LENGTH = 130

def main():
    print sys.argv[1]
    f = open(sys.argv[1],'rb')
    n = 1
    empty_slots = 0
    not_empty_slots = 0
    while n < GRID_WIDTH * GRID_LENGTH:
      f.seek(0x1000 * n,0)
      b = f.read(1)
      if ord(b) == 0x00:
        print "Empty",n
        empty_slots += 1
      else:
        print "Not empty",n
        not_empty_slots += 1
      print n, b
      n = n + 1
   
    print "Empty slots: ",  empty_slots
    print "Occupied slot: ", not_empty_slots

if __name__ == "__main__":
    main()
