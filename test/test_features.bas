10 REM Test various GW-BASIC features
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
200 PRINT
210 REM FOR loop
220 PRINT "Counting: ";
230 FOR I = 1 TO 5
240 PRINT I; " ";
250 NEXT I
260 PRINT
270 PRINT
280 PRINT "=== Test Complete ==="
290 END
