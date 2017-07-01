# csv2db.py
# takes serial monitor output
# already in csv format
# shoves it into a sqlite3 db for further review

import csv
import re
import sqlite3
import sys

if len(sys.argv) < 2:
    # throw exception
    sys.exit()

fileName = sys.argv[1]
pCsvFile = '\.csv$'
match = re.search(pCsvFile, fileName)
if not match:
    raise ValueError("input file must end in '.csv': " + fileName)

# numerator,ratio,hue,invertedHue,scaledHue,preppedHue,currentHue,millisecond
# 55.70,26.52,27,229,114,174,174,750
dbFileName = re.sub(pCsvFile, '.db', fileName)
conn = sqlite3.connect(dbFileName)
c = conn.cursor()
createTable = []
createTable.append('create table if not exists hueCalculations (')
createTable.append('id INTEGER PRIMARY KEY,')
createTable.append('numerator FLOAT, ratio FLOAT, hue INTEGER,')
createTable.append('invertedHue INTEGER, scaledHue INTEGER,')
createTable.append('preppedHue INTEGER, currentHue INTEGER,')
createTable.append('millisecond INTEGER )')
createSQL = ' '.join(createTable)
c.execute(createSQL)
conn.commit()

insertRow = []
insertRow.append('insert into hueCalculations (')
insertRow.append('numerator, ratio, hue,')
insertRow.append('invertedHue, scaledHue,')
insertRow.append('preppedHue, currentHue,')
insertRow.append('millisecond')
insertRow.append(') values ( ?, ?, ?, ?, ?, ?, ?, ? )')
insertSQL = ' '.join(insertRow)

counter = 0
with open(fileName) as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        if 'millisecond' not in row:
            continue
        bind = (row['numerator'], row['ratio'], row['hue'],
                row['invertedHue'], row['scaledHue'],
                row['preppedHue'], row['currentHue'],
                row['millisecond'])
        c.execute(insertSQL, bind)
        conn.commit()
        counter += 1

print str(counter) + " record[s] inserted in " + dbFileName + "\n\n"
