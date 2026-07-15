#pragma once

#ifdef	ENGINE_EXPORTS
#define ENGINE_DLL		_declspec(dllexport)
#else
#define ENGINE_DLL		_declspec(dllimport)
#endif

//#define CHECK_HR(hr) \
//    if (FAILED(hr)) { \
//        wchar_t szBuffer[512]; \
//        swprintf_s(szBuffer, L"File: %s\nLine: %d\nError: 0x%08X", \
//                   TEXT(__FILE__), __LINE__, hr); \
//        MessageBox(NULL, szBuffer, L"Render Error", MB_OK); \
//        return hr; \
//    }

#define CHECK_HR(hr, fmt, ...) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            wchar_t szFinalMsg[1024]; \
            wchar_t szUserMsg[512]; \
            /* 유저가 입력한 커스텀 메시지 구성 */ \
            swprintf_s(szUserMsg, fmt, ##__VA_ARGS__); \
            /* 위치 정보와 합치기 */ \
            swprintf_s(szFinalMsg, L"Message: %s\n\nFile: %s\nLine: %d\nError: 0x%08X", \
                       szUserMsg, TEXT(__FILE__), __LINE__, _hr); \
            MessageBox(NULL, szFinalMsg, L"System Error", MB_OK | MB_ICONERROR); \
            return _hr; \
        } \
    } while (0)

#define CHECK_HR_NULL(hr, fmt, ...) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            wchar_t szFinalMsg[1024]; \
            wchar_t szUserMsg[512]; \
            /* 유저가 입력한 커스텀 메시지 구성 */ \
            swprintf_s(szUserMsg, fmt, ##__VA_ARGS__); \
            /* 위치 정보와 합치기 */ \
            swprintf_s(szFinalMsg, L"Message: %s\n\nFile: %s\nLine: %d\nError: 0x%08X", \
                       szUserMsg, TEXT(__FILE__), __LINE__, _hr); \
            MessageBox(NULL, szFinalMsg, L"System Error", MB_OK | MB_ICONERROR); \
            return nullptr; \
        } \
    } while (0)

#ifndef			MSG_BOX
#define			MSG_BOX(_message)			MessageBox(NULL, TEXT(_message), L"System Message", MB_OK)
#define			MSG_BOX_STR(_message)			MessageBox(NULL, _message, L"System Message", MB_OK)
#endif

#define			NS_BEGIN(NAMESPACE)		namespace NAMESPACE {
#define			NS_END						}

#define			NS_USING(NAMESPACE)	using namespace NAMESPACE;

//#define NO_COPY(CLASSNAME)											\
//			private:												\
//			CLASSNAME(const CLASSNAME&) = delete;					\
//			CLASSNAME& operator = (const CLASSNAME&) = delete;		
//
//#define DECLARE_SINGLETON(CLASSNAME)								\
//			NO_COPY(CLASSNAME)										\
//		public:														\
//			static CLASSNAME& Get(void) {							\
//			static CLASSNAME Instance;								\
//			return Instance;										\
//		}

#define NODE_ACTION_M    \
X(ACTION)\
X(ANIMATION)            \
X(DECORATOR)            \
X(EFFECT)               \
X(SELECTOR)\
X(SEQUENCE)\
X(ROOT)\
X(END)

#define MOVE_M    \
X(LEFT)			     \
X(RIGHT)            \
X(STRAIGHT)            \
X(BACKWARD)               \
X(END)
#define IMGUI_ENABLE

#define MAX_LIGHT_COUNT 8
#define MAX_LIGHT_MAPCOUNT 6
