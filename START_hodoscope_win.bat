rem "Starting EURUN"
start bin\euRun.exe
timeout /t 2

rem "Starting EULOG"
start bin\euLog.exe
timeout /t 1

rem "Starting Data collector"
start bin\euCliCollector.exe -n DirectSaveDataCollector -t dc1
timeout /t 1

rem "Starting AHCAL producer"
start bin\AHCALProducer.exe
rem "DONE"
timeout /t 1

rem "Startin EASIROC modules"
start bin\euCliProducer.exe -n CaliceHodoscopeProducer -t hodoscope1
start bin\euCliProducer.exe -n CaliceHodoscopeProducer -t hodoscope2
rem "done."
