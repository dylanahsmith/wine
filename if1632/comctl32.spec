name	comctl32
type	win32

# Functions exported by the Win95 comctl32.dll 
# (these need to have these exact ordinals, because some win95 dlls 
#  import comctl32.dll by ordinal)

  2 stub MenuHelp
  3 stub ShowHideMenuCtl
  4 stub GetEffectiveClientRect
  5 stdcall DrawStatusTextA(long ptr ptr long) DrawStatusText32A
  6 stdcall CreateStatusWindowA(long ptr long long) CreateStatusWindow32A
  7 stub CreateToolbar
  8 stub CreateMappedBitmap
  9 stub Cctl1632_ThunkData32
 10 stub CreatePropertySheetPage
 11 stub CreatePropertySheetPageA
 12 stub CreatePropertySheetPageW
 13 stub MakeDragList
 14 stub LBItemFromPt
 15 stub DrawInsert
 16 stdcall CreateUpDownControl(long long long long long long long long long long long long) CreateUpDownControl
 17 stdcall InitCommonControls() InitCommonControls
 18 stub CreateStatusWindow
 19 stub CreateStatusWindowW
 20 stub CreateToolbarEx
 21 stub DestroyPropertySheetPage
 22 stub DllGetVersion
 23 stub DrawStatusText
 24 stub DrawStatusTextW
 25 stub ImageList_Add
 26 stub ImageList_AddIcon
 27 stub ImageList_AddMasked
 28 stub ImageList_BeginDrag
 29 stub ImageList_Copy
 30 stub ImageList_Create
 31 stub ImageList_Destroy
 32 stub ImageList_DragEnter
 33 stub ImageList_DragLeave
 34 stub ImageList_DragMove
 35 stub ImageList_DragShowNolock
 36 stub ImageList_Draw
 37 stub ImageList_DrawEx
 38 stub ImageList_EndDrag
 39 stub ImageList_GetBkColor
 40 stub ImageList_GetDragImage
 41 stub ImageList_GetIcon
 42 stub ImageList_GetIconSize
 43 stub ImageList_GetImageCount
 44 stub ImageList_GetImageInfo
 45 stub ImageList_GetImageRect
 46 stub ImageList_LoadImage
 47 stub ImageList_LoadImageA
 48 stub ImageList_LoadImageW
 49 stub ImageList_Merge
 50 stub ImageList_Read
 51 stub ImageList_Remove
 52 stub ImageList_Replace
 53 stub ImageList_ReplaceIcon
 54 stub ImageList_SetBkColor
 55 stub ImageList_SetDragCursorImage
 56 stub ImageList_SetFilter
 57 stub ImageList_SetIconSize
 58 stub ImageList_SetImageCount
 59 stub ImageList_SetOverlayImage
 60 stub ImageList_Write
 61 stub InitCommonControlsEx
 62 stub PropertySheet
 63 stub PropertySheetA
 64 stub PropertySheetW
 65 stub _TrackMouseEvent
