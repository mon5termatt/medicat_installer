@Echo off

Set _Ver=4.0

::The following Function is created by kvc...don't modify it...if you don't know what are you doing...
::it takes following arguments...
:: it is the ver.4.0 of Box function... and it contains diffrnet types of Box printing options... #kvc
rem :: Unwanted parameters are removed ... (i.e. Dialogue Box...) Make the color code for FG and BG same for simply getting a dialogue box...

:: [%1 = X-ordinate]
:: [%2 = Y-co_ordinate]
:: [%3 = height of box]
:: [%4 = width of box] 
:: [%5 = width From where to separate box,if don't specified or specified '-' (minus),then box will not be separated.]
:: [%6 = Background element of Box,if not specified or specified '-' (minus),then no background will be shown...It should be a single Character...]
:: [%7 = the colour Code for the Box,e.g. fc,08,70,07 etc...don't define it if you want default colour...or type '-' (minus) for no color change...]
:: [%8 = The Style / type of box you want to print.]
:: [%9 = _Variable to save output to.]

rem #kvc

rem :: Visit www.thebateam.org for more extensions / plug-ins like this.... :]
rem #TheBATeam

If /i "%~1" == "" (goto :help)
If /i "%~1" == "/?" (goto :help)
If /i "%~1" == "-h" (goto :help)
If /i "%~1" == "help" (goto :help)
If /i "%~1" == "-help" (goto :help)

If /i "%~1" == "ver" (echo.4.0&&goto :eof)


If /i "%~2" == "" (goto :help)
If /i "%~3" == "" (goto :help)
If /i "%~4" == "" (goto :help)

:Box
setlocal Enabledelayedexpansion
set _string=
set "_SpaceWidth=/d ""
set _final=

set x_val=%~1
set y_val=%~2
set sepr=%~5
if /i "!sepr!" == "-" (set sepr=)
set char=%~6
if /i "!char!" == "-" (set char=)
if defined char (set char=!char:~0,1!) ELSE (set "char= ")
set color=%~7
if defined color (if /i "!color!" == "-" (set color=) Else (set "color=/c 0x%~7"))

Set Type=%~8
If not defined Type (Set Type=1)
If %Type% Gtr 4 (Set Type=1)

If /i "%Type%" == "0" (
	If /I "%~6" == "-" (
		set _Hor_line=/a 32
		set _Ver_line=/a 32
		set _Top_sepr=/a 32
		set _Base_sepr=/a 32
		set _Top_left=/a 32
		set _Top_right=/a 32
		set _Base_right=/a 32
		set _Base_left=/a 32
		) ELSE (
		set _Hor_line=/d "%char%"
		set _Ver_line=/d "%char%"
		set _Top_sepr=/d "%char%"
		set _Base_sepr=/d "%char%"
		set _Top_left=/d "%char%"
		set _Top_right=/d "%char%"
		set _Base_right=/d "%char%"
		set _Base_left=/d "%char%"
		)
)

If /i "%Type%" == "1" (
set _Hor_line=/a 196
set _Ver_line=/a 179
set _Top_sepr=/a 194
set _Base_sepr=/a 193
set _Top_left=/a 218
set _Top_right=/a 191
set _Base_right=/a 217
set _Base_left=/a 192
)

If /i "%Type%" == "2" (
set _Hor_line=/a 205
set _Ver_line=/a 186
set _Top_sepr=/a 203
set _Base_sepr=/a 202
set _Top_left=/a 201
set _Top_right=/a 187
set _Base_right=/a 188
set _Base_left=/a 200
)

If /i "%Type%" == "3" (
set _Hor_line=/a 205
set _Ver_line=/a 179
set _Top_sepr=/a 209
set _Base_sepr=/a 207
set _Top_left=/a 213
set _Top_right=/a 184
set _Base_right=/a 190
set _Base_left=/a 212
)

If /i "%Type%" == "4" (
set _Hor_line=/a 196
set _Ver_line=/a 186
set _Top_sepr=/a 210
set _Base_sepr=/a 208
set _Top_left=/a 214
set _Top_right=/a 183
set _Base_right=/a 189
set _Base_left=/a 211
)

set /a _char_width=%~4-2
set /a _box_height=%~3-2

for /l %%A in (1,1,!_char_width!) do (
	if /i "%%A" == "%~5" (
	set "_string=!_string! !_Top_sepr!"
	set "_SpaceWidth=!_SpaceWidth!" !_Ver_line! /d ""
	) ELSE (
	set "_string=!_string! !_Hor_line!"
	set "_SpaceWidth=!_SpaceWidth!!char!"
	)
)

set "_SpaceWidth=!_SpaceWidth!""
set "_final=/g !x_val! !y_val! !_Top_left! !_string! !_Top_right! !_final! "
set /a y_val+=1

for /l %%A in (1,1,!_box_height!) do (
set "_final=/g !x_val! !y_val! !_Ver_line! !_SpaceWidth! !_Ver_line! !_final! "
set /a y_val+=1
)

REM Made Some Advanced changes to algortihm, like switched the way the algorithm generates the variables...
Set _To_Replace=!_Top_sepr:~-3!
Set _Replace_With=!_Base_sepr:~-3!

For %%A in ("!_To_Replace!") do For %%B in ("!_Replace_With!") do set "_final=/g !x_val! !y_val! !_Base_left! !_string:%%~A=%%~B! !_Base_right! !_final! "

IF /i "%~9" == "" (batbox %color% %_final% /c 0x07) ELSE (ENDLOCAL && Set "%~9=%color% %_final% /c 0x07")
goto :eof


:help
Echo.
Echo. This function helps in Adding a little GUI effect into your batch program...
echo. It Prints simple box on the cmd console at specified X Y co-ordinate...
echo.
echo. Syntax: call Box [X] [Y] [Height] [Width] [Sepration] [BG_Char] [color] [Type]
Echo.              [_Var]
echo. Syntax: call Box [help ^| /^? ^| -h ^| -help]
echo. Syntax: call Box ver
echo.
echo. Where:-
echo. X		= X-ordinate of top-left corner of box
echo. Y		= Y-co_ordinate of top-left corner of box
echo. Height		= height of box
echo. Width		= width of box
echo. Sepration	= width From where to separate box,if don't specified or
echo.		  specified '-' (minus),then box will not be separated.
echo. BG_char	= Background element of Box,if not specified or specified
echo.		  '-' (minus),then no background will be shown...It should be
echo.		  a single Character...
echo. color		= the color Code for the Box,e.g. fc,08,70,07 etc...
echo.		  Don't define it if you want default colour...or type '-'
echo.		  (minus) for no color change...
echo. Type 		= The style / type of the Box you want, double Border, single
echo.		  Border etc. New, No Border Option added [Valid values: 0 to 4]
Echo. _Var 		= Variable to Save Output, instead of Printing Directly.
Echo.			(Optional)
echo. ver		= Version of Box function
echo.
echo. This version 4.0 of Box function contains much more GUI Capabilities.
echo. As it uses batbox.exe and calls it only once at the end of calculation...
echo. Visit www.thebateam.org for more...
echo. #Kvc with #TheBATeam
goto :eof