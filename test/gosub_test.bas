10 PRINT "Testing GOSUB/RETURN"
20 GOSUB 100
30 PRINT "Back from subroutine"
40 GOSUB 100
50 PRINT "Done!"
60 END
100 REM Subroutine
110 PRINT "In subroutine"
120 RETURN
