$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$build = "firmware\test\host\build"
New-Item -ItemType Directory -Force -Path $build | Out-Null
$include = "/I firmware\components\pcf8575\include /I firmware\components\field_io\include /I firmware\components\config_store\include /I firmware\components\calibration\include /I firmware\components\machine\include"
cmd /c "call `"$vcvars`" >nul && cl /nologo /W4 $include /Fo:$build\ /Fe:$build\test_pcf8575.exe firmware\test\host\test_pcf8575.c firmware\components\pcf8575\pcf8575.c && $build\test_pcf8575.exe && cl /nologo /W4 $include /Fo:$build\ /Fe:$build\test_field_io.exe firmware\test\host\test_field_io.c firmware\components\field_io\field_io.c && $build\test_field_io.exe && cl /nologo /W4 $include /Fo:$build\ /Fe:$build\test_machine.exe firmware\test\host\test_machine.c firmware\components\machine\machine.c firmware\components\config_store\config_store.c firmware\components\config_store\alarm.c firmware\components\calibration\calibration.c firmware\components\field_io\field_io.c && $build\test_machine.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
