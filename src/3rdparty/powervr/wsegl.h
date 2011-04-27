/******************************************************************************
 Name         : wsegl.h
 Copyright    :	Copyright (c) Imagination Technologies Limited.
				This specification is protected by copyright laws and contains
				material proprietary to Imagination Technologies Limited.
				You may use and distribute this specification free of charge for implementing
				the functionality therein, without altering or removing any trademark, copyright,
				or other notice from the specification.
 Platform     : ANSI
*****************************************************************************/


#if !defined(__WSEGL_H__)
#define __WSEGL_H__

#ifdef __cplusplus
extern "C" {
#endif 

/*
// WSEGL Platform-specific definitions
*/
#define WSEGL_EXPORT
#define WSEGL_IMPORT

/*
// WSEGL API Version Number
*/

#define WSEGL_VERSION 1
#define WSEGL_DEFAULT_DISPLAY 0
#define WSEGL_DEFAULT_NATIVE_ENGINE 0

#define WSEGL_FALSE		0
#define WSEGL_TRUE		1
#define WSEGL_NULL		0

#define	WSEGL_UNREFERENCED_PARAMETER(param) (param) = (param)

/*
// WSEGL handles
*/
typedef void *WSEGLDisplayHandle;
typedef void *WSEGLDrawableHandle;

/*
// Display capability type
*/
typedef enum WSEGLCapsType_TAG
{
	WSEGL_NO_CAPS = 0,
	WSEGL_CAP_MIN_SWAP_INTERVAL = 1, /* System default value = 1 */
	WSEGL_CAP_MAX_SWAP_INTERVAL = 2, /* System default value = 1 */
	WSEGL_CAP_WINDOWS_USE_HW_SYNC = 3, /* System default value = 0 (FALSE) */
	WSEGL_CAP_PIXMAPS_USE_HW_SYNC = 4, /* System default value = 0 (FALSE) */

} WSEGLCapsType;

/*
// Display capability
*/
typedef struct WSEGLCaps_TAG
{
	WSEGLCapsType eCapsType;
	unsigned long ui32CapsValue;

} WSEGLCaps;

/*
// Drawable type
*/
#define WSEGL_NO_DRAWABLE			0x0
#define WSEGL_DRAWABLE_WINDOW		0x1
#define WSEGL_DRAWABLE_PIXMAP		0x2


/*
// Pixel format of display/drawable
*/
typedef enum WSEGLPixelFormat_TAG
{
	WSEGL_PIXELFORMAT_565 = 0,
	WSEGL_PIXELFORMAT_4444 = 1,
	WSEGL_PIXELFORMAT_8888 = 2,
	WSEGL_PIXELFORMAT_1555 = 3

} WSEGLPixelFormat;

/*
// Transparent of display/drawable
*/
typedef enum WSEGLTransparentType_TAG
{
	WSEGL_OPAQUE = 0,
	WSEGL_COLOR_KEY = 1,

} WSEGLTransparentType;

/*
// Display/drawable configuration
*/
typedef struct WSEGLConfig_TAG
{
	/*
	// Type of drawables this configuration applies to -
	// OR'd values of drawable types. 
	*/
	unsigned long ui32DrawableType;

	/* Pixel format */
	WSEGLPixelFormat ePixelFormat;

	/* Native Renderable  - set to WSEGL_TRUE if native renderable */
	unsigned long ulNativeRenderable;

	/* FrameBuffer Level Parameter */
	unsigned long ulFrameBufferLevel;

	/* Native Visual ID */
	unsigned long ulNativeVisualID;

	/* Native Visual */
	void *hNativeVisual;

	/* Transparent Type */
	WSEGLTransparentType eTransparentType;

	/* Transparent Color - only used if transparent type is COLOR_KEY */
	unsigned long ulTransparentColor; /* packed as 0x00RRGGBB */


} WSEGLConfig;

/*
// WSEGL errors
*/
typedef enum WSEGLError_TAG
{
	WSEGL_SUCCESS = 0,
	WSEGL_CANNOT_INITIALISE = 1,
	WSEGL_BAD_NATIVE_DISPLAY = 2,
	WSEGL_BAD_NATIVE_WINDOW = 3,
	WSEGL_BAD_NATIVE_PIXMAP = 4,
	WSEGL_BAD_NATIVE_ENGINE = 5,
	WSEGL_BAD_DRAWABLE = 6,
	WSEGL_BAD_CONFIG = 7,
	WSEGL_OUT_OF_MEMORY = 8

} WSEGLError; 

/*
// Drawable orientation (in degrees anti-clockwise)
*/
typedef enum WSEGLRotationAngle_TAG
{
	WSEGL_ROTATE_0 = 0,
	WSEGL_ROTATE_90 = 1,
	WSEGL_ROTATE_180 = 2,
	WSEGL_ROTATE_270 = 3

} WSEGLRotationAngle; 

/*
// Drawable information required by OpenGL-ES driver
*/
typedef struct WSEGLDrawableParams_TAG
{
	/* Width in pixels of the drawable */
	unsigned long	ui32Width;

	/* Height in pixels of the drawable */
	unsigned long	ui32Height;

	/* Stride in pixels of the drawable */
	unsigned long	ui32Stride;

	/* Pixel format of the drawable */
	WSEGLPixelFormat	ePixelFormat;

	/* User space cpu virtual address of the drawable */
	void   			*pvLinearAddress;

	/* HW address of the drawable */
	unsigned long	ui32HWAddress;

	/* Private data for the drawable */
	void			*hPrivateData;

} WSEGLDrawableParams;


/*
// Table of function pointers that is returned by WSEGL_GetFunctionTablePointer()
//
// The first entry in the table is the version number of the wsegl.h header file that
// the module has been written against, and should therefore be set to WSEGL_VERSION
*/
typedef struct WSEGL_FunctionTable_TAG
{
	unsigned long ui32WSEGLVersion;

	WSEGLError (*pfnWSEGL_IsDisplayValid)(NativeDisplayType);

	WSEGLError (*pfnWSEGL_InitialiseDisplay)(NativeDisplayType, WSEGLDisplayHandle *, const WSEGLCaps **, WSEGLConfig **);

	WSEGLError (*pfnWSEGL_CloseDisplay)(WSEGLDisplayHandle);

	WSEGLError (*pfnWSEGL_CreateWindowDrawable)(WSEGLDisplayHandle, WSEGLConfig *, WSEGLDrawableHandle *, NativeWindowType, WSEGLRotationAngle *);

	WSEGLError (*pfnWSEGL_CreatePixmapDrawable)(WSEGLDisplayHandle, WSEGLConfig *, WSEGLDrawableHandle *, NativePixmapType, WSEGLRotationAngle *);

	WSEGLError (*pfnWSEGL_DeleteDrawable)(WSEGLDrawableHandle);

	WSEGLError (*pfnWSEGL_SwapDrawable)(WSEGLDrawableHandle, unsigned long);

	WSEGLError (*pfnWSEGL_SwapControlInterval)(WSEGLDrawableHandle, unsigned long);

	WSEGLError (*pfnWSEGL_WaitNative)(WSEGLDrawableHandle, unsigned long);

	WSEGLError (*pfnWSEGL_CopyFromDrawable)(WSEGLDrawableHandle, NativePixmapType);

	WSEGLError (*pfnWSEGL_CopyFromPBuffer)(void *, unsigned long, unsigned long, unsigned long, WSEGLPixelFormat, NativePixmapType);

	WSEGLError (*pfnWSEGL_GetDrawableParameters)(WSEGLDrawableHandle, WSEGLDrawableParams *, WSEGLDrawableParams *);


} WSEGL_FunctionTable;


WSEGL_IMPORT const WSEGL_FunctionTable *WSEGL_GetFunctionTablePointer(void);

#ifdef __cplusplus
}
#endif 

#endif /* __WSEGL_H__ */

/******************************************************************************
 End of file (wsegl.h)
******************************************************************************/
