@echo off
Setlocal Enabledelayedexpansion

Set _Ver=2.0


:: This Function is created by Kvc At '5:35 PM 2/17/2017'
:: Use 'Button /?' for help...on using this function...

:: this function prints a Button like interface / image on cmd console...using batbox.exe,getlen Function
:: you have to specify the text of the button... like "OK, cancel, Retry etc..." as a parameter...
:: if the name contains spaces...then use Double quotes to write the name of button in the parameter...

:: Unlike the previous version, This version prints the buttons and generates a 'Check_Button_Click.bat' file for 
:: the sake of easiness of the Programmer. He can directly call this generated file to check which button form the list
:: is being clicked. :)

:: I Hope it'll make programming in batch much more easier than it was before. Try 'Button /?' for knowing more about using
:: This button function v.2.0 by Kvc


:: [%1 = X-ordinate]
:: [%2 = Y-co_ordinate]
:: [%3 = the colour Code for the List,e.g. fc,08,70,07 etc...]
:: [%4 = Text of the Button. e.g. Ok, Retry etc...]
:: [%5 = 'X' - Sequence Terminating character]
:: [%6 = _variable to save button co-ordinates position.]
:: [%7 = _variable to save Mouse hover color for the buttons.]

:: E.g. : Call Button.bat [X] [Y] [Color] "Button 1" [X2] [Y2] [Color2] "Button 2" [X3] [Y3] [Color3] "Button 3" ... X [_Variable_Box] [_Variable_Hover]

:: If you like my work, feel free to motivate me through your comments @ www.thebateam.org. I'll be happy to read them.
:: You can also find more batch plugins like this one from there. :) Happy coding!
rem #TheBATeam


If /i "%~1" == "" (goto :help)
If /i "%~1" == "/?" (goto :help)
If /i "%~1" == "-h" (goto :help)
If /i "%~1" == "help" (goto :help)
If /i "%~1" == "-help" (goto :help)

If /i "%~1" == "ver" (echo.%_Ver%&&goto :eof)


If /i "%~2" == "" (goto :help)
If /i "%~3" == "" (goto :help)
If /i "%~4" == "" (goto :help)
If /i "%~5" == "" (goto :help)
If /i "%~6" == "" (goto :help)
If /i "%~7" == "" (goto :help)


REM Setting-up variables for necessary calculations.
Set _Hover=
Set _Box=
Set _Text=
Set Button_height=3


:Loop_of_button_fn
REM Getting Button parameters...
Set _X=%~1
Set _Y=%~2
set color=%~3
Set _Invert_Color=%Color:~1,1%%Color:~0,1%
set "Button_text=%~4"

REM Loop Breaking Statements...
if not defined _X (goto :EOF)
if /i "%_X%" == "X" (Goto :End)

REM Checking the length of button according to Button_text
Call Getlen "..%button_text%.."
set button_width=%errorlevel%

REM Little math is important... :)
Set /A _X_Text=%_X% + 2
Set /A _Y_Text=%_Y% + 1
Set /A _X_End=%_X% + %button_width% - 1
Set /A _Y_End=%_Y% + %Button_height% - 1

REM Printing a Button like layout using Box Function!
Call Box %_X% %_Y% %Button_height% %button_width% - - %Color% 0

REM Saving Global variables...
Set "_Text=!_Text!/g !_X_Text! !_Y_Text! /c 0x!color! /d "!Button_text!" "
Set "_Hover=!_Hover!!_Invert_Color! "
Set "_Box=!_Box!!_X! !_Y! !_X_End! !_Y_End! "

REM Shifting the parameters for next button Code...
For /L %%A In (1,1,4) Do (Shift /1)
Goto :Loop_of_button_fn

:End
Batbox %_Text% /c 0x07
Endlocal && set "%~2=%_Box%" && set "%~3=%_Hover%"
Goto :EOF

:help
Echo.
Echo. This function helps in Adding a little GUI effect into your batch program...
echo. It Prints simple Button on the cmd console at specified X Y co-ordinate...
Echo. After printing, Returns the area of the button on CMD and the Inverted color
Echo. You can use this information in the main program with GetInput.exe Plugin of
Echo. Batch. And, it will make your life much more easier than before.
Echo. 
echo.
echo. Syntax: call Button [Item 1] [Item 2] [Item 3] ... X _Var_Box _Var_Hover
echo. Syntax: call Button [help ^| /^? ^| -h ^| -help]
echo. Syntax: call Button ver
echo.
echo. Where:-
Echo.
Echo. [Item] is short for 	= [X] [Y] [color] "[Button Text]"
Echo.
echo. [X]	= X-ordinate of top-left corner of Button
echo. [Y]	= Y-co_ordinate of top-left corner of Button
echo. ver	= Version of Button function
Echo. [Item #]= The Items to display. And, Item Represents elements as told above.
Echo. X 	= Here, the Single 'X' (Must be third last element) indicates the Loop
Echo.	 Sequence breaker for the Function. After 'X' You need to provide two -
Echo.	 Variables. Which will return the Corresponding values for the GetInput
Echo.	 Batch Plugin by aacini.
Echo. _Var_Box = The Second last parameter, which is name of the variable to save
Echo.	 the values of All Buttons on the console.
Echo. _Var_Hover = The Last parameter, which is name of the variable to save the 
Echo.	 mouse hover color for each button seperately.
Echo.
Echo. Use the above two output variables with 'GetInput.exe' Plugin as follows:
Echo. GetInput /M %%_Var_Box%% /H %%_Var_Hover%%
echo.
echo. This version %_Ver% of Button function contains much more GUI Capabilities.
echo. As it uses batbox.exe and calls it only once at the end of calculation...
Echo. For the most efficient output. This ver. uses GetInput By Aacini too. For the
Echo. Advanced Output on the console.
Echo.
echo. Visit www.thebateam.org for more...
echo. #TheBATeam
goto :EOF