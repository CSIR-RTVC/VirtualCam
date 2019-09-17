if not exist ..build mkdir ..\build
  
cd ..\build
cmake ..

if %ERRORLEVEL% EQU 0 (
  ECHO "Building Release version"
  cmake --build . --config Release 
  ECHO "Building Debug version"
  cmake --build . --config Debug 
) else (
  ECHO "Error generating build files"
)
  
cd ..\scripts  