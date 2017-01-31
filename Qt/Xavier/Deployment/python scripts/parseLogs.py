from logFunctions import*
from sys import argv
script, basestationLogPath, cerebroLogPath, showGraph = argv

showGraph = int(showGraph)

baseDF,baseData = parseBaseStation(basestationLogPath)
cerebroDF,cerebroData = parseCerebroLog(cerebroLogPath)
combinedDF,compareData = compare(baseDF,cerebroDF,cerebroData['paramNames'])
summaryStrings = printSummary(combinedDF,baseData['sessionLength'],cerebroData,compareData)

if showGraph:
    matplotlibGraph(cerebroLogPath,combinedDF,compareData,summaryStrings)
else:
    writeSummary(cerebroLogPath,combinedDF,baseData,cerebroData,compareData)
