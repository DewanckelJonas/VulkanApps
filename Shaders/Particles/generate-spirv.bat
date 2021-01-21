for %%i in (*.vert *.frag) do "glslangValidator.exe" -V "%%~i" -o "%%~i.spv"
pause
