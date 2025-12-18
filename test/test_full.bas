10 REM GW-BASIC C Port Test Suite
20 PRINT "=== GW-BASIC C Port Test ==="
30 PRINT
40 REM Variables
50 A = 10
60 B = 3
70 PRINT "A = "; A
80 PRINT "B = "; B
90 PRINT "A + B = "; A + B
100 PRINT "A * B = "; A * B
110 PRINT "A / B = "; A / B
120 PRINT
130 REM Strings
140 N$ = "World"
150 PRINT "Hello, "; N$; "!"
160 PRINT
170 REM Math functions
180 PRINT "SQR(16) = "; SQR(16)
190 PRINT "ABS(-5) = "; ABS(-5)
200 PRINT "INT(3.7) = "; INT(3.7)
210 PRINT
220 REM FOR loop
230 PRINT "Counting: ";
240 FOR I = 1 TO 5
250 PRINT I; " ";
260 NEXT I
270 PRINT
280 PRINT
290 PRINT "=== Test Complete ==="
300 END
