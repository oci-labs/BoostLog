set _VSVARS_DEF_64="C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\VsDevCmd.bat"

call %_VSVARS_DEF_64% >nul: 2>&1
set _VSVARS_DEF_64=

set BOOST_ROOT=d:\dev_tools\boost_1_60_0
set MPC_ROOT=d:\dev_tools\MPC-MPC_4_1_2

perl %MPC_ROOT%\mwc.pl -type vc12 All.mwc
