// system
#include <cstdio>
#include <cstdint>
#include <Windows.h>

// gl
#include <gl/GL.h>
#include "glext.h"
#include "gldefs.h"

// config
#include "config.h"

// GDI
#include <gdiplus.h>

struct GdiplusInit
{
	GdiplusInit()
	{
		Gdiplus::GdiplusStartupInput inp;
		Gdiplus::GdiplusStartupOutput outp;
		if (Gdiplus::Ok != GdiplusStartup(&token_, &inp, &outp))
		{
			//throw runtime_error("GdiplusStartup");
			ExitProcess(1);
		}
	}
	~GdiplusInit()
	{
		Gdiplus::GdiplusShutdown(token_);
	}
private:
	ULONG_PTR token_;
};

// shaders
#include "frag_draw.h"
#undef VAR_IRESOLUTION
#undef VAR_FRAGCOLOR
#include "frag_present.h"

// requirements for capture mode
#if CAPTURE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <ctime>
#endif

// shaders
GLuint gShaderDraw;
GLuint gShaderPresent;

// framebuffers
GLuint fbAccumulator;

// textures
GLuint proseTex;

// uniform bindings
int const kUniformResolution = 0;
int const kUniformFrame = 1;
int const kSamplerAccumulatorTex = 0;

// === resolutions ===
#if WINDOW_AUTO_SIZE
// the ugliest comma operator hack I will ever write
int const kCanvasWidth = (SetProcessDPIAware(), GetSystemMetrics(SM_CXSCREEN));
int const kCanvasHeight = GetSystemMetrics(SM_CYSCREEN);
#else
#define kCanvasWidth CANVAS_WIDTH
#define kCanvasHeight CANVAS_HEIGHT
#endif

#define kWindowWidth kCanvasWidth
#define kWindowHeight kCanvasHeight
// =====================

#if _DEBUG
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}
#endif



uint32_t lfsr = 0xACE1u;
uint32_t myrand()
{
	unsigned lsb = lfsr & 1u;  /* Get LSB (i.e., the output bit). */
	lfsr >>= 1;                /* Shift register */
	if (lsb)                   /* If the output bit is 1, */
		lfsr ^= 0xB400u;       /*  apply toggle mask. */
	return lfsr >> 4;
}


//A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint32_t hash(uint32_t x)
{
	x += (x << 10u);
	x ^= (x >> 6u);
	x += (x << 3u);
	x ^= (x >> 11u);
	x += (x << 15u);
	return x;
}


GLuint createProseTexture()
{
	// setup gdi+
	ULONG_PTR token_;
	Gdiplus::GdiplusStartupInput inp;
	Gdiplus::GdiplusStartupOutput outp;
#if _DEBUG
	// seems to work fine replacing &outp with NULL
	if (Gdiplus::Ok != GdiplusStartup(&token_, &inp, &outp))
	{
		ExitProcess(1);
	}
#else
	GdiplusStartup(&token_, &inp, &outp);
#endif
	// setup gdi+

	GLuint backing;
	{
		auto const SCALE = 16;
		auto const width = 256 * SCALE;
		auto const height = 256 * SCALE;

		//Create a bitmap
		Gdiplus::Bitmap myBitmap(width, height, PixelFormat32bppARGB);
		Gdiplus::Graphics g(&myBitmap);
		g.Clear(Gdiplus::Color::White);
		//g.Clear(Gdiplus::Color(255,128,128,128));
		g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

		const auto FONT_SIZE = 9 * SCALE;

		Gdiplus::FontFamily  fontFamily(L"Georgia");
		Gdiplus::Font        boldFont(&fontFamily, FONT_SIZE, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
		Gdiplus::Font        font(&fontFamily, FONT_SIZE, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
		Gdiplus::SolidBrush  solidBrush(Gdiplus::Color(255, 0, 0, 0));

		//Gdiplus::Pen         debugPen(Gdiplus::Color::Blue);

		WCHAR const* title = L"Fragments of Self";

		WCHAR const* poem[] = {
			L", rev. 24",
			0,
			0,
			0,
			L"I used to call myself a graphics programmer.",
			L"But somewhere along the way, I lost that self.",
			//L"A hostile environment left her broken.",
			L"She lost her passion and went into hiding.",
			0,
			L"Perhaps, one day, I’ll ﬁnd her again.",
			0,
			L"– Luna",
			0,
			0,
			0,
			L"She wrote those words,",
			L"in hopes I’d draw for her.",
			L"I’m not gone, just… resting.",
			0,
			L"I hope this is enough, for now.",
			0,
			L"– yx",
		};

		const auto reset_x = 2*SCALE;
		
		Gdiplus::PointF pos(reset_x, 2*SCALE);
		
		Gdiplus::StringFormat stringFormat = Gdiplus::StringFormat::GenericTypographic();
		Gdiplus::RectF boundingBox;
		g.DrawString(title, -1, &boldFont, pos, &stringFormat, &solidBrush);
		g.MeasureString(title, -1, &boldFont, pos, &stringFormat, &boundingBox);
		
		//g.DrawRectangle(&debugPen, boundingBox);

		pos.X += boundingBox.Width;
		for (WCHAR const* line : poem)
		{
			g.DrawString(line, -1, &font, pos, &stringFormat, &solidBrush);
			pos.X = reset_x;
			pos.Y += FONT_SIZE * 1.35f;
		}

		Gdiplus::Rect lockRect = Gdiplus::Rect(Gdiplus::Point(0, 0), Gdiplus::Size(width, height));
		Gdiplus::BitmapData bitmapData;
		myBitmap.LockBits(
			&lockRect,
			Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,
			PixelFormat32bppARGB,
			&bitmapData
		);

		uint32_t seed = 0xdecea5ed;
		for (int i = 0; i < width * height; ++i)
		{
			seed = hash(seed);
			((char*)bitmapData.Scan0)[i * 4] = seed & 0xff;
		}

		glGenTextures(1, &backing);
		glBindTexture(GL_TEXTURE_2D, backing);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmapData.Scan0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

#if _DEBUG
		// Save bitmap (as a png)
		CLSID pngClsid;
		int result = GetEncoderClsid(L"image/png", &pngClsid);
		if (result == -1)
		{
			ExitProcess(1);
		}
		if (Gdiplus::Ok != myBitmap.Save(L"test.png", &pngClsid, NULL))
		{
			ExitProcess(1);
		}
#endif
	}

	// cleanup gdi+
#if _DEBUG
	Gdiplus::GdiplusShutdown(token_);
#endif
	// cleanup gdi+

	return backing;
}




// capture GL errors
#if _DEBUG
void __stdcall
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);

	__debugbreak();
}
#endif

GLuint makeFramebuffer()
{
	GLuint name, backing;
	glGenFramebuffers(1, &name);
	glBindFramebuffer(GL_FRAMEBUFFER, name);
	glGenTextures(1, &backing);
	glBindTexture(GL_TEXTURE_2D, backing);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, kWindowWidth, kWindowHeight, 0, GL_RGBA, GL_FLOAT, 0);

	// don't remove these!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, backing, 0);
	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);
	return name;
}

GLuint makeShader(const char* source)
{
#if _DEBUG
	GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(shader, 1, &source, 0);
	glCompileShader(shader);

	// shader compiler errors
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		const int maxLength = 1024;
		GLchar errorLog[maxLength];
		glGetShaderInfoLog(shader, maxLength, 0, errorLog);
		puts(errorLog);
		glDeleteShader(shader);
		__debugbreak();
	}

	// link shader
	GLuint m_program = glCreateProgram();
	glAttachShader(m_program, shader);
	glLinkProgram(m_program);

	GLint isLinked = 0;
	glGetProgramiv(m_program, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		const int maxLength = 1024;
		GLchar errorLog[maxLength];
		glGetProgramInfoLog(m_program, maxLength, 0, errorLog);
		puts(errorLog);
		glDeleteProgram(m_program);
		__debugbreak();
	}
	
	return m_program;
#else
	return glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &source);
#endif

}

void bindSharedUniforms()
{
	glUniform4f(
		kUniformResolution,
		(float)kCanvasWidth,
		(float)kCanvasHeight,
		(float)kCanvasWidth / (float)kCanvasHeight,
		(float)kCanvasHeight / (float)kCanvasWidth);
}

static inline void accumulatorSetup()
{
	glUseProgram(gShaderDraw);
	glBindFramebuffer(GL_FRAMEBUFFER, fbAccumulator);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	bindSharedUniforms();
}

static inline void presentSetup(int destFb)
{
	glUseProgram(gShaderPresent);
	glBindFramebuffer(GL_FRAMEBUFFER, destFb);

	glBlendFunc(GL_ONE, GL_ZERO);

	bindSharedUniforms();
	glBindTexture(GL_TEXTURE_2D, fbAccumulator);
}

static inline void accumulatorRender(int sampleCount)
{
	glUniform1i(kUniformFrame, sampleCount);
	glBindTexture(GL_TEXTURE_2D, proseTex);
	glRecti(-1, -1, 1, 1);

#ifndef RENDER_EXACT_SAMPLES
	// deliberately block so we don't queue up more work than we have time for
	glFinish();
#endif
}

static inline void presentRender(HDC hDC)
{
	glRecti(-1, -1, 1, 1);
	SwapBuffers(hDC);
}

#if defined(RELEASE)
int WinMainCRTStartup()
#else
int main()
#endif
{
#ifndef RENDER_EXACT_SAMPLES
	unsigned int startTime = timeGetTime();
#endif

	DEVMODE screenSettings = {
		{0}, 0, 0, sizeof(screenSettings), 0, DM_PELSWIDTH | DM_PELSHEIGHT,
		{0}, 0, 0, 0, 0, 0, {0}, 0, 0, (DWORD)kWindowWidth, (DWORD)kWindowHeight, 0, 0,
		#if(WINVER >= 0x0400)
			0, 0, 0, 0, 0, 0,
			#if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
				0, 0
			#endif
		#endif
	};
	const PIXELFORMATDESCRIPTOR pfd = {
		sizeof(pfd), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
		32, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 32, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};
	
	#if WINDOW_FULLSCREEN
		ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN);
		ShowCursor(0);
		HDC hDC = GetDC(CreateWindow((LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0));
	#else
		HDC hDC = GetDC(CreateWindow((LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE, 0, 0, kWindowWidth, kWindowHeight, 0, 0, 0, 0));
	#endif

	// set pixel format and make opengl context
	SetPixelFormat(hDC, ChoosePixelFormat(hDC, &pfd), &pfd);
	wglMakeCurrent(hDC, wglCreateContext(hDC));
	SwapBuffers(hDC);

	// enable opengl debug messages
#if _DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
#endif

	// make framebuffer
	fbAccumulator = makeFramebuffer();

	// optional extra buffers/textures
#if CAPTURE
	GLuint const fbCapture = makeFramebuffer();
	float* cpuFramebufferFloat = new float[kCanvasWidth * kCanvasHeight * 4];
	uint8_t* cpuFramebufferU8 = new uint8_t[kCanvasWidth * kCanvasHeight * 3];
#endif

	// render text
	proseTex = createProseTexture();

	// make shaders
	gShaderDraw = makeShader(draw_frag);
	gShaderPresent = makeShader(present_frag);

	// main accumulator loop
	accumulatorSetup();
#if !DESPERATE
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
#endif
	for (
		int sampleCount = 0;
#ifdef RENDER_EXACT_SAMPLES
		sampleCount < RENDER_EXACT_SAMPLES;
#else
	#ifdef RENDER_MIN_SAMPLES
			(sampleCount < RENDER_MIN_SAMPLES) ||
	#endif
	#ifdef RENDER_MAX_SAMPLES
			(sampleCount < RENDER_MAX_SAMPLES) &&
	#endif
			(timeGetTime() < startTime + RENDER_MAX_TIME_MS);
#endif
		++sampleCount
	)
	{
#if _DEBUG
		printf("accumulate sample %d\n", sampleCount);
#endif

		#if !DESPERATE
		PeekMessage(0, 0, 0, 0, PM_REMOVE);
		#endif
		accumulatorRender(sampleCount);

		// To prevent accidentally hitting esc during long renders. Use Alt+F4 instead.
		#if !CAPTURE
		if (GetAsyncKeyState(VK_ESCAPE))
			goto abort;
		#endif


		#if RENDER_PROGRESSIVE
			#if CAPTURE
			if ((sampleCount&(sampleCount-1))==0)
			#endif
			{
				presentSetup(0);
				presentRender(hDC);

				accumulatorSetup();
			}
		#endif
	}

#if CAPTURE
	presentSetup(fbCapture);
	presentRender(hDC);
	glFinish();
	glReadPixels(0, 0, kCanvasWidth, kCanvasHeight, GL_RGBA, GL_FLOAT, cpuFramebufferFloat);

	for (int y = 0; y < kCanvasHeight; ++y) {
		int invy = kCanvasHeight - y - 1;
		for (int x = 0; x < kCanvasWidth; ++x) {
			for (int i = 0; i < 3; ++i) {
				float const fval = cpuFramebufferFloat[(invy*kCanvasWidth + x) * 4 + i];
				uint8_t const u8val = fval < 0 ? 0 : (fval > 1 ? 255 : (uint8_t)(fval * 255));
				cpuFramebufferU8[(y*kCanvasWidth + x) * 3 + i] = u8val;
			}
		}
	}
	delete[] cpuFramebufferFloat;

	char name[256];
	sprintf(name, "%llu_%dx%d_%dspp", time(NULL), kCanvasWidth, kCanvasHeight, RENDER_EXACT_SAMPLES);

#if CAPTURE_SAVE_U8_BIN
	{
		OutputDebugString("saving u8 bin\n");
		char binname[256];
		sprintf(binname, "%s.u8.bin", name);
		FILE* fh = fopen(binname, "wb");
		fwrite(cpuFramebufferU8, 1, kCanvasWidth * kCanvasHeight * 3, fh);
		fclose(fh);
	}
#endif

#if CAPTURE_SAVE_F32_BIN
	{
		OutputDebugString("saving f32 bin\n");
		char binname[256];
		sprintf(binname, "%s.f32.bin", name);
		FILE* fh = fopen(binname, "wb");
		fwrite(cpuFramebufferFloat, 4, kCanvasWidth * kCanvasHeight * 4, fh);
		fclose(fh);
	}
#endif

#if CAPTURE_SAVE_JPG
	{
		OutputDebugString("saving jpg\n");
		char jpgname[256];
		sprintf(jpgname, "%s.jpg", name);
		stbi_write_jpg(jpgname, kCanvasWidth, kCanvasHeight, 3, cpuFramebufferU8, 100);
	}
#endif

#if CAPTURE_SAVE_PNG
	{
		OutputDebugString("saving png\n");
		char pngname[256];
		sprintf(pngname, "%s.png", name);
		stbi_write_png(pngname, kCanvasWidth, kCanvasHeight, 3, cpuFramebufferU8, kCanvasWidth * 3);
	}
#endif

	delete[] cpuFramebufferU8;
#else
	presentSetup(0);
	while (!GetAsyncKeyState(VK_ESCAPE))
	{
		PeekMessage(0, 0, 0, 0, PM_REMOVE);
		presentRender(hDC);
	}
#endif

abort:
	ExitProcess(0);
	return 0;
}
