import getopt
import os.path
import sys
print "caution:this script requires source file paths as 1st parameter"
print "destination file paths as 2nd   "
filein='' #input file path
fileout='' #output file path
opts, args=getopt.getopt(sys.argv[1:],'h') 
if len(args)<2:# input or output parameter missing
 print "input parameters missing"
 sys.exit(0)
else:
 if not os.path.isfile(args[0]):#check existence of input file, exit if not
   print args[0]+" can't be found "
   sys.exit(0)
 if os.path.isfile(args[1]):#check existence of output file, exit if yes
   print args[0]+" already exists "
   sys.exit(0)
 if os.path.isfile(args[0]) and (not os.path.isfile(args[1])):# normal parameters configuration
   filein=open(args[0], 'r')
   fileout=open(args[1], 'w')
   line=filein.readline()#process the input file line by line
   while line:# directly copy to output file and stop when blank line encountered
    fileout.write(line)
    line=filein.readline()
   print "copy finished"
   fileout.close()
   filein.close()
