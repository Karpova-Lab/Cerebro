from logFunctions import*
from sys import argv
script, basestationLogPath, cerebroLogPath,showGraph = argv

basestationLogPath = basestationLogPath[1:-1]
cerebroLogPath = cerebroLogPath[1:-1]
showGraph = int(showGraph)

baseDF,baseData = parseBaseStation(basestationLogPath)
cerebroDF,cerebroData = parseCerebroLog(cerebroLogPath)
combinedDF,compareData = compare(baseDF,cerebroDF,cerebroData['paramNames'])
summaryStrings = printSummary(combinedDF,baseData['sessionLength'],cerebroData['paramRanges'],compareData)
if showGraph:
    matplotlibGraph(cerebroLogPath,combinedDF,compareData,summaryStrings)
else:
    writeSummary(cerebroLogPath,baseData,cerebroData,summaryStrings,combinedDF)
