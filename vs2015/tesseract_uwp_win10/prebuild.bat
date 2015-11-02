REM Walkthrough: Creating an SDK using C++
REM https://msdn.microsoft.com/en-us/library/jj127117(v=vs.140).aspx#createclasslibrary

del "$(ProjectDir)\Redist\Debug\Arm\*.*" /q
del "$(ProjectDir)\Redist\Debug\x86\*.*" /q
del "$(ProjectDir)\Redist\Debug\x64\*.*" /q
del "$(ProjectDir)\Redist\Retail\Arm\*.*" /q
del "$(ProjectDir)\Redist\Retail\x86\*.*" /q
del "$(ProjectDir)\Redist\Retail\x64\*.*" /q
del "$(ProjectDir)\References\CommonConfiguration\Neutral\*.*" /q

REM copy *.dll to \Redist\Debug\
copy "$(SolutionDir)\Release\tesseract304\Tesseract.dll"        "$(ProjectDir)\Redist\Debug\x86"
copy "$(SolutionDir)\x64\Release\tesseract304\Tesseract.dll"    "$(ProjectDir)\Redist\Debug\x64"
copy "$(SolutionDir)\ARM\Release\tesseract304\Tesseract.dll"    "$(ProjectDir)\Redist\Debug\Arm"

REM copy *.dll to \Redist\Retail\
copy "$(SolutionDir)\Release\tesseract304\Tesseract.dll"        "$(ProjectDir)\Redist\Retail\x86"
copy "$(SolutionDir)\x64\Release\tesseract304\Tesseract.dll"    "$(ProjectDir)\Redist\Retail\x64"
copy "$(SolutionDir)\ARM\Release\tesseract304\Tesseract.dll"    "$(ProjectDir)\Redist\Retail\Arm"

REM copy *.winmd to \References\CommonConfiguration\[Neutral|ARM|X86|x64]\"
copy "$(SolutionDir)\Release\tesseract304\Tesseract.winmd"      "$(ProjectDir)\References\CommonConfiguration\Neutral\"