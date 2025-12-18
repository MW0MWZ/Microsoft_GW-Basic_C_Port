10 REM GW-BASIC Demo Program
20 PRINT "========================================="
30 PRINT "GW-BASIC C Port Demonstration"
40 PRINT "========================================="
50 PRINT ""
60 PRINT "Testing arithmetic:"
70 PRINT "10 + 20 = "; 10 + 20
80 PRINT "5 * 6 = "; 5 * 6
90 PRINT "2 ^ 8 = "; 2 ^ 8
100 PRINT ""
110 PRINT "Testing string functions:"
120 LET A$ = "HELLO"
130 PRINT "String: "; A$
140 PRINT "Length: "; LEN(A$)
150 PRINT ""
160 PRINT "Countdown:"
170 FOR I = 5 TO 1 STEP -1
180 PRINT I; "..."
190 NEXT I
200 PRINT "Blastoff!"
210 PRINT ""
220 PRINT "Program complete."
230 END
