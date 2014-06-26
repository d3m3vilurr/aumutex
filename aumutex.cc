#include <node.h>
#include <v8.h>

using namespace v8;

#define REQUIRE_ARGUMENT_FUNCTION(i, var)                                       \
    if (args.Length() <= (i) || !args[i]->IsFunction()) {                       \
        return ThrowException(Exception::TypeError(                             \
            String::New("Argument " #i " must be a function"))                  \
            );                                                                  \
    }                                                                           \
    Local<Function> var = Local<Function>::Cast(args[i]);

#define REQUIRE_ARGUMENT_OBJECT(i, var)                                         \
    if (args.Length() <= (i) || !args[i]->IsObject()) {                         \
        return ThrowException(Exception::TypeError(                             \
            String::New("Argument " #i " must be a object"))                    \
            );                                                                  \
    }                                                                           \
    Local<Object> var = Local<Object>::Cast(args[i]);

#define REQUIRE_ARGUMENT_STRING(i, var)                                         \
    if (args.Length() <= (i) || !args[i]->IsString()) {                         \
        return ThrowException(Exception::TypeError(                             \
            String::New("Argument " #i " must be a string"))                    \
            );                                                                  \
    }                                                                           \
    String::Utf8Value var(args[i]->ToString());

#define REQUIRE_ARGUMENT_ASCII_STRING(i, var)                                   \
    if (args.Length() <= (i) || !args[i]->IsString()) {                         \
        return ThrowException(Exception::TypeError(                             \
            String::New("Argument " #i " must be a string"))                    \
            );                                                                  \
    }                                                                           \
    String::AsciiValue var(args[i]->ToString());

#define REQUIRE_ARGUMENT_EXTERNAL(i, type, var)                                 \
    if (args.Length() <= (i)) {                                                 \
        return ThrowException(Exception::TypeError(                             \
            String::New("Argument " #i " invalid"))                             \
            );                                                                  \
    }                                                                           \
    type var = (type)External::Unwrap(args[i]);


#if defined (_WIN32) || defined (_WIN64)
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

Handle<Value> ErrorCallback(Local<Function>& callback, const char *format, ...) {
    HandleScope scope;
    char error[256];
    va_list vl;
    
	va_start(vl, format);
	vsprintf(error, format, vl);
	va_end(vl);    
    
    Handle<Value> argv[] = {Exception::Error(String::New(error))};
    
    return scope.Close(callback->Call(Context::GetCurrent()->Global(), 1, argv));
}

Handle<Value> ErrorException(const char *format, ...) {
    HandleScope scope;
    char error[256];
    va_list vl;

    va_start(vl, format);
    vsprintf(error, format, vl);
    va_end(vl);

    return scope.Close(ThrowException(Exception::TypeError(String::New(error))));
}

Handle<Value> create(const Arguments& args) {
    HandleScope scope;
    REQUIRE_ARGUMENT_ASCII_STRING(0, param_name);
    TCHAR name[256];
    HANDLE mutex;

    _stprintf(name, _T("Global\\aumutex_%s"), *param_name);

    mutex = CreateMutex(NULL, FALSE, name);
    if (mutex == NULL) {
        return scope.Close(
            ErrorException("CreateMutex error:%d", GetLastError())
            );
    }

    Local<ObjectTemplate> templ = ObjectTemplate::New();
    templ->SetInternalFieldCount(1);
    
    Local<Object> inst = templ->NewInstance();
    inst->SetInternalField(0, External::New((void*)mutex));
    
    return scope.Close(inst);
}

Handle<Value> enter(const Arguments& args) {
    HandleScope scope;
    REQUIRE_ARGUMENT_OBJECT(0, wrapper);
    Handle<External> field = Handle<External>::Cast(wrapper->GetInternalField(0));
    DWORD wait;
    HANDLE mutex;
   
    mutex = (HANDLE)(field->Value());
    
    wait = WaitForSingleObject(mutex, INFINITE);
    if (wait == WAIT_FAILED) {
        return scope.Close(ErrorException(
                "WaitForSingleObject error:%d (wait result:%d)", 
                GetLastError(), 
                wait));
    }

    return scope.Close(Undefined());
}

Handle<Value> leave(const Arguments& args) {
    HandleScope scope;
    REQUIRE_ARGUMENT_OBJECT(0, wrapper);
    Handle<External> field = Handle<External>::Cast(wrapper->GetInternalField(0));
    HANDLE mutex;
    
    mutex = (HANDLE)(field->Value());
    
    if (!ReleaseMutex(mutex)) {
        return scope.Close(ErrorException(
                "ReleaseMutex error:%d", 
                GetLastError()));
    }
    
    return scope.Close(Undefined());
}

Handle<Value> close(const Arguments& args) {
    HandleScope scope;
    REQUIRE_ARGUMENT_OBJECT(0, wrapper);
    Handle<External> field = Handle<External>::Cast(wrapper->GetInternalField(0));
    HANDLE mutex;
    
    mutex = (HANDLE)(field->Value());

    ReleaseMutex(mutex);
    CloseHandle(mutex);

    return scope.Close(Undefined());
}

#else /* #if defined (_WIN32) || defined (_WIN64) */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

Handle<Value> ErrorCallback(Local<Function>& callback, const char *format, ...) {
    HandleScope scope;
    char error[256];
    va_list vl;
    
	va_start(vl, format);
	vsprintf(error, format, vl);
	va_end(vl);    
    
    Handle<Value> argv[] = {Exception::Error(String::New(error))};
    
    return scope.Close(callback->Call(Context::GetCurrent()->Global(), 1, argv));
}

Handle<Value> ErrorException(const char *format, ...) {
    HandleScope scope;
    char error[256];
    va_list vl;

    va_start(vl, format);
    vsprintf(error, format, vl);
    va_end(vl);

    return scope.Close(ThrowException(Exception::TypeError(String::New(error))));
}

Handle<Value> create(const Arguments& args) {
    HandleScope scope;
    REQUIRE_ARGUMENT_ASCII_STRING(0, param_name);
    int fd;
    
    fd = open(*param_name, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    if (fd == -1) {
        return scope.Close(
            ErrorException("fopen(%s) error:%d", *param_name, errno)
            );
    }
    
    Local<ObjectTemplate> templ = ObjectTemplate::New();
    templ->SetInternalFieldCount(1);
    
    Local<Object> inst = templ->NewInstance();
    inst->SetInternalField(0, External::New((void*)fd));
    
    return scope.Close(inst);
}

Handle<Value> enter(const Arguments& args) {
    HandleScope scope;
    REQUIRE_ARGUMENT_OBJECT(0, wrapper);
    Handle<External> field = Handle<External>::Cast(wrapper->GetInternalField(0));
    struct flock lock;
    int fd;
   
    fd = (long)(field->Value());
    
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        return scope.Close(ErrorException("fcntl(lock) error:%d", errno));
    }

    return scope.Close(Undefined());
}

Handle<Value> leave(const Arguments& args) {
    HandleScope scope;
    REQUIRE_ARGUMENT_OBJECT(0, wrapper);
    Handle<External> field = Handle<External>::Cast(wrapper->GetInternalField(0));
    struct flock lock;
    int fd;
    
    fd = (long)(field->Value());
    
    lock.l_type = F_UNLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        return scope.Close(ErrorException("fcntl(unlock) error:%d", errno));
    }

    return scope.Close(Undefined());
}

Handle<Value> close(const Arguments& args) {
    HandleScope scope;
    REQUIRE_ARGUMENT_OBJECT(0, wrapper);
    Handle<External> field = Handle<External>::Cast(wrapper->GetInternalField(0));
    int fd;
    
    fd = (long)(field->Value());

    close(fd);

    return scope.Close(Undefined());
}
#endif

void init(Handle<Object> exports) {
    exports->Set(String::NewSymbol("create"), FunctionTemplate::New(create)->GetFunction());
    exports->Set(String::NewSymbol("enter"), FunctionTemplate::New(enter)->GetFunction());
    exports->Set(String::NewSymbol("leave"), FunctionTemplate::New(leave)->GetFunction());
    exports->Set(String::NewSymbol("close"), FunctionTemplate::New(close)->GetFunction());
}

NODE_MODULE(aumutex, init)
