/*
 * Windows Device Context initialisation functions
 *
 * Copyright 1996 John Harvey
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "windows.h"
#include "win16drv.h"

#include "callback.h"
#include "stddebug.h"
#include "debug.h"

#define MAX_PRINTER_DRIVERS 	16
static LOADED_PRINTER_DRIVER *gapLoadedPrinterDrivers[MAX_PRINTER_DRIVERS];


static void GetPrinterDriverFunctions(HINSTANCE16 hInst, LOADED_PRINTER_DRIVER *pLPD)
{
#define LoadPrinterDrvFunc(A) pLPD->fn[FUNC_##A] = \
      GetProcAddress16(hInst, MAKEINTRESOURCE(ORD_##A))

      LoadPrinterDrvFunc(BITBLT);
      LoadPrinterDrvFunc(COLORINFO);
      LoadPrinterDrvFunc(CONTROL);
      LoadPrinterDrvFunc(DISABLE);
      LoadPrinterDrvFunc(ENABLE);
      LoadPrinterDrvFunc(ENUMDFONTS);
      LoadPrinterDrvFunc(ENUMOBJ);
      LoadPrinterDrvFunc(OUTPUT);
      LoadPrinterDrvFunc(PIXEL);
      LoadPrinterDrvFunc(REALIZEOBJECT);
      LoadPrinterDrvFunc(STRBLT);
      LoadPrinterDrvFunc(SCANLR);
      LoadPrinterDrvFunc(DEVICEMODE);
      LoadPrinterDrvFunc(EXTTEXTOUT);
      LoadPrinterDrvFunc(GETCHARWIDTH);
      LoadPrinterDrvFunc(DEVICEBITMAP);
      LoadPrinterDrvFunc(FASTBORDER);
      LoadPrinterDrvFunc(SETATTRIBUTE);
      LoadPrinterDrvFunc(STRETCHBLT);
      LoadPrinterDrvFunc(STRETCHDIBITS);
      LoadPrinterDrvFunc(SELECTBITMAP);
      LoadPrinterDrvFunc(BITMAPBITS);
      LoadPrinterDrvFunc(EXTDEVICEMODE);
      LoadPrinterDrvFunc(DEVICECAPABILITIES);
      LoadPrinterDrvFunc(ADVANCEDSETUPDIALOG);
      LoadPrinterDrvFunc(DIALOGFN);
      LoadPrinterDrvFunc(PSEUDOEDIT);
      dprintf_win16drv (stddeb,"got func CONTROL 0x%p enable 0x%p enumDfonts 0x%p realizeobject 0x%p extextout 0x%p\n",
              pLPD->fn[FUNC_CONTROL],
              pLPD->fn[FUNC_ENABLE],
              pLPD->fn[FUNC_ENUMDFONTS],
              pLPD->fn[FUNC_REALIZEOBJECT],
              pLPD->fn[FUNC_EXTTEXTOUT]);
      

}


static LOADED_PRINTER_DRIVER *FindPrinterDriverFromName(const char *pszDriver)
{
    LOADED_PRINTER_DRIVER *pLPD = NULL;
    int		nDriverSlot = 0;

    /* Look to see if the printer driver is already loaded */
    while (pLPD == NULL && nDriverSlot < MAX_PRINTER_DRIVERS)
    {
	LOADED_PRINTER_DRIVER *ptmpLPD;
	ptmpLPD = gapLoadedPrinterDrivers[nDriverSlot++];
	if (ptmpLPD != NULL)
	{
	    dprintf_win16drv(stddeb, "Comparing %s,%s\n",ptmpLPD->szDriver,pszDriver);
	    /* Found driver store info, exit loop */
	    if (lstrcmpi32A(ptmpLPD->szDriver, pszDriver) == 0)
	      pLPD = ptmpLPD;
	}
    }
    if (pLPD == NULL) fprintf(stderr,"Couldn't find driver %s\n", pszDriver);
    return pLPD;
}

static LOADED_PRINTER_DRIVER *FindPrinterDriverFromPDEVICE(SEGPTR segptrPDEVICE)
{
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    /* Find the printer driver associated with this PDEVICE */
    /* Each of the PDEVICE structures has a PDEVICE_HEADER structure */
    /* just before it */
    if (segptrPDEVICE != (SEGPTR)NULL)
    {
	PDEVICE_HEADER *pPDH = (PDEVICE_HEADER *)
	  (PTR_SEG_TO_LIN(segptrPDEVICE) - sizeof(PDEVICE_HEADER)); 
        pLPD = pPDH->pLPD;
    }
    return pLPD;
}

/* 
 * Load a printer driver, adding it self to the list of loaded drivers.
 */

LOADED_PRINTER_DRIVER *LoadPrinterDriver(const char *pszDriver)
{                                        
    HINSTANCE16	hInst;	
    LOADED_PRINTER_DRIVER *pLPD = NULL;
    int		nDriverSlot = 0;
    BOOL32	bSlotFound = FALSE;

    /* First look to see if driver is loaded */
    pLPD = FindPrinterDriverFromName(pszDriver);
    if (pLPD != NULL)
    {
	/* Already loaded so increase usage count */
	pLPD->nUsageCount++;
	return pLPD;
    }

    /* Not loaded so try and find an empty slot */
    while (!bSlotFound && nDriverSlot < MAX_PRINTER_DRIVERS)
    {
	if (gapLoadedPrinterDrivers[nDriverSlot] == NULL)
	  bSlotFound = TRUE;
	else
	  nDriverSlot++;
    }
    if (!bSlotFound)
    {
	fprintf(stderr,"Too many printers drivers loaded\n");
	return NULL;
    }

    {
        char *drvName = malloc(strlen(pszDriver)+5);
        strcpy(drvName, pszDriver);
        strcat(drvName, ".DRV");
        hInst = LoadLibrary16(drvName);
    }
    dprintf_win16drv(stddeb, "Loaded the library\n");

    
    if (hInst <= 32)
    {
	/* Failed to load driver */
	fprintf(stderr, "Failed to load printer driver %s\n", pszDriver);
    }
    else
    {
	HANDLE16 hHandle;

	/* Allocate some memory for printer driver info */
	pLPD = malloc(sizeof(LOADED_PRINTER_DRIVER));
	memset(pLPD, 0 , sizeof(LOADED_PRINTER_DRIVER));
	
	pLPD->hInst = hInst;
	strcpy(pLPD->szDriver,pszDriver);

	/* Get DS for the printer module */
	pLPD->ds_reg = hInst;

	dprintf_win16drv(stddeb, "DS for %s is %x\n", pszDriver, pLPD->ds_reg);

	/* Get address of printer driver functions */
	GetPrinterDriverFunctions(hInst, pLPD);

	/* Set initial usage count */
	pLPD->nUsageCount = 1;

	/* Create a thunking buffer */
	hHandle = GlobalAlloc16(GHND, (1024 * 8));
	pLPD->hThunk = hHandle;
	pLPD->ThunkBufSegPtr = WIN16_GlobalLock16(hHandle);
	pLPD->ThunkBufLimit = pLPD->ThunkBufSegPtr + (1024*8);

	/* Update table of loaded printer drivers */
	pLPD->nIndex = nDriverSlot;
	gapLoadedPrinterDrivers[nDriverSlot] = pLPD;
    }

    return pLPD;
}

/* 
 * Thunking utility functions
 */

static BOOL32 AddData(SEGPTR *pSegPtr, const void *pData, int nSize, SEGPTR Limit)
{
    BOOL32 bRet = FALSE;
    char *pBuffer = PTR_SEG_TO_LIN((*pSegPtr));
    char *pLimit = PTR_SEG_TO_LIN(Limit);


    if ((pBuffer + nSize) < pLimit)
    {
	DWORD *pdw = (DWORD *)pSegPtr;
	SEGPTR SegPtrOld = *pSegPtr;
	SEGPTR SegPtrNew;

	dprintf_win16drv(stddeb, "AddData: Copying %d from %p to %p(0x%x)\n", nSize, pData, pBuffer, (UINT32)*pSegPtr);
	memcpy(pBuffer, pData, nSize); 
	SegPtrNew = (SegPtrOld + nSize + 1);
	*pdw = (DWORD)SegPtrNew;
    }
    return bRet;
}


static BOOL32 GetParamData(SEGPTR SegPtrSrc,void *pDataDest, int nSize)
{
    char *pSrc =  PTR_SEG_TO_LIN(SegPtrSrc);
    char *pDest = pDataDest;

    dprintf_win16drv(stddeb, "GetParamData: Copying %d from %lx(%lx) to %lx\n", nSize, (DWORD)pSrc, (DWORD)SegPtrSrc, (DWORD)pDataDest);
    memcpy(pDest, pSrc, nSize);
    return TRUE;
}

/*
 *  Control (ordinal 3)
 */
INT16 PRTDRV_Control(LPPDEVICE lpDestDev, WORD wfunction, SEGPTR lpInData, SEGPTR lpOutData)
{
    /* wfunction == Escape code */
    /* lpInData, lpOutData depend on code */

    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    dprintf_win16drv(stddeb, "PRTDRV_Control: %08x 0x%x %08lx %08lx\n", (unsigned int)lpDestDev, wfunction, lpInData, lpOutData);

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP3, lP4;
	WORD wP2;

	if (pLPD->fn[FUNC_CONTROL] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_Control: Not supported by driver\n");
	    return 0;
	}

	lP1 = (SEGPTR)lpDestDev;
	wP2 = wfunction;
	lP3 = (SEGPTR)lpInData;
	lP4 = (SEGPTR)lpOutData;

	wRet = CallTo16_word_lwll(pLPD->fn[FUNC_CONTROL], 
                                  lP1, wP2, lP3, lP4);
    }
    dprintf_win16drv(stddeb, "PRTDRV_Control: return %x\n", wRet);
    return wRet;

    return 0;
}

/*
 *	Enable (ordinal 5)
 */
WORD PRTDRV_Enable(LPVOID lpDevInfo, WORD wStyle, LPCSTR  lpDestDevType, 
                          LPCSTR lpDeviceName, LPCSTR lpOutputFile, LPVOID lpData)
{
    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    dprintf_win16drv(stddeb, "PRTDRV_Enable: %s %s\n",lpDestDevType, lpOutputFile);

    /* Get the printer driver info */
    if (wStyle == INITPDEVICE)
    {
	pLPD = FindPrinterDriverFromPDEVICE((SEGPTR)lpDevInfo);
    }
    else
    {
	pLPD = FindPrinterDriverFromName((char *)lpDeviceName);
    }
    if (pLPD != NULL)
    {
	LONG lP1, lP3, lP4, lP5;
	WORD wP2;
	SEGPTR SegPtr = pLPD->ThunkBufSegPtr;
	SEGPTR Limit = pLPD->ThunkBufLimit;
	int   nSize;

	if (pLPD->fn[FUNC_ENABLE] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_Enable: Not supported by driver\n");
	    return 0;
	}

	if (wStyle == INITPDEVICE)
	{
	    /* All ready a 16 address */
	    lP1 = (SEGPTR)lpDevInfo;
	}
	else
	{
	    /* 32 bit data */
	    lP1 = SegPtr;
	    nSize = sizeof(DeviceCaps);
	    AddData(&SegPtr, lpDevInfo, nSize, Limit);	
	}
	
	wP2 = wStyle;
	
	lP3 = SegPtr;
	nSize = strlen(lpDestDevType) + 1;
	AddData(&SegPtr, lpDestDevType, nSize, Limit);	

	lP4 = SegPtr; 
	nSize = strlen(lpOutputFile) + 1;
	AddData(&SegPtr, lpOutputFile, nSize, Limit);	

	lP5 = (LONG)lpData;
        

	wRet = CallTo16_word_lwlll(pLPD->fn[FUNC_ENABLE], 
				   lP1, wP2, lP3, lP4, lP5);

	/* Get the data back */
	if (lP1 != 0 && wStyle != INITPDEVICE)
	{
	    nSize = sizeof(DeviceCaps);
	    GetParamData(lP1, lpDevInfo, nSize);
	}
    }
    dprintf_win16drv(stddeb, "PRTDRV_Enable: return %x\n", wRet);
    return wRet;
}


/*
 *	EnumDFonts (ordinal 6)
 */
WORD PRTDRV_EnumDFonts(LPPDEVICE lpDestDev, LPSTR lpFaceName,  
		       FARPROC16 lpCallbackFunc, LPVOID lpClientData)
{
    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    dprintf_win16drv(stddeb, "PRTDRV_EnumDFonts:\n");

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP2, lP3, lP4;

	SEGPTR SegPtr = pLPD->ThunkBufSegPtr;
	SEGPTR Limit = pLPD->ThunkBufLimit;
	int   nSize;

	if (pLPD->fn[FUNC_ENUMDFONTS] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_EnumDFonts: Not supported by driver\n");
	    return 0;
	}

	lP1 = (SEGPTR)lpDestDev;
	
	if (lpFaceName == NULL)
	{
	    lP2 = 0;
	}
	else
	{
	    lP2 = SegPtr;
	    nSize = strlen(lpFaceName) + 1;
	    AddData(&SegPtr, lpFaceName, nSize, Limit);	
	}

	lP3 = (LONG)lpCallbackFunc; 

	lP4 = (LONG)lpClientData;
        
	wRet = CallTo16_word_llll(pLPD->fn[FUNC_ENUMDFONTS], 
                                  lP1, lP2, lP3, lP4);
    }
    else 
        fprintf(stderr,"Failed to find device\n");
    
    dprintf_win16drv(stddeb, "PRTDRV_EnumDFonts: return %x\n", wRet);
    return wRet;
}
/*
 *	EnumObj (ordinal 7)
 */
BOOL16 PRTDRV_EnumObj(LPPDEVICE lpDestDev, WORD iStyle, 
		       FARPROC16 lpCallbackFunc, LPVOID lpClientData)
{
    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    dprintf_win16drv(stddeb, "PRTDRV_EnumDFonts:\n");

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP3, lP4;
	WORD wP2;

	if (pLPD->fn[FUNC_ENUMDFONTS] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_EnumDFonts: Not supported by driver\n");
	    return 0;
	}

	lP1 = (SEGPTR)lpDestDev;
	
	wP2 = iStyle;

	/* 
	 * Need to pass addres of function conversion function that will switch back to 32 bit code if necessary
	 */
	lP3 = (LONG)lpCallbackFunc; 

	lP4 = (LONG)lpClientData;
        
	wRet = CallTo16_word_lwll(pLPD->fn[FUNC_ENUMOBJ], 
                                  lP1, wP2, lP3, lP4);
    }
    else 
        fprintf(stderr,"Failed to find device\n");
    
    dprintf_win16drv(stddeb, "PRTDRV_EnumDFonts: return %x\n", wRet);
    return wRet;
}

/* 
 * RealizeObject (ordinal 10)
 */
DWORD PRTDRV_RealizeObject(LPPDEVICE lpDestDev, WORD wStyle, 
                           LPVOID lpInObj, LPVOID lpOutObj,
                           SEGPTR lpTextXForm)
{
    WORD dwRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;
    
    dprintf_win16drv(stddeb, "PRTDRV_RealizeObject:\n");
    
    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP3, lP4, lP5;  
	WORD wP2;
	SEGPTR SegPtr = pLPD->ThunkBufSegPtr;
	SEGPTR Limit = pLPD->ThunkBufLimit;
	int   nSize;

	if (pLPD->fn[FUNC_REALIZEOBJECT] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_RealizeObject: Not supported by driver\n");
	    return 0;
	}

	lP1 = lpDestDev;
	wP2 = wStyle;
	
	lP3 = SegPtr;
	switch (wStyle)
	{
        case 3: 
            nSize = sizeof(LOGFONT16); 
            break;
        default:
	    fprintf(stderr,"PRTDRV_RealizeObject: Object type %d not supported\n", wStyle);
            nSize = 0;
            
	}
	AddData(&SegPtr, lpInObj, nSize, Limit);	
	
	lP4 = (LONG)lpOutObj;

        lP5 = lpTextXForm;

	dwRet = CallTo16_long_lwlll(pLPD->fn[FUNC_REALIZEOBJECT], 
                                    lP1, wP2, lP3, lP4, lP5);

    }
    dprintf_win16drv(stddeb, "PRTDRV_RealizeObject: return %x\n", dwRet);
    return dwRet;
}


DWORD PRTDRV_ExtTextOut(LPPDEVICE lpDestDev, WORD wDestXOrg, WORD wDestYOrg,
                        RECT16 *lpClipRect, LPCSTR lpString, WORD wCount, 
                        SEGPTR lpFontInfo, SEGPTR lpDrawMode, 
                        SEGPTR lpTextXForm, SHORT *lpCharWidths,
                        RECT16 *     lpOpaqueRect, WORD wOptions)
{
    DWORD dwRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;
    
    dprintf_win16drv(stddeb, "PRTDRV_ExtTextOut:\n");
    
    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP4, lP5, lP7, lP8, lP9, lP10, lP11;  
	WORD wP2, wP3, wP12;
        INT16 iP6;

	SEGPTR SegPtr = pLPD->ThunkBufSegPtr;
	SEGPTR Limit = pLPD->ThunkBufLimit;
	int   nSize;

	if (pLPD->fn[FUNC_EXTTEXTOUT] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_ExtTextOut: Not supported by driver\n");
	    return 0;
	}

	lP1 = lpDestDev;
	wP2 = wDestXOrg;
	wP3 = wDestYOrg;
	
	if (lpClipRect != NULL)
	{
	    lP4 = SegPtr;
	    nSize = sizeof(RECT16);
            dprintf_win16drv(stddeb, "Adding lpClipRect\n");
            
	    AddData(&SegPtr, lpClipRect, nSize, Limit);	
	}
	else
	  lP4 = 0L;

	if (lpString != NULL)
	{
	    /* TTD WARNING THIS STRING ISNT NULL TERMINATED */
	    lP5 = SegPtr;
	    nSize = strlen(lpString);
            nSize = abs(wCount);
            dprintf_win16drv(stddeb, "Adding string size %d\n",nSize);
            
	    AddData(&SegPtr, lpString, nSize, Limit);	
	}
	else
	  lP5 = 0L;
	
	iP6 = wCount;
	
	/* This should be realized by the driver, so in 16bit data area */
	lP7 = lpFontInfo;
        lP8 = lpDrawMode;
        lP9 = lpTextXForm;
	
	if (lpCharWidths != NULL) 
	  dprintf_win16drv(stddeb, "PRTDRV_ExtTextOut: Char widths not supported\n");
	lP10 = 0;
	
	if (lpOpaqueRect != NULL)
	{
	    lP11 = SegPtr;
	    nSize = sizeof(RECT16);
            dprintf_win16drv(stddeb, "Adding opaqueRect\n");
	    AddData(&SegPtr, lpOpaqueRect, nSize, Limit);	
	}
	else
	  lP11 = 0L;
	
	wP12 = wOptions;
	dprintf_win16drv(stddeb, "Calling exttextout 0x%lx 0x%x 0x%x 0x%lx\n0x%lx 0x%x 0x%lx 0x%lx\n"
               "0x%lx 0x%lx 0x%lx 0x%x\n",lP1, wP2, wP3, lP4, 
					   lP5, iP6, lP7, lP8, lP9, lP10,
					   lP11, wP12);
	dwRet = CallTo16_long_lwwllwlllllw(pLPD->fn[FUNC_EXTTEXTOUT], 
                                           lP1, wP2, wP3, lP4, 
					   lP5, iP6, lP7, lP8, lP9, lP10,
					   lP11, wP12);
    }
    dprintf_win16drv(stddeb, "PRTDRV_ExtTextOut: return %lx\n", dwRet);
    return dwRet;
}


