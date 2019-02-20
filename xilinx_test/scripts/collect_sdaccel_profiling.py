#!/usr/bin/env python3
import argparse
import os
import re

def collect_profiling(folder, regex, output):
  """Collects timing information from all SDAccel timing profile summary files
     in the given folder matching the filename pattern regex, outputting the
     information into a csv file output"""

  filePattern = re.compile(regex)
  totalTimePattern = re.compile("clWaitForEvents,[^,]+,([^,]+)")
  kernelTimePattern = re.compile(
      "Kernel Execution\n[^\n]+\n[^,]+,[^,]+,([^,]+)")

  filesProcessed = 0

  with open(output, "w") as outFile:

    outFile.write("time_total,time_kernel\n")

    for filename in os.listdir(folder):

      m = filePattern.match(filename)
      if not m:
        continue

      with open(os.path.join(folder, filename), "r") as inFile:
        content = inFile.read()
        totalTime = float(totalTimePattern.search(content).group(1))
        kernelTime = float(kernelTimePattern.search(content).group(1))
        outFile.write("{},{}\n".format(totalTime, kernelTime))


if __name__ == "__main__":

  argParser = argparse.ArgumentParser()
  argParser.add_argument("folder", type=str)
  argParser.add_argument("regex", type=str)
  argParser.add_argument("output", type=str)
  args = vars(argParser.parse_args())

  collect_profiling(args["folder"], args["regex"], args["output"])
