name	keyboard
type	win16

#1	pascal	Inquire
#2	pascal	Enable
#3	pascal	Disable
4   pascal16 ToAscii(word word ptr ptr word) ToAscii16
5   pascal16 AnsiToOem(str ptr) AnsiToOem16
6   pascal16 OemToAnsi(str ptr) OemToAnsi16
7   return   SetSpeed 2 65535
#100	pascal	ScreenSwitchEnable
#126	pascal	GetTableSeg
#127	pascal	NewTable
128 pascal   OemKeyScan(word) OemKeyScan
129 pascal16 VkKeyScan(word) VkKeyScan16
130 pascal16 GetKeyboardType(word) GetKeyboardType16
131 pascal16 MapVirtualKey(word word) MapVirtualKey16
132 pascal16 GetKBCodePage() GetKBCodePage16
133 pascal16 GetKeyNameText(long ptr word) GetKeyNameText16
134 pascal16 AnsiToOemBuff(ptr ptr word) AnsiToOemBuff16
135 pascal16 OemToAnsiBuff(ptr ptr word) OemToAnsiBuff16
#136	pascal	EnableKbSysReq
#137	pascal	GetBiosKeyProc

